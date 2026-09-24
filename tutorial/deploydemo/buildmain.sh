#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#   -new_api    Build New (class-based) API host (host_new_api.cpp -> host_new_api.o)

BUILDMAIN_ARGS=("$@")
set --
PETALINUX_SETTINGS="/proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh"
if [[ -f "$PETALINUX_SETTINGS" ]]; then
    source "$PETALINUX_SETTINGS"
fi
set -- "${BUILDMAIN_ARGS[@]}"
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
    aarch64-none-elf-gcc -I./ -I./dependency/aie -I./dependency/psv_cortexa72_0/include -I./ -mcpu=cortex-a72 -g -c -std=c++17 -DAEG_INHERITANCE_BASED_API -o host_new_api.o ./host_new_api.cpp
else
    aarch64-none-elf-gcc -I./ -I./dependency/psv_cortexa72_0/include -mcpu=cortex-a72 -g -c -std=c++17 -o host.o ./host.cpp
fi
