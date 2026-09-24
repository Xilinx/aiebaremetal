#!/usr/bin/python
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

#USAGE:
#	source /proj/xbuilds/2025.1_daily_latest/installs/lin64/HEAD/Vitis/settings64.sh
#	./script/gen_compile_commands.py > compile_commands.json

import os
import sys

cwd = os.getcwd()

walk_list = [
	"./src/",
	"./src/include/",
	"./example/",
	"./tutorial/",
	"./thirdparty/aielib/aie-rt/",
	"./thirdparty/aielib/",
]

#walk = os.walk(cwd)

arch_dir1 = "./thirdparty/arch/ps/psv_cortexa72_0/include"
arch_dir2 = "./thirdparty/arch/ps/psv_cortexr5_0/include"
arch_dir3 = "./thirdparty/arch/ps/cortexa78_0/standalone_cortexa78_0/bsp/"

include_dir_cflag = []
include_dir = [
	"./src/include",
	"./src/include/common_layer/",
	"./src/include/common_layer/aegothers/",
	"./thirdparty/aielib/include/",
	"./thirdparty/aielib/aie-rt/driver/internal/",
	"./thirdparty/aielib/aie-rt/driver/include/",
	"./thirdparty/aielib/aie-rt/driver/include/xaiengine/",
	"./thirdparty/aielib/aie-rt/fal/src/",
	"./thirdparty/aielib/aie-rt/fal/build/src/include/",
	"./thirdparty/embeddedsw/lib/sw_services/xiltimer/src/",
#	"./thirdparty/arch/ps/cortexa78_0/standalone_cortexa78_0/bsp/include/",
	"./thirdparty/arch/ps/cortexr52_0/standalone_cortexr52_0/bsp/include/",
#	"./thirdparty/embeddedsw/lib/bsp/standalone/src/arm/ARMv8/64bit/armclang/",
	"./thirdparty/embeddedsw/lib/bsp/standalone/src/arm/cortexr5/armclang/",
	"" + arch_dir1,
	"" + arch_dir2,
	"" + arch_dir3,
	"./thirdparty/xtf/hw/export/hw/sw/hw/mybsp/bspinclude/include",
	"./thirdparty/embeddedsw/ThirdParty/sw_services/lwip220/src/lwip-2.2.0/contrib/ports/xilinx/include/",
	"./thirdparty/embeddedsw/ThirdParty/sw_services/lwip220/src/lwip-2.2.0/src/include/",
	"./thirdparty/embeddedsw/XilinxProcessorIPLib/drivers/ttcps/src/",
	"./thirdparty/xtf/app/src/",
	"./include/common/net/",
	"" + str(os.getenv("XILINX_VITIS")) + "/aietools/include",
	"" + str(os.getenv("XILINX_VITIS")) + "/aietools/include/aie_api/",
	"" + str(os.getenv("XILINX_VITIS")) + "/aietools/include/drivers/aiengine/",
	"./example/example_ai_model/mllib/L1/include/",
	"./example/example_ai_model/mllib/L2/include/",
	"./example/example_ai_model/mllib/L1/include/common/",
	"./example/example_ai_model/mllib/L2/include/common/",
	"./thirdparty/ELFIO/elfio/",
	"./thirdparty/ELFIO/",
	"./thirdparty/cert/handshake/",
	"./thirdparty/cert/api/",
]

#cflags = ["-DXAIE_FEATURE_ALL", "-Wall", "-D__AIESIM__", "-DXAIE_DEBUG"]
#cflags = ["-DXAIE_FEATURE_ALL", "-Wall", "-DXAIE_DEBUG"]
cflags = [
	"-DXAIE_FEATURE_ALL", "-Wall", "-DXAIE_DEBUG", "-D__AIELINUX__", "-D__AIESIM__",
	"-D__AIECONTROLCODE__", "-D__AIEMETAL__", "-D__AIECDO__", "-D__AIEIPU__",
	"-D__AIESOCKET__", "-D__AIEDEBUG__", "-D__AIEBAREMETAL__",
	"-std=c++17", "-D__AIE_ARCH__=22",
]
CC = "gcc"
#CC = "clang"
#CC= "aarch64-none-elf-gcc"

for dir in include_dir:
	include_dir_cflag.append("-I" + os.path.abspath(dir))

for walk in walk_list:
	dir_list = os.walk(os.path.abspath(walk))
	for dirs in dir_list:
		if dirs[2]:
			for filename in dirs[2]:
				if filename[-2:] == ".h":
					include_dir_cflag.append("-I" + dirs[0])
					break

cflags = cflags + include_dir_cflag
s_cflags = " ".join(cflags)

json_out = "[\n"
print(json_out)

for  walk in walk_list:
	dir_list = os.walk(os.path.abspath(walk))
	for dirs in dir_list:
		if dirs[2]:
			for filename in dirs[2]:
				if filename[-2:] == ".c":
					cmd = ""
					cmd = cmd + "{\n"
					cmd = cmd + """    "directory": "{}",\n""".format(dirs[0])
					cmd = cmd + """    "file": "{}",\n""".format(filename)
					cmd = cmd + """    "command": "{} {} -c -o {} {}"\n""".format(CC, s_cflags, filename[:-2] + ".o", filename)
					cmd = cmd + "},\n"
					#json_out = json_out + cmd
					print(cmd)
				elif filename[-3:] == ".cc":
					cmd = ""
					cmd = cmd + "{\n"
					cmd = cmd + """    "directory": "{}",\n""".format(dirs[0])
					cmd = cmd + """    "file": "{}",\n""".format(filename)
					cmd = cmd + """    "command": "{} {} -c -o {} {}"\n""".format(CC, s_cflags, filename[:-3] + ".o", filename)
					cmd = cmd + "},\n"
					#json_out = json_out + cmd
					print(cmd)
				elif filename[-4:] == ".c++" or filename[-4:] == ".cpp":
					cmd = ""
					cmd = cmd + "{\n"
					cmd = cmd + """    "directory": "{}",\n""".format(dirs[0])
					cmd = cmd + """    "file": "{}",\n""".format(filename)
					cmd	= cmd + """    "command": "{} {} -c -o {} {}"\n""".format(CC, s_cflags, filename[:-4] + ".o", filename)
					cmd = cmd + "},\n"
					#json_out = json_out + cmd
					print(cmd)

#json_out = json_out + "]\n"
print("]\n")

#print(json_out)

