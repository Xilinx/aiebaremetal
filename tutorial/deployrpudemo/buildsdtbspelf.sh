# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

#!/usr/bin/env bash
#   -new_api

BSP_LIB_DIR="./dependency/integration_flow/test_bsp/lib"
# Check BSP lib dir
if [[ ! -d "$BSP_LIB_DIR" ]]; then
  echo "Error: BSP library directory not found: $BSP_LIB_DIR" >&2
  echo "Please ensure the BSP is generated/populated before building." >&2
  exit 1
fi
BUILDELF_ARGS=("$@")
set --
PETALINUX_SETTINGS="/proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh"
[[ -f "$PETALINUX_SETTINGS" ]] && source "$PETALINUX_SETTINGS"
set -- "${BUILDELF_ARGS[@]}"
if ! command -v armr5-none-eabi-gcc &>/dev/null; then
    echo "Error: armr5-none-eabi-gcc not found. Source Petalinux or Vitis settings, or set PATH to your ARM toolchain." >&2
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
    armr5-none-eabi-g++ host_new_api.o aiemetadata.o -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5 -Wl,-T -Wl,./lscript.ld -L./ -L${BSP_LIB_DIR} -laiebaremetal -Wl,--start-group,-lxil,-lxilpm,-lxiltimer,-lxilstandalone,-lgcc,-lc,-lstdc++,--end-group -o main_newapi.elf
else
    armr5-none-eabi-g++ host.o aiemetadata.o -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5 -Wl,-T -Wl,./lscript.ld -L./ -L${BSP_LIB_DIR} -laiebaremetal -Wl,--start-group,-lxil,-lxilpm,-lxiltimer,-lxilstandalone,-lgcc,-lc,-lstdc++,--end-group -o main.elf
fi
