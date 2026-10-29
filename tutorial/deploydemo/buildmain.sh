#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#   -new_api    Build New (class-based) API host (host_new_api.cpp -> host_new_api.o)
#   -local      Build against the repo headers, for the libaiebaremetal_api.a that
#               build_baremetal.sh compiles (pair with buildlocalbspelf.sh)

BUILDMAIN_ARGS=("$@")
set --
_TUTORIAL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "${_TUTORIAL_DIR}/vitis_env.sh" || return 1 2>/dev/null || exit 1
set -- "${BUILDMAIN_ARGS[@]}"
if ! command -v aarch64-none-elf-gcc &>/dev/null; then
    echo "Error: aarch64-none-elf-gcc not found. Source Vitis settings64.sh first." >&2
    return 1 2>/dev/null || exit 1
fi

new_api=false
local_lib=false
while [[ $# -gt 0 ]]; do
    case "$1" in
        -new_api)
            new_api=true
            shift
            ;;
        -local)
            local_lib=true
            shift
            ;;
        *)
            echo "Unknown option: $1" >&2
            return 1 2>/dev/null || exit 1
            ;;
    esac
done

# Take the API headers from whichever library the ELF will link against, otherwise the
# host is built against one version of the API and linked against another.
BSP_INC="./dependency/integration_flow/test_bsp/include"
BSP_AIE_INC="./dependency/integration_flow/test_bsp/libsrc/aiebaremetal/src/include"
REPO_INC="../../src/include"

if [[ "$local_lib" == true ]]; then
    # buildlocalbspelf.sh links libaiebaremetal_api.a, built from these sources.
    if [[ ! -d "$REPO_INC" ]]; then
        echo "Error: repo headers not found at $REPO_INC" >&2
        return 1 2>/dev/null || exit 1
    fi
    API_INC="-I${REPO_INC}"
else
    # buildsdtbspelf.sh links libaiebaremetal.a, built by gensdtbsp.sh from these sources.
    if [[ ! -d "$BSP_AIE_INC" ]]; then
        echo "Error: BSP headers not found at $BSP_AIE_INC" >&2
        echo "Run gensdtbsp.sh first, or pass -local to build against the repo sources." >&2
        return 1 2>/dev/null || exit 1
    fi
    API_INC="-I${BSP_AIE_INC} -I${BSP_INC}"
fi

if [[ "$new_api" == true ]]; then
    aarch64-none-elf-gcc ${API_INC} -I./ -I./dependency/aie -I./dependency/psv_cortexa72_0/include -mcpu=cortex-a72 -g -c -std=c++17 -DAEG_INHERITANCE_BASED_API -o host_new_api.o ./host_new_api.cpp
else
    aarch64-none-elf-gcc ${API_INC} -I./ -I./dependency/psv_cortexa72_0/include -mcpu=cortex-a72 -g -c -std=c++17 -o host.o ./host.cpp
fi
