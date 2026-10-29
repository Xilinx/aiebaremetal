#!/usr/bin/env bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

_TUTORIAL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "${_TUTORIAL_DIR}/vitis_env.sh" || return 1 2>/dev/null || exit 1
if ! command -v armr5-none-eabi-g++ &>/dev/null; then
    echo "Error: armr5-none-eabi-g++ not found. Source Vitis settings64.sh first." >&2
    return 1 2>/dev/null || exit 1
fi
armr5-none-eabi-g++ -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -DARMR5 -g -c -std=c++17 -o aiemetadata.o ./dependency/baremetal_metadata.cpp
