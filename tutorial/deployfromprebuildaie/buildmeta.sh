# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#pushd ../
#./generate_json_c.sh --work_folder ./deploydemo/Work/
#mv ./baremetal_metadata.cpp ./deploydemo/
#popd
_TUTORIAL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "${_TUTORIAL_DIR}/vitis_env.sh" || return 1 2>/dev/null || exit 1
if ! command -v aarch64-none-elf-gcc &>/dev/null; then
    echo "Error: aarch64-none-elf-gcc not found. Source Vitis settings64.sh first." >&2
    return 1 2>/dev/null || exit 1
fi
aarch64-none-elf-gcc -mcpu=cortex-a72 -g -c -std=c++17 -o aiemetadata.o ./dependency/baremetal_metadata.cpp
