#!/usr/bin/env python3
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#
import vitis
import sys
import os
import argparse
import shutil
from datetime import datetime
#
app_path= os.environ['PWD']
print(f"app_path: {app_path}")

parser = argparse.ArgumentParser()
parser.add_argument("--xsa", type=str, dest="xsa")

args = parser.parse_args()
xsa = args.xsa
workspace = "./workspace"
#print(workspace)
#Delete the workspace if already exists.
if (os.path.isdir(workspace)):
    shutil.rmtree(workspace)
    print(f"Deleted workspace {workspace}")
print ("\n")
#
client = vitis.create_client()
client.set_workspace(path=workspace)

#advanced_options = client.create_advanced_options_dict("")

platform_name = "platform_baremetal"
#platform_path=app_path+"/workspace/"+platform_name+"/export/"+platform_name+"/"+platform_name+".xpfm"
platform_path = os.path.join(app_path, "workspace", platform_name, "export", platform_name, f"{platform_name}.xpfm")

platform = client.create_platform_component(name = platform_name,hw_design = os.path.join(app_path,xsa),os = "standalone",cpu = "cortexa78_0",domain_name = "standalone_cortexa78_0",generate_dtb = False,compiler = "gcc")

platform = client.get_component(name=platform_name)
status = platform.build()

# comp = client.create_app_component(name="main",platform = platform_path ,domain = "standalone_cortexa78_0")
# comp = client.get_component(name="main")
# status = comp.import_files(from_loc=os.path.join(app_path, "sw"), files=["main.cpp"], dest_dir_in_cmp = "src")
# status = comp.import_files(from_loc=os.path.join(app_path, "Work", "ps", "c_rts"), files=["aie_control.cpp"], dest_dir_in_cmp = "src")

# comp.set_app_config(key = 'USER_COMPILE_DEFINITIONS', values = "__PS_BARE_METAL__")
# comp.set_app_config(key = 'USER_INCLUDE_DIRECTORIES', values = [os.path.join(app_path,"src"),os.path.join(app_path, "Work", "ps", "c_rts"), os.path.join(app_path, "sw"),os.path.join(app_path, "src", "aie"),os.path.join(app_path, "src", "graph"),os.path.join(app_path, "sw"),'/proj/xbuilds/2025.1_daily_latest/installs/lin64/2025.1/Vitis/aietools/include'])
# comp.set_app_config(key = 'USER_LINK_LIBRARIES', values = 'adf_api')
# comp.set_app_config(key = 'USER_LINK_DIRECTORIES', values = '/proj/xbuilds/SWIP/2025.1_0421_1532/installs/lin64/2025.1/Vitis/aietools/lib/aarchnone64.o')


vitis.dispose()
