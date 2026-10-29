#!/usr/bin/bash
# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

# Override with `export VITIS_SOURCE=...` for your site/version. The default
# below tracks the daily-latest install; its internal layout has drifted before
# (e.g. Vitis/HEAD -> 2025.2/Vitis), so if the build cannot find xsct, point
# VITIS_SOURCE at a valid settings64.sh.
VITIS_SOURCE="${VITIS_SOURCE:-/proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/settings64.sh}"
export XTF_PATH=`realpath ../thirdparty/xtf/`
export EMBEDDEDSW_PATH=`realpath ../thirdparty/embeddedsw`
TCL_SCRIPT_PATH=`realpath ../script/build_liblwip.tcl`

if [ ! -f "$VITIS_SOURCE" ]; then
    echo "ERROR: Vitis settings not found at '$VITIS_SOURCE'." >&2
    echo "       Set VITIS_SOURCE to a valid settings64.sh and retry." >&2
    exit 1
fi
source "$VITIS_SOURCE"
if ! command -v xsct >/dev/null 2>&1; then
    echo "ERROR: 'xsct' not on PATH after sourcing '$VITIS_SOURCE'." >&2
    exit 1
fi

pushd $XTF_PATH

pwd
rm .Xil/ .analytics .metadata/ IDE.log app_system/ app/ app_system/ hw/ -rf
echo "before xsct $TCL_SCRIPT_PATH"
# xsct needs an X display; run it under a virtual framebuffer when one isn't
# available. Xilinx launchers otherwise fail to start their own Xvfb (and may
# still exit 0), which used to leave a bogus empty libxtflwipall.a behind.
if command -v xvfb-run >/dev/null 2>&1; then
    xvfb-run -a xsct "$TCL_SCRIPT_PATH"
else
    xsct "$TCL_SCRIPT_PATH"
fi
check_xsct=$?
popd

# Trust the produced artifact, not xsct's exit code: verify platform.h exists.
if [ $check_xsct -ne 0 ] || [ ! -f "$XTF_PATH/app/src/platform.h" ]; then
    echo "ERROR: lwIP BSP generation failed ('$XTF_PATH/app/src/platform.h' not produced)." >&2
    echo "       Check the xsct output above (DISPLAY/Xvfb or Vitis version)." >&2
    exit 1
fi
