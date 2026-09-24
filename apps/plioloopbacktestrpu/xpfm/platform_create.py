#!/usr/bin/env python3

# © Copyright 2019 - 2022 Xilinx, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#set description "A base platform targeting Versal AI Core and edge series evaluation kit, enabling designers to develop solutions using AI and DSP engines capable of delivering over 100X greater compute performance compared to current server class CPUs."

#platform create -name base -desc "Test description" -hw base.xsa -out ./ -no-boot-bsp

## Create the Linux domain
#domain create -name linux -os linux -proc ps11_0_cortexa78
#domain config -generate-bif

## Create the aie domain
#domain create -name aiengine -os aie_runtime -proc {ai_engine}

#platform generate

import sys
import vitis
import os
import shutil

print ("\n------------------------")
print ("Platform creation with Python")

client = vitis.create_client()

proj_name = "base"
workspace = "./temp_ws_" + proj_name
source_dir = workspace + "/" + proj_name
dest_dir = "./" + proj_name

if (os.path.isdir(workspace)):
    shutil.rmtree(workspace)
    print("Deleted workspace " + workspace)

if (os.path.isdir(dest_dir)):
    shutil.rmtree(dest_dir)
    print("Deleted old platform " + dest_dir)

prjLocation = os.path.join(workspace, proj_name)
client.set_workspace(path=workspace)
print(f"Workspace set to : " + workspace)

description = "A base platform targeting Versal AI Core and edge series evaluation kit, enabling designers to develop solutions using AI and DSP engines capable of delivering over 100X greater compute performance compared to current server class CPUs."

platform = client.create_platform_component(name="base", hw_design="./base.xsa", desc="description", os="linux", cpu="psv_cortexr5_0", domain_name="linux", no_boot_bsp=True)
platform.add_domain(name="aiengine", cpu="ai_engine", os="aie_runtime")
domain_linux = platform.get_domain("linux")
domain_linux.generate_bif()

platform.build()

# Copy exported platform to expected directory
try:
    shutil.copytree(source_dir, dest_dir)
    print(f"Directory '{source_dir}' copied to '{dest_dir}' successfully.")
except FileExistsError:
    print(f"Error: Destination directory '{dest_dir}' already exists.")
except Exception as e:
    print(f"An error occurred: {e}")