#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

KERNELPLIO_WORK_DIR="$(dirname "$(realpath -e "${BASH_SOURCE[0]}")")"

VITIS_SETUP="${VITIS_SETUP:-/proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/settings64.sh}"

MODE="full"
AIE_GEN="2"
NEW_API_FLAG=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        --host-only) MODE="host-only";        shift ;;
        -new_api)    NEW_API_FLAG="-new_api"; shift ;;
        [0-9]*)      AIE_GEN="$1";            shift ;;
        *) echo "Unknown option: $1" >&2; return 1 2>/dev/null || exit 1 ;;
    esac
done

HOST_LABEL="${NEW_API_FLAG:+graph_new_api.cpp (aeg::pl_kernel API)}${NEW_API_FLAG:-graph.cpp (legacy)}"

echo "========================================"
echo "example_kernelplio QEMU/hw_emu"
echo "MODE:    $MODE"
echo "AIE_GEN: $AIE_GEN"
echo "HOST:    $HOST_LABEL"
echo "WORKDIR: $KERNELPLIO_WORK_DIR"
echo "========================================"

if [[ -f "$VITIS_SETUP" ]]; then
    echo "Sourcing: $VITIS_SETUP"
    set +u
    source "$VITIS_SETUP"
    set -u
elif [[ -z "${XILINX_VITIS:-}" ]]; then
    echo "ERROR: XILINX_VITIS not set and not found: $VITIS_SETUP" >&2
    return 1 2>/dev/null || exit 1
else
    echo "Using pre-set XILINX_VITIS=$XILINX_VITIS"
fi

export PL_TARGET="hw_emu"

case "$MODE" in
    full)
        echo ""
        echo "--- buildall.sh $AIE_GEN $NEW_API_FLAG qemu ---"
        cd "$KERNELPLIO_WORK_DIR"
        source buildall.sh "$AIE_GEN" $NEW_API_FLAG qemu
        ;;
    host-only)
        echo ""
        echo "--- host-only: recompile ELF + repackage ---"
        cd "$KERNELPLIO_WORK_DIR/script"
        source builda72.sh "$AIE_GEN" $NEW_API_FLAG
        cd "$KERNELPLIO_WORK_DIR"
        ;;
esac

HW_EMU_SCRIPT=""
for candidate in "$KERNELPLIO_WORK_DIR/launch_hw_emu.sh" \
                 "$KERNELPLIO_WORK_DIR/pl_kernels/package.hw_emu/launch_hw_emu.sh" \
                 "$KERNELPLIO_WORK_DIR/pl_kernels/launch_hw_emu.sh"; do
    if [[ -f "$candidate" ]]; then
        HW_EMU_SCRIPT="$candidate"
        break
    fi
done

if [[ -z "$HW_EMU_SCRIPT" ]]; then
    echo "ERROR: launch_hw_emu.sh not found after build." >&2
    echo "  Expected at $KERNELPLIO_WORK_DIR/launch_hw_emu.sh" >&2
    echo "  Re-run without --host-only to do a full build first." >&2
    return 1 2>/dev/null || exit 1
fi

echo ""
echo "--- launch hw_emu: $HW_EMU_SCRIPT ---"
HW_EMU_DIR="$(dirname "$HW_EMU_SCRIPT")"
cd "$HW_EMU_DIR"
source launch_hw_emu.sh
