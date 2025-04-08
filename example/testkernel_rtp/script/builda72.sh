#!/bin/bash
# Copyright (C) 2025 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may
# not use this file except in compliance with the License. A copy of the
# License is located at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

# Source the environment variables
AIE_GEN=1
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
#clear the parameter number to avoid enclose script inherit the param and get confuse
set --

echo ${AIE_GEN}

HW_GEN=$(python3 vercheck.py)

if [ ${AIE_GEN} != ${HW_GEN}  ]; then
	echo "HW gen is ${HW_GEN} but request to build AIE_GEN ${AIE_GEN}, info mismatch"
	return
fi

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

#build the vek280/a72 app
source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh
aarch64-none-elf-gcc -mcpu=cortex-a72 -Wl,-T -Wl,$ARCH_72_DIR/lscript.ld -I../../thirdparty/aielib/aie-rt/driver/internal/ -I$ARCH_72_DIR/include/ -L$ARCH_72_DIR/lib/ -L../../build/ -o ./${ELF} ../src/graph.cpp -Wl,--start-group,-laiebaremetala72,-lxil,-lgcc,-lc,-lstdc++,--end-group
#generate the boot.bin
#FIX ME SSIT device AIE_GEN is 7 8 but it is AIE 1
/proj/xbuilds/2022.2_daily_latest/installs/lin64/Vitis/2022.2/bin/bootgen -arch versal -image ${BIF} -o ${BOOTBIN} -w
mv ./${BOOTBIN} ../../build/
