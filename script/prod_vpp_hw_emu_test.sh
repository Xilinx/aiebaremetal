#!/usr/bin/env bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

REPO_ROOT="${REPO_ROOT:?REPO_ROOT is required}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/qemu_hw_emu_common.sh"

VERSION="${VERSION:-HEAD}"
AIE_GEN="${AIE_GEN:-2}"
QEMU_TIMEOUT="${QEMU_TIMEOUT:-600}"
PASS_STRING="${PASS_STRING:-Test Passed}"
DEPLOYDEMO="${DEPLOYDEMO:-$REPO_ROOT/tutorial/deploydemo}"
LIBADF="${LIBADF:-$DEPLOYDEMO/dependency/aie/libadf.a}"
ELF="${ELF:-$DEPLOYDEMO/main.elf}"
QEMU_LOG_TAIL="${QEMU_LOG_TAIL:-$DEPLOYDEMO/prod_hw_emu_tail.txt}"
TEST_NAME="${TEST_NAME:-Prod hw_emu Test}"

VITIS_ROOT="/proj/xbuilds/${VERSION}_daily_latest/installs/lin64/${VERSION}/Vitis"
VITIS_SETTINGS="$VITIS_ROOT/settings64.sh"

echo "========================================"
echo "$TEST_NAME"
echo "========================================"
echo "Date: $(date)"
echo "VERSION: $VERSION"
echo "ELF: $ELF"
echo "libadf.a: $LIBADF"
echo "QEMU_TIMEOUT: ${QEMU_TIMEOUT}s"
echo ""

qemu_log_stage 1 5 "Environment — Vitis and QEMU (baremetal hw_emu)"
qemu_log_hint "FAIL AREA: Vitis not installed at expected path, or qemu-system-aarch64 missing."

if [[ ! -f "$VITIS_SETTINGS" ]]; then
  echo "[prod_vpp_hw_emu_test] Error: Vitis settings not found: $VITIS_SETTINGS" >&2
  exit 1
fi
if [[ ! -f "$LIBADF" ]]; then
  echo "[prod_vpp_hw_emu_test] Error: missing $LIBADF" >&2
  exit 1
fi
if [[ ! -f "$ELF" ]]; then
  echo "[prod_vpp_hw_emu_test] Error: missing $ELF" >&2
  exit 1
fi

set +u; source "$VITIS_SETTINGS"; set -u
export XILINX_VITIS_AIETOOLS="${XILINX_VITIS}/aietools"

if ! command -v v++ >/dev/null 2>&1; then
  echo "[prod_vpp_hw_emu_test] Error: v++ not found after sourcing $VITIS_SETTINGS" >&2
  exit 1
fi
require_qemu_bin "$XILINX_VITIS"

resolve_platform() {
  local version="$1"
  local aie_gen="$2"
  local root="/proj/xbuilds/${version}_daily_latest/internal_platforms"
  local candidate=""

  case "$aie_gen" in
    2)
      for candidate in \
        "$root"/xilinx_vek280_base_*/xilinx_vek280_base_*.xpfm \
        "$root/vek280_base/vek280_base.xpfm"; do
        if [[ -f "$candidate" ]]; then
          echo "$candidate"
          return 0
        fi
      done
      ;;
    1)
      for candidate in \
        "$root"/xilinx_vck190_base_*/xilinx_vck190_base_*.xpfm \
        "$root/vck190_base/vck190_base.xpfm"; do
        if [[ -f "$candidate" ]]; then
          echo "$candidate"
          return 0
        fi
      done
      ;;
    5)
      for candidate in \
        "$root/vek385_base/vek385_base.xpfm" \
        "$root"/vek385_base_*/vek385_base_*.xpfm; do
        if [[ -f "$candidate" ]]; then
          echo "$candidate"
          return 0
        fi
      done
      ;;
  esac

  echo "[prod_vpp_hw_emu_test] Error: no platform .xpfm found under $root for AIE_GEN=$aie_gen" >&2
  return 1
}

PLATFORM="$(resolve_platform "$VERSION" "$AIE_GEN")"
echo "  Platform: $PLATFORM"
echo "  ELF: $ELF"
echo "  libadf.a: $LIBADF"

VPP_WORK="$(mktemp -d -p "${PROD_FLOW_TMPDIR:-$REPO_ROOT}")"
cleanup() {
  if [[ "${KEEP_PROD_VPP_WORK:-0}" == "1" ]]; then
    echo "  Preserved work directory: $VPP_WORK"
  else
    rm -rf "$VPP_WORK"
  fi
}
trap cleanup EXIT

qemu_log_stage 2 5 "Build — v++ link + package (build_hw_qemu_bsp.sh)"
qemu_log_hint "FAIL AREA: v++ link/package errors, wrong platform part, or missing artifacts."

cp "$LIBADF" "$VPP_WORK/"
pushd "$VPP_WORK" >/dev/null

export BAREMETAL_SCRIPT_DIR="$SCRIPT_DIR"
export ELF="$ELF"
export AIE_GEN="$AIE_GEN"
export PLATFORM="$PLATFORM"

source "$BAREMETAL_SCRIPT_DIR/build_hw_qemu_bsp.sh"

if [[ ! -f launch_hw_emu.sh ]]; then
  echo "[prod_vpp_hw_emu_test] Error: launch_hw_emu.sh was not generated" >&2
  exit 1
fi

qemu_log_stage 3 5 "Artifacts — baremetal launch_hw_emu.sh"
echo "  launch_hw_emu.sh: present"
head -5 launch_hw_emu.sh | sed 's/^/    /'

export PASS_STRING QEMU_TIMEOUT
if run_hw_emu_launch_test "$VPP_WORK" "$QEMU_LOG_TAIL"; then
  popd >/dev/null
  echo ""
  echo "========================================"
  echo "PASS: $TEST_NAME"
  echo "========================================"
  exit 0
fi

popd >/dev/null
echo ""
echo "========================================"
echo "FAIL: $TEST_NAME"
echo "========================================"
exit 1
