# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
rm -rf ./base
rm -rf ./temp_ws_base
rm -rf vivado*
rm base.xsa
source /proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/settings64.sh
#cp /scratch/staff/huaj/aiebr/RPU/Vitis_Embedded_Platform_Source/Xilinx_Official_Platforms/xilinx_vck190_base/hw/build/my_project/vitis_design_wrapper.xsa ./base.xsa
#cp /proj/xsjsswstaff/huaj/aegapi/xsa/vck190/rpu/rpubase.xsa ./base.xsa
cp /proj/xsjsswstaff/huaj/aegapi/xsa/vck190/rpu/rpubase1g.xsa  ./base.xsa
vitis -s platform_create.py