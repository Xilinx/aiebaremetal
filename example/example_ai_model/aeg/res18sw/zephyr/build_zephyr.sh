#!/usr/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Build the ResNet18 demo as a Zephyr image for VEK280 (gen-1 Versal AI Edge,
# APU/A72, AIE-ML). Mirrors example/testkernel_zephyr/build_zephyr.sh but reuses
# the baremetal host sources in ../sw (see CMakeLists.txt) instead of copying.

set -x

# Vitis ships an OpenSSL whose CA store is elsewhere, which breaks the https
# fetches in west update/lopper-install. The steps that need Vitis (../sw
# drivers, bootgen) source settings64.sh themselves.
unset LD_LIBRARY_PATH LD_PRELOAD

CURRDIR=$PWD
AIE_GEN=2
BOOTGEN=-bootgen
GRAPH_BUILD=true
WEST_UPDATE=true
ZEPHYR_DIR=
# Number of parallel ResNet18 pipeline instances pasted onto the AIE array:
#   1  = single-instance  (../sw/build.sh)
#   10 = inst10 design    (../sw/buildbatch110.sh, aie/batch1_inst10/)
# Batch stays 1 per instance; this knob is instances, not batch size. Also
# drives the app's NUM_INST macro (and BATCH_GRAPH when >1).
NUM_INST=1
# Rounds per 'b' benchmark run (-bench_loops N); empty keeps the app default.
# -bench_seconds N instead bounds the run by wall-clock, which is what the power
# measurements use (fixed-length plateau regardless of per-round time). Set here
# rather than inherited, so a stale exported value can't silently apply.
BENCH_LOOPS=
BENCH_SECONDS=

# Repo root: this app lives at <repo>/example/example_ai_model/aeg/res18sw/zephyr.
AIEBAREMETAL_DIR=$(realpath ../../../../..)
RES18_SW_DIR=$(realpath ../sw)

# --- VEK280 / gen-2 target (resnet18 is gen-1 only for now) ---
ZBOARD=versal_apu
LOPPER_PROC=psv_cortexa72_0
AIE_OVERLAY=${AIEBAREMETAL_DIR}/script/aiengine_vek280.overlay
SDT_CHROOT=${AIEBAREMETAL_DIR}/thirdparty/sdtgen-dts-vek280
SYSTEM_TOP_DTS=${SDT_CHROOT}/latest/system-top.dts
SDT_GEN_BOARD=versal-vek280-reva
XSA=/proj/xbuilds/2026.1_daily_latest/internal_platforms/vek280_base/hw/hw.xsa
SYSTEM_TOP_DTS_SET=false

echo "Current directory: $CURRDIR"
echo "aiebaremetal directory: $AIEBAREMETAL_DIR"

while [[ $# -gt 0 ]]; do
    case "$1" in
        -bootgen|--bootgen)    BOOTGEN="-bootgen"; shift ;;
        -zephyr_dir|--zephyr_dir) ZEPHYR_DIR="$2"; shift 2 ;;
        -no_graph_build|--no_graph_build) GRAPH_BUILD=false; shift ;;
        -no_west_update|--no_west_update) WEST_UPDATE=false; shift ;;
        -inst10|--inst10) NUM_INST=10; shift ;;
        -num_inst|--num_inst) NUM_INST="$2"; shift 2 ;;
        -bench_loops|--bench_loops) BENCH_LOOPS="$2"; shift 2 ;;
        -bench_seconds|--bench_seconds) BENCH_SECONDS="$2"; shift 2 ;;
        -system_top_dts|--system_top_dts) SYSTEM_TOP_DTS="$2"; SYSTEM_TOP_DTS_SET=true; shift 2 ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

if [ "$SYSTEM_TOP_DTS_SET" != true ]; then
    SYSTEM_TOP_DTS=${SDT_CHROOT}/latest/system-top.dts
fi

BENCH_CMAKE_ARGS=
if [ -n "$BENCH_LOOPS" ]; then
    BENCH_CMAKE_ARGS="${BENCH_CMAKE_ARGS} -DBENCH_LOOPS=${BENCH_LOOPS}"
fi
if [ -n "$BENCH_SECONDS" ]; then
    BENCH_CMAKE_ARGS="${BENCH_CMAKE_ARGS} -DBENCH_SECONDS=${BENCH_SECONDS}"
fi

# Build the AIE design + mladfpatch + baremetal artifacts that the Zephyr app
# links against (Work/ CDOs, generated mladfpatch headers, ctrl-code/metadata
# pulled in by add_aiebaremetal_library). NUM_INST (instance count) selects the
# driver:
#   1  -> ../sw/build.sh           (single-instance,  ../aie/Work)
#   10 -> ../sw/buildbatch110.sh   (inst10 design,    ../aie/batch1_inst10/Work)
# Both repoint the sw/Work symlink at their own Work/, so the bootgen BIF's
# relative CDO paths resolve to the matching design automatically.
# TODO: these drivers also compile the baremetal vek280.elf via compile.sh
# -lwip, which the Zephyr build does not need. Trim to a graph-only target once
# the resnet18_top.cpp Zephyr path is wired up.
if [ "$NUM_INST" -gt 1 ]; then
    GRAPH_DRIVER=buildbatch110.sh
    AIE_WORK_SUBDIR=batch1_inst10/Work
else
    GRAPH_DRIVER=build.sh
    AIE_WORK_SUBDIR=Work
fi
if [ "$GRAPH_BUILD" = true ]; then
    pushd "${RES18_SW_DIR}"
    bash -c "source ${GRAPH_DRIVER} ${BOOTGEN}"
    popd

    # The graph drivers install their own design's metadata now (see
    # sw/sync_graph_metadata.sh), so no codegen is needed here.
fi
CRTS=${RES18_SW_DIR}/../aie/${AIE_WORK_SUBDIR}/ps/c_rts

# Fail fast on a stale/wrong graph config. testkernel_zephyr and resnet18 share
# src/include/generated_graphs.h and example/build/baremetal_metadata.cpp (+
# ctrl_code_elf.cc), so building another design -- or skipping the graph build --
# leaves someone else's config installed. Two failure modes, both silent at boot:
# a config without 'compute_graph' makes init() fail and the A72 data-abort on a
# null buffer, while the 1- vs 10-instance configs BOTH register 'compute_graph'
# and instead hang the first inference in a DMA mask-poll. So compare against
# this design's own copy byte for byte rather than just checking the name.
GENERATED_GRAPHS=${AIEBAREMETAL_DIR}/src/include/generated_graphs.h
GRAPH_META=${AIEBAREMETAL_DIR}/example/build/baremetal_metadata.cpp
if [ -f "${CRTS}/baremetal_metadata.cpp" ]; then
    if ! cmp -s "${CRTS}/baremetal_metadata.cpp" "${GRAPH_META}"; then
        echo "graph metadata installed for another design; reinstalling ${AIE_WORK_SUBDIR}"
        if ! bash "${RES18_SW_DIR}/sync_graph_metadata.sh" \
                  "${RES18_SW_DIR}/../aie/${AIE_WORK_SUBDIR}"; then
            echo "ERROR: graph metadata sync failed" >&2
            exit 1
        fi
    fi
elif ! grep -q "compute_graph" "${GENERATED_GRAPHS}" 2>/dev/null; then
    echo "ERROR: graph config does not register 'compute_graph' (stale, from" >&2
    echo "       another design, or graph build skipped). Checked:" >&2
    echo "         ${GENERATED_GRAPHS}" >&2
    echo "         ${GRAPH_META}" >&2
    echo "       Re-run without -no_graph_build." >&2
    exit 1
fi

# zephyr.cmake needs src/ctrl_code_elf.cc, which example/Makefile generates and
# git does not track; only the testkernel build produces it, so a fresh clone
# fails in cmake. The Makefile target name comes from $(PWD), hence example/../src.
CTRL_CODE_CC=${AIEBAREMETAL_DIR}/src/ctrl_code_elf.cc
if [ ! -s "${CTRL_CODE_CC}" ]; then
    pushd ${AIEBAREMETAL_DIR}/example
    make "$PWD/../src/ctrl_code_elf.cc"
    popd
    if [ ! -s "${CTRL_CODE_CC}" ]; then
        echo "ERROR: failed to generate ${CTRL_CODE_CC}" >&2
        exit 1
    fi
fi

setup_zephyr() {
    ZEPHYR_DIR=${AIEBAREMETAL_DIR}/zephyr_dir
    if ! ${AIEBAREMETAL_DIR}/script/setup_zephyr.sh --target_dir ${ZEPHYR_DIR}; then
        echo "ERROR: Zephyr workspace setup failed; see errors above." >&2
        exit 1
    fi
}

build_system_top_dts() {
    if [ ! -d "${SDT_CHROOT}" ]; then
        mkdir -p "${SDT_CHROOT}"
    fi
    if [ ! -d "${AIEBAREMETAL_DIR}/thirdparty/system-device-tree-xlnx" ]; then
         pushd "${AIEBAREMETAL_DIR}/thirdparty/"
         git clone https://github.com/AMD-AECG-SSW-PUBLIC/system-device-tree-xlnx.git
         popd
    fi
    ${AIEBAREMETAL_DIR}/script/gensdt.sh \
        --chroot "${SDT_CHROOT}" \
        --xsa "${XSA}" \
        --branch master-next \
        --repo "${AIEBAREMETAL_DIR}/thirdparty/system-device-tree-xlnx" \
        --origin origin \
        --board ${SDT_GEN_BOARD}
    SYSTEM_TOP_DTS="${SDT_CHROOT}/latest/system-top.dts"
}

build_zephyr() {
    if [ -z "$ZEPHYR_DIR" ] || [ ! -d "$ZEPHYR_DIR" ]; then
        echo "Error: Zephyr directory '$ZEPHYR_DIR' not set or missing." >&2
        exit 1
    fi
    # Without this, an incomplete workspace surfaces as a pile of west
    # "unknown command" errors instead of naming the cause.
    for required in .west .venv/bin/activate zephyr; do
        if [ ! -e "${ZEPHYR_DIR}/${required}" ]; then
            echo "Error: '${ZEPHYR_DIR}' is not a complete Zephyr workspace (no ${required})." >&2
            echo "       Re-run: ${AIEBAREMETAL_DIR}/script/setup_zephyr.sh --target_dir ${ZEPHYR_DIR}" >&2
            exit 1
        fi
    done

    pushd "$ZEPHYR_DIR" || exit 1
    source .venv/bin/activate
    # FindZephyr-sdk.cmake dereferences ZEPHYR_TOOLCHAIN_VARIANT unguarded, so an
    # unset variant aborts cmake ("Unknown arguments specified") instead of
    # defaulting. setup_zephyr.sh installs the Zephyr SDK, so name it here.
    export ZEPHYR_TOOLCHAIN_VARIANT=${ZEPHYR_TOOLCHAIN_VARIANT:-zephyr}
    pushd zephyr || exit 1
    rm build -rf
    if [ "$WEST_UPDATE" = true ]; then
        west update
        west lopper-install
        LOPPER_DTC_FLAGS="-b 0 -@" west lopper-command -p ${LOPPER_PROC} -s "$SYSTEM_TOP_DTS" -w .
    fi
    west build -b ${ZBOARD} ${CURRDIR} --pristine -- -DDTC_OVERLAY_FILE=${AIE_OVERLAY} -DAIE_GEN=${AIE_GEN} -DNUM_INST=${NUM_INST} ${BENCH_CMAKE_ARGS}

    if [ ! -f build/zephyr/zephyr.elf ]; then
        echo "ERROR: west build did not produce build/zephyr/zephyr.elf. See errors above." >&2
        exit 1
    fi
    popd # zephyr
    deactivate
    popd # "$ZEPHYR_DIR"
    echo "Zephyr build completed successfully."
}

build_bootgen_vek280() {
    # Single .BIN for xsdb "device program": PLM (PDI) loads platform + AIE design
    # (aie_cdo_*), then Zephyr @EL3. Reuse the committed resnet18 BIF, swapping the
    # baremetal a72 partition for zephyr.elf. Run bootgen from the BIF's own dir so
    # its relative PDI/CDO paths (../../Work/..., thirdparty/arch) stay valid.
    ZEPHYR_ELF=${ZEPHYR_DIR}/zephyr/build/zephyr/zephyr.elf
    BIF_DIR=${RES18_SW_DIR}/mladfpatch/baremetal
    cp ${ZEPHYR_ELF} ${BIF_DIR}/zephyr.elf
    cp ${BIF_DIR}/vek280_boot_image.bif ${BIF_DIR}/vek280_boot_image_zephyr.bif
    sed -i "s|{ core=a72-0, file=./vek280.elf }|{ core=a72-0, exception_level=el-3, file=./zephyr.elf }|" ${BIF_DIR}/vek280_boot_image_zephyr.bif

    source /proj/xbuilds/2026.1_daily_latest/installs/lin64/2026.1/Vitis/settings64.sh
    mkdir -p ${CURRDIR}/build
    pushd ${BIF_DIR}
    bootgen -arch versal -image vek280_boot_image_zephyr.bif -o ${CURRDIR}/build/Vek280_zephyr.BIN -w
    popd
}

if [ -z "$ZEPHYR_DIR" ]; then
    setup_zephyr
fi

if [ ! -f "$SYSTEM_TOP_DTS" ]; then
    build_system_top_dts
fi

build_zephyr
build_bootgen_vek280
