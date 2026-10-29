#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
AIE_GEN=1
TARGET="${PL_TARGET:-hw}"
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
if [ $# -gt 1 ] && [ "$2" = "hw_emu" ]; then
	TARGET=hw_emu
fi

# Take the release from whatever Vitis settings.sh already put on the path, so
# the toolchain and the platform can never come from different releases.
REL="$(basename "$(dirname "${XILINX_VITIS:?XILINX_VITIS is not set - source script/settings.sh first}")")"
# The release-tagged platforms (xilinx_vek280_base_202620_1 and friends) are
# deprecated in 2026.2 and go away after it; these carry the same XSA.
case "$AIE_GEN" in
  5) PFM_NAME="vek385_base" ;;
  2) PFM_NAME="vek280_base" ;;
  *) PFM_NAME="vck190_base" ;;
esac
PLATFORM="/proj/xbuilds/${REL}_daily_latest/internal_platforms/${PFM_NAME}/${PFM_NAME}.xpfm"
if [ ! -f "$PLATFORM" ]; then
  echo "ERROR: no platform at $PLATFORM" >&2
  return 1 2>/dev/null || exit 1
fi
echo "pl_kernels: Vitis ${REL}, platform $PLATFORM"
export PLATFORM
v++ -c --platform $PLATFORM --save-temps -g -k mm2s mm2s.cpp -o mm2s.xo || { return 1 2>/dev/null || exit 1; }
v++ -c --platform $PLATFORM --save-temps -g -k softmax_pl softmax_pl.cpp -o softmax_pl.xo || { return 1 2>/dev/null || exit 1; }
v++ -c --platform $PLATFORM --save-temps -g -k s2mm s2mm.cpp -o s2mm.xo || { return 1 2>/dev/null || exit 1; }
v++ -l --platform $PLATFORM  mm2s.xo softmax_pl.xo s2mm.xo ../libadf.a -t $TARGET --save-temps -g --config system.cfg -o new.xsa || { return 1 2>/dev/null || exit 1; }

# Stamp what this new.xsa was built for so builda72.sh can spot a stale one:
# AIE_GEN and release both change the .xpfm.
echo "${AIE_GEN} ${REL}" > .built_for_aie_gen
