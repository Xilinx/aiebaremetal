#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

ci_fail() {
    echo "$*" >&2
    exit 1
}

ci_compile_failure_lines() {
    local log="$1"
    grep -E ':[0-9]+:([0-9]+:)? error:|fatal error:|make: \*\*\*|Error: .*failed' "$log" 2>/dev/null \
        | grep -Ev '0 errors|ERROR:0' || true
}

ci_assert_no_compile_failures() {
    local log="$1"
    local hits
    hits="$(ci_compile_failure_lines "$log")"
    if [ -n "$hits" ]; then
        echo "Compile/make failures in ${log}:" >&2
        echo "$hits" >&2
        ci_fail "Log contains compile/make failures; ignoring later success messages."
    fi
}

ci_assert_build_log() {
    local log="$1"
    local ret="$2"
    [ -f "$log" ] || ci_fail "Missing log: $log"
    ci_assert_no_compile_failures "$log"
    if [ "$ret" -ne 0 ]; then
        ci_fail "Build command exited with code $ret."
    fi
    if ! grep -q "Build completed successfully" "$log"; then
        ci_fail "Log does not contain 'Build completed successfully'."
    fi
}

ci_sim_crash_lines() {
    local log="$1"
    grep -nE 'core dumped|Segmentation fault|TclStackFree: incorrect freePtr|Tcl_AppendLimitedToObj called with shared object' \
        "$log" 2>/dev/null | head -5 || true
}

ci_assert_sim_log() {
    local log="$1"
    local ret="$2"
    local success_re="${3:-}"
    [ -n "$success_re" ] || success_re='(^|[[:space:]])Test Passed([[:space:]]|$|\()'

    [ -f "$log" ] || ci_fail "Missing log: $log"
    ci_assert_no_compile_failures "$log"

    local crash
    crash="$(ci_sim_crash_lines "$log")"
    if [ -n "$crash" ]; then
        echo "Simulator crash in ${log}:" >&2
        echo "$crash" >&2
        ci_fail "Simulator crashed during the run."
    fi

    if grep -qiE 'test.*fail|fail.*test' "$log"; then
        echo "Failing test lines in ${log}:" >&2
        grep -iE 'test.*fail|fail.*test' "$log" >&2 || true
        ci_fail "Simulation log reports a failing test."
    fi
    if ! grep -qiE "$success_re" "$log"; then
        ci_fail "Simulation log does not match success marker: ${success_re}"
    fi
    if ! grep -qE 'Simulation Finished, Sim result: 0' "$log"; then
        ci_fail "Simulation log does not contain 'Simulation Finished, Sim result: 0'."
    fi
    if [ "$ret" -ne 0 ]; then
        ci_fail "Simulation launcher exited with code $ret."
    fi
}
