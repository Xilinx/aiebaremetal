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
A72_ARCHIEVE=aarch64-none-elf-ar
R5_ARCHIEVE=armr5-none-eabi-ar 
SCRIPT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")
PLATFORM=/proj/xbuilds/SWIP/2023.1_0507_1903/installs/lin64/Vitis/2023.1/base_platforms/xilinx_vek280_es1_base_202310_1/xilinx_vek280_es1_base_202310_1.xpfm
XSA=/proj/xbuilds/2023.1_daily_latest/internal_platforms/xilinx_vek280_es1_base_202310_1/hw/hw.xsa
source $SCRIPT_DIR/envaie2pst50.sh
export PFM_APU="apu_baremetal"
export PFM_RPU="rpu_baremetal"
FILE_APU_LIBXIL_A_PATH=$SCRIPT_DIR/../thirdparty/arch/ps/psv_cortexa72_0/lib/origin/libxil.a
FILE_RPU_LIBXIL_A_PATH=$SCRIPT_DIR/../thirdparty/arch/ps/psv_cortexr5_0/lib/origin/libxil.a
#COMPILE the libxaiengine.a
if [ ! -f "$FILE_APU_LIBXIL_A_PATH" ]; then
    #generate the libxil.a for apu a72
    generate-platform.sh -name $PFM_APU -hw $XSA -domain psv_cortexa72_0:standalone -domain ai_engine:aie_runtime
    cp -rf $PFM_APU/psv_cortexa72_0/standalone_domain/bsp/psv_cortexa72_0/include $SCRIPT_DIR/../thirdparty/arch/ps/psv_cortexa72_0/
    mkdir -p $SCRIPT_DIR/../thirdparty/arch/ps/psv_cortexa72_0/lib/origin/
    #copy the a72 files
    cp -rf $PFM_APU/psv_cortexa72_0/standalone_domain/bsp/psv_cortexa72_0/lib/* $SCRIPT_DIR/../thirdparty/arch/ps/psv_cortexa72_0/lib/origin/
    pushd $SCRIPT_DIR/../thirdparty/arch/ps/psv_cortexa72_0/lib/origin/
    $A72_ARCHIEVE rcs libxil.a *.o
    popd
    rm -rf ./$PFM_APU
fi

if [ ! -f "$FILE_RPU_LIBXIL_A_PATH" ]; then
    #generate the libxil.a for rpu r5
    generate-platform.sh -name $PFM_RPU -hw $XSA -domain psv_cortexr5_0:standalone -domain ai_engine:aie_runtime
    #copy the r5 files
    cp -rf $PFM_RPU/psv_cortexr5_0/standalone_domain/bsp/psv_cortexr5_0/include $SCRIPT_DIR/../thirdparty/arch/ps/psv_cortexr5_0/
    mkdir -p $SCRIPT_DIR/../thirdparty/arch/ps/psv_cortexr5_0/lib/origin/
    #copy the include and the library into dependency folder
    cp -rf $PFM_RPU/psv_cortexr5_0/standalone_domain/bsp/psv_cortexr5_0/lib/* $SCRIPT_DIR/../thirdparty/arch/ps/psv_cortexr5_0/lib/origin/
    pushd $SCRIPT_DIR/../thirdparty/arch/ps/psv_cortexr5_0/lib/origin/
    $R5_ARCHIEVE rcs libxil.a *.o
    popd
    rm -rf ./$PFM_RPU
fi




