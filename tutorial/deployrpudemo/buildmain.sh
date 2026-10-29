#!/usr/bin/env bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#   -new_api    Build New (class-based) API host (host_new_api.cpp -> host_new_api.o)

BUILDMAIN_ARGS=("$@")
set --
_TUTORIAL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "${_TUTORIAL_DIR}/vitis_env.sh" || return 1 2>/dev/null || exit 1
set -- "${BUILDMAIN_ARGS[@]}"
if ! command -v armr5-none-eabi-g++ &>/dev/null; then
    echo "Error: armr5-none-eabi-g++ not found. Source Vitis settings64.sh first." >&2
    return 1 2>/dev/null || exit 1
fi

new_api=false
while [[ $# -gt 0 ]]; do
    case "$1" in
        -new_api)
            new_api=true
            shift
            ;;
        *)
            echo "Unknown option: $1" >&2
            return 1 2>/dev/null || exit 1
            ;;
    esac
done

# Take the API headers from the BSP that gensdtbsp.sh built, so the host is compiled
# against the same version of the API that libaiebaremetal.a provides.
BSP_INC="./dependency/integration_flow/test_bsp/include"
BSP_AIE_INC="./dependency/integration_flow/test_bsp/libsrc/aiebaremetal/src/include"

if [[ ! -d "$BSP_AIE_INC" ]]; then
    echo "Error: BSP headers not found at $BSP_AIE_INC. Run gensdtbsp.sh first." >&2
    return 1 2>/dev/null || exit 1
fi

if [[ "$new_api" == true ]]; then
    armr5-none-eabi-g++ -I${BSP_AIE_INC} -I${BSP_INC} -I./ -I./dependency/aie -I./dependency/psv_cortexr5_0/include -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5 -g -c -std=c++17 -DAEG_INHERITANCE_BASED_API -o host_new_api.o ./host_new_api.cpp
else
    armr5-none-eabi-g++ -I${BSP_AIE_INC} -I${BSP_INC} -I./ -I./dependency/psv_cortexr5_0/include -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5 -g -c -std=c++17 -o host.o ./host.cpp
fi
