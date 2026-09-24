#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
AIE_GEN=2
SCRIPT_DIR=$(dirname "$(realpath "$0")")
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
echo ${AIE_GE}
bash -c "source envaie2pst50.sh;make compile  CFLAGS='-rdynamic' AIE_GEN=${AIE_GEN} -j32"
pushd .
cd ../
#make ./build/libaiebaremetal.so AIE_KERNEL_LOC=$SCRIPT_DIR -j32
make  hw AIE_GEN=2 ./build/libaiebaremetal.so AIE_KERNEL_LOC=./example_auto_radar_2dfft -j32
popd
