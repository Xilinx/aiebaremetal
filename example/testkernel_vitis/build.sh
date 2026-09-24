#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ./build.sh 2 -bootgen -rpu
set -e
CURRDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

AIE_GEN=1
RPUFLAG=
BOOTGEN=
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
if [ $# -gt 1 ]; then
	BOOTGEN=$2
	#-bootgen
fi
if [ $# -gt 2 ]; then
	RPUFLAG=$3
	#-rpu
fi


echo ${AIE_GEN}
source $CURRDIR/../../script/settings.sh 1
$CURRDIR/../../script/compile.sh --compiler v++ --partition_option "--enable-partition=0:12:pr0" --runtime_source_file ./src/graph.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
compile_status=$?
if [ $compile_status -ne 0 ]; then
    echo "Error: compile.sh failed with status $compile_status" >&2
    exit $compile_status
fi
echo "Build completed successfully with AIE_GEN=${AIE_GEN}, BOOTGEN=${BOOTGEN}, RPUFLAG=${RPUFLAG}"
