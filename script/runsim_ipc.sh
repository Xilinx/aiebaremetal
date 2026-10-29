#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Run an AIE example via the IPC simulator flow.
#
# Usage: runsim_ipc.sh [<example_dir>] [--log] [--new-api]
#
#   --log       Write per-command IPC traces beside ipc_sim.log
#   --new-api   Build/rebuild ipc_app from graph_new_api.cpp (AEGGraph<>)
#   --hold N    Keep the simulator alive N seconds after the app finishes, so
#               the read-only debug socket stays open for live register reads
#
# Environment:
#   AEG_SIM_SOCKET_TIMEOUT  Seconds to wait for simulator IPC startup (default: 180)
#   AEG_PS_IPC_HOLD_SEC     Same as --hold (the flag wins if both are given)

set -euo pipefail

LOG_MODE=false
NEW_API=false
EXAMPLE_DIR=""
SOCKET_TIMEOUT="${AEG_SIM_SOCKET_TIMEOUT:-180}"
HOLD_SEC="${AEG_PS_IPC_HOLD_SEC:-0}"

if ! [[ "$SOCKET_TIMEOUT" =~ ^[1-9][0-9]*$ ]]; then
    echo "ERROR: AEG_SIM_SOCKET_TIMEOUT must be a positive integer." >&2
    exit 1
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        --log)      LOG_MODE=true; shift ;;
        --new-api)  NEW_API=true; shift ;;
        --hold)     HOLD_SEC="${2:-}"; shift 2 ;;
        --hold=*)   HOLD_SEC="${1#*=}"; shift ;;
        -*)         echo "Unknown flag: $1" >&2; exit 1 ;;
        *)          EXAMPLE_DIR="$1"; shift ;;
    esac
done

if ! [[ "$HOLD_SEC" =~ ^[0-9]+$ ]]; then
    echo "ERROR: --hold/AEG_PS_IPC_HOLD_SEC must be a non-negative integer." >&2
    exit 1
fi
export AEG_PS_IPC_HOLD_SEC="$HOLD_SEC"

if [ -z "$EXAMPLE_DIR" ]; then
    EXAMPLE_DIR="$(pwd)"
fi
EXAMPLE_DIR="$(cd "$EXAMPLE_DIR" && pwd)"

SCRIPT_REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [ -n "${BAREMETAL_DIR:-}" ] && [ -f "$BAREMETAL_DIR/script/runsim_ipc.sh" ]; then
    REPO_ROOT="$(cd "$BAREMETAL_DIR" && pwd)"
else
    REPO_ROOT="$SCRIPT_REPO_ROOT"
fi

IPC_DIR="$EXAMPLE_DIR/ipc"
LOG_FILE="${AEG_SIM_LOG:-$EXAMPLE_DIR/ipc_sim.log}"
SIM_PID_FILE="$EXAMPLE_DIR/ipc_sim.pid"

if $NEW_API; then
    echo "[runsim_ipc] --new-api: rebuilding sim artifacts with AEGGraph<> API..."
    "$REPO_ROOT/script/build_sim.sh" "$EXAMPLE_DIR" --new-api
fi

BUILD_ENV="$IPC_DIR/build_sim.env"
if [ ! -f "$BUILD_ENV" ]; then
    echo "ERROR: $BUILD_ENV not found." >&2
    echo "       Run build.sh first (which calls build_sim.sh automatically)," >&2
    echo "       or run: build_sim.sh $EXAMPLE_DIR" >&2
    exit 1
fi
source "$BUILD_ENV"

FULL_APP="$BUILD_SIM_FULL_APP"
LIBAIEBAREMETAL_IPC="$BUILD_SIM_LIBAIEBAREMETAL_IPC"

if [ ! -f "$FULL_APP" ]; then
    echo "ERROR: ipc_app not found at $FULL_APP" >&2
    echo "       Rerun build.sh (or build_sim.sh) to rebuild simulator artifacts." >&2
    exit 1
fi

if [ ! -f "$LIBAIEBAREMETAL_IPC" ]; then
    echo "ERROR: libaiebaremetal_ipc.so not found at $LIBAIEBAREMETAL_IPC" >&2
    echo "       Rerun build.sh (or build_sim.sh) to rebuild simulator artifacts." >&2
    exit 1
fi

if [ ! -d "$EXAMPLE_DIR/Work/ps/c_rts/systemC" ]; then
    echo "ERROR: Work/ not found in $EXAMPLE_DIR — run build.sh first." >&2
    exit 1
fi

export AEG_PS_IPC_SERVER=1
IPC_SOCKET_CREATED_BY_RUNSIM=false
if [ -n "${AEG_PS_IPC_SOCKET:-}" ]; then
    IPC_SOCKET="$AEG_PS_IPC_SOCKET"
else
    IPC_SOCKET="ipc/aeg_ps_ipc_${USER:-user}_$$.sock"
    IPC_SOCKET_CREATED_BY_RUNSIM=true
fi
export AEG_PS_IPC_SOCKET="$IPC_SOCKET"
case "$IPC_SOCKET" in
    /*) IPC_SOCKET_FILE="$IPC_SOCKET" ;;
    *) IPC_SOCKET_FILE="$EXAMPLE_DIR/$IPC_SOCKET" ;;
esac
cleanup_ipc_socket() {
    if [ "$IPC_SOCKET_CREATED_BY_RUNSIM" = true ]; then
        rm -f "$IPC_SOCKET_FILE"
    fi
}
trap cleanup_ipc_socket EXIT

if [ "$LOG_MODE" = true ]; then
    export AEG_IPC_LOG_SERVER="$EXAMPLE_DIR/ipc_server.log"
    export AEG_IPC_LOG="$EXAMPLE_DIR/ipc_client.log"
    echo "[runsim_ipc] Server IPC log -> $AEG_IPC_LOG_SERVER"
    echo "[runsim_ipc] Client IPC log -> $AEG_IPC_LOG"
fi

echo "[runsim_ipc] Starting simulator  (example: $(basename "$EXAMPLE_DIR"))"
echo "[runsim_ipc] Socket: $IPC_SOCKET"
echo "[runsim_ipc] Debug:  ${IPC_SOCKET}.dbg"
echo "[runsim_ipc] Log:    $LOG_FILE"
if [ "$HOLD_SEC" -gt 0 ]; then
    echo "[runsim_ipc] Hold:   ${HOLD_SEC}s after app exit (debug socket stays live)"
fi

(
    set +u
    cd "$EXAMPLE_DIR"
    source "$REPO_ROOT/script/envaie2pst50.sh"
    source "$REPO_ROOT/script/aieldpath.sh"
    SIM_ARGS=(--pkg-dir="./Work"
              --disable-unused-mem-tiles=false
              --enable-uc-uart-to-stdout=true)
    if [ "${AEG_SIM_PROFILE:-1}" = "1" ]; then
        SIM_ARGS+=(--profile)
    fi
    exec "$XILINX_VITIS/aietools/bin/aiesimulator" "${SIM_ARGS[@]}"
) >"$LOG_FILE" 2>&1 &

SIM_PID=$!
echo "$SIM_PID" >"$SIM_PID_FILE"
echo "[runsim_ipc] Simulator PID: $SIM_PID"

echo "[runsim_ipc] Waiting up to ${SOCKET_TIMEOUT}s for IPC socket..."
DEADLINE=$(( $(date +%s) + SOCKET_TIMEOUT ))
while [ "$(date +%s)" -lt "$DEADLINE" ]; do
    [ -S "$IPC_SOCKET_FILE" ] && { echo "[runsim_ipc] Socket ready."; break; }
    if ! kill -0 "$SIM_PID" 2>/dev/null; then
        echo "ERROR: Simulator exited before creating IPC socket: $IPC_SOCKET" >&2
        wait "$SIM_PID" 2>/dev/null || true
        cat "$LOG_FILE"
        rm -f "$SIM_PID_FILE"
        exit 1
    fi
    sleep 0.2
done

if [ ! -S "$IPC_SOCKET_FILE" ]; then
    echo "ERROR: Timed out waiting for IPC socket: $IPC_SOCKET" >&2
    kill "$SIM_PID" 2>/dev/null || true
    cat "$LOG_FILE"
    rm -f "$SIM_PID_FILE"
    exit 1
fi

CLIENT_EXIT=0
APP_NAME="$(basename "$FULL_APP")"
echo "[runsim_ipc] Running $APP_NAME..."
APP_LOG="$EXAMPLE_DIR/ipc_app.log"
set +e
(
    set +u
    cd "$EXAMPLE_DIR"
    source "$REPO_ROOT/script/envaie2pst50.sh"
    source "$REPO_ROOT/script/aieldpath.sh"
    export AEG_PS_IPC_SOCKET="$IPC_SOCKET"
    export AIE_BAREMETAL_LIB="$LIBAIEBAREMETAL_IPC"
    "$FULL_APP" 2>&1
) | tee "$APP_LOG"
APP_PIPE_EXIT=${PIPESTATUS[0]}
set -e

if [ "$APP_PIPE_EXIT" -eq 0 ] || grep -q "Test Passed" "$APP_LOG" 2>/dev/null; then
    echo "[runsim_ipc] $APP_NAME: PASSED (exit $APP_PIPE_EXIT)."
    CLIENT_EXIT=0
else
    echo "[runsim_ipc] $APP_NAME: FAILED (exit $APP_PIPE_EXIT)." >&2
    CLIENT_EXIT=${APP_PIPE_EXIT:-1}
    [ "$CLIENT_EXIT" -eq 0 ] && CLIENT_EXIT=1
fi

echo "[runsim_ipc] Waiting for simulator to exit..."
if wait "$SIM_PID"; then
    SIM_EXIT=0
else
    SIM_EXIT=$?
fi
rm -f "$SIM_PID_FILE"

SIM_CRASH_PATTERN="core dumped|Segmentation fault|TclStackFree: incorrect freePtr|Tcl_AppendLimitedToObj called with shared object"
SIM_CRASH_LINES="$(grep -nE "$SIM_CRASH_PATTERN" "$LOG_FILE" 2>/dev/null | head -5 || true)"

echo
echo "[runsim_ipc] Simulator log tail:"
tail -20 "$LOG_FILE"

echo
if [ -n "$SIM_CRASH_LINES" ]; then
    echo "[runsim_ipc] ERROR: simulator crashed during the run (see $LOG_FILE):" >&2
    echo "$SIM_CRASH_LINES" >&2
    [ "$SIM_EXIT" -eq 0 ] && SIM_EXIT=134
fi

if [ "$CLIENT_EXIT" -eq 0 ] && [ "$SIM_EXIT" -eq 0 ]; then
    echo "[runsim_ipc] PASS (client=$CLIENT_EXIT, sim=$SIM_EXIT)"
    exit 0
else
    echo "[runsim_ipc] FAIL (client=$CLIENT_EXIT, sim=$SIM_EXIT)" >&2
    exit 1
fi
