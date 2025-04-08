#!/bin/bash
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

# Source the environment variables
A72_ARCHIEVE := aarch64-none-elf-ar
SCRIPT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")
PLATFORM=/proj/xbuilds/SWIP/2023.1_0507_1903/installs/lin64/Vitis/2023.1/base_platforms/xilinx_vek280_es1_base_202310_1/xilinx_vek280_es1_base_202310_1.xpfm
XSA=/proj/xbuilds/2023.1_daily_latest/internal_platforms/xilinx_vek280_es1_base_202310_1/hw/hw.xsa
source ../../script/envaie2pst50.sh
export PFM_NAME="pfm_baremetal"
#COMPILE the libxaiengine.a
pushd $SCRIPT_DIR/../../test
make apuhw
cd $SCRIPT_DIR/dependency/psv_cortexa72_0/
mkdir lib
cd lib
$A72_ARCHIEVE -x $SCRIPT_DIR/../../example/build/libxaienginea72.a 
popd
generate-platform.sh -name $PFM_NAME -hw $XSA -domain psv_cortexa72_0:standalone -domain ai_engine:aie_runtime
#copy the include and the library into dependency folder
cp -rf ./$PFM_NAME/psv_cortexa72_0/standalone_domain/bsp/psv_cortexa72_0/include ./dependency/psv_cortexa72_0/
cp -rf ./$PFM_NAME/psv_cortexa72_0/standalone_domain/bsp/psv_cortexa72_0/lib ./dependency/psv_cortexa72_0/
cd $SCRIPT_DIR/dependency/psv_cortexa72_0/lib/
$A72_ARCHIEVE rcs libxil.a *.o
#rm -rf ./$PFM_NAME



