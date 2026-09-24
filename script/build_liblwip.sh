#!/usr/bin/bash
# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

VITIS_SOURCE=/proj/xbuilds/2025.2_daily_latest/installs/lin64/Vitis/HEAD/settings64.sh
export XTF_PATH=`realpath ../thirdparty/xtf/`
export EMBEDDEDSW_PATH=`realpath ../thirdparty/embeddedsw`
TCL_SCRIPT_PATH=`realpath ../script/build_liblwip.tcl`

source $VITIS_SOURCE

pushd $XTF_PATH

pwd
rm .Xil/ .analytics .metadata/ IDE.log app_system/ app/ app_system/ hw/ -rf
echo "before xsct $TCL_SCRIPT_PATH"
xsct $TCL_SCRIPT_PATH
popd
