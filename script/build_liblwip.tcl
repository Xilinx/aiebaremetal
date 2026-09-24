# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
global env

setws $env(XTF_PATH)

repo -set $env(EMBEDDEDSW_PATH)

#platform remove hw
#xhd site is slow,change to local site
#platform create -name hw -hw /proj/ssw_xhd/verification/no_delete/ssw_designs_300GB/2024.2/designs_stable_latest/versal/prod/vek280_isolation/outputs/vek280.xsa
#
#getprocessors /proj/ssw_xhd/verification/no_delete/ssw_designs_300GB/2024.2/designs_stable_latest/versal/prod/vek280_isolation/outputs/vek280.xsa
#getprocessors /proj/xbuilds/2025.2_daily_latest/internal_platforms/xilinx_vek280_base_202520_1/hw/hw.xsa
platform create -name hw -hw /proj/xbuilds/2025.2_daily_latest/internal_platforms/xilinx_vek280_base_202520_1/hw/hw.xsa
#platform create -name hw -hw /proj/xsjsswstaff/huaj/no_delete/xsa/vek280.xsa
#domain create -name mybsp -proc versal_cips_0_pspmc_0_psv_cortexa72_0 -os standalone -support-app "lwip_echo_server"
domain create -name mybsp -proc CIPS_0_pspmc_0_psv_cortexa72_0 -os standalone -support-app "lwip_echo_server"
#platform generate
#app remove app
app create -name app -plat hw -dom mybsp -os standalone -template "lwIP Echo Server"
app build -name app
