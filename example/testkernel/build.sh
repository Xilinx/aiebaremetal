#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Usage: source ./build.sh [options]
# Options:
#   <1|2|5>                 AIE generation (default: 1)
#   -bootgen                Enable bootgen
#   -rpu                    Build for RPU
#   -hw_qemu                Build for HW QEMU
#   -new_api                Use new inheritance-based API
#   -aeg_debug              Define AEG_DEBUG for verbose AEG_LOG output (see script/compile.sh)
#   -linux                  Build Linux ELF (aarch64-linux-gnu) instead of baremetal
#   -fusa                   Compile aie-rt with -DXAIE_FEATURE_FUSA_LIBC (GCC,
#                           no armclang/real FuSa library)

set -e
CURRDIR=$PWD

AIE_GEN=1
RPUFLAG=
BOOTGEN=
HW_QEMU=
new_api=false
AEG_DEBUG_FLAG=
LINUX_FLAG=
FUSA_FLAG=

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
        -hw_qemu|--hw_qemu)
            HW_QEMU="--hw_qemu"
            shift
            ;;
        -new_api)
            new_api=true
            shift
            ;;
        -aeg_debug|--aeg_debug)
            AEG_DEBUG_FLAG="-aeg_debug"
            shift
            ;;
        -linux)
            LINUX_FLAG="-linux -hw_only"
            shift
            ;;
        -fusa|--fusa)
            FUSA_FLAG="-fusa"
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

echo "AIE_GEN=${AIE_GEN}"
source $CURRDIR/../../script/settings.sh 1

if [[ "$new_api" == true ]]; then
    $CURRDIR/../../script/compile.sh ${LINUX_FLAG} -new_api ${AEG_DEBUG_FLAG} ${FUSA_FLAG} --runtime_source_file ./src/graph_new_api.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN} ${HW_QEMU}
else
    $CURRDIR/../../script/compile.sh ${LINUX_FLAG} ${AEG_DEBUG_FLAG} ${FUSA_FLAG} --runtime_source_file ./src/graph.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN} ${HW_QEMU}
fi
compile_status=$?
if [ $compile_status -ne 0 ]; then
    echo "Error: compile.sh failed with status $compile_status" >&2
    exit $compile_status
fi

if [[ -z "$LINUX_FLAG" ]]; then
    if [[ "$new_api" == true ]]; then
        build_sim.sh --runtime_source_file ./src/graph_new_api.cpp -new_api
    else
        build_sim.sh --runtime_source_file ./src/graph.cpp
    fi
fi

echo "Build completed successfully with AIE_GEN=${AIE_GEN}, BOOTGEN=${BOOTGEN}, RPUFLAG=${RPUFLAG}, LINUX_FLAG=${LINUX_FLAG}, new_api=${new_api}, AEG_DEBUG_FLAG=${AEG_DEBUG_FLAG:-<unset>}, FUSA_FLAG=${FUSA_FLAG:-<unset>}"
