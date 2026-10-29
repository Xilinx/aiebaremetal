#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
BAREMETAL_DIR=$(cd "$(pwd)/../.." && pwd)
BAREMETAL_TEST_DIR=$BAREMETAL_DIR/example/
ARCH_DIR=$BAREMETAL_DIR/thirdparty/arch/ps/
ARCH_53_DIR=$ARCH_DIR/psv_cortexr5_0/
sysrootinclude="$ARCH_53_DIR/include"
sysrootlib="$ARCH_53_DIR/lib"
ELF=vek280.elf
runtime_source_file=./src/graph.cpp
source ../../script/settings.sh 1
#build host
armr5-none-eabi-g++ -D__AIE_ARCH__=20 -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -D_RPU_ -Wl,-T -Wl,$BAREMETAL_DIR/thirdparty/arch/ps/psv_cortexr5_0/lscript.ld -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/fal/build/src/include/ -I/$BAREMETAL_DIR/include -I$sysrootinclude -L$sysrootlib -L$BAREMETAL_TEST_DIR/build/ -o ./${ELF} $runtime_source_file -Wl,--start-group,-laiebaremetalr53,-lxil,-lgcc,-lc,-lstdc++,--end-group
/proj/xbuilds/2022.2_daily_latest/installs/lin64/Vitis/2022.2/bin/bootgen -arch versal -image vek280_rpu_boot_image.bif -o boot.bin -w
