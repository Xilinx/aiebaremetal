# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#+----------------------------------------------------------------------------
#| Vitis Makefile- Developed using new makefile code
#| This make file is generated when the test is run
#| Generated: 2024-04-03 03:25:26.749994
#+----------------------------------------------------------------------------
# Target and command definitions
CXX = $(XILINX_VITIS)/gnu/aarch64/lin/aarch64-linux/bin/aarch64-linux-gnu-g++
XCXX = v++
XAIE = aiecompiler
VIVADO = vivado
EMCONFIGUTIL = emconfigutil
RUNDIR = $(PWD)
SHELL = /bin/bash
RM = rm -rf
CPY = cp -prf
TARGET = hw
PLATFORM = /proj/xbuilds/2025.1_daily_latest/internal_platforms/vek385_base_202510_1/vek385_base_202510_1.xpfm
DEVICE = xilinx_vek385_base_202510_1
WIC_PATH  = /proj/xbuilds/2025.1_daily_latest/internal_platforms/sw/ve2_basecamp/amd-cortexa78-mali-common_edf-linux-disk-image/edf-linux-disk-image-amd-cortexa78-mali-common.rootfs.wic
#SDT variables for vek385
BOARD_DTSI = versal2-vek385-revA
PROCESSER =cortexa78_0
INPUT_DTSI =lop-a78-imux.dts

XILINX_VITIS_AIETOOLS := $(XILINX_VITIS)/aietools
SYSROOT := /proj/xbuilds/2025.1_daily_latest/internal_platforms/sw/versal/xilinx-versal-common-v2025.1/sysroots/aarch64-xilinx-linux

EDGE_COMMON_SW = ${SYSROOT}/../../


# SoC variables
EMBEDDED_EXEC_SCRIPT = embedded_exec.sh
EMBEDDED_PACKAGE_OUT = package.$(TARGET)
EMBEDDED_HOST_PRE_EXEC = /proj/rdi-xco/testcases/xtc/PROD/HEAD/auto/pre_exec_script.sh

# SoC embedded emulator variables
EMBEDDED_EMULATOR = $(EMBEDDED_PACKAGE_OUT)/launch_$(TARGET).sh
EMBEDDED_EMFLAGS =  -verbose -no-reboot -login "petalinux" -password "petalinux"

# SoC zboard variables
RESULT_STRING = TEST PASSED
TARGET_OS = a53_linux

# Kernel definitions
 
KERNEL_1 = loopback
KERNEL_1_XO = libsdf.a
KERNEL_1_SRC = src/graph.cpp
KERNEL_1_COMPILE_FLAGS = --nodot-graph=true  -test-iterations=32 -v   -genArchive   --keep-intermediate-results=true -include="./src" -include="./data" -include="./" --pl-freq=150 --output-archive=$(KERNEL_1_XO)

KERNEL_2 = s2mm
KERNEL_2_XO = s2mm.xo
KERNEL_2_SRC = s2mm.cpp
KERNEL_2_COMPILE_FLAGS =  --config s2mm.xo_config.ini 

KERNEL_3 = mm2s
KERNEL_3_XO = mm2s.xo
KERNEL_3_SRC = mm2s.cpp
KERNEL_3_COMPILE_FLAGS =  --config mm2s.xo_config.ini 

KERNEL_4 = square
KERNEL_4_XO = square.xo
KERNEL_4_SRC = square.cpp
KERNEL_4_COMPILE_FLAGS =  --config square.xo_config.ini 


# Xclbin definitions
XCLBIN_1 = loopback.xclbin
KERNEL_2_CU_1 = s2mm
KERNEL_3_CU_1 = mm2s
KERNEL_4_CU_1 = square
XCLBIN_SYS_1 = sys_loopback.xsa
XCLBIN_SYS_1_XOS = $(KERNEL_2_XO) $(KERNEL_3_XO) $(KERNEL_4_XO) $(KERNEL_1_XO) 
XCLBIN_1_XOS = $(XCLBIN_SYS_1) $(KERNEL_1_XO)
XCLBIN_SYS_1_LINK_OPTS = --config loopback.xclbin_config.ini --config system.cfg --clock.freqHz 100000000:mm2s --clock.freqHz 100000000:s2mm --clock.freqHz 100000000:square 
XCLBIN_1_LINK_OPTS = --config system.cfg --clock.freqHz 100000000:mm2s --clock.freqHz 100000000:s2mm --clock.freqHz 100000000:square 

# Common compilers settings
XCXX_COMMON_OPTS =  -s --config global.ini  -t $(TARGET) -I . --config package_link.ini
XAIE_COMMON_OPTS =  -include="${XILINX_VITIS_AIETOOLS}/include" -include="./" -workdir "./_aie" 
XCXX_PACKAGE_COMMON_OPTS =  --package.sd_dir /proj/xbuilds/2024.1_daily_latest/internal_platforms/sw/versal/xrt --package.sd_dir /proj/xbuilds/2024.1_daily_latest/internal_platforms/sw/versal/aie 
XCXX_PACKAGE_LINK_COMMON_OPTS =  --config package_link.ini


# Host settings
TARGET_FLAGS = -DTARGET_FLOW_HW=1 -DTARGET_FLOW_HW_EMU=0 -DTARGET_FLOW_SW_EMU=0


#User defined host flags and Default template host flags :
HOST_CFLAGS =  -I./ -Isrc -Isw  $(TARGET_FLAGS) -DTARGET_DEVICE=\"$(DEVICE)\"  -std=c++17 -O0 -g -Wall -fmessage-length=0 -I${GITHUB_COMMON_REPO}/common/includes/xcl2 --sysroot=${SYSROOT} -I=/usr/include -I=/usr/include/xrt -L=/usr/lib -ldl -lcrypt -lstdc++ -lxilinxopencl -lpthread -lrt -I$(XILINX_VITIS_AIETOOLS)/include -L$(XILINX_VITIS_AIETOOLS)/lib/aarch64.o -I${XILINX_VITIS}/include -I${XILINX_HLS}/include

# Host files
HOST_PREAMBLE = 
HOST_EXE = host.exe
HOST_SRC = sw/host.cpp
HOST_ARGS = loopback.xclbin
HOST_EXEC_SCRIPT = 

# Set up the emconfigutil related variables
EMCONFIG_FILE = emconfig.json
NUM_DEVICES = 1

GRAPH    = src/graph.cpp
LIBADF  = libadf.a
LIBAIEBAREMETAL = ../build/libaiebaremetala78.a
AIE_CMPL_CMD = aiecompiler -v  --platform=${PLATFORM} --disable-multirate-analysis src/graph.cpp --include=./src
BIF = vek385.bif
BOOTBIN = vek385PLIOBOOT.BIN
ELF = vek385.elf
export SW_DOMAIN = ${PWD}/workspace/platform_baremetal/cortexa78_0/standalone_cortexa78_0/bsp
export ARCH_DIR=`pwd`/../../thirdparty/arch/ps/
export ARCH_78_DIR=${ARCH_DIR}/cortexa78_0/standalone_cortexa78_0/bsp



# Primary build targets
.PHONY: all build host run clean cleanall help

all: clean build run

sdt_lopper:
	@echo "Generating system.dtb and pl.dtbo"
	@source sdt_lopper.sh $(XCLBIN_SYS_1) SDT_OUTPUT $(BOARD_DTSI) $(PROCESSER) $(INPUT_DTSI)


package:
	$(XCXX) -p -s -f ${XCLBIN_SYS_1} ${KERNEL_1_XO} --package.out_dir $(EMBEDDED_PACKAGE_OUT) -o $(XCLBIN_1)
	@mkdir -p $(EMBEDDED_PACKAGE_OUT)/sd_card
	@pdi_name=$(shell echo $(XCLBIN_1) | cut -f 1 -d '.' ); \
	mv $(EMBEDDED_PACKAGE_OUT)/$$pdi_name.pdi  $(EMBEDDED_PACKAGE_OUT)/vpl_gen_fixed_pld.pdi
	@cp -rf pl.dtbo $(XCLBIN_1) $(HOST_EXE) $(EMBEDDED_PACKAGE_OUT)/vpl_gen_fixed_pld.pdi $(EMBEDDED_EXEC_SCRIPT)  $(EMBEDDED_PACKAGE_OUT)/sd_card
	@cp -rf $(EMBEDDED_PACKAGE_OUT) ../



build: $(XCLBIN_SYS_1) $(KERNEL_1_XO) $(LIBADF) $(LIBAIEBAREMETAL) bsp build_elf


${LIBADF}: ${GRAPH}
	${AIE_CMPL_CMD}
${LIBAIEBAREMETAL}:  
	bash -c "cd .. && \
	make hw_external_apu78 AIE_KERNEL_LOC=./loopback_baremetal AIE_GEN=5 -j32 && \
	make build/libaiebaremetal.so AIE_KERNEL_LOC=./loopback_baremetal AIE_GEN=5 -j32 && \
	cd loopback_baremetal"  
	touch ${LIBAIEBAREMETAL} 

bsp:
	vitis -s platform_baremetal.py --xsa ${XCLBIN_SYS_1}

build_elf:${LIBADF} ${LIBAIEBAREMETAL} ${XCLBIN_SYS_1} 
	aarch64-none-elf-gcc -mcpu=cortex-a78 -Wl,-T -Wl,${ARCH_78_DIR}/lscript.ld -I../../thirdparty/aielib/aie-rt/driver/internal/ -I../../src/include/ -I${XILINX_VITIS}/aietools/include/ -I${SW_DOMAIN}/include -L${SW_DOMAIN}/lib -L../build/  -o ./${ELF} sw/host.cpp -Wl,--start-group,-laiebaremetala78,-lxil,-lxiltimer,-lxilstandalone,-lgcc,-lc,-lstdc++,--end-group
	/proj/xbuilds/2025.1_daily_latest/installs/lin64/2025.1/Vitis/bin/bootgen -arch versal_2ve_2vm -image ${BIF} -o ${BOOTBIN} -w



run:
	@echo "INFO: Running zboard run-test."
	/proj/sdxbf/emhw/bin/zboard -d /dev/ttyUSB1 run-test -e ${EMBEDDED_EXEC_SCRIPT} -i ${WIC_PATH} -m ospi_sd  | tee embedded_run.log; exit $${PIPESTATUS[0]}

# Kernel rules
$(KERNEL_1_XO): $(KERNEL_1_SRC)
	$(RM) $@
	$(XAIE) $(XAIE_COMMON_OPTS) --platform $(PLATFORM) $(KERNEL_1_COMPILE_FLAGS) $+

$(KERNEL_2_XO): $(KERNEL_2_SRC)
	$(RM) $@
	$(XCXX) $(XCXX_COMMON_OPTS) --platform $(PLATFORM) -c -o $@ $+ -k $(KERNEL_2) $(KERNEL_2_COMPILE_FLAGS)

$(KERNEL_3_XO): $(KERNEL_3_SRC)
	$(RM) $@
	$(XCXX) $(XCXX_COMMON_OPTS) --platform $(PLATFORM) -c -o $@ $+ -k $(KERNEL_3) $(KERNEL_3_COMPILE_FLAGS)

$(KERNEL_4_XO): $(KERNEL_4_SRC)
	$(RM) $@
	$(XCXX) $(XCXX_COMMON_OPTS) --platform $(PLATFORM) -c -o $@ $+ -k $(KERNEL_4) $(KERNEL_4_COMPILE_FLAGS)

# xclbin rules
$(XCLBIN_SYS_1): $(XCLBIN_SYS_1_XOS)
	$(RM) $@
	$(XCXX) $(XCXX_COMMON_OPTS) --platform $(PLATFORM) -l -o $@ $+  $(XCLBIN_SYS_1_LINK_OPTS)

# Host rules
host: $(HOST_SRC)
	mkdir -p host_code_dir
	$(CXX) $+ -o $(HOST_EXE) $(HOST_CFLAGS)
	@echo "INFO: Compiled Host Executable: $(HOST_EXE)"

clean:
	$(RM) $(EMCONFIG_FILE) $(HOST_EXE)

cleanall: clean
	$(RM) loopback.xclbin
	$(RM) _aie *.xsa
	$(RM) $(EMBEDDED_PACKAGE_OUT)
	$(RM) _x *.log *.compile_summary *.link_summary
	$(RM)  libsdf.a s2mm.xo mm2s.xo square.xo

help:
	 @echo 'Makefile usage:'
	 @echo '  make all TARGET=<sw_emu/hw_emu/hw> PLATFORM=<FPGA platform XPFM>'
	 @echo '      Clean the environment, build host, build xclbin and run the application.'
	 @echo ''
	 @echo '  make run TARGET=<sw_emu/hw_emu/hw> PLATFORM=<FPGA platform XPFM>'
	 @echo '      Build host, build xclbin and run the application.'
	 @echo ''
	 @echo '  make build TARGET=<sw_emu/hw_emu/hw> PLATFORM=<FPGA platform XPFM>'
	 @echo '      Build xclbin.'
	 @echo ''
	 @echo '  make host'
	 @echo '      Build host executable.'
	 @echo ''
	 @echo '  make clean '
	 @echo '      Remove the host executable and emconfig file.'
	 @echo ''
	 @echo '  make cleanall'
	 @echo '      Remove all, keep only the sources.'

