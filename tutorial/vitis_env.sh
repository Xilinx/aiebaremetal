#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Tutorial scripts use the Vitis already in the environment.
# Do not pin 2026.1, 2026.2, or HEAD — source settings64.sh first.

if [[ -z "${XILINX_VITIS:-}" ]]; then
    echo "Error: XILINX_VITIS is not set. Source Vitis settings64.sh first." >&2
    return 1 2>/dev/null || exit 1
fi

if [[ ! -d "$XILINX_VITIS" ]]; then
    echo "Error: XILINX_VITIS is not a directory: $XILINX_VITIS" >&2
    return 1 2>/dev/null || exit 1
fi

# Same-tree embeddedsw as the sourced Vitis (this is what must match aiemetacpp.py).
: "${EMBEDDEDSW:=${XILINX_VITIS}/data/embeddedsw}"
