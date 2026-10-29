#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

VERSION="${VERSION:-HEAD}"
VITIS_ROOT="/proj/xbuilds/${VERSION}_daily_latest/installs/lin64/${VERSION}/Vitis"
INTERNAL_PLATFORMS="/proj/xbuilds/${VERSION}_daily_latest/internal_platforms"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPLOYDEMO="$REPO_ROOT/tutorial/deploydemo"
BSP_LINK="$DEPLOYDEMO/dependency/integration_flow"
VITIS_SETTINGS="$VITIS_ROOT/settings64.sh"

resolve_vek280_sdt() {
  local candidate
  for candidate in \
    "$INTERNAL_PLATFORMS"/xilinx_vek280_base_*/hw/sdt/system-top.dts \
    "$INTERNAL_PLATFORMS/vek280_base/hw/sdt/system-top.dts"; do
    if [[ -f "$candidate" ]]; then
      echo "$candidate"
      return 0
    fi
  done
  echo "[prod_flow] Error: VEK280 SDT not found under $INTERNAL_PLATFORMS" >&2
  return 1
}

SDT="${BSP_SDT:-$(resolve_vek280_sdt)}"

set +u; source "$VITIS_SETTINGS"; set -u
export XILINX_VITIS_AIETOOLS="${XILINX_VITIS}/aietools"
source "$REPO_ROOT/script/aieldshim.sh"
export VERSION VITIS_SETTINGS

WORK_DIR="$(mktemp -d -p "${PROD_FLOW_TMPDIR:-$REPO_ROOT}")"
INTEGRATION_FLOW="$WORK_DIR/integration_flow"
BSP_INCLUDE="$INTEGRATION_FLOW/test_bsp/include"
BSP_LIB="$INTEGRATION_FLOW/test_bsp/lib"

cleanup() {
  if [[ -L "$BSP_LINK" ]] && [[ "$(readlink -f "$BSP_LINK")" == "$(readlink -f "$INTEGRATION_FLOW")" ]]; then
    rm -f "$BSP_LINK"
  fi
  rm -rf "$WORK_DIR"
}
trap cleanup EXIT

echo "[prod_flow] Building BSP with repo overlay..."
echo "[prod_flow] VEK280 SDT: $SDT"
mkdir -p "$INTEGRATION_FLOW"
pushd "$INTEGRATION_FLOW" >/dev/null

ln -sfn "$VITIS_ROOT/data/embeddedsw" ./embeddedsw

VITIS_DRIVER="$VITIS_ROOT/data/embeddedsw/XilinxProcessorIPLib/drivers/aiebaremetal_v1_0"
mkdir -p ./overlay/XilinxProcessorIPLib/drivers/aiebaremetal_v1_0
cp -R "$VITIS_DRIVER/." ./overlay/XilinxProcessorIPLib/drivers/aiebaremetal_v1_0/
cp -Rf "$REPO_ROOT/src/." ./overlay/XilinxProcessorIPLib/drivers/aiebaremetal_v1_0/src/

empyro repo       -st ./overlay ./embeddedsw
empyro create_bsp -w test_bsp -s "$SDT" -p psv_cortexa72_0 -t hello_world
empyro config_bsp -d test_bsp -al aiebaremetal
empyro build_bsp  -d test_bsp

popd >/dev/null

if [[ -e "$BSP_LINK" && ! -L "$BSP_LINK" ]]; then
  echo "[prod_flow] Error: $BSP_LINK exists and is not a symlink" >&2
  exit 1
fi
ln -sfn "$INTEGRATION_FLOW" "$BSP_LINK"

echo "[prod_flow] Compiling deploydemo AIE graph..."
pushd "$DEPLOYDEMO/dependency/aie" >/dev/null
rm -rf Work Work_* *.lock temp libadf.a
AIE_PART="${BSP_AIE_PART:-xcve2802}"
aiecompiler ./graph.cpp \
  --part="$AIE_PART" \
  --disable-multirate-analysis \
  --workdir=Work \
  --include=./src \
  --include="${XILINX_VITIS_AIETOOLS}/include/aie_api"
popd >/dev/null

echo "[prod_flow] Generating metadata and boot image..."
pushd "$DEPLOYDEMO/dependency" >/dev/null
rm -f ./arch
source ./genmetacpp.sh
source ./genbootbin.sh
popd >/dev/null

echo "[prod_flow] Building deploydemo host application..."
pushd "$DEPLOYDEMO" >/dev/null
aarch64-none-elf-gcc \
  -I./ \
  -I"$BSP_INCLUDE" \
  -I./dependency/psv_cortexa72_0/include \
  -mcpu=cortex-a72 -g -c -std=c++17 \
  -o host.o ./host.cpp
source ./buildmeta.sh
source ./buildsdtbspelf.sh
popd >/dev/null

test -f "$DEPLOYDEMO/main.elf"
test -f "$DEPLOYDEMO/dependency/testkernel.BIN"

echo "[prod_flow] Running v++ hw_emu runtime test..."
REPO_ROOT="$REPO_ROOT" DEPLOYDEMO="$DEPLOYDEMO" VERSION="$VERSION" \
  bash "$REPO_ROOT/script/prod_vpp_hw_emu_test.sh"

echo "[prod_flow] Success:"
echo "  $DEPLOYDEMO/main.elf"
echo "  $DEPLOYDEMO/dependency/testkernel.BIN"
