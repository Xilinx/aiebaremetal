#!/bin/bash
# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
AIE_GEN=2

export ARCH_DIR=`pwd`/../../arch/
export KERNEL_DIR=$ARCH_DIR/kernels/
export ARCH_72_DIR=$ARCH_DIR/psv_cortexa72_0/
(
  source $ARCH_DIR/env.sh
)

if [ ${AIE_GEN} -ge 2 ]; then
	BIF=vek280_boot_image.bif
	BOOTBIN=Vek280BOOT.BIN
	ELF=vek280.elf
else
	BIF=vck190_boot_image.bif
	BOOTBIN=Vck190BOOT.BIN
	ELF=vck190.elf
fi

#rebuild the adf with 2023.1, as we used the prebuild libxil.a which include aiedriver and is 2023.1
#source /proj/xbuilds/2023.1_daily_latest/installs/lin64/Vitis/2023.1/settings64.sh
#source /proj/petalinux/2023.1/petalinux-v2023.1_daily_latest/tool/petalinux-v2023.1-final/settings.sh
pushd .
cd ../
#source ../envaie20231.sh
#source ./build2023.1.sh 2
source ./envaie2pst50.sh
source ./build.sh 2
cd ../
make apuhw
popd
#build run time
#build the vek280/a72 app
aarch64-none-elf-gcc -mcpu=cortex-a72 -D__PS_BARE_METAL__ -D__AIEBAREMETAL__ -D__AIE_ARCH__=20 -std=c++17 -Wl,-T -Wl,$ARCH_72_DIR/lscript.ld -I../../thirdparty/aielib/aie-rt/driver/internal/ -I$ARCH_72_DIR/include/ -I${XILINX_VITIS}/aietools/include -I./ -L$ARCH_72_DIR/lib/ -o ./${ELF} ../src/graph.adf.cpp ../Work/ps/c_rts/aie_control.cpp -L${XILINX_VITIS}/aietools/lib/aarchnone64.o -ladf_api -Wl,--start-group,-lxil,-lgcc,-lc,-lstdc++,--end-group
#generate the boot.bin
#FIX ME SSIT device AIE_GEN is 7 8 but it is AIE 1
${XILINX_VITIS}/bin/bootgen -arch versal -image ${BIF} -o ${BOOTBIN} -w
mv ./${BOOTBIN} ../../build/
