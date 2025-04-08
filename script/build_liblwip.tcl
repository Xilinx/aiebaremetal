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

global env

setws $env(XTF_PATH)

repo -set $env(EMBEDDEDSW_PATH)

#platform remove hw

platform create -name hw -hw /proj/ssw_xhd/verification/no_delete/ssw_designs_300GB/2024.2/designs_stable_latest/versal/prod/vek280_isolation/outputs/vek280.xsa
domain create -name mybsp -proc versal_cips_0_pspmc_0_psv_cortexa72_0 -os standalone -support-app "lwip_echo_server"
platform generate
#app remove app
app create -name app -plat hw -dom mybsp -os standalone -template "lwIP Echo Server"
app build -name app
