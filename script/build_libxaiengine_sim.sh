#!/usr/bin/env bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

DRIVER_SRC="$REPO_ROOT/thirdparty/aielib/aie-rt/driver/src"
INTERNAL_DIR="$REPO_ROOT/thirdparty/aielib/aie-rt/driver/internal"
OUTPUT="$DRIVER_SRC/libxaiengine_sim.so"

MAIN_RTS_INC="$REPO_ROOT/src/include"

FORCE=false
for arg in "$@"; do [[ "$arg" == "--force" ]] && FORCE=true; done

_needs_build() {
    $FORCE && return 0
    [ ! -f "$OUTPUT" ] && return 0
    while IFS= read -r -d '' src; do
        [ "$src" -nt "$OUTPUT" ] && return 0
    done < <(find "$DRIVER_SRC" -mindepth 2 -maxdepth 3 -name "*.c" \
                 ! -path "*/liburing/*" -print0 2>/dev/null)
    [ "$MAIN_RTS_INC/main_rts.h" -nt "$OUTPUT" ] 2>/dev/null && return 0
    return 1
}

if ! _needs_build; then
    echo "[build_libxaiengine_sim] libxaiengine_sim.so is up to date."
    exit 0
fi

echo "[build_libxaiengine_sim] Building libxaiengine_sim.so (aie-rt + -D__AIESIM__)..."

if [ ! -d "$INTERNAL_DIR/xaiengine" ]; then
    echo "[build_libxaiengine_sim] Staging internal headers (make include)..."
    make -C "$DRIVER_SRC" -f Makefile.Linux include \
        2>&1 | grep -v "^$" | sed 's/^/  /'
fi

mapfile -d '' SRCS < <(find "$DRIVER_SRC" -mindepth 2 -maxdepth 3 -name "*.c" \
    ! -path "*/liburing/*" -print0 2>/dev/null | sort -z)

NPROC=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)
OBJ_DIR="$DRIVER_SRC"

CFLAGS=(
    -Wall -Wextra --std=c11
    -D__AIESIM__ -D_DEFAULT_SOURCE
    -I"$MAIN_RTS_INC"
    -I"$INTERNAL_DIR"
    -I"$INTERNAL_DIR/xaiengine"
    -fPIC
)

OBJS=()
PIDS=()
for src in "${SRCS[@]}"; do
    obj="${src%.c}.sim.o"
    OBJS+=("$obj")
    gcc "${CFLAGS[@]}" -c "$src" -o "$obj" &
    PIDS+=($!)
    if (( ${#PIDS[@]} >= NPROC )); then
        wait "${PIDS[0]}"; PIDS=("${PIDS[@]:1}")
    fi
done
for pid in "${PIDS[@]}"; do wait "$pid"; done

gcc "${OBJS[@]}" \
    -shared -Wl,-soname,libxaiengine_sim.so \
    -o "$OUTPUT"

echo "[build_libxaiengine_sim] Built: $OUTPUT"
