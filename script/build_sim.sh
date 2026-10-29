#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

NEW_API=false
EXAMPLE_DIR=""
RUNTIME_SOURCE_FILE=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        -new_api|--new-api)
            NEW_API=true
            shift
            ;;
        --runtime_source_file)
            RUNTIME_SOURCE_FILE="$2"
            shift 2
            ;;
        -*)
            echo "Unknown flag: $1" >&2; exit 1
            ;;
        *)
            EXAMPLE_DIR="$1"
            shift
            ;;
    esac
done

if [ -z "$EXAMPLE_DIR" ]; then
    EXAMPLE_DIR="$(pwd)"
fi
EXAMPLE_DIR="$(cd "$EXAMPLE_DIR" && pwd)"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ -n "${BAREMETAL_DIR:-}" ] && [ -f "$BAREMETAL_DIR/script/build_sim.sh" ]; then
    REPO_ROOT="$(cd "$BAREMETAL_DIR" && pwd)"
else
    REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
fi

IPC_DIR="$EXAMPLE_DIR/ipc"
FULL_APP="$IPC_DIR/ipc_app"
LIBAIEBAREMETAL_IPC="$IPC_DIR/libaiebaremetal_ipc.so"
XAIENGINE_SIM_DIR="$REPO_ROOT/thirdparty/aielib/aie-rt/driver/src"

REPO_IPC="$REPO_ROOT/src/ipc"
AIE_DRIVER_SRC="$REPO_ROOT/thirdparty/aielib/aie-rt/driver/src"
AIE_DRIVER_INC="$REPO_ROOT/thirdparty/aielib/aie-rt/driver/include"
BSP_INC="$REPO_ROOT/thirdparty/arch/ps/psv_cortexa72_0/include"
SRC_INC="$REPO_ROOT/src/include"

_find_graph() {
    local name="$1"
    if [ -f "$EXAMPLE_DIR/src/$name" ]; then       echo "$EXAMPLE_DIR/src/$name"
    elif [ -f "$EXAMPLE_DIR/kernel/src/$name" ]; then echo "$EXAMPLE_DIR/kernel/src/$name"
    fi
}

if [ -n "$RUNTIME_SOURCE_FILE" ]; then
    if [[ "$RUNTIME_SOURCE_FILE" = /* ]]; then
        GRAPH_SRC="$RUNTIME_SOURCE_FILE"
    else
        GRAPH_SRC="$(cd "$EXAMPLE_DIR" && realpath -m "$RUNTIME_SOURCE_FILE")"
    fi
    if [ ! -f "$GRAPH_SRC" ]; then
        echo "ERROR: --runtime_source_file not found: $GRAPH_SRC" >&2
        exit 1
    fi
elif $NEW_API; then
    GRAPH_SRC=$(_find_graph graph_new_api.cpp)
    if [ -z "$GRAPH_SRC" ]; then
        echo "ERROR: --new-api requested but graph_new_api.cpp not found in $EXAMPLE_DIR" >&2
        exit 1
    fi
else
    GRAPH_SRC=$(_find_graph graph.cpp)
    [ -z "$GRAPH_SRC" ] && GRAPH_SRC="$EXAMPLE_DIR/src/graph.cpp"
fi

if ! $NEW_API && [[ "$(basename "$GRAPH_SRC")" == "graph_new_api.cpp" ]]; then
    NEW_API=true
fi

GRAPH_EXTRA_FLAGS=""
if $NEW_API; then
    GRAPH_EXTRA_FLAGS="-DAEG_INHERITANCE_BASED_API -I$EXAMPLE_DIR/Work/ps/c_rts"
fi

BACKEND_SRC="$REPO_IPC/aeg_ps_ipc_backend.cpp"
BACKEND_OBJ="$IPC_DIR/aeg_ps_ipc_backend.o"
GRAPH_OBJ="$IPC_DIR/graph.o"

_app_needs_build() {
    [ ! -f "$FULL_APP" ] && return 0
    local prev_env="$IPC_DIR/build_sim.env"
    if [ -f "$prev_env" ]; then
        local prev_src
        prev_src=$(grep "^BUILD_SIM_GRAPH_SRC=" "$prev_env" | cut -d= -f2-)
        [ "$prev_src" != "$GRAPH_SRC" ] && return 0
    fi
    for src in "$GRAPH_SRC" "$BACKEND_SRC" \
               "$SRC_INC/aiebaremetal.h" "$SRC_INC/xaiemem.h" \
               "$REPO_IPC/ps_ipc_protocol.h" "$REPO_IPC/aeg_ps_ipc_backend.h"; do
        [ -f "$src" ] && [ "$src" -nt "$FULL_APP" ] && return 0
    done
    if $NEW_API; then
        local ggh="$SRC_INC/generated_graphs.h"
        [ -f "$ggh" ] && [ "$ggh" -nt "$FULL_APP" ] && return 0
    fi
    return 1
}

if _app_needs_build; then
    echo "[build_sim] Building PS app..."
    mkdir -p "$IPC_DIR"
    (
        set +u
        source "$REPO_ROOT/script/envaie2pst50.sh"
        VITIS_INC="-I$XILINX_VITIS/aietools/include -I$XILINX_VITIS/aietools/include/drivers/aiengine"
        BASE_FLAGS="-std=c++17 -O2 -D_BSD_SOURCE -D_DEFAULT_SOURCE -DAIE_GEN=2 -DNO_JSON
            -I$REPO_IPC -I$SRC_INC -I$SRC_INC/common_layer -I$SRC_INC/common_layer/aegothers
            -I$EXAMPLE_DIR/src -I$AIE_DRIVER_SRC -I$AIE_DRIVER_INC -I$BSP_INC $VITIS_INC"
        g++ $BASE_FLAGS -D__AIESIM__ -c "$BACKEND_SRC" -o "$BACKEND_OBJ"
        g++ $BASE_FLAGS -D__AIESIM__ $GRAPH_EXTRA_FLAGS -c "$GRAPH_SRC" -o "$GRAPH_OBJ"
        g++ -o "$FULL_APP" "$BACKEND_OBJ" "$GRAPH_OBJ" \
            -Wl,-E -Wl,--unresolved-symbols=ignore-in-object-files \
            -L"$AIE_DRIVER_SRC" -Wl,--disable-new-dtags -Wl,-rpath,"$AIE_DRIVER_SRC" \
            -lxaiengine -ldl
    ) 2>&1 | grep -E "error:|warning: creating|g\+\+ -o" || true
    if [ ! -f "$FULL_APP" ]; then
        echo "ERROR: ipc_app build failed — check output above" >&2
        exit 1
    fi
    echo "[build_sim] PS app built."
else
    echo "[build_sim] PS app up to date."
fi

if [ ! -d "$EXAMPLE_DIR/Work/ps/c_rts/systemC" ]; then
    echo "ERROR: Work/ not found in $EXAMPLE_DIR — run build.sh first." >&2
    exit 1
fi

CONTROL_JSON="$EXAMPLE_DIR/Work/ps/c_rts/aie_control_config.json"

bash "$REPO_ROOT/script/build_libxaiengine_sim.sh"

_needs_baremetal_ipc_rebuild() {
    [ ! -f "$LIBAIEBAREMETAL_IPC" ] && return 0
    [ -f "$CONTROL_JSON" ] && [ "$CONTROL_JSON" -nt "$LIBAIEBAREMETAL_IPC" ] && return 0
    for f in "$REPO_ROOT/src"/*.cc "$REPO_ROOT/src"/*.cpp \
             "$REPO_ROOT/src/common_layer"/*.cpp \
             "$REPO_ROOT/src/common_layer/aegothers"/*.cpp; do
        [ -f "$f" ] && [ "$f" -nt "$LIBAIEBAREMETAL_IPC" ] && return 0
    done
    return 1
}

if [ -f "$CONTROL_JSON" ] && _needs_baremetal_ipc_rebuild; then
    echo "[build_sim] Building libaiebaremetal_ipc.so..."
    (
        set +u
        source "$REPO_ROOT/script/envaie2pst50.sh"
        export XILINX_VITIS_AIETOOLS="${XILINX_VITIS_AIETOOLS:-$XILINX_VITIS/aietools}"
        mkdir -p "$IPC_DIR/baremetal_ipc_objs"

        METADATA_CPP="$IPC_DIR/baremetal_ipc_objs/baremetal_metadata.cpp"
        METADATA_TOOL="$REPO_ROOT/script/aiemetacpp.py"
        PARTITION_JSON="$EXAMPLE_DIR/Work/ps/c_rts/aie_partition.json"
        ( cd "$REPO_ROOT/example"
          if [ -f "$PARTITION_JSON" ]; then
              python3 "$METADATA_TOOL" "$CONTROL_JSON" "$PARTITION_JSON"
          else
              python3 "$METADATA_TOOL" "$CONTROL_JSON"
          fi
        ) 2>/dev/null
        cp "$REPO_ROOT/example/baremetal_metadata.cpp" "$METADATA_CPP"

        REPO="$REPO_ROOT"
        IPC_CFLAGS="-std=c++17 -O0 -g -fPIC -D__AIESIM__ -DAIE_GEN=2 -DNO_JSON
            -D_BSD_SOURCE -D_DEFAULT_SOURCE -DBOOST_BIND_GLOBAL_PLACEHOLDERS
            -I$REPO/src/include
            -I$REPO/src/include/common_layer
            -I$REPO/src/include/common_layer/aegothers
            -I$REPO/thirdparty/aielib/aie-rt/driver/internal
            -I$REPO/thirdparty/aielib/include
            -I$REPO/thirdparty/aielib/aie-rt/fal/src
            -I$REPO/thirdparty/aielib/aie-rt/fal/build/src/include
            -I$REPO/thirdparty/arch/ps/psv_cortexa72_0/include
            -I$REPO/thirdparty/ELFIO
            -I$REPO/thirdparty/ELFIO/elfio
            -I$REPO/thirdparty/cert/handshake
            -I$REPO/thirdparty/cert/api
            -I$XILINX_VITIS_AIETOOLS/include"

        OBJS=()
        SRCS=(
            "$REPO/src/aeg_aiebaremetal.cc"
            "$REPO/src/aeg_cdo_parser.cc"
            "$REPO/src/aeg_configure.cc"
            "$REPO/src/aeg_pl_kernel.cc"
            "$REPO/src/cert_elf.cc"
            "$REPO/src/cert_elf_loader.cc"
            "$REPO/src/cert_host.cc"
            "$REPO/src/ctrl_code_elf.cc"
            "$REPO/src/xaie_generate_events.cc"
            "$REPO/src/common_layer/aeg_api_message.cpp"
            "$REPO/src/common_layer/aeg_runtime_api.cpp"
            "$REPO/src/common_layer/aegothers/aeg_profiling_api.cpp"
            "$REPO/src/common_layer/aegothers/fal_util.cpp"
            "$METADATA_CPP"
        )
        NPROC=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
        for src in "${SRCS[@]}"; do
            [ -f "$src" ] || continue
            obj="$IPC_DIR/baremetal_ipc_objs/$(basename "${src%.*}").o"
            OBJS+=("$obj")
            echo "  CC  $(basename "$src")"
            g++ $IPC_CFLAGS -c "$src" -o "$obj" &
            if [[ $(jobs -r | wc -l) -ge $NPROC ]]; then wait -n 2>/dev/null || wait; fi
        done
        wait
        UCFIRM_O="$REPO_ROOT/example/testuc/ucfirmware/output/ucfw_exe.o"
        [ -f "$UCFIRM_O" ] && OBJS+=("$UCFIRM_O")

        echo "  LD  libaiebaremetal_ipc.so"
        g++ -shared -o "$LIBAIEBAREMETAL_IPC" "${OBJS[@]}" \
            -L"$XAIENGINE_SIM_DIR" \
            -Wl,--disable-new-dtags -Wl,-rpath,"$XAIENGINE_SIM_DIR" \
            -lxaiengine_sim -ldl
    ) 2>&1
    echo "[build_sim] libaiebaremetal_ipc.so built."
elif [ ! -f "$CONTROL_JSON" ]; then
    echo "[build_sim] WARNING: No aie_control_config.json, skipping build of libaiebaremetal_ipc.so."
else
    echo "[build_sim] libaiebaremetal_ipc.so up to date."
fi

SYSTEMC_DIR="$EXAMPLE_DIR/Work/ps/c_rts/systemC"
GEN_SRC_DIR="$SYSTEMC_DIR/generated-source"
GEN_OBJ_DIR="$SYSTEMC_DIR/generated-objects"

WRAPPER_CPP=$(ls "$GEN_SRC_DIR"/genwrapper_for_ps_i*.cpp 2>/dev/null | grep -v '_ipc' | head -1)
if [ -z "$WRAPPER_CPP" ]; then
    echo "ERROR: No genwrapper_for_ps_i*.cpp in $GEN_SRC_DIR" >&2
    exit 1
fi
WRAPPER_BASE=$(basename "$WRAPPER_CPP" .cpp)
PS_IP=$(echo "$WRAPPER_BASE" | sed 's/genwrapper_for_//')
PS_SO="$GEN_OBJ_DIR/${PS_IP}.so"
WRAPPER_OBJ="$GEN_OBJ_DIR/${WRAPPER_BASE}.o"
IPC_SERVER_OBJ="$GEN_OBJ_DIR/aeg_ipc_server.o"
IPC_SERVER_SRC="$REPO_ROOT/src/ipc/aeg_ipc_server.cpp"
IPC_GRAPH_INIT_OBJ="$GEN_OBJ_DIR/aeg_ipc_graph_init.o"
IPC_GRAPH_INIT_SRC="$GEN_OBJ_DIR/aeg_ipc_graph_init.cpp"
XIOUTILS_OBJ="$GEN_OBJ_DIR/xioutils.o"
XIOUTILS_SRC="$REPO_ROOT/src/ipc/xioutils.cpp"
AEG_STUBS_OBJ="$GEN_OBJ_DIR/aeg_sim_stubs.o"
AEG_STUBS_SRC="$REPO_ROOT/src/ipc/aeg_sim_stubs.cpp"
PS_MAIN_STUB="$REPO_ROOT/src/ipc/ipc_ps_main_decl.h"

if [ -f "$CONTROL_JSON" ]; then
    GRAPH_NAMES=$(python3 -c "
import json, sys
try:
    d = json.load(open('$CONTROL_JSON'))
    meta = d.get('aie_metadata', d)
    graphs = meta.get('graphs', {})
    if isinstance(graphs, dict):
        names = [v.get('name','') for v in graphs.values()]
    elif isinstance(graphs, list):
        names = [g.get('name', g.get('graph_name','')) for g in graphs]
    else:
        names = []
    print(','.join(n for n in names if n))
except Exception as e:
    sys.stderr.write('Warning: '+str(e)+chr(10))
" 2>/dev/null || true)
    if [ -n "$GRAPH_NAMES" ]; then
        export AEG_IPC_GRAPH_NAMES="$GRAPH_NAMES"
        echo "[build_sim] Graph names: $GRAPH_NAMES"
    fi
fi

echo "[build_sim] Building IPC server + PS library ($PS_IP.so)..."
(
    set +u
    cd "$EXAMPLE_DIR"
    source "$REPO_ROOT/script/envaie2pst50.sh"
    export XILINX_VITIS_AIETOOLS="$XILINX_VITIS/aietools"
    source "$REPO_ROOT/script/aieldpath.sh"

    mkdir -p "$GEN_OBJ_DIR"

    VITIS_CLANG="$XILINX_VITIS_AIETOOLS/lnx64.o/tools/clang/bin/clang++"
    GCC_DIR=$(ls -d "$XILINX_VITIS_AIETOOLS/tps/lnx64/gcc/lib/gcc/x86_64-pc-linux-gnu"/*/  2>/dev/null | sort -V | tail -1 | sed 's|/$||')
    GCC_VER=$(basename "$GCC_DIR")

    VITIS_CLANG_FLAGS=(
        --gcc-install-dir="$GCC_DIR"
        -fPIC -fpermissive -std=c++17
        -D__AIE_ARCH__=20 -D__LOCK_FENCE_MODE__=2
        -DAIE_OPTION_SCALAR_FLOAT_ON_VECTOR -DAIE2_FP32_EMULATION_ACCURACY_FAST
        -D__DEVICE_MEM_TILE_ROWS__=2 -D__AIE_DEVICE_SHIM_ROWS__=1
        -D__AIECOMPILER_BOOST_CLANG_20_FIX
        -DSC_INCLUDE_DYNAMIC_PROCESSES -D__AIESIM__ -D__PS_INIT_AIE__ -DXAIE_DEBUG -O1
        -I"$REPO_ROOT/src/ipc"
        -I"$XILINX_VITIS_AIETOOLS/include"
        -I"$XILINX_VITIS_AIETOOLS/include/drivers/aiengine"
        -I"$XILINX_VITIS_AIETOOLS/data/osci_systemc/include"
        -I"$XILINX_VITIS_AIETOOLS/tps/boost_1_72_0"
        -I"$XILINX_VITIS_AIETOOLS/include/xtlm/include"
        -I"$XILINX_VITIS_AIETOOLS/tps/lnx64/gcc/include/c++/$GCC_VER"
        -I"$XILINX_VITIS_AIETOOLS/tps/lnx64/gcc/include/c++/$GCC_VER/x86_64-pc-linux-gnu"
        -I"$REPO_ROOT/src/include"
        -I"$REPO_ROOT/src/include/common_layer"
        -I"$REPO_ROOT/src/include/common_layer/aegothers"
        -I"$REPO_ROOT/thirdparty/aielib/aie-rt/driver/internal"
        -I"$REPO_ROOT/thirdparty/aielib/include"
        -I"$REPO_ROOT/thirdparty/aielib/aie-rt/fal/src"
        -I"$REPO_ROOT/thirdparty/aielib/aie-rt/fal/build/src/include"
        -DAIE_GEN=2
    )

    PART_JSON=""
    for _pj in "$EXAMPLE_DIR/Work/arch/aie_partition.json" \
               "$EXAMPLE_DIR/Work/ps/c_rts/aie_partition.json"; do
        [ -f "$_pj" ] && { PART_JSON="$_pj"; break; }
    done

    DRIVER_CONFIG=$(python3 -c "
import json, sys
try:
    d = json.load(open('$CONTROL_JSON'))
    meta = d.get('aie_metadata', d)
    dc = meta.get('driver_config', {})
    num_columns = int(dc.get('num_columns', 38))
    start_col = 0
    part_num_cols = num_columns
    part_json = '$PART_JSON'
    if part_json:
        try:
            pj = json.load(open(part_json))
            aie = pj.get('AIE', {})
            dev = next(iter(aie.values())) if isinstance(aie, dict) and aie else {}
            parts = dev.get('partitions') or []
            node = parts[0] if parts else dev
            start_col = int(node.get('startColumn', dev.get('startColumn', 0)))
            part_num_cols = int(node.get('numColumns', dev.get('numColumns', num_columns)))
        except Exception as pe:
            sys.stderr.write('Warning (partition json): '+str(pe)+chr(10))
    print(dc.get('hw_gen', 2),
          hex(int(dc.get('base_address', 0x20000000000))),
          dc.get('column_shift', 25),
          dc.get('row_shift', 20),
          num_columns,
          dc.get('num_rows', 11),
          dc.get('shim_row', 0),
          dc.get('mem_tile_row_start', 1),
          dc.get('mem_tile_num_rows', 2),
          dc.get('aie_tile_row_start', 3),
          dc.get('aie_tile_num_rows', 8),
          start_col,
          part_num_cols)
except Exception as e:
    sys.stderr.write('Warning: '+str(e)+chr(10))
    print('2 0x20000000000 25 20 38 11 0 1 2 3 8 0 38')
" 2>/dev/null)
    read HW_GEN BASE_ADDR COL_SHIFT ROW_SHIFT NUM_COLS NUM_ROWS \
         SHIM_ROW MEM_START MEM_ROWS AIE_START AIE_ROWS \
         PART_START_COL PART_NUM_COLS <<< "$DRIVER_CONFIG"

    PART_BASE_ADDR=$(printf '0x%x' "$(( BASE_ADDR + (PART_START_COL << COL_SHIFT) ))")

    GRAPH_DECLS=""
    GRAPH_DISPATCH=""
    for gname in ${AEG_IPC_GRAPH_NAMES//,/ }; do
        GRAPH_DECLS+="
bool ${gname}_load_elf(const std::string &, std::vector<std::string> &);
int  ${gname}_init(const std::string &);"
        GRAPH_DISPATCH+="
    if (name == \"${gname}\") {
        std::vector<std::string> e;
        ${gname}_load_elf(wd, e);
        ${gname}_init(wd);
        return 0;
    }"
    done

    cat > "$IPC_GRAPH_INIT_SRC" << GENEOF
// AUTO-GENERATED by build_sim.sh — do not edit.
#include <cstring>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <aie_codegen.h>

extern XAie_DevInst DevInst;
extern XAie_Config  ConfigPtr;

int  broadcastConfigForCoreEnable();
$GRAPH_DECLS

static std::map<uint64_t, XAie_MemInst *> g_mem_map;

extern "C" {

void aeg_ipc_devinst_init() {
    DevInst.IsReady = 0U;
    XAie_SetupConfig(ConfigPtr,
        static_cast<u8>($HW_GEN),
        $BASE_ADDR,
        $COL_SHIFT, $ROW_SHIFT,
        $NUM_COLS, $NUM_ROWS,
        $SHIM_ROW,
        $MEM_START, $MEM_ROWS,
        $AIE_START,  $AIE_ROWS);
    XAie_SetupPartitionConfig(&DevInst, $PART_BASE_ADDR,
        static_cast<u8>($PART_START_COL),
        static_cast<u8>($PART_NUM_COLS));
    XAie_CfgInitialize(&DevInst, &ConfigPtr);
}

int aeg_ipc_run_graph_init(const char *graph_name, const char *work_dir) {
    std::string name(graph_name ? graph_name : "");
    std::string wd(work_dir ? work_dir : "");

    broadcastConfigForCoreEnable();
    $GRAPH_DISPATCH

    fprintf(stderr, "[AEG_PS_IPC] aeg_ipc_run_graph_init: unknown graph '%s'\n",
            name.c_str());
    return -1;
}

uint64_t aeg_ipc_alloc_mem(uint32_t size) {
    if (!size) return 0;
    XAie_MemInst *mi = XAie_MemAllocate(&DevInst, static_cast<uint64_t>(size),
                                         XAIE_MEM_NONCACHEABLE);
    if (!mi) return 0;
    uint64_t addr = XAie_MemGetDevAddr(mi);
    g_mem_map[addr] = mi;
    return addr;
}

void aeg_ipc_free_mem(uint64_t dev_addr) {
    auto it = g_mem_map.find(dev_addr);
    if (it != g_mem_map.end()) {
        XAie_MemFree(it->second);
        g_mem_map.erase(it);
    }
}

} // extern "C"
GENEOF

    "$VITIS_CLANG" "${VITIS_CLANG_FLAGS[@]}" \
        -c "$XIOUTILS_SRC" \
        -o "$XIOUTILS_OBJ"

    "$VITIS_CLANG" "${VITIS_CLANG_FLAGS[@]}" \
        -c "$AEG_STUBS_SRC" \
        -o "$AEG_STUBS_OBJ"

    "$VITIS_CLANG" "${VITIS_CLANG_FLAGS[@]}" \
        -c "$IPC_SERVER_SRC" \
        -o "$IPC_SERVER_OBJ"

    "$VITIS_CLANG" "${VITIS_CLANG_FLAGS[@]}" \
        -c "$IPC_GRAPH_INIT_SRC" \
        -o "$IPC_GRAPH_INIT_OBJ"

    # TEST_BENCH stub keeps the wrapper from pulling in the user's graph source
    make -C "$SYSTEMC_DIR" \
        "TEST_BENCH=$PS_MAIN_STUB" \
        "$WRAPPER_BASE"

    "$VITIS_CLANG" \
        --gcc-install-dir="$GCC_DIR" \
        -stdlib=libstdc++ \
        -o "$PS_SO" \
        "$WRAPPER_OBJ" "$IPC_SERVER_OBJ" "$IPC_GRAPH_INIT_OBJ" \
        "$XIOUTILS_OBJ" "$AEG_STUBS_OBJ" \
        -D__AIECOMPILER_BOOST_CLANG_20_FIX \
        -Wl,--as-needed -shared \
        -laiecompiler_runtime -lsystemc -lxtlm -lstdc++fs \
        -L "$XILINX_VITIS_AIETOOLS/lib/lnx64.o" \
        -L "$XILINX_VITIS_AIETOOLS/data/osci_systemc/lib/lnx64"

) 2>&1 | tee /tmp/aeg_ps_ipc_build_$$.log
if [ ! -f "$PS_SO" ]; then
    echo "ERROR: failed to build $PS_SO (see /tmp/aeg_ps_ipc_build_$$.log)" >&2
    exit 1
fi
echo "[build_sim] PS library built."

mkdir -p "$IPC_DIR"
cat > "$IPC_DIR/build_sim.env" << EOF
# Written by build_sim.sh — do not edit manually.
BUILD_SIM_GRAPH_SRC=$GRAPH_SRC
BUILD_SIM_NEW_API=$NEW_API
BUILD_SIM_FULL_APP=$FULL_APP
BUILD_SIM_LIBAIEBAREMETAL_IPC=$LIBAIEBAREMETAL_IPC
BUILD_SIM_PS_SO=$PS_SO
BUILD_SIM_REPO_ROOT=$REPO_ROOT
EOF

echo "[build_sim] Done. Artifacts written to $IPC_DIR"
