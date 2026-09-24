# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#+----------------------------------------------------------------------------
#| Vitis Makefile- Developed using new makefile code
#| This make file is generated when the test is run
#| Generated: 2025-08-04 03:25:26.749994
#+----------------------------------------------------------------------------
# Target and command definitions
CXX = $(XILINX_VITIS)/gnu/armr5/lin/gcc-arm-none-eabi/bin/armr5-none-eabi-g++
XCXX = v++
AIECOMPILER = aiecompiler
VIVADO = vivado
RUNDIR = $(PWD)
SHELL = /bin/bash
RM = rm -rf
CPY = cp -prf
TARGET = hw
WORKFOLDER = Work
#PLATFORM =  /proj/xbuilds/2025.2_daily_latest/internal_platforms/xilinx_vek280_base_202510_1/xilinx_vek280_base_202510_1.xpfm
#PLATFORM = /proj/xbuilds/2025.2_daily_latest/internal_platforms/xilinx_vek280_base_202520_1/xilinx_vek280_base_202520_1.xpfm
PLATFORM = /proj/xsjsswstaff/huaj/aegapi/xpfm/rpuvck1901gbase/base.xpfm
#PLATFORM = /scratch/staff/huaj/aiebr/aiebaremetal2/apps/plioloopbacktestrpu/xpfm/base/export/base/base.xpfm
#EMBBEDEDSW = /proj/xbuilds/SWIP/2025.2_1002_0237/installs/lin64/2025.2/Vitis/data/embeddedsw/
EMBBEDEDSW = $(XILINX_VITIS)/data/embeddedsw/
# SoC variables
EMBEDDED_EXEC_SCRIPT = embedded_exec.sh
EMBEDDED_PACKAGE_OUT = package.$(TARGET)
EMBEDDED_HOST_PRE_EXEC = /proj/rdi-xco/testcases/xtc/PROD/HEAD/auto/pre_exec_script.sh

# Kernel definitions
KERNEL_1 = loopback
AIELOGICARCH = libsdf.a
AIE_APP_SRC = src/graph.cpp
KERNEL_1_COMPILE_FLAGS = --nodot-graph=true  -test-iterations=32 -include="./src" -include="./data" -include="./" --pl-freq=150 --output-archive=$(AIELOGICARCH)

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
EXTERNSION_XSA = sys_loopback.xsa
#BSP_XSA = /proj/xsjsswstaff/huaj/aegapi/xsa/vck190/rpu/vpl_gen_fixed.xsa
BSP_XSA=$(EXTERNSION_XSA)
EXTERNSION_XSA_XOS = $(KERNEL_2_XO) $(KERNEL_3_XO) $(KERNEL_4_XO) $(AIELOGICARCH) 
XCLBIN_1_XOS = $(EXTERNSION_XSA) $(AIELOGICARCH)
EXTERNSION_XSA_LINK_OPTS = --config loopback.xclbin_config.ini --config system.cfg --clock.freqHz 100000000:mm2s --clock.freqHz 100000000:s2mm --clock.freqHz 100000000:square 
XCLBIN_1_LINK_OPTS = --config system.cfg --clock.freqHz 100000000:mm2s --clock.freqHz 100000000:s2mm --clock.freqHz 100000000:square

# Common compilers settings
XCXX_COMMON_OPTS =  -s --config global.ini  -t $(TARGET) -I . --config package_link.ini
AIECOMPILER_COMMON_OPTS =  -include="${XILINX_VITIS_AIETOOLS}/include" -include="./" -workdir "./${WORKFOLDER}" 
XCXX_PACKAGE_COMMON_OPTS =  --package.sd_dir /proj/xbuilds/2024.1_daily_latest/internal_platforms/sw/versal/xrt --package.sd_dir /proj/xbuilds/2024.1_daily_latest/internal_platforms/sw/versal/aie 
XCXX_PACKAGE_LINK_COMMON_OPTS =  --config package_link.ini

GRAPH    = src/graph.cpp
AIECDOS  = libadf.a
AIE_CMPL_CMD = aiecompiler -v  --platform=${PLATFORM} --disable-multirate-analysis src/graph.cpp --include=./src
BIF = vck190_rpu_boot_image.bif
BOOTBIN = vck190RPUPLIOBOOT.BIN
ELF = vck190rpu.elf

# Primary build targets

#build: $(EXTERNSION_XSA) $(AIELOGICARCH) $(AIECDOS) bsp a72bsp build_elf
build: $(EXTERNSION_XSA) $(AIELOGICARCH) $(AIECDOS) bsp build_elf

bsp: $(EXTERNSION_XSA)
		rm -rf embeddedsw
		rm -rf ./outdir
		rm -rf ./test_bsp
		sdtgen sdt.tcl $(BSP_XSA) outdir
		ln -s $(EMBBEDEDSW) ./
		empyro repo -st ./embeddedsw 
		empyro create_bsp -w test_bsp -s ./outdir/system-top.dts -p psv_cortexr5_0  -t hello_world
		empyro config_bsp -d test_bsp -al aiebaremetal
		empyro build_bsp -d test_bsp
        
a72bsp: $(EXTERNSION_XSA)
		rm -rf a72
		mkdir a72
		( pushd ./a72&& \
		      sdtgen ../sdt.tcl ../$(EXTERNSION_XSA) outdir &&\
				ln -s $(EMBBEDEDSW) ./&&\
				empyro repo -st ./embeddedsw &&\
				empyro create_bsp -w test_bsp -s ./outdir/system-top.dts -p psv_cortexa72_0  -t hello_world&&\
				empyro config_bsp -d test_bsp -al aiebaremetal&&\
				empyro build_bsp -d test_bsp&&\
				popd)
		
build_elf: ${AIECDOS} ${EXTERNSION_XSA} 
		python3 ../../script/aiemetacpp.py ${WORKFOLDER}/ps/c_rts/aie_control_config.json
		armr5-none-eabi-g++ -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -g -c -std=c++17 -o aiemetadata.o ./baremetal_metadata.cpp
		armr5-none-eabi-g++ -I ./test_bsp/include -I ./test_bsp/libsrc/aiebaremetal/src/include -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -g -c -std=c++17 -o host.o ./sw/host.cpp
		armr5-none-eabi-g++ host.o aiemetadata.o -mcpu=cortex-r5 -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -Wl,-T -Wl,./lscript.ld -L ./test_bsp/lib -laiebaremetal -Wl,--start-group,-lxil,-lxiltimer,-lxilstandalone,-lgcc,-lc,-lstdc++,--end-group  -o ${ELF}
		bootgen -arch versal -image ${BIF} -o ${BOOTBIN} -w
package:
	$(XCXX) -p -s -f ${EXTERNSION_XSA} ${AIELOGICARCH} --package.out_dir $(EMBEDDED_PACKAGE_OUT) -o $(XCLBIN_1)

${AIECDOS}: ${GRAPH}
	${AIE_CMPL_CMD}

# Kernel rules
$(AIELOGICARCH): $(AIE_APP_SRC)
	$(RM) $@
	$(AIECOMPILER) $(AIECOMPILER_COMMON_OPTS) --platform $(PLATFORM) $(KERNEL_1_COMPILE_FLAGS) $+

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
$(EXTERNSION_XSA): $(EXTERNSION_XSA_XOS)
	$(RM) $@
	$(XCXX) $(XCXX_COMMON_OPTS) --platform $(PLATFORM) -l -o $@ $+  $(EXTERNSION_XSA_LINK_OPTS)

clean:
	$(RM) $(EMCONFIG_FILE) $(HOST_EXE)

cleanall: clean
	$(RM) loopback.xclbin
	$(RM) ${WORKFOLDER} *.xsa
	$(RM) $(EMBEDDED_PACKAGE_OUT)
	$(RM) _x *.log *.compile_summary *.link_summary
	$(RM)  libsdf.a  libadf.a s2mm.xo mm2s.xo square.xo
	$(RM) build
	$(RM) host.o
	$(RM) embeddedsw
	$(RM) *.o *.BIN
	$(RM) baremetal_metadata.cpp 
	$(RM) outdir test_bsp Work Map_Report.csv vck190rpu.elf reports xocc_log sol.db
help:
	 @echo '  make build '
	 @echo '      Generate the .bin and elf file.'
	 @echo ''
	 @echo '  make clean '
	 @echo '      Remove the host executable and emconfig file.'
	 @echo ''
	 @echo '  make cleanall'
	 @echo '      Remove all, keep only the sources.'

