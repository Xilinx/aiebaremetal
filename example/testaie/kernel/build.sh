#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ./build.sh 2 -bootgen -rpu
set -e
CURRDIR=$PWD

check_status() {
    status=$1
    cmd_desc=$2
    
    if [ $status -ne 0 ]; then
        echo "Error: ${cmd_desc:-Command} failed with exit code $status" >&2
        exit $status
    fi
    
    return 0
}

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
#set this is is local env
source $CURRDIR/../../../script/settings.sh 1

if [ $AIE_GEN == 5 ]; then
	$CURRDIR/../../../script/compile.sh --runtime_source_file ./src/graph.cpp --partition_option "--enable-partition=0:12:pr0" --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
	check_status $? "compile.sh with AIE_GEN=${AIE_GEN}, BOOTGEN=${BOOTGEN}, RPUFLAG=${RPUFLAG}"
else
	$CURRDIR/../../../script/compile.sh --runtime_source_file ./src/graph.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
	check_status $? "compile.sh with AIE_GEN=${AIE_GEN}, BOOTGEN=${BOOTGEN}, RPUFLAG=${RPUFLAG}"
fi

echo "Build completed successfully with AIE_GEN=${AIE_GEN}, BOOTGEN=${BOOTGEN}, RPUFLAG=${RPUFLAG}"