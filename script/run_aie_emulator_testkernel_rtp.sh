#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# AIE Emulator Automation Test for testkernel_rtp example
# This script builds and runs AIE simulation for testkernel_rtp and verifies pass/fail

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKDIR="$SCRIPT_DIR/../example/testkernel_rtp"
TEST_NAME="testkernel_rtp AIE Emulator Test"

echo "========================================"
echo "$TEST_NAME"
echo "========================================"
echo "Date: $(date)"
echo ""

# Ensure Vitis is sourced
if [ -z "$XILINX_VITIS" ]; then
    echo "Sourcing Vitis environment..."
    source /proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/settings64.sh
fi

# Check if aiesimulator is available
if ! command -v aiesimulator &> /dev/null; then
    echo "ERROR: aiesimulator command not found"
    echo "FAIL: $TEST_NAME"
    exit 1
fi

cd "$WORKDIR"

# Step 1: Build the AIE application
echo "Step 1: Building AIE application for testkernel_rtp..."
if ! source ./build.sh 2; then
    echo "ERROR: Build failed"
    echo "FAIL: $TEST_NAME"
    exit 1
fi

# Step 2: Run AIE simulation
echo ""
echo "Step 2: Running AIE simulation..."
if ! source runsim.sh 2>&1 | tee aie_sim_test.log; then
    echo "ERROR: AIE simulation failed"
    echo "FAIL: $TEST_NAME"
    exit 1
fi

# Step 3: Check for success indicators
echo ""
echo "Step 3: Verifying simulation results..."

# Check for aiesimulator output directory
if [ ! -d "aiesimulator_output" ]; then
    echo "ERROR: aiesimulator_output directory not found"
    echo "FAIL: $TEST_NAME"
    exit 1
fi

# Check the AIE run summary for CS_PASSED status
if [ -f "aiesimulator_output/default.aierun_summary" ]; then
    if grep -q '"state": "CS_PASSED"' aiesimulator_output/default.aierun_summary; then
        echo "✓ AIE simulation completed with CS_PASSED status"
        SUCCESS=true
    else
        echo "✗ AIE simulation did not pass"
        SUCCESS=false
    fi
else
    echo "ERROR: default.aierun_summary not found"
    SUCCESS=false
fi

# Check AIECompiler.log for errors (count actual error lines, not summary "ERROR:0")
if [ -f "AIECompiler.log" ]; then
    ERRORS=$(grep -c "ERROR: \[" AIECompiler.log 2>/dev/null || true)
    if [ "${ERRORS:-0}" -eq 0 ]; then
        echo "✓ No errors in AIECompiler.log"
    else
        echo "✗ Found $ERRORS error(s) in AIECompiler.log"
        SUCCESS=false
    fi
fi

# Final result
echo ""
echo "========================================"
if [ "$SUCCESS" = true ]; then
    echo "PASS: $TEST_NAME"
    echo "========================================"
    exit 0
else
    echo "FAIL: $TEST_NAME"
    echo "========================================"
    exit 1
fi
