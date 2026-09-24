#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
SCRIPT_DIR=$(dirname "$(realpath "$0")")
AIE_GEN=1
FORCEBUILDLIB=1
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
if [ $# -gt 1 ]; then
	FORCEBUILDLIB=$2
fi

echo ${AIE_GE}
bash -c "source envaie20231.sh;make  compile  CFLAGS='-rdynamic' AIE_GEN=${AIE_GEN} -j32"
pushd .
cd ../
#default rebuild libaiebaremetal.so
if [[ ${FORCEBUILDLIB} -eq 1 ]]; then
	make targetforce AIE_KERNEL_LOC=$SCRIPT_DIR AIE_GEN=${AIE_GEN} -j32
fi

popd
