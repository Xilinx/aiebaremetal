#!/usr/bin/bash
# Copyright (C) 2025 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may
# not use this file except in compliance with the License. A copy of the
# License is located at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

VITIS_SOURCE=/proj/xbuilds/2024.1_daily_latest/installs/lin64/Vitis/HEAD/settings64.sh
export XTF_PATH=`realpath thirdparty/xtf/`
export EMBEDDEDSW_PATH=`realpath thirdparty/embeddedsw`
TCL_SCRIPT_PATH=`realpath build_liblwip.tcl`

source $VITIS_SOURCE

pushd $XTF_PATH

pwd
rm .Xil/ .analytics .metadata/ IDE.log app_system/ app/ app_system/ hw/ -rf
xsct $TCL_SCRIPT_PATH
popd
