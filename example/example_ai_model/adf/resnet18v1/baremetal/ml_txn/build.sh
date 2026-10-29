# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
!/usr/bin/bash
file_bak_runtime_control="../../Work/ps/c_rts/aie_runtime_control.cpp.bak"
file_new_runtime_control="../../Work/ps/c_rts/aie_runtime_control.cpp"
runtime_control="../../Work/ps/c_rts/"
CURRENT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
AIECOMPILERPATH=$(which aiecompiler)
AIETOOLPATH=$(dirname "$(dirname "$AIECOMPILERPATH")")
export XILINX_VITIS_AIETOOLS=$AIETOOLPATH
export LD_LIBRARY_PATH="$AIETOOLPATH/lib/lnx64.o:${LD_LIBRARY_PATH}"
rm $CURRENT_PATH/Work
ln -s $CURRENT_PATH/../../Work/ $CURRENT_PATH/
echo $CURRENT_PATH

if [ ! -f "$file_bak_runtime_control" ]; then
  # If /a/b/c does not exist, copy /a/b/d to /a/b/c
  cp $file_new_runtime_control $file_bak_runtime_control
  echo "Copied $file_new_runtime_control to $file_bak_runtime_control"
fi

# Copy /a/b/c to /a/b/d
cp $file_bak_runtime_control $file_new_runtime_control

python3 ../../patchddraddr.py
cp aiepatch.h $runtime_control
cp ./output.cpp $file_new_runtime_control

make write_txn_bin
source ./m_txn_hex.sh ./ml_txn.bin ./mltxn.h

