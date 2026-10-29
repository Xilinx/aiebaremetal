#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#   -new_api

BUILDELF_ARGS=("$@")
set --
_TUTORIAL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "${_TUTORIAL_DIR}/vitis_env.sh" || return 1 2>/dev/null || exit 1
set -- "${BUILDELF_ARGS[@]}"
if ! command -v aarch64-none-elf-gcc &>/dev/null; then
    echo "Error: aarch64-none-elf-gcc not found. Source Vitis settings64.sh first." >&2
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

if [[ "$new_api" == true ]]; then
    aarch64-none-elf-gcc host_new_api.o aiemetadata.o -mcpu=cortex-a72 -Wl,-T -Wl,./lscript.ld -L./ -L./dependency/psv_cortexa72_0/lib/ -laiebaremetal_api -Wl,--start-group,-lxil,-lgcc,-lc,-lstdc++,--end-group -o main_newapi.elf
else
    aarch64-none-elf-gcc host.o aiemetadata.o -mcpu=cortex-a72 -Wl,-T -Wl,./lscript.ld -L./ -L./dependency/psv_cortexa72_0/lib/ -laiebaremetal_api -Wl,--start-group,-lxil,-lgcc,-lc,-lstdc++,--end-group -o main.elf
fi
