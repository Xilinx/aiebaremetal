#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# Usage: source ./build.sh [options]
# Options:
#   <1|2|5>                 AIE generation (default: 1)
#   -bootgen                Enable bootgen
#   -rpu                    Build for RPU
#   -new_api                Use new inheritance-based API

set -e
CURRDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

AIE_GEN=1
RPUFLAG=
BOOTGEN=
new_api=false

while [[ $# -gt 0 ]]; do
	case "$1" in
		-bootgen)
			BOOTGEN="-bootgen"
			shift
			;;
		-rpu)
			RPUFLAG="-rpu"
			shift
			;;
		-new_api)
			new_api=true
			shift
			;;
		*)
			if [[ $1 =~ ^[0-9]+$ ]]; then
				AIE_GEN=$1
				shift
			else
				echo "Unknown option: $1" >&2
				exit 1
			fi
			;;
	esac
done

if [[ "$new_api" == true ]]; then
	NEW_API_FLAG=-new_api
	SOURCE_FILE=./src/graph_new_api.cpp
else
	NEW_API_FLAG=
	SOURCE_FILE=./src/graph.cpp
fi

source $CURRDIR/../../script/settings.sh 1
if [ $AIE_GEN == 5 ]; then
	$CURRDIR/../../script/compile.sh ${NEW_API_FLAG} --runtime_source_file ${SOURCE_FILE} --partition_option "--enable-partition=0:12:pr0" --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
else
	$CURRDIR/../../script/compile.sh ${NEW_API_FLAG} --runtime_source_file ${SOURCE_FILE} --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
fi

compile_status=$?
if [ $compile_status -ne 0 ]; then
    echo "Error: compile.sh failed with status $compile_status" >&2
    exit $compile_status
fi
echo "Build completed successfully with AIE_GEN=${AIE_GEN}, BOOTGEN=${BOOTGEN}, RPUFLAG=${RPUFLAG}, new_api=${new_api}"
