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
AIE_GEN=1
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi

if [ ${AIE_GEN} -ge 2 ]; then
  PLATFORM=/proj/xbuilds/SWIP/2023.1_0507_1903/installs/lin64/Vitis/2023.1/base_platforms/xilinx_vek280_es1_base_202310_1/xilinx_vek280_es1_base_202310_1.xpfm
else
  PLATFORM=/proj/xbuilds/2023.2_daily_latest/internal_platforms/xilinx_vck190_base_202320_1/xilinx_vck190_base_202320_1.xpfm
fi

source ./env2024.sh
v++ -c --platform $PLATFORM --save-temps -g -k s2mm s2mm.cpp -o s2mm.xo
v++ -c --platform $PLATFORM --save-temps -g -k mm2s mm2s.cpp -o mm2s.xo
v++ -l --platform $PLATFORM  s2mm.xo mm2s.xo ../libadf.a -t hw --save-temps -g --config system.cfg -o new.xsa
