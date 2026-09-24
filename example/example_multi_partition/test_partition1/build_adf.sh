#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
AIE_GEN=1
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
echo ${AIE_GEN}
bash -c "source envaie2pst50.sh;make compile  CFLAGS='-rdynamic' AIE_GEN=${AIE_GEN} -j32"

