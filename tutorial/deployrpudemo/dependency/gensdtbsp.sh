#!/usr/bin/env bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Build an SDT-based RPU BSP from the Vitis already in the environment.
# Override HW_XSA / EMBEDDEDSW to point at a different XSA or embeddedsw tree.

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

# The vek280_base platform maps the AIE aperture for the APU but not for the R5
# cluster, so empyro would reject aiebaremetal with "requires aienginev2
# hardware instance to be present". Add the mapping when it is missing.
if ! python3 - outdir/system-top.dts <<'PATCH_R5_AIE'
import re
import sys

path = sys.argv[1]
dts = open(path).read()
match = re.search(r'cpus_r5_0: cpus-r5@0 \{.*?\n\t\};', dts, re.S)
if match is None:
    sys.exit("cpus_r5_0 node not found in %s" % path)

node = match.group(0)
if '&ai_engine_0' in node:
    print("gensdtbsp: R5 address-map already covers the AIE")
    sys.exit(0)

eol = node.index('\n', node.index('address-map = ')) + 1
entry = '\t\t\t      <0x40000000 &ai_engine_0 0x40000000 0x40000000>, \n'
open(path, 'w').write(dts.replace(node, node[:eol] + entry + node[eol:], 1))
print("gensdtbsp: added the AIE aperture to the R5 address-map")
PATCH_R5_AIE
then
    echo "Error: could not add the AIE aperture to outdir/system-top.dts." >&2
    return 1 2>/dev/null || exit 1
fi

mkdir integration_flow
pushd integration_flow

ln -sfn "$EMBEDDEDSW" ./embeddedsw
empyro repo -st ./embeddedsw &&
    empyro create_bsp -w test_bsp -s ../outdir/system-top.dts -p psv_cortexr5_0 -t hello_world &&
    empyro config_bsp -d test_bsp -al aiebaremetal &&
    empyro build_bsp -d test_bsp
_bsp_status=$?
popd

if [[ $_bsp_status -ne 0 ]]; then
    echo "Error: BSP generation failed, see the empyro errors above." >&2
    return 1 2>/dev/null || exit 1
fi
