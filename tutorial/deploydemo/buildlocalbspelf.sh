#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#   -new_api

BUILDELF_ARGS=("$@")
set --
PETALINUX_SETTINGS="/proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh"
[[ -f "$PETALINUX_SETTINGS" ]] && source "$PETALINUX_SETTINGS"
set -- "${BUILDELF_ARGS[@]}"
if ! command -v aarch64-none-elf-gcc &>/dev/null; then
    echo "Error: aarch64-none-elf-gcc not found. Source Petalinux or Vitis settings, or set PATH to your ARM toolchain." >&2
    exit 1
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
            exit 1
            ;;
    esac
done

if [[ "$new_api" == true ]]; then
    aarch64-none-elf-gcc host_new_api.o aiemetadata.o -mcpu=cortex-a72 -Wl,-T -Wl,./lscript.ld -L./ -L./dependency/psv_cortexa72_0/lib/ -laiebaremetal_api -Wl,--start-group,-lxil,-lgcc,-lc,-lstdc++,--end-group -o main_newapi.elf
else
    aarch64-none-elf-gcc host.o aiemetadata.o -mcpu=cortex-a72 -Wl,-T -Wl,./lscript.ld -L./ -L./dependency/psv_cortexa72_0/lib/ -laiebaremetal_api -Wl,--start-group,-lxil,-lgcc,-lc,-lstdc++,--end-group -o main.elf
fi
