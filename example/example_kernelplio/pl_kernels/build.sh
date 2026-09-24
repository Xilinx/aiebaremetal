#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
AIE_GEN=1
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi

if [ "$AIE_GEN" == "5" ]; then
  #PLATFORM=../../../thirdparty/arch/platform/vek385_xpfm/base/export/base/base.xpfm
  #  PLATFORM=/proj/xbuilds/2025.1_daily_latest/internal_platforms/vek385_base_202510_1/vek385_base_202510_1.xpfm
  #  PLATFORM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/vek385_base_202520_1/vek385_base_202520_1.xpfm
   PLATFORM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/vek385_base/vek385_base.xpfm
  #  PLATFORM=/proj/xbuilds/2026.1_daily_latest/internal_platforms/vek385_base/vek385_base.xpfm

elif [ "$AIE_GEN" == "2" ]; then
  #PLATFORM=/proj/xbuilds/SWIP/2023.1_0507_1903/installs/lin64/Vitis/2023.1/base_platforms/xilinx_vek280_es1_base_202310_1/xilinx_vek280_es1_base_202310_1.xpfm
  # PLATFORM=/proj/xbuilds/2024.2_daily_latest/internal_platforms/xilinx_vek280_base_202420_1/xilinx_vek280_base_202420_1.xpfm
  PLATFORM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/xilinx_vek280_base_202520_1/xilinx_vek280_base_202520_1.xpfm
  # PLATFORM=/proj/xbuilds/2026.1_daily_latest/internal_platforms/xilinx_vek280_base_202610_1/xilinx_vek280_base_202610_1.xpfm

else
  # PLATFORM=/proj/xbuilds/2024.2_daily_latest/internal_platforms/xilinx_vck190_base_202520_1/xilinx_vck190_base_202520_1.xpfm
  PLATFORM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/xilinx_vck190_base_202520_1/xilinx_vck190_base_202520_1.xpfm
  # PLATFORM=/proj/xbuilds/2026.1_daily_latest/internal_platforms/xilinx_vck190_base_202610_1/xilinx_vck190_base_202610_1.xpfm
fi

if [ "$AIE_GEN" == "5" ]; then
  source ./env2025.sh
  # source ./env2026.sh
else
  source ./env2025.sh
  # source ./env2026.sh
fi
v++ -c --platform $PLATFORM --save-temps -g -k s2mm s2mm.cpp -o s2mm.xo
v++ -c --platform $PLATFORM --save-temps -g -k mm2s mm2s.cpp -o mm2s.xo
v++ -l --platform $PLATFORM  s2mm.xo mm2s.xo ../libadf.a -t hw --save-temps -g --config system.cfg -o new.xsa
