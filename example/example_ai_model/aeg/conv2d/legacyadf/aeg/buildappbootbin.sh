#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
BAREMETAL_DIR=$(cd "$(pwd)/../../../../../../" && pwd)
BAREMETAL_TEST_DIR=$BAREMETAL_DIR/example/
ARCH_DIR=$BAREMETAL_DIR/thirdparty/arch/ps/
ARCH_72_DIR=$ARCH_DIR/psv_cortexa72_0/
sysrootinclude="$ARCH_72_DIR/include"
sysrootlib="$ARCH_72_DIR/lib"
ELF=vek280.elf
runtime_source_file=./src/graph.cpp
#source ../../../../../../script/settings.sh 1
source ./envaie2pst50.sh
#build host
aarch64-none-elf-gcc -D__AIE_ARCH__=20 -mcpu=cortex-a72 -Wl,-T -Wl,$BAREMETAL_DIR/thirdparty/arch/ps/psv_cortexa72_0/lscript.ld -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/fal/build/src/include/ -I/$BAREMETAL_DIR/include -I$sysrootinclude -L$sysrootlib -L$BAREMETAL_TEST_DIR/build/ -o ./${ELF} $runtime_source_file -Wl,--start-group,-laiebaremetala72,-lxil,-lgcc,-lc,-lstdc++,--end-group
ln -s $BAREMETAL_DIR/thirdparty/arch ./arch
/proj/xbuilds/2022.2_daily_latest/installs/lin64/Vitis/2022.2/bin/bootgen -arch versal -image vek280_boot_image.bif -o boot.bin -w