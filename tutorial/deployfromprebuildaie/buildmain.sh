#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

usage() {
  echo "Usage: $0 <source.cpp> [extra compiler flags]"
  echo "Example: $0 my_app.cpp -O2 -DDEBUG"
  exit 1
}

# Help
if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
fi

SRC=""
if [[ "$#" -ge 1 ]]; then
  SRC="$1"
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

shift || true  # shift so any remaining args are extra flags

if [[ ! -f "$SRC" ]]; then
  SRC="${SCRIPT_DIR}/../../example/example_oob/src/graph.cpp"
fi

# Derive object name from source (e.g., host.cpp -> host.o)
BASE="$(basename "$SRC")"
OBJ="${BASE%.*}.o"

INCLUDE_XAIE="../../thirdparty/aielib/aie-rt/driver/internal/"
INCLUDE_BSP="../../thirdparty/arch/ps/cortexa78_0/standalone_cortexa78_0/bsp/include/"

# Source the PetaLinux environment
# Adjust the path if your environment differs
source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh

# Compile
# Note: using aarch64-none-elf-gcc for C++; consider aarch64-none-elf-g++ if you need C++ standard library linking.
aarch64-none-elf-gcc -I../../src/include -I${INCLUDE_XAIE} -I${INCLUDE_BSP} -mcpu=cortex-a72 -g -c -std=c++17 -o "$OBJ" "$SRC" "$@"

echo "Built $OBJ"

#source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh
#aarch64-none-elf-gcc -I../../src/include -mcpu=cortex-a72 -g -c -std=c++17 -o host.o ./host.cpp
