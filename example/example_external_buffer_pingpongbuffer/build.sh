#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Usage: source ./build.sh [options]
# Options:
#   <1|2|5>                 AIE generation (default: 2)
#   -bootgen                Enable bootgen
#   -rpu                    Build for RPU
#   -new_api                Use new inheritance-based API

set -e
CURRDIR=$PWD

AIE_GEN=2
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

echo ${AIE_GEN}
source $CURRDIR/../../script/settings.sh 1

if [ $AIE_GEN == 5 ]; then
    if [[ "$new_api" == true ]]; then
        $CURRDIR/../../script/compile.sh -new_api --runtime_source_file ./src/graph_new_api.cpp --partition_option "--enable-partition=0:12:pr0" --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
    else
        $CURRDIR/../../script/compile.sh --runtime_source_file ./src/graph.cpp --partition_option "--enable-partition=0:12:pr0" --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
    fi
else
    if [[ "$new_api" == true ]]; then
        $CURRDIR/../../script/compile.sh -new_api --runtime_source_file ./src/graph_new_api.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
    else
        $CURRDIR/../../script/compile.sh --runtime_source_file ./src/graph.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
    fi
fi
compile_status=$?
if [ $compile_status -ne 0 ]; then
    echo "Error: compile.sh failed with status $compile_status" >&2
    exit $compile_status
fi
echo "Build completed successfully with AIE_GEN=${AIE_GEN}, BOOTGEN=${BOOTGEN}, RPUFLAG=${RPUFLAG}, new_api=${new_api}"
