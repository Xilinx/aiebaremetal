#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Build an SDT-based A72 BSP from the Vitis already in the environment.
# Override HW_XSA / EMBEDDEDSW if the default vek280_base XSA cannot be found.

_TUTORIAL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
# shellcheck disable=SC1091
source "${_TUTORIAL_DIR}/vitis_env.sh" || return 1 2>/dev/null || exit 1

if [[ ! -d "$EMBEDDEDSW" ]]; then
    echo "Error: embeddedsw not found at $EMBEDDEDSW" >&2
    return 1 2>/dev/null || exit 1
fi

if [[ -z "${HW_XSA:-}" ]]; then
    _xbuilds="$(cd "${XILINX_VITIS}/../../../.." && pwd)"
    _rel="$(basename "$(dirname "$XILINX_VITIS")")"
    for _c in \
        "${_xbuilds}/internal_platforms/vek280_base/hw/hw.xsa" \
        "/proj/xbuilds/${_rel}_daily_latest/internal_platforms/vek280_base/hw/hw.xsa"
    do
        if [[ -f "$_c" ]]; then
            HW_XSA="$_c"
            break
        fi
    done
fi
if [[ -z "${HW_XSA:-}" || ! -f "$HW_XSA" ]]; then
    echo "Error: VEK280 hw.xsa not found. Set HW_XSA to the platform XSA." >&2
    return 1 2>/dev/null || exit 1
fi

echo "gensdtbsp: XILINX_VITIS=$XILINX_VITIS"
echo "gensdtbsp: embeddedsw=$EMBEDDEDSW"
echo "gensdtbsp: HW_XSA=$HW_XSA"

rm -rf outdir integration_flow
sdtgen sdt.tcl "$HW_XSA" outdir

mkdir integration_flow
pushd integration_flow

ln -sfn "$EMBEDDEDSW" ./embeddedsw
empyro repo -st ./embeddedsw &&
    empyro create_bsp -w test_bsp -s ../outdir/system-top.dts -p psv_cortexa72_0 -t hello_world &&
    empyro config_bsp -d test_bsp -al aiebaremetal &&
    empyro build_bsp -d test_bsp
_bsp_status=$?
popd

if [[ $_bsp_status -ne 0 ]]; then
    echo "Error: BSP generation failed, see the empyro errors above." >&2
    return 1 2>/dev/null || exit 1
fi
