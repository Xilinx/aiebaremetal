#!/usr/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
file_bak_runtime_control="../../../Work/ps/c_rts/aie_runtime_control.cpp.bak"
file_new_runtime_control="../../../Work/ps/c_rts/aie_runtime_control.cpp"
runtime_control="../../../Work/ps/c_rts/"
CURRENT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
AIECOMPILERPATH=$(which aiecompiler)
AIETOOLPATH=$(dirname "$(dirname "$AIECOMPILERPATH")")
export XILINX_VITIS_AIETOOLS=$AIETOOLPATH
export LD_LIBRARY_PATH="$AIETOOLPATH/lib/lnx64.o:${LD_LIBRARY_PATH}"
rm $CURRENT_PATH/Work
ln -s $CURRENT_PATH/../../../Work/ $CURRENT_PATH/
echo $CURRENT_PATH

# patchddraddr.py rewrites aie_runtime_control.cpp IN PLACE (it always inserts
# `#include "aiepatch.h"`), so after any build the file in Work/ is the PATCHED
# control. We keep the pristine aiecompiler output in .bak and always re-patch
# from it. The marker tells pristine vs patched so we never snapshot a patched
# file as the "original" (which would double-patch on the next build and produce
# a mis-patched txn -> N=10 broadcast-kick deadlock). This makes the flow safe
# across design (single<->inst10) and runtime (baremetal<->Zephyr) switches.
PATCH_MARKER='#include "aiepatch.h"'
if grep -qF "$PATCH_MARKER" "$file_new_runtime_control" 2>/dev/null; then
  # Already patched: recover the pristine source from a clean .bak.
  if [ ! -f "$file_bak_runtime_control" ] || grep -qF "$PATCH_MARKER" "$file_bak_runtime_control" 2>/dev/null; then
    echo "ERROR: $file_new_runtime_control is already patched and no pristine" >&2
    echo "       snapshot is available ($file_bak_runtime_control missing or itself patched)." >&2
    echo "       Regenerate the AIE control: rm -rf the design Work/ and re-run 'make compile'." >&2
    exit 1
  fi
else
  # Pristine (e.g. fresh make compile): (re)snapshot, refreshing any stale .bak
  # left by a previous design/runtime build.
  cp "$file_new_runtime_control" "$file_bak_runtime_control"
  echo "Refreshed pristine snapshot: $file_bak_runtime_control"
fi

# Always patch from the verified-pristine snapshot.
cp "$file_bak_runtime_control" "$file_new_runtime_control"

python3 ../../patchddraddr.py $file_new_runtime_control
cp ./output.cpp $file_new_runtime_control
cp aiepatch.h $runtime_control

make write_txn_bin
source ./m_txn_hex.sh ./ml_txn.bin ./mltxn.h

