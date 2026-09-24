#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
AIE_GEN=5
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
#clear the parameter number to avoid enclose script inherit the param and get confuse
set --

echo ${AIE_GEN}

HW_GEN=$(python3 vercheck.py 2>/dev/null) || true

if [[ -z "${HW_GEN}" ]]; then
	echo "WARNING: vercheck.py did not produce hw_gen (need ../Work/ps/c_rts/aie_control_config.json from a successful aiecompiler run)." >&2
elif [[ "${AIE_GEN}" != "${HW_GEN}" ]]; then
	echo "HW gen is ${HW_GEN} but request to build AIE_GEN ${AIE_GEN}, info mismatch" >&2
	return 1
fi

export ARCH_DIR=`pwd`/../../../thirdparty/arch/ps/
export ARCH_78_DIR=$ARCH_DIR/cortexa78_0/
#(
#  source ../../../script/env.sh
#)


BIF=vek385.bif
BOOTBIN=Vek385PLIOBOOT.BIN
ELF=vek385.elf

# compile plio kernels and link the platform
pushd .
cd ../pl_kernels
source ./build.sh ${AIE_GEN} || {
	popd
	return 1
}
popd

if [ ! -f "../pl_kernels/new.xsa" ]; then
	echo "ERROR: Missing ../pl_kernels/new.xsa after pl_kernels build." >&2
	return 1
fi

# generate platform
export PFM_NAME="pfm_baremetal"
#generate-platform.sh -name $PFM_NAME -hw ../pl_kernels/new.xsa -domain psv_cortexa72_0:standalone -domain ai_engine:aie_runtime
vitis -s platform_baremetal.py --xsa ../pl_kernels/new.xsa || return 1

#build the a78 app for vek385
#source /proj/petalinux/2024.2/petalinux-v2024.2_daily_latest/tool/petalinux-v2024.2-final/settings.sh
#export SW_DOMAIN=./$PFM_NAME/export/$PFM_NAME/sw/$PFM_NAME/standalone_domain

export SW_DOMAIN=./workspace/platform_baremetal/cortexa78_0/standalone_cortexa78_0/bsp
if [ ! -f "${SW_DOMAIN}/include/xil_cache.h" ]; then
	echo "ERROR: Missing ${SW_DOMAIN}/include/xil_cache.h after platform generation." >&2
	return 1
fi
aarch64-none-elf-gcc -mcpu=cortex-a78 -Wl,-T -Wl,$ARCH_78_DIR//standalone_cortexa78_0/bsp/lscript.ld -D__AIE_ARCH__=22 -I../../../thirdparty/aielib/aie-rt/driver/internal/ -I../../../src/include/ -I$XILINX_VITIS/aietools/include/ -I$SW_DOMAIN/include -L$SW_DOMAIN/lib -L../../build/ -o ./${ELF} ../src/graph.cpp -Wl,--start-group,-laiebaremetala78,-lxil,-lxiltimer,-lxilstandalone,-lgcc,-lc,-lstdc++,--end-group
if [ ! -f "./${ELF}" ]; then
	echo "ERROR: Failed to build ${ELF}." >&2
	return 1
fi
#aarch64-none-elf-gcc -mcpu=cortex-a78 -Wl,-T -Wl,$ARCH_78_DIR//standalone_cortexa78_0/bsp/lscript.ld -I../../../thirdparty/aielib/aie-rt/driver/internal/ -I../../../include/ -I$XILINX_VITIS/aietools/include/ -I$SW_DOMAIN/include -L$ARCH_78_DIR/standalone_cortexa78_0/bsp/lib -L../../build/ -o ./${ELF} ../src/graph.cpp -Wl,--start-group,-laiebaremetala78,-lxil,-lgcc,-lc,-lstdc++,--end-group

#generate the boot.bin
#FIX ME SSIT device AIE_GEN is 7 8 but it is AIE 1
if [ ! -f "../pl_kernels/_x/link/vivado/vpl/prj/prj.runs/impl_1/vitis_design_wrapper_boot.pdi" ]; then
	echo "ERROR: Missing linked PDI at ../pl_kernels/_x/link/vivado/vpl/prj/prj.runs/impl_1/vitis_design_wrapper_boot.pdi." >&2
	return 1
fi

BOOTGEN_BIN="${XILINX_VITIS}/bin/bootgen"
if [ ! -x "${BOOTGEN_BIN}" ]; then
	BOOTGEN_BIN="/proj/xbuilds/2025.1_daily_latest/installs/lin64/2025.1/Vitis/bin/bootgen"
fi
if [ ! -x "${BOOTGEN_BIN}" ]; then
	echo "ERROR: bootgen executable not found." >&2
	return 1
fi

"${BOOTGEN_BIN}" -arch versal_2ve_2vm -image ${BIF} -o ${BOOTBIN} -w || return 1
if [ ! -f "./${BOOTBIN}" ]; then
	echo "ERROR: Failed to generate ${BOOTBIN}." >&2
	return 1
fi
mv ./${BOOTBIN} ../../build/
