#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

usage() {
  echo "Usage: $0 [object_file]"
  echo "  object_file: primary object to link (default: graph.o)"
  exit 1
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
fi

# Resolve script directory (optional; useful if you run this from elsewhere)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Optional parameter with default to graph.o
OBJ="${1:-graph.o}"

echo $OBJ
_saved_argv=("$@")
set --  # clears $1..$n for the current shell
# Source the environment
source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh

# Sanity check
if [[ ! -f "$OBJ" ]]; then
  echo "Error: object file '$OBJ' not found."
  echo "Tip: run '$0 -h' for help."
  exit 1
fi
if [[ ! -f "aiemetadata.o" ]]; then
  echo "Error: required object 'aiemetadata.o' not found in current directory."
  exit 1
fi

# Link
aarch64-none-elf-gcc \
  "$OBJ" aiemetadata.o \
  -mcpu=cortex-a78 \
  -Wl,-T -Wl,./lscript.ld \
  -L./ -L./dependency/arch/ps/cortexa78_0/standalone_cortexa78_0/bsp/lib/ \
  -laiebaremetal_api \
  -Wl,--start-group,-lxil,-lxilstandalone,-lxiltimer,-lgcc,-lc,-lstdc++,--end-group \
  -o main.elf

echo "Linked main.elf using $OBJ and aiemetadata.o"

#source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh
#aarch64-none-elf-gcc host.o aiemetadata.o -mcpu=cortex-a72 -Wl,-T -Wl,./lscript.ld -L./ -L./dependency/psv_cortexa72_0/lib/ -laiebaremetal_api -Wl,--start-group,-lxil,-lgcc,-lc,-lstdc++,--end-group -o main.elf
