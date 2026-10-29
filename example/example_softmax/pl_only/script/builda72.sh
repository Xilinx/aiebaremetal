#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
AIE_GEN=1

# Parse args:
#   <1|2|5>     positional AIE generation (default: 1)
while [[ $# -gt 0 ]]; do
	case "$1" in
		*)
			if [[ $1 =~ ^[0-9]+$ ]]; then
				AIE_GEN=$1
				shift
			else
				echo "Unknown option: $1" >&2
				return 1 2>/dev/null || exit 1
			fi
			;;
	esac
done
#clear the parameter number to avoid enclose script inherit the param and get confuse
set --

echo "AIE_GEN=${AIE_GEN}"

# Same release pl_kernels/build.sh derives, for the stamp comparison below.
VITIS_REL="$(basename "$(dirname "${XILINX_VITIS:?XILINX_VITIS is not set - source script/settings.sh first}")")"

HW_GEN=$(python3 vercheck.py 2>/dev/null) || true

if [[ -z "${HW_GEN}" ]]; then
	echo "WARNING: vercheck.py did not produce hw_gen (need ../Work/ps/c_rts/aie_control_config.json from a successful aiecompiler run)." >&2
elif [[ "${AIE_GEN}" != "${HW_GEN}" ]]; then
	echo "HW gen is ${HW_GEN} but request to build AIE_GEN ${AIE_GEN}, info mismatch" >&2
	return 1
fi

export ARCH_DIR=`pwd`/../../../../thirdparty/arch/ps/
export ARCH_72_DIR=$ARCH_DIR/psv_cortexa72_0/

if [ ${AIE_GEN} -ge 2 ]; then
	BIF=vek280_boot_image.bif
	BOOTBIN=Vek280SoftmaxPLBOOT.BIN
	ELF=vek280.elf
else
	BIF=vck190_boot_image.bif
	BOOTBIN=Vck190SoftmaxPLBOOT.BIN
	ELF=vck190.elf
fi

# compile plio kernels and link the platform
pushd .
cd ../pl_kernels
STAMP_FILE="./.built_for_aie_gen"
# Must match what pl_kernels/build.sh stamps.
STAMP_EXPECTED="${AIE_GEN} ${VITIS_REL}"
pl_kernels_rebuilt=false
if [[ -f "./new.xsa" ]] && [[ -f "$STAMP_FILE" ]] && [[ "$(cat "$STAMP_FILE")" == "$STAMP_EXPECTED" ]]; then
    echo "  Skipping pl_kernels/build.sh — new.xsa already present and built for '${STAMP_EXPECTED}' (rm pl_kernels/new.xsa to force a v++ rebuild)."
else
    # Either no cached new.xsa, no stamp at all (e.g. artifacts left over
    # from before this stamping fix existed), or a stamp for a different
    # AIE_GEN (e.g. a prior VEK385 build left a new.xsa that VEK280 must not
    # reuse — the two target different platforms/.xpfm and produce
    # incompatible PDIs). In every case here we're about to rebuild, so wipe
    # v++/Vivado's incremental build state first — build.sh never relies on
    # incremental v++ reuse anyway, and this guarantees the new v++ -l run
    # can't pick up cached checkpoints from a different platform (same
    # artifact list as this repo's documented "leave untracked" set for
    # pl_kernels/).
    if [[ -f "./new.xsa" ]] || [[ -d "_x" ]]; then
        echo "  Cleaning stale pl_kernels build state (new.xsa missing/stale for '${STAMP_EXPECTED}') before rebuild."
        rm -rf _x .Xil .ipcache analyzer_input extracted drivers
        rm -f ./*.xo ./*.xo.compile_summary new.xsa new.xsa.link_summary vpl_gen_fixed.isd vpl_gen_fixed.pdi
    fi
    source ./build.sh ${AIE_GEN} || { popd; return 1; }
    pl_kernels_rebuilt=true
fi
popd

# Generate the BSP with the Vitis Python flow, as builda78.sh already does.
# It replaces generate-platform.sh, which needs XSCT -- removed in Vitis 2026.1.
export PFM_NAME="platform_baremetal"
export PFM_CPU="psv_cortexa72_0"
export SW_DOMAIN="./workspace/$PFM_NAME/$PFM_CPU/standalone_$PFM_CPU/bsp"
# Regenerate whenever pl_kernels was rebuilt: "BSP exists" says nothing about
# whether new.xsa's contents changed.
if [[ "$pl_kernels_rebuilt" == true ]] || [[ ! -f "$SW_DOMAIN/include/xil_cache.h" ]]; then
    vitis -s platform_baremetal.py --xsa ../pl_kernels/new.xsa --cpu "$PFM_CPU"
    if [[ $? -ne 0 ]]; then
        echo "ERROR: platform_baremetal.py failed - $PFM_NAME BSP was not created." >&2
        return 1 2>/dev/null || exit 1
    fi
else
    echo "  Skipping platform generation — $PFM_NAME BSP already present."
fi

if [[ ! -f "$SW_DOMAIN/include/xil_cache.h" ]]; then
    echo "ERROR: $PFM_NAME BSP still missing after platform generation - aborting before host ELF compile." >&2
    return 1 2>/dev/null || exit 1
fi

#build the a72 app for vck190/vek280
source /proj/petalinux/2024.2/petalinux-v2024.2_daily_latest/tool/petalinux-v2024.2-final/settings.sh

# Host source: src/graph_new_api.cpp (typed AEG API). AEG_INHERITANCE_BASED_API
# makes aiebaremetal.h pull in generated_graphs.h.
HOST_SRC=../src/graph_new_api.cpp
HOST_DEFS=-DAEG_INHERITANCE_BASED_API
# aiemetacpp.py wrote generated_graphs.h into example/ during `make hw`;
# stage it on the baremetal include path so aiebaremetal.h finds it.
GENERATED_GRAPHS_SRC=../../../generated_graphs.h
if [ ! -f "$GENERATED_GRAPHS_SRC" ]; then
	echo "ERROR: $GENERATED_GRAPHS_SRC not found - 'make hw' must run before builda72.sh." >&2
	return 1 2>/dev/null || exit 1
fi
cp "$GENERATED_GRAPHS_SRC" ../../../../src/include/generated_graphs.h

# The Python-flow BSP uses include/ and lib/, and splits libxil.a three ways.
aarch64-none-elf-gcc $HOST_DEFS -mcpu=cortex-a72 -Wl,-T -Wl,$ARCH_72_DIR/lscript.ld -I../../../../thirdparty/aielib/aie-rt/driver/internal/ -I../../../../src/include/ -I$XILINX_VITIS/aietools/include/ -I$SW_DOMAIN/include -L$SW_DOMAIN/lib -L../../../build/ -o ./${ELF} $HOST_SRC -Wl,--start-group,-laiebaremetala72,-lxil,-lxiltimer,-lxilstandalone,-lgcc,-lc,-lstdc++,-lm,--end-group
HOSTELF_RC=$?
if [[ $HOSTELF_RC -ne 0 ]]; then
    echo "ERROR: host ELF compile failed - not running bootgen against a stale ./${ELF}." >&2
    return 1 2>/dev/null || exit 1
fi

if [ "${PL_TARGET:-hw}" != "hw_emu" ]; then
    /proj/xbuilds/2022.2_daily_latest/installs/lin64/Vitis/2022.2/bin/bootgen -arch versal -image ${BIF} -o ${BOOTBIN} -w
    mkdir -p ../build
    mv ./${BOOTBIN} ../build/
else
    echo "  Skipping bootgen."
fi
