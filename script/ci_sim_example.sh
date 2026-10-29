#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

set -uo pipefail

NAME="${1:?name required}"
REL_PATH="${2:?example path required}"
GEN="${3:?aie gen required}"
MODE="${4:?mode (sim|build) required}"
shift 4 || true
EXTRA_ARGS=("$@")

SCRIPT_DIR="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
# shellcheck source=ci_judge.sh
source "$SCRIPT_DIR/ci_judge.sh"
WORKDIR="${GITHUB_WORKSPACE:-$REPO_ROOT}"

RESULTS_DIR="$WORKDIR/sim-results"
mkdir -p "$RESULTS_DIR"
LOG="$RESULTS_DIR/${NAME}.txt"
: > "$LOG"

EXAMPLE_DIR="$REPO_ROOT/example/$REL_PATH"

log() { echo "$@" | tee -a "$LOG"; }

fail() {
    log "RESULT: FAIL ${NAME} :: $*"
    exit 1
}

log "==== ${NAME} (path=example/${REL_PATH}, gen=${GEN}, mode=${MODE}, extra='${EXTRA_ARGS[*]:-}') ===="

if [ ! -d "$EXAMPLE_DIR" ]; then
    fail "example directory not found: $EXAMPLE_DIR"
fi

set +u
source "$REPO_ROOT/script/settings.sh" 1 >>"$LOG" 2>&1 || true
set -u

if [ "${CI_SIM_SKIP_MAKE_CLEAN:-0}" != "1" ] && [ -f "$REPO_ROOT/example/Makefile" ]; then
    log "---- make clean (example/) ----"
    if ! ( cd "$REPO_ROOT/example" && make clean ) >>"$LOG" 2>&1; then
        log "WARNING: 'make clean' returned non-zero; continuing"
    fi
fi

cd "$EXAMPLE_DIR" || fail "cannot cd into $EXAMPLE_DIR"

rm -rf ./Work

log "---- build ----"
(
    set -x
    set +u
    source ./build.sh "$GEN" ${EXTRA_ARGS[@]+"${EXTRA_ARGS[@]}"}
) 2>&1 | tee -a "$LOG"
build_ret=${PIPESTATUS[0]}

if [ "$build_ret" -ne 0 ]; then
    log "build.sh exited with code $build_ret"
    fail "build failed"
fi

ci_assert_no_compile_failures "$LOG"

if [ ! -d ./Work ]; then
    fail "build produced no ./Work directory"
fi
log "build OK"

if [ "$MODE" = "build" ]; then
    log "RESULT: PASS ${NAME} (build-only)"
    exit 0
fi

if [ ! -f ./Work/config/scsim_config.json ]; then
    log "no Work/config/scsim_config.json -> nothing to simulate"
    log "RESULT: PASS ${NAME} (build-only)"
    exit 0
fi

log "---- sim ----"
SIM_LOG="$RESULTS_DIR/${NAME}.sim.txt"
: > "$SIM_LOG"
stdbuf -oL -eL bash -lc "source \"$REPO_ROOT/script/runsim.sh\"" 2>&1 | tee -a "$SIM_LOG" | tee -a "$LOG"
sim_ret=${PIPESTATUS[0]}

ci_assert_no_compile_failures "$SIM_LOG"

if [ "$sim_ret" -ne 0 ] && grep -qE 'Simulation Finished, Sim result: 0' "$SIM_LOG"; then
    log "Simulator reported 'Sim result: 0'; ignoring launcher exit code $sim_ret"
    sim_ret=0
fi

failing_tests=()
mapfile -t failing_tests < <(grep -iE 'test.*fail|fail.*test' "$SIM_LOG" 2>/dev/null) || true
if [ "${#failing_tests[@]}" -gt 0 ]; then
    log "Failing test lines detected:"
    printf '  %s\n' "${failing_tests[@]}" | tee -a "$LOG"
    fail "simulation reported failing test(s)"
fi

if grep -qiE '(^|[[:space:]])Test Passed([[:space:]]|$|\()' "$SIM_LOG"; then
    log "RESULT: PASS ${NAME} (Test Passed)"
    exit 0
fi

if [ "$sim_ret" -eq 139 ]; then
    fail "simulator SIGSEGV (exit 139)"
fi

if [ "$sim_ret" -ne 0 ]; then
    fail "simulator exit code $sim_ret"
fi

log "RESULT: PASS ${NAME} (sim completed, no errors)"
exit 0
