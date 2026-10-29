#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# QEMU Automation Test for testkernel_rtp example
# Builds with build.sh 2 qemu, runs in QEMU, verifies "Test Passed"
# Requires: Vitis, qemu-system-aarch64. Set QEMU_DTB to device tree path if needed.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKDIR="$SCRIPT_DIR/../example/testkernel_rtp"
TEST_NAME="${TEST_NAME:-testkernel_rtp QEMU Test}"
RUNTIME_SOURCE="${RUNTIME_SOURCE:-./src/graph.cpp}"
COMPILE_API_FLAG="${COMPILE_API_FLAG:-}"

# Jenkins-friendly stages: search console for [STAGE] or FAIL AREA
qemu_log_stage() {
    local n="$1"
    local total="$2"
    local title="$3"
    echo ""
    echo "================================================================================"
    echo "[STAGE $n/$total] $title"
    echo "================================================================================"
}
qemu_log_hint() {
    echo "  [hint] $*"
}
# Allow up to 10 min for QEMU/launch on CI (slow nodes). Override with QEMU_TIMEOUT env if needed.
QEMU_TIMEOUT="${QEMU_TIMEOUT:-600}"
# ELF name depends on platform (compile.sh sets vck190.elf or vek280.elf); detect after build
ELF_NAME=""

echo "========================================"
echo "$TEST_NAME"
echo "========================================"
echo "Date: $(date)"
echo "WORKDIR: $WORKDIR"
echo "QEMU_TIMEOUT: ${QEMU_TIMEOUT}s (override with env QEMU_TIMEOUT)"
echo ""

qemu_log_stage 1 6 "Environment — Vitis and QEMU binary"
qemu_log_hint "FAIL AREA: Vitis not installed at expected path, or qemu-system-aarch64 missing on agent PATH."
# Govind: this path sets PATH for qemu — always use it when present
VITIS_ROOT="${VITIS_ROOT:-/proj/xbuilds/2026.2_daily_latest/installs/lin64/2026.2/Vitis}"
VITIS_SETUP="$VITIS_ROOT/settings64.sh"

source $SCRIPT_DIR/../script/settings.sh 1
source "$SCRIPT_DIR/qemu_hw_emu_common.sh"

if [ -f "$VITIS_SETUP" ]; then
    echo "Sourcing Vitis environment ($VITIS_SETUP)..."
    source "$VITIS_SETUP"
    export XILINX_VITIS_AIETOOLS="$XILINX_VITIS/aietools"
elif [ -z "$XILINX_VITIS" ]; then
    echo "Sourcing Vitis environment..."
    source "$VITIS_SETUP"
fi

if ! require_qemu_bin "$XILINX_VITIS"; then
    echo "ERROR: qemu-system-aarch64 not found."
    echo "  [FAIL AREA: STAGE 1] Vitis was sourced (XILINX_VITIS=${XILINX_VITIS:-unset}) but QEMU not found."
    echo "Install QEMU on the Jenkins agent, e.g.: sudo apt-get install -y qemu-system-arm"
    echo "Then ensure qemu-system-aarch64 is on PATH when this script runs."
    echo "FAIL: $TEST_NAME"
    exit 1
fi

cd "$WORKDIR"

# Verbose logs by default (full build + emulator via tee) so Jenkins shows where failures occur.
# Set JENKINS_QEMU_QUIET=1 to restore quiet CI mode if step logs hit FileLogStorage EOF / size limits.
BUILD_QUIET=false
QEMU_QUIET=false
if [ -n "${JENKINS_QEMU_QUIET:-}" ] && [ "${JENKINS_QEMU_QUIET}" != "0" ]; then
    BUILD_QUIET=true
    QEMU_QUIET=true
fi
echo "Log mode: BUILD_QUIET=$BUILD_QUIET QEMU_QUIET=$QEMU_QUIET (JENKINS_QEMU_QUIET=${JENKINS_QEMU_QUIET:-unset})"

qemu_log_stage 2 6 "Build — inlined build.sh 2 qemu (Vitis 2026.2)"
qemu_log_hint "FAIL AREA: compile/link errors, missing Vitis libs, or wrong example dir."
run_inline_build() {
    local compile_api_args=()
    if [ -n "$COMPILE_API_FLAG" ]; then
        compile_api_args+=("$COMPILE_API_FLAG")
    fi
    source ../../script/settings.sh 1
    if [ -f "$VITIS_SETUP" ]; then
        source "$VITIS_SETUP"
        export XILINX_VITIS_AIETOOLS="$XILINX_VITIS/aietools"
    fi
    echo "  v++ Vitis: ${XILINX_VITIS:-unset}"
    source compile.sh "${compile_api_args[@]}" --runtime_source_file "$RUNTIME_SOURCE" --aie_version 2 --hw_qemu
}
if [ "$BUILD_QUIET" = true ]; then
    BUILD_LOG="${TMPDIR:-/tmp}/qemu_build_$$.log"
    if ! run_inline_build >> "$BUILD_LOG" 2>&1; then
        echo "ERROR: Build failed. Last 300 lines of build log:"
        tail -n 300 "$BUILD_LOG"
        echo "FAIL: $TEST_NAME"
        exit 1
    fi
    echo "Build completed (full log in $BUILD_LOG)."
else
    if ! run_inline_build; then
        echo "ERROR: Build failed"
        echo "FAIL: $TEST_NAME"
        exit 1
    fi
fi
echo "  Build step finished OK."

qemu_log_stage 3 6 "Artifacts — ELF and device tree"
qemu_log_hint "FAIL AREA: build did not produce vck190.elf or vek280.elf, or sim DTB missing for direct QEMU."
for e in vck190.elf vek280.elf; do
    if [ -f "$e" ]; then
        ELF_NAME="$e"
        break
    fi
done
if [ -z "$ELF_NAME" ] || [ ! -f "$ELF_NAME" ]; then
    echo "ERROR: No ELF found after build (looked for vck190.elf, vek280.elf)"
    echo "  [FAIL AREA: STAGE 3]"
    echo "FAIL: $TEST_NAME"
    exit 1
fi
echo "  ELF: $ELF_NAME"

# DTB required for arm-generic-fdt; use generated one if QEMU_DTB not set
if [ -z "$QEMU_DTB" ]; then
    for d in sim/behav_waveform/xsim/board-versal-xcve2802-ps-cosim-vitis-virt.dtb sim/behav_waveform/xsim/board-versal-ps-cosim-vitis-vck190.dtb; do
        if [ -f "$WORKDIR/$d" ]; then
            QEMU_DTB="$WORKDIR/$d"
            break
        fi
    done
fi
if [ -n "$QEMU_DTB" ] && [ -f "$QEMU_DTB" ]; then
    echo "  DTB: $QEMU_DTB"
else
    echo "  DTB: (none — OK if using launch_hw_emu.sh only)"
fi
if [ -f "$WORKDIR/launch_hw_emu.sh" ]; then
    echo "  launch_hw_emu.sh: present (full hw_emu: QEMU + PMC + xsim)"
else
    echo "  launch_hw_emu.sh: absent — will use direct qemu-system-aarch64 (Versal may need -machine-path; see script comments)"
fi

# Step 3: Run QEMU / hw_emu
# Write log to TMPDIR (or /tmp) to avoid filling Jenkins workspace (No space left on device)
QEMU_LOG_DIR="${TMPDIR:-/tmp}"
QEMU_LOG="$QEMU_LOG_DIR/qemu_test_$$.log"

# Prefer launch_hw_emu.sh when present (full hw_emu: QEMU + PMC + xsim with correct chardev/machine-path).
# Direct qemu-system-aarch64 fails on Versal with "Missing chardesc prop. Forgot -machine-path?"
# Run in background and pass as soon as "Test Passed" is seen, then kill emulator—avoids Jenkins timeout
# while emulator takes minutes to shut down after "Disconnected".
# Verbose vs quiet: BUILD_QUIET/QEMU_QUIET set near start of script (JENKINS_QEMU_QUIET=1 for quiet).
EARLY_PASS=false
[ "$QEMU_QUIET" = true ] && set +x
if [ -f "$WORKDIR/launch_hw_emu.sh" ]; then
    qemu_log_stage 4 6 "Emulator — start hw_emu (launch_hw_emu.sh)"
    qemu_log_hint "FAIL AREA: QEMU/xsim/PMC never ready, wrong boot files, or hang before serial output."
    qemu_log_hint "This stage wraps ./launch_hw_emu.sh with timeout ${QEMU_TIMEOUT}s; log file: $QEMU_LOG"
    qemu_log_hint "We stop the wrapper as soon as 'Test Passed' appears (emulator may otherwise run until timeout)."
    [ "$QEMU_QUIET" = true ] && echo "  Quiet mode: most emulator output only in $QEMU_LOG (periodic status here)."

    setsid bash -c "cd '$WORKDIR' && exec timeout $QEMU_TIMEOUT ./launch_hw_emu.sh 2>&1" > "$QEMU_LOG" 2>&1 &
    EMU_PID=$!
    EMU_PGID=$(ps -o pgid= -p "$EMU_PID" 2>/dev/null | tr -d ' ')
    [ -z "$EMU_PGID" ] && EMU_PGID="$EMU_PID"
    echo "  Emulator started (PID $EMU_PID, PGID $EMU_PGID); output -> $QEMU_LOG"
    # Verbose: stream the log live to the CI console (replaces old tee).
    TAIL_PID=""
    if [ "$QEMU_QUIET" != true ]; then
        tail -n +1 -f "$QEMU_LOG" 2>/dev/null &
        TAIL_PID=$!
    fi
    stop_emulator() {
        kill -TERM "-$EMU_PGID" 2>/dev/null || true
        for _ in 1 2 3 4 5 6; do
            kill -0 "-$EMU_PGID" 2>/dev/null || break
            sleep 0.5
        done
        kill -KILL "-$EMU_PGID" 2>/dev/null || true
        [ -n "$TAIL_PID" ] && kill "$TAIL_PID" 2>/dev/null || true
    }
    POLL_START=$(date +%s)
    POLL_END=$((POLL_START + QEMU_TIMEOUT + 120))
    LAST_STATUS=0
    while [ "$(date +%s)" -lt "$POLL_END" ]; do
        sleep 10
        if [ -f "$QEMU_LOG" ] && grep -q "Test Passed" "$QEMU_LOG" 2>/dev/null; then
            echo "  ✓ 'Test Passed' seen in $QEMU_LOG — stopping emulator pipeline."
            EARLY_PASS=true
            break
        fi
        if [ -f "$WORKDIR/qemu_output.log" ] && grep -q "Test Passed" "$WORKDIR/qemu_output.log" 2>/dev/null; then
            echo "  ✓ 'Test Passed' seen in qemu_output.log — stopping emulator pipeline."
            EARLY_PASS=true
            break
        fi
        if ! kill -0 "-$EMU_PGID" 2>/dev/null; then
            echo "  Emulator (PGID $EMU_PGID) exited before early-pass detection."
            break
        fi
        ELAPSED=$(($(date +%s) - POLL_START))
        # Quiet: status every 60s. Verbose: lighter heartbeat every 120s (tee already streams output).
        if [ "$QEMU_QUIET" = true ]; then
            if [ $((ELAPSED - LAST_STATUS)) -ge 60 ]; then
                echo "  ... still running ${ELAPSED}s — waiting for 'Test Passed' or process exit (full log: $QEMU_LOG)"
                LAST_STATUS=$ELAPSED
            fi
        else
            if [ $((ELAPSED - LAST_STATUS)) -ge 120 ]; then
                echo "  ... still running ${ELAPSED}s — waiting for 'Test Passed' or pipeline exit"
                LAST_STATUS=$ELAPSED
            fi
        fi
    done
    if [ "$EARLY_PASS" != true ] && [ "$(date +%s)" -ge "$POLL_END" ]; then
        echo "  [WARN] Poll window ended (~$((POLL_END - POLL_START))s after start); stopping pipeline."
        qemu_log_hint "FAIL AREA: hung boot, xsim/QEMU stuck, or test never printed 'Test Passed'."
    fi
    echo "  Stopping emulator process group (PGID $EMU_PGID)..."
    stop_emulator
    wait "$EMU_PID" 2>/dev/null || true
    echo "  Emulator stop complete."
    if [ -f "$WORKDIR/qemu_output.log" ]; then
        echo "  Merging qemu_output.log into combined log for verification."
        cat "$WORKDIR/qemu_output.log" >> "$QEMU_LOG"
    fi
else
    qemu_log_stage 4 6 "Emulator — direct qemu-system-aarch64 (no launch_hw_emu.sh)"
    qemu_log_hint "FAIL AREA: Versal may need launch_hw_emu.sh (-machine-path); missing DTB; or QEMU args."
    echo "  Log: $QEMU_LOG  timeout: ${QEMU_TIMEOUT}s"
    QEMU_ARGS=(
        -M arm-generic-fdt
        -serial null -serial null -serial mon:stdio
        -device "loader,file=$ELF_NAME,cpu-num=0"
        -device loader,addr=0xFD1A0300,data=0x8000000e,data-len=4
        -m 512M
        -display none
    )
    if [ -n "$QEMU_DTB" ] && [ -f "$QEMU_DTB" ]; then
        QEMU_ARGS+=(-hw-dtb "$QEMU_DTB")
    fi
    if timeout "$QEMU_TIMEOUT" $QEMU_BIN "${QEMU_ARGS[@]}" 2>&1 | tee "$QEMU_LOG"; then
        true
    else
        exitcode=$?
        if [ $exitcode -eq 124 ]; then
            echo "  [WARN] QEMU timed out after ${QEMU_TIMEOUT}s (exit 124). Check log for 'Test Passed' or hang."
            qemu_log_hint "FAIL AREA: slow boot, infinite loop, or wrong machine/DTB for direct QEMU."
        fi
    fi
fi

qemu_log_stage 5 6 "Verify — search for 'Test Passed'"
qemu_log_hint "FAIL AREA: test failed on target, wrong image, or output went only to a file we did not merge."
if [ "$EARLY_PASS" = true ]; then
    echo "  ✓ Test Passed (detected during emulator run)"
    SUCCESS=true
elif [ -f "$QEMU_LOG" ] && grep -q "Test Passed" "$QEMU_LOG" 2>/dev/null; then
    echo "  ✓ Found 'Test Passed' in combined QEMU log"
    SUCCESS=true
elif [ -f "$WORKDIR/qemu_output.log" ] && grep -q "Test Passed" "$WORKDIR/qemu_output.log" 2>/dev/null; then
    echo "  ✓ Found 'Test Passed' in qemu_output.log"
    SUCCESS=true
else
    echo "  ✗ 'Test Passed' not found in QEMU log or qemu_output.log"
    SUCCESS=false
fi

qemu_log_stage 6 6 "Archive — save last 200 lines to workspace"
# Keep only last 200 lines in workspace for archive (avoids "No space left on device" in Jenkins)
if [ -f "$QEMU_LOG" ]; then
    tail -n 200 "$QEMU_LOG" > "$WORKDIR/qemu_test_tail.txt" 2>/dev/null || true
    echo "  Wrote $WORKDIR/qemu_test_tail.txt (tail of run; full temp log removed)"
    rm -f "$QEMU_LOG"
else
    echo "  No temp QEMU log file (direct path may have used tee to workspace only)."
fi
if [ "$SUCCESS" != true ]; then
    qemu_log_hint "Inspect example/testkernel_rtp/qemu_test_tail.txt and Jenkins console above [STAGE 4]."
fi

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
