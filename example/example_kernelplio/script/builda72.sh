#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
AIE_GEN=1
new_api=false

# Parse args:
#   <1|2|5>     positional AIE generation (default: 1)
#   -new_api    compile ../src/graph_new_api.cpp (typed AEG API) instead of
#               the legacy ../src/graph.cpp, and define AEG_INHERITANCE_BASED_API
#               so aiebaremetal.h pulls in generated_graphs.h.
while [[ $# -gt 0 ]]; do
	case "$1" in
		-new_api)
			new_api=true
			shift
			;;
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

echo "AIE_GEN=${AIE_GEN}, new_api=${new_api}"

HW_GEN=$(python3 vercheck.py 2>/dev/null) || true

if [[ -z "${HW_GEN}" ]]; then
	echo "WARNING: vercheck.py did not produce hw_gen (need ../Work/ps/c_rts/aie_control_config.json from a successful aiecompiler run)." >&2
elif [[ "${AIE_GEN}" != "${HW_GEN}" ]]; then
	echo "HW gen is ${HW_GEN} but request to build AIE_GEN ${AIE_GEN}, info mismatch" >&2
	return 1
fi

export ARCH_DIR=`pwd`/../../../thirdparty/arch/ps/
export ARCH_72_DIR=$ARCH_DIR/psv_cortexa72_0/
#(
#  source ../../../script/env.sh
#)

if [ ${AIE_GEN} -ge 2 ]; then
	BIF=vek280_boot_image.bif
	BOOTBIN=Vek280PLIOBOOT.BIN
	ELF=vek280.elf
else
	BIF=vck190_boot_image.bif
	BOOTBIN=Vck190PLIOBOOT.BIN
	ELF=vck190.elf
fi

# compile plio kernels and link the platform
pushd .
cd ../pl_kernels
source ./build.sh ${AIE_GEN}
popd

export PFM_NAME="pfm_baremetal"
if [[ ! -d "./$PFM_NAME/export/$PFM_NAME/sw/$PFM_NAME/standalone_domain/bspinclude" ]]; then
    # Pin to Vitis 2025.2: rolling nightly builds may have XSCT disabled.
    #
    # Route through xvfb-run: xsct's own Xvfb auto-launch can exhaust its
    # scanned display range on this shared host.
    GEN_PLATFORM_CMD="source ../pl_kernels/env2025.sh && generate-platform.sh -name '$PFM_NAME' -hw ../pl_kernels/new.xsa -domain psv_cortexa72_0:standalone -domain ai_engine:aie_runtime"
    if command -v xvfb-run >/dev/null 2>&1; then
        xvfb-run -a bash -c "$GEN_PLATFORM_CMD"
    else
        bash -c "$GEN_PLATFORM_CMD"
    fi
    GENPLATFORM_RC=$?
    if [[ $GENPLATFORM_RC -ne 0 ]]; then
        echo "ERROR: generate-platform.sh failed - $PFM_NAME BSP was not created." >&2
        return 1 2>/dev/null || exit 1
    fi
else
    echo "  Skipping generate-platform.sh — $PFM_NAME BSP already present."
fi

if [[ ! -d "./$PFM_NAME/export/$PFM_NAME/sw/$PFM_NAME/standalone_domain/bspinclude" ]]; then
    echo "ERROR: $PFM_NAME BSP still missing after generate-platform.sh - aborting before host ELF compile." >&2
    return 1 2>/dev/null || exit 1
fi

#build the a72 app for vck190/vek280
source /proj/petalinux/2024.2/petalinux-v2024.2_daily_latest/tool/petalinux-v2024.2-final/settings.sh
# source /proj/petalinux/2026.1/petalinux-v2026.1_daily_latest/tool/petalinux-v2026.1-final/settings.sh
export SW_DOMAIN=./$PFM_NAME/export/$PFM_NAME/sw/$PFM_NAME/standalone_domain

# Pick host source + flags based on -new_api.
HOST_SRC=../src/graph.cpp
HOST_DEFS=
if [[ "$new_api" == true ]]; then
	HOST_SRC=../src/graph_new_api.cpp
	HOST_DEFS=-DAEG_INHERITANCE_BASED_API
	# aiemetacpp.py wrote generated_graphs.h into example/ during `make hw`;
	# stage it on the baremetal include path so aiebaremetal.h finds it.
	GENERATED_GRAPHS_SRC=../../generated_graphs.h
	if [ ! -f "$GENERATED_GRAPHS_SRC" ]; then
		echo "ERROR: $GENERATED_GRAPHS_SRC not found - 'make hw' must run before builda72.sh." >&2
		return 1 2>/dev/null || exit 1
	fi
	cp "$GENERATED_GRAPHS_SRC" ../../../src/include/generated_graphs.h
fi

aarch64-none-elf-gcc $HOST_DEFS -mcpu=cortex-a72 -Wl,-T -Wl,$ARCH_72_DIR/lscript.ld -I../../../thirdparty/aielib/aie-rt/driver/internal/ -I../../../src/include/ -I$XILINX_VITIS/aietools/include/ -I$SW_DOMAIN/bspinclude/include -L$SW_DOMAIN/bsplib/lib -L../../build/ -o ./${ELF} $HOST_SRC -Wl,--start-group,-laiebaremetala72,-lxil,-lgcc,-lc,-lstdc++,--end-group
HOSTELF_RC=$?
if [[ $HOSTELF_RC -ne 0 ]]; then
    echo "ERROR: host ELF compile failed - not running bootgen against a stale ./${ELF}." >&2
    return 1 2>/dev/null || exit 1
fi

if [ "${PL_TARGET:-hw}" != "hw_emu" ]; then
    /proj/xbuilds/2022.2_daily_latest/installs/lin64/Vitis/2022.2/bin/bootgen -arch versal -image ${BIF} -o ${BOOTBIN} -w
    mv ./${BOOTBIN} ../../build/
else
    echo "  Skipping bootgen."
fi

if [ "${PL_TARGET:-hw}" = "hw_emu" ]; then
    echo "Running v++ --package for hw_emu (generates launch_hw_emu.sh)..."
    pushd ../pl_kernels
    v++ -p -t hw_emu \
        -f new.xsa \
        --package.domain aiengine \
        --package.boot_mode sd \
        --package.image_format=fat32 \
        --advanced.param software.os=baremetal \
        --package.defer_aie_run \
        --package.ps_elf ../script/${ELF},a72-0 \
        --package.out_dir package.hw_emu \
        ../libadf.a
    if [ -f package.hw_emu/launch_hw_emu.sh ]; then
        cp package.hw_emu/launch_hw_emu.sh ../
        echo "  launch_hw_emu.sh -> $(realpath ..)/launch_hw_emu.sh"
    fi
    popd
fi
