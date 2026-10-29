#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
AIE_GEN=1
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
echo ${AIE_GE}
bash -c "source ./envaie20231.sh;make compile  CFLAGS='-rdynamic' AIE_GEN=${AIE_GEN} -j32"
pushd .
cd ../
make ./build/libaiebaremetal.so -j32
popd
