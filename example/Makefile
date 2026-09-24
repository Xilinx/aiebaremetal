# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
AIE_GEN                ?= 1
#this will enable aie-rt debug
#ENABLE_AIE_RT_DEBUG = -DXAIE_DEBUG
#ENABLE_AIE_DEBUG_BACKEND = -D_AIEDEBUG_
ARCH_APU_A72_ARC_PATH  ?= ../thirdparty/arch/ps/psv_cortexa72_0/
ARCH_APU_A72_ARC_DIC   ?= $(ARCH_APU_A72_ARC_PATH)/include
ARCH_APU_A72_ARC_LIB   ?= $(ARCH_APU_A72_ARC_PATH)/lib

ARCH_RPU_R53_ARC_PATH  ?= ../thirdparty/arch/ps/psv_cortexr5_0/
ARCH_RPU_R53_ARC_DIC   ?= $(ARCH_RPU_R53_ARC_PATH)/include
ARCH_RPU_R53_ARC_LIB   ?= $(ARCH_RPU_R53_ARC_PATH)/lib

ARCH_RPU_R52_ARC_PATH  ?= ../thirdparty/arch/ps/cortexr52_0/standalone_cortexr52_0/bsp/
ARCH_RPU_R52_ARC_DIC   ?= $(ARCH_RPU_R52_ARC_PATH)/include
ARCH_RPU_R52_ARC_LIB   ?= $(ARCH_RPU_R52_ARC_PATH)/lib

ARCH_APU_A78_ARC_DIC   ?= ../thirdparty/arch/ps/cortexa78_0/standalone_cortexa78_0/bsp/include
ARCH_APU_A78_ARC_LIB   ?= ../thirdparty/arch/ps/cortexa78_0/standalone_cortexa78_0/bsp/lib
AIE_DRIVER_PARENT_DIC  ?= ../thirdparty/aielib
AIE_BAREMETAL_INCLUDE  ?= ../src/include
AIE_BAREMETAL_INC_COMMON  ?= ../src/include/common_layer/
AIE_BAREMETAL_INC_COMMON_OTHER  ?= ../src/include/common_layer/aegothers/
CERT_HANDSHAKE_INCLUDE	?= ../thirdparty/cert/handshake/
CERT_API_INCLUDE	?= ../thirdparty/cert/api/
ELFIO_INCLUDE		?= ../thirdparty/ELFIO/
ELFIO_INCLUDE2		?= ../thirdparty/ELFIO/elfio/
ARCH_INCLUDE ?= ../thirdparty/arch/ps/cortexa78_0/standalone_cortexa78_0/bsp/include/
AIE_DRIVER             ?= $(AIE_DRIVER_PARENT_DIC)/aie-rt/driver
AIE_FAL                ?= $(AIE_DRIVER_PARENT_DIC)/aie-rt/fal/
A72_EXAMPLE             = ./build/Vek280BOOT.BIN
AIE_KERNEL_EXAMPLE      = $(PWD)/testkernel/
AIE_KERNEL_LOC         ?= $(AIE_KERNEL_EXAMPLE)
AIE_COMPILE_WORK       ?= $(AIE_KERNEL_LOC)/Work/
AIE_KERNEL_BUILD        = aiekernel
#PETALINUX_PATH         ?= /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/
#PETALINUX_PATH         ?= /proj/petalinux/2025.1/petalinux-v2025.1_daily_latest/tool/petalinux-v2025.1-final/
PETALINUX_PATH         ?= /proj/petalinux/2025.2/petalinux-v2025.2_daily_latest/tool/petalinux-v2025.2-final/
INCLUDE                 = $(AIE_DRIVER)/internal
INCLUDE_XAIE            = $(AIE_DRIVER_PARENT_DIC)/include/
INCLUDE_AIETOOLS        = /proj/xbuilds/HEAD_qualified_latest/installs/lin64/HEAD/Vitis/aietools/include
#aie fal header
AIEFAL_INCLUDE          = $(AIE_FAL)/src/
AIEFAL_INCLUDE2          = $(AIE_FAL)/build/src/include/
LD_PATH                 = $(AIE_DRIVER)/src
BUILD                   = build
SRCS_DIR                = ../src ../src/common_layer/ ../src/common_layer/aegothers/
CC := $(shell \
	if [ -f "${XILINX_VITIS}/aietools/bin/aie_g++" ]; then \
		echo "${XILINX_VITIS}/aietools/bin/aie_g++"; \
	elif [ -f "${XILINX_VITIS}/aietools/bin/g++" ]; then \
		echo "${XILINX_VITIS}/aietools/bin/g++"; \
	elif command -v g++ >/dev/null 2>&1; then \
		echo "g++"; \
	else \
		echo "g++"; \
	fi)
A72_CC                 := aarch64-none-elf-gcc
A72_ARCHIEVE           := aarch64-none-elf-ar
A72_LD                 := aarch64-none-elf-ld
R53_LD                 := armr5-none-eabi-ld
R52_LD                 := armr5-none-eabi-ld
EXTRA_CPPFLAGS         ?=
CFLAGS                 := -Wall -Wextra -O0 -g -std=c++17 -D_BSD_SOURCE -D_DEFAULT_SOURCE -I$(AIE_BAREMETAL_INCLUDE) -I$(AIE_BAREMETAL_INC_COMMON) -I$(AIE_BAREMETAL_INC_COMMON_OTHER) -I$(INCLUDE) -I$(INCLUDE_XAIE) -I$(AIEFAL_INCLUDE) -I$(AIEFAL_INCLUDE2) -I$(INCLUDE_AIETOOLS) -DAIE_GEN=$(AIE_GEN) -I$(ARCH_INCLUDE) -I$(ELFIO_INCLUDE) -I$(ELFIO_INCLUDE2) -I$(CERT_HANDSHAKE_INCLUDE) -I$(CERT_API_INCLUDE) -DNO_JSON -fPIC $(EXTRA_CPPFLAGS)
LDFLAGS                := -L$(LD_PATH) -L$(BUILD) $(ENABLE_AIE_RT_DEBUG)
LDLIBS                 := -lxaiengine
TESTLDLIBS             := -laiebaremetal -ldl $(LDLIBS)
TESTSIMLDLIBS          := -ldl
TEST_IS_LOCAL_MACRO    ?=

UCFIRM_O                = ./testuc/ucfirmware/output/ucfw_exe.o
UCFIRM_ELF              = ./testuc/ucfirmware/output/*.elf
UCFIRM_APP_O            = ./testuc/ucfirmware/output/*.o
TESTUC                  = ./testuc/
TESTAIE                 = ./testaie/

CTRL_CODE_ELF		= move_memtile_simple.elf
CTRL_CODE_CC		= $(PWD)/../src/ctrl_code_elf.cc

CERT_CC			= $(PWD)/../src/cert_elf.cc
CERT_DIR       = $(PWD)/../thirdparty/cert/build/main
CERT_ELF       = $(CERT_DIR)/cert_ve2.elf

TESTTARGET             := $(BUILD)/test
TESTSIMTARGET          := $(BUILD)/testsim
TARGET                 := $(BUILD)/libaiebaremetal.so

AIE_CONTROL_JSON_PATH     ?= $(AIE_COMPILE_WORK)/ps/c_rts/
AIE_CONTROL_JSON_FILE     = aie_control_config.json
AIE_CONTROL_JSON          ?= $(AIE_CONTROL_JSON_PATH)/$(AIE_CONTROL_JSON_FILE)

AIE_PARTITION_JSON_PATH     ?= $(AIE_COMPILE_WORK)/ps/c_rts/
AIE_PARTITION_JSON_FILE     = aie_partition.json
AIE_PARTITION_JSON          ?= $(AIE_PARTITION_JSON_PATH)/$(AIE_PARTITION_JSON_FILE)

SCSIM_CONFIG_JSON_FILE  = scsim_config.json
SCSIM_CONFIG_JSON_PATH  = $(AIE_COMPILE_WORK)/config/
SCSIM_CONFIG_JSON       = $(SCSIM_CONFIG_JSON_PATH)/$(SCSIM_CONFIG_JSON_FILE)
SCSIM_CJSON_O           = $(BUILD)/scsimconfigjson.o

AIE_METADATATOOL        = ../script/aiemetacpp.py
AIE_BARMETALDATA        = $(BUILD)/baremetal_metadata.cpp

AIELIBAPU_ALIB         := ./build/libxaienginea72.a
AIELIBAPU78_ALIB       := ./build/libxaienginea78.a
AIELIBRPU_ALIB         := ./build/libxaienginer53.a
AIELIBRPUR52_ALIB         := ./build/libxaienginer52.a

TARGETR53              := ./build/libaiebaremetalr53.a
TARGETR52              := ./build/libaiebaremetalr52.a
TARGETA72              := ./build/libaiebaremetala72.a
TARGETA78              := ./build/libaiebaremetala78.a

TARGET_LWIP            := ./build/liblwip4.a
TARGET_LWIP_XTF        := ../thirdparty/xtf/hw/export/hw/sw/hw/mybsp/bsplib/lib/liblwip4.a

HW_LIB_XTF_ALL         := ./build/libxtflwipall.a

AIELIB                 := $(AIE_DRIVER)/src/libxaiengine.so
AIE_FAL_HPP            := $(AIE_FAL)/build/src/include/xaiefal.hpp
#SRCS := ./src/aiebaremetal.cc
SRCS_CPP                = $(foreach dir, $(SRCS_DIR), $(wildcard $(dir)/*.cpp))
SRCS_CC                 = $(foreach dir, $(SRCS_DIR), $(wildcard $(dir)/*.cc))
TEST_UC_C               = $(foreach dir, $(TESTUC), $(wildcard $(dir)/*.c))
TEST_AIE_C              = $(foreach dir, $(TESTAIE), $(wildcard $(dir)/*.c))
# before do compile the lwip *.o not exist yet, put the logic after compile
#HW_LIB_XTF_ALL_O_FILES := $(filter-out %main.o %echo.o, $(shell find ../thirdparty/xtf/ -name "*.o"))

OBJS_CPP                = $(SRCS_CPP:%.cpp=$(BUILD)/%.o)
OBJS_CC                 = $(SRCS_CC:%.cc=$(BUILD)/%.o)
OBJ_TEST_UC_C           = $(TEST_UC_C:%.c=$(BUILD)/%.o)
OBJ_TEST_AIE_C          = $(TEST_AIE_C:%.c=$(BUILD)/%.o)

TEST_SRCS              := ./testut/test.c ./testut/main_rts.c
TEST_SIM_SRCS          := ./testut/testsim.c ./testut/main_rts.c

BUILD_DIR := build

#set the is local macro for test and testsim
# ifeq ($(MAKECMDGOALS), $(TESTTARGET))
# 	TEST_IS_LOCAL_MACRO=-D__LOCAL_TEST__
# else
# 	ifeq ($(MAKECMDGOALS), $(TESTSIMTARGET))
# 		TEST_IS_LOCAL_MACRO=-D__LOCAL_TEST__
# 	endif
# endif
$(BUILD_DIR):
		@mkdir -p $(BUILD_DIR)

all: 
	$(MAKE) $(TARGET)
	$(MAKE) $(AIELIBAPU_ALIB)
	$(MAKE) $(AIELIBAPU78_ALIB)
	$(MAKE) $(AIELIBRPU_ALIB)
	$(MAKE) $(AIELIBRPUR52_ALIB)

hw: $(A72_EXAMPLE)

hw_external_apu: $(TARGETA72)
hw_external_apu78: $(TARGETA78)
hw_external_rpu: $(TARGETR53)
hw_external_rpu_r52: $(TARGETR52)

#alib: $(AIELIB) $(AIE_FAL_HPP)
alib: $(AIELIB)


$(AIE_CONTROL_JSON): $(AIELIB)
	echo $(MAKECMDGOALS)
	echo $(TEST_IS_LOCAL_MACRO)
	echo "build $(AIE_CONTROL_JSON)"
#	set AIE_GEN and disable force build libaiebaremetal.so
#	bash -c "cd $(AIE_KERNEL_LOC)/; source ./build.sh $(AIE_GEN)"

$(SCSIM_CONFIG_JSON): $(AIE_CONTROL_JSON)

$(TESTTARGET): testtargetlib $(TEST_SRCS)
	$(CC) $(CFLAGS) -D__AIESIM__ -o $@ $(TEST_SRCS) $(LDFLAGS) $(TESTLDLIBS)

$(TESTSIMTARGET): testtargetlib $(TEST_SIM_SRCS)
	$(CC) $(CFLAGS) -D__AIESIM__ -rdynamic -o $@ $(TEST_SIM_SRCS) $(TESTSIMLDLIBS)
#	$(CC) $(CFLAGS) -D__AIESIM__ -o $@ $(TEST_SIM_SRCS) $(TESTSIMLDLIBS)

$(OBJ_TEST_UC_C): $(AIELIB)
$(OBJ_TEST_AIE_C): $(AIELIB)
$(OBJS_CPP): $(AIELIB)
$(OBJS_CC): $(AIELIB) ${CERT_CC} ${CTRL_CODE_CC}

testtargetlib: cleanlib
	make $(TARGET) TEST_IS_LOCAL_MACRO=-D__LOCAL_TEST__

targetforce: cleanlib
	make $(TARGET) 

#$(TARGET): $(OBJS_CC) $(OBJ_TEST_UC_C) $(OBJ_TEST_AIE_C) $(OBJS_CPP) \
#					 $(AIELIB) $(AIE_CJSON_O) $(SCSIM_CJSON_O) ${UCFIRM_O}     \
#					 $(DRIVER_CONFIG_O) $(GRAPH_CONFIG_O) $(GMIOS_CONFIG_O)    \
#					 $(RTPS_CONFIG_O)
#	echo "$(TARGET) build"
#	$(CC) $(CFLAGS) -D__AIESIM__ $(TEST_IS_LOCAL_MACRO) -shared -o $@ $^ $(LDFLAGS) $(LDLIBS)

# the aiebaremetal.cc should enabled NO_JSON
$(TARGET): $(OBJS_CC) $(OBJ_TEST_UC_C) $(OBJ_TEST_AIE_C) $(OBJS_CPP) $(AIE_BARMETALDATA)\
					 $(AIELIB) $(SCSIM_CJSON_O) ${UCFIRM_O}
	echo "$(TARGET) build"
	$(CC) $(CFLAGS) -D__AIESIM__ $(TEST_IS_LOCAL_MACRO) -shared -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(TARGETA72): $(AIELIBAPU_ALIB) $(SRCS_CC) $(SRCS_CPP) $(AIE_BARMETALDATA) 
	bash -c "rm -f ../src/*.o;rm -f ../src/common_layer/*.o"
	bash -c "source $(PETALINUX_PATH)/settings.sh;                          \
	         make -f ../Makefile                                            \
	          \"CC=aarch64-none-elf-gcc\"                             \
	          \"AS=aarch64-none-elf-as\"                             \
	          \"AR=aarch64-none-elf-ar\"                              \
	          \"CFLAGS= -O2 -c $(CFLAGS)                            \
	          			 -g -Wall -Wextra -Dversal -DARMA72_EL3 \
	                                 -fno-tree-loop-distribute-patterns\"   \
	          \"BAREMETAL_SRCS_DIR=$(PWD)/../src/\"                            \
	          \"BAREMETAL_CPP_SRCS=$(SRCS_CPP)\"                            \
	          \"OTHER_LIB=$(PWD)/$(AIELIBAPU_ALIB)\"                        \
	          \"AIEMETASRC=$(AIE_BARMETALDATA)\""
#	mv $(AIE_DRIVER_PARENT_DIC)/lib/libxil.a $(AIELIBAPU_ALIB)

$(TARGETA78): $(AIELIBAPU78_ALIB) $(SRCS_CC) $(SRCS_CPP) $(AIE_BARMETALDATA) 
	bash -c "rm -f ../src/*.o;rm -f ../src/common_layer/*.o"
	bash -c "source $(PETALINUX_PATH)/settings.sh;                          \
	         make a78 -f ../Makefile                                            \
	          \"CC=aarch64-none-elf-gcc\"                             \
	          \"AS=aarch64-none-elf-as\"                             \
	          \"AR=aarch64-none-elf-ar\"                              \
	          \"CFLAGS= -O2 -c $(CFLAGS)                            \
	          			 -g -Wall -Wextra -Dversal -DARMA72_EL3 \
	                                 -fno-tree-loop-distribute-patterns\"   \
	          \"BAREMETAL_SRCS_DIR=$(PWD)/../src/\"                            \
	          \"BAREMETAL_CPP_SRCS=$(SRCS_CPP)\"                            \
	          \"OTHER_LIB=$(PWD)/$(AIELIBAPU78_ALIB)\"                        \
	          \"AIEMETASRC=$(AIE_BARMETALDATA)\""
#	mv $(AIE_DRIVER_PARENT_DIC)/lib/libxil.a $(AIELIBAPU_ALIB)

$(TARGETR53): $(AIELIBRPU_ALIB) $(SRCS_CC) $(SRCS_CPP) $(AIE_BARMETALDATA)
	bash -c "rm -f ../src/*.o;rm -f ../src/common_layer/*.o"
	bash -c "source $(PETALINUX_PATH)/settings.sh;                          \
	         make rpu -f ../Makefile                                            \
	          \"CC=armr5-none-eabi-gcc\"                             \
	          \"AS=armr5-none-eabi-as\"                             \
	          \"AR=armr5-none-eabi-ar\"                              \
	          \"CFLAGS= -O2 -c $(CFLAGS)                             \
	                                 -g -DARMR5 -Wall -Wextra -mfloat-abi=hard   \
	                                 -mfpu=vfpv3-d16                             \
	                                 -fno-tree-loop-distribute-patterns -Dversal \
	                                 -fno-tree-loop-distribute-patterns\"   \
	          \"BAREMETAL_SRCS_DIR=$(PWD)/../src/\"                            \
	          \"BAREMETAL_CPP_SRCS=$(SRCS_CPP)\"                            \
	          \"OTHER_LIB=$(PWD)/$(AIELIBRPU_ALIB)\"                        \
	          \"AIEMETASRC=$(AIE_BARMETALDATA)\""

$(TARGETR52): $(AIELIBRPUR52_ALIB) $(SRCS_CC) $(SRCS_CPP) $(AIE_BARMETALDATA)
	bash -c "rm -f ../src/*.o;rm -f ../src/common_layer/*.o"
	bash -c "source $(PETALINUX_PATH)/settings.sh;                          \
	         make rpu_r52 -f ../Makefile                                            \
	          \"CC=armr5-none-eabi-gcc\"                             \
	          \"AS=armr5-none-eabi-as\"                             \
	          \"AR=armr5-none-eabi-ar\"                              \
	          \"CFLAGS= -O2 -c $(CFLAGS)                             \
	                                 -g -DARMR5 -Wall -Wextra -mfloat-abi=hard   \
	                                 -mfpu=vfpv3-d16                             \
	                                 -fno-tree-loop-distribute-patterns -Dversal \
	                                 -fno-tree-loop-distribute-patterns\"   \
	          \"BAREMETAL_SRCS_DIR=$(PWD)/../src/\"                            \
	          \"BAREMETAL_CPP_SRCS=$(SRCS_CPP)\"                            \
	          \"OTHER_LIB=$(PWD)/$(AIELIBRPUR52_ALIB)\"                        \
	          \"AIEMETASRC=$(AIE_BARMETALDATA)\""

$(A72_EXAMPLE): $(TARGETA72)
	#bash -c "cd $(AIE_KERNEL_LOC)/script/; source ./builda72.sh ${AIE_GEN}"
	#bash -c "cd $(AIE_KERNEL_LOC)/; source ./build.sh ${AIE_GEN}"

$(A72_EXAMPLE_EXTERNAL): $(TARGETA72)

$(SCSIM_CJSON_O): $(SCSIM_CONFIG_JSON)
	cp $(SCSIM_CONFIG_JSON) ./
	ld -EL -r -b binary -o $@ $(SCSIM_CONFIG_JSON_FILE)
	rm ./$(SCSIM_CONFIG_JSON_FILE)

$(BUILD)/%.o: %.cpp
	echo $(AIE_KERNEL_LOC)
	echo $(MAKECMDGOALS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -D__AIESIM__ $(TEST_IS_LOCAL_MACRO) -c $< -o $@

$(BUILD)/%.o: %.cc
	echo $(AIE_KERNEL_LOC)
	echo $(MAKECMDGOALS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -D__AIESIM__ $(TEST_IS_LOCAL_MACRO) -c $< -o $@

$(BUILD)/%.o: %.c
	echo $(MAKECMDGOALS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -D__AIESIM__ $(TEST_IS_LOCAL_MACRO) -c $< -o $@

${UCFIRM_O}: $(AIELIB)
	bash -c "cd ./testuc/ucfirmware; source ./env.sh;make clean;make"
	bash -c "cd ./testuc/ucfirmware/output; ld -EL -r -b binary -o ucfw_exe.o ./app.elf"

# src/cert_elf.cc is a dummy file initially. Do not build cert if .cc file is available. Uncomment
# or rm src/cert_elf.cc to for build.
# .PHONY: ${CERT_CC}
${CERT_CC}:
	bash -c "cd ../thirdparty/cert/main; LOG_LEVEL=LOG_LEVEL_INFO_CONSOLE make"
	bash -c "cd $(CERT_DIR); cp ${CERT_ELF} cert.elf; xxd -i cert.elf ${CERT_CC}"

${CTRL_CODE_CC}:
	bash -c "cd ../thirdparty/cert-elf-binaries/; cp ${CTRL_CODE_ELF} ctrl_code.elf; xxd -i ctrl_code.elf ${CTRL_CODE_CC}"

$(TARGET_LWIP): $(TARGET_LWIP_XTF)
	cp $(TARGET_LWIP_XTF) $(TARGET_LWIP)

$(TARGET_LWIP_XTF): $(BUILD_DIR)
	../script/build_liblwip.sh

$(HW_LIB_XTF_ALL): $(TARGET_LWIP_XTF)
	$(AR) rvs $(HW_LIB_XTF_ALL) $(filter-out %main.o %echo.o, $(shell find ../thirdparty/xtf/ -name "*.o"))

#$(AIE_FAL_HPP):
#	bash -c "cd $(AIE_FAL); [ ! -d \"build\" ] && mkdir build; cd ./build; cmake ../ -DWITH_TESTS=OFF && make"

$(AIELIB):
#	make -C $(AIE_DRIVER)/src CFLAGS='$(ENABLE_AIE_RT_DEBUG) -g -D__AIELINUX__' -f Makefile.Linux
	bash -c "cd $(AIE_FAL); [ ! -d \"build\" ] && mkdir build; cd ./build; cmake ../  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DWITH_TESTS=OFF -DWITH_DOC=off && make"
	bash -c "cd $(AIE_FAL)/src/;make include"
	mkdir -p ./build
#	make -C $(AIE_DRIVER)/src CFLAGS='$(ENABLE_AIE_RT_DEBUG) -D__AIESIM__ -g -I../../../include/' -f Makefile.Linux
#	make -C $(AIE_DRIVER)/src CFLAGS='-D__AIESIM__ $(ENABLE_AIE_RT_DEBUG) -g -I../../../include/' -f Makefile.Linux
	bash -c "make -C $(AIE_DRIVER)/src CC=gcc CXX=g++ CFLAGS='-D__AIESIM__ -g -std=c99 -I../../../../../src/include/' -f Makefile.Linux"
	chmod +x ../script/aieldpath.sh
	../script/aieldpath.sh

alibclean:
	bash -c "cd $(AIE_DRIVER_PARENT_DIC); rm -rf ./include/*.h; mkdir -p ./include/xaiengine;"
	bash -c "cd $(AIE_DRIVER_PARENT_DIC); make -C aie-rt/driver/src -s include; make -C aie-rt/driver/src clean;"
	bash -c "cd $(AIE_KERNEL_LOC); rm -rf ./Work;"

	@if [ -f $(AIELIB) ]; then rm $(AIELIB); fi

	@if [ -f $(AIELIBAPU_ALIB) ]; then rm $(AIELIBAPU_ALIB); fi
	@if [ -f $(AIELIBAPU78_ALIB) ]; then rm $(AIELIBAPU78_ALIB); fi

	@if [ -f $(AIELIBRPU_ALIB) ]; then rm $(AIELIBRPU_ALIB); fi
	@if [ -f $(AIELIBRPUR52_ALIB) ]; then rm $(AIELIBRPUR52_ALIB); fi

$(AIELIBAPU_ALIB): $(AIE_CONTROL_JSON)
	make apuhw
$(AIELIBAPU78_ALIB): $(AIE_CONTROL_JSON)
	make apu78hw
apuhw:  $(AIELIB)
	bash -c "make clean -C $(AIE_DRIVER_PARENT_DIC)/aie-rt/driver/src/"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/include/*.h"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/lib/libxil.a"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/include/xaiengine"
	bash -c "cp -rf $(AIE_DRIVER_PARENT_DIC)/aie-rt/driver/internal/* $(AIE_DRIVER_PARENT_DIC)/include/"
	bash -c "cp $(ARCH_APU_A72_ARC_DIC)/*.h $(AIE_DRIVER_PARENT_DIC)/include/"
	bash -c "source $(PETALINUX_PATH)/settings.sh; cd $(AIE_DRIVER_PARENT_DIC); \
	         make -C aie-rt/driver/src -s libs                                  \
	          \"SHELL=/bin/sh\"                                                 \
	          \"COMPILER=aarch64-none-elf-gcc\"                                 \
	          \"ASSEMBLER=aarch64-none-elf-as\"                                 \
	          \"ARCHIVER=aarch64-none-elf-ar\"                                  \
	          \"COMPILER_FLAGS= -O2 -c $(ENABLE_AIE_RT_DEBUG) $(ENABLE_AIE_DEBUG_BACKEND) -DAIE_GEN=$(AIE_GEN)\"      \
	          \"EXTRA_COMPILER_FLAGS=-g -Wall -Wextra -Dversal -DARMA72_EL3     \
	                                 -fno-tree-loop-distribute-patterns\";"
	mv $(AIE_DRIVER_PARENT_DIC)/lib/libxil.a $(AIELIBAPU_ALIB)
	bash -c "source $(PETALINUX_PATH)/settings.sh; \
		cp $(ARCH_APU_A72_ARC_LIB)/origin/libxil.a $(ARCH_APU_A72_ARC_LIB)/libxil.a; \
		(echo \"OPEN $(ARCH_APU_A72_ARC_LIB)/libxil.a\"; echo \"ADDLIB $(AIELIBAPU_ALIB)\"; echo \"SAVE\"; echo \"END\") | aarch64-none-elf-ar -M;"

apu78hw:  $(AIELIB)
	bash -c "make clean -C $(AIE_DRIVER_PARENT_DIC)/aie-rt/driver/src/"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/include/*.h"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/lib/libxil.a"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/include/xaiengine"
	bash -c "cp -rf $(AIE_DRIVER_PARENT_DIC)/aie-rt/driver/internal/* $(AIE_DRIVER_PARENT_DIC)/include/"
	bash -c "cp $(ARCH_APU_A78_ARC_DIC)/*.h $(AIE_DRIVER_PARENT_DIC)/include/"
	bash -c "source $(PETALINUX_PATH)/settings.sh; cd $(AIE_DRIVER_PARENT_DIC); \
	         make -C aie-rt/driver/src -s libs                                  \
	          \"SHELL=/bin/sh\"                                                 \
	          \"COMPILER=aarch64-none-elf-gcc\"                                 \
	          \"ASSEMBLER=aarch64-none-elf-as\"                                 \
	          \"ARCHIVER=aarch64-none-elf-ar\"                                  \
	          \"COMPILER_FLAGS= -O2 -c -DXAIE_DEBUG -DAIE_GEN=$(AIE_GEN)\"      \
	          \"EXTRA_COMPILER_FLAGS=-g -Wall -Wextra -Dversal -DARMA72_EL3     \
	                                 -fno-tree-loop-distribute-patterns\";"
	mv $(AIE_DRIVER_PARENT_DIC)/lib/libxil.a $(AIELIBAPU78_ALIB)
	bash -c "source $(PETALINUX_PATH)/settings.sh;"
#	cp $(ARCH_APU_A78_ARC_LIB)/origin/libxil.a $(ARCH_APU_A72_ARC_LIB)/libxil.a; \
		(echo \"OPEN $(ARCH_APU_A78_ARC_LIB)/libxil.a\"; echo \"ADDLIB $(AIELIBAPU78_ALIB)\"; echo \"SAVE\"; echo \"END\") | aarch64-none-elf-ar -M;"


$(AIELIBRPU_ALIB): $(AIE_CONTROL_JSON)
	make rpuhw

$(AIELIBRPUR52_ALIB): $(AIE_CONTROL_JSON)
	make rpuhw_r52

rpuhw: $(AIELIB)
	bash -c "make clean -C $(AIE_DRIVER_PARENT_DIC)/aie-rt/driver/src/"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/./include/*.h"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/lib/libxil.a"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/include/xaiengine"
	bash -c "cp -rf $(AIE_DRIVER_PARENT_DIC)/aie-rt/driver/internal/* $(AIE_DRIVER_PARENT_DIC)/./include/"
	bash -c "cp $(ARCH_RPU_R53_ARC_DIC)/*.h $(AIE_DRIVER_PARENT_DIC)/include/"
	bash -c "source $(PETALINUX_PATH)/settings.sh; cd $(AIE_DRIVER_PARENT_DIC);  \
	         make -C aie-rt/driver/src -s libs                                   \
	          \"SHELL=/bin/sh\"                                                  \
	          \"COMPILER=armr5-none-eabi-gcc\"                                   \
	          \"ASSEMBLER=armr5-none-eabi-as\"                                   \
	          \"ARCHIVER=armr5-none-eabi-ar\"                                    \
	          \"COMPILER_FLAGS= -D__AIEBAREMETAL__ $(ENABLE_AIE_DEBUG_BACKEND)   \
	                            -O2 -c -mcpu=cortex-r5 -DAIE_GEN=$(AIE_GEN)\"    \
	          \"EXTRA_COMPILER_FLAGS=-g -DARMR5 -Wall -Wextra -mfloat-abi=hard   \
	                                 -mfpu=vfpv3-d16                             \
	                                 -fno-tree-loop-distribute-patterns -Dversal \
	                                 -fno-tree-loop-distribute-patterns\";"
	mv $(AIE_DRIVER_PARENT_DIC)/lib/libxil.a $(AIELIBRPU_ALIB)
	bash -c "source $(PETALINUX_PATH)/settings.sh; \
		cp $(ARCH_RPU_R53_ARC_LIB)/origin/libxil.a $(ARCH_RPU_R53_ARC_LIB)/libxil.a; \
		(echo \"OPEN $(ARCH_RPU_R53_ARC_LIB)/libxil.a\"; echo \"ADDLIB $(AIELIBRPU_ALIB)\"; echo \"SAVE\"; echo \"END\") | armr5-none-eabi-ar -M;"

rpuhw_r52: $(AIELIB)
	bash -c "make clean -C $(AIE_DRIVER_PARENT_DIC)/aie-rt/driver/src/"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/./include/*.h"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/lib/libxil.a"
	bash -c "rm -rf $(AIE_DRIVER_PARENT_DIC)/include/xaiengine"
	bash -c "cp -rf $(AIE_DRIVER_PARENT_DIC)/aie-rt/driver/internal/* $(AIE_DRIVER_PARENT_DIC)/./include/"
	bash -c "cp $(ARCH_RPU_R52_ARC_DIC)/*.h $(AIE_DRIVER_PARENT_DIC)/include/"
	bash -c "source $(PETALINUX_PATH)/settings.sh; cd $(AIE_DRIVER_PARENT_DIC);  \
	         make -C aie-rt/driver/src -s libs                                   \
	          \"SHELL=/bin/sh\"                                                  \
	          \"COMPILER=armr5-none-eabi-gcc\"                                   \
	          \"ASSEMBLER=armr5-none-eabi-as\"                                   \
	          \"ARCHIVER=armr5-none-eabi-ar\"                                    \
	          \"COMPILER_FLAGS= -D__AIEBAREMETAL__ $(ENABLE_AIE_DEBUG_BACKEND)   \
	                            -O2 -c -mcpu=cortex-r52 -DAIE_GEN=$(AIE_GEN)	\
				    -D_RPU_\"    \
	          \"EXTRA_COMPILER_FLAGS=-g -DARMR5 -Wall -Wextra -mfloat-abi=hard   \
	                                 -mfpu=vfpv3-d16                             \
	                                 -fno-tree-loop-distribute-patterns -Dversal \
	                                 -fno-tree-loop-distribute-patterns\";"
	mv $(AIE_DRIVER_PARENT_DIC)/lib/libxil.a $(AIELIBRPUR52_ALIB)
	bash -c "source $(PETALINUX_PATH)/settings.sh; \
		cp $(ARCH_RPU_R52_ARC_LIB)/origin/libxil.a $(ARCH_RPU_R52_ARC_LIB)/libxil.a; \
		(echo \"OPEN $(ARCH_RPU_R52_ARC_LIB)/libxil.a\"; echo \"ADDLIB $(AIELIBRPUR52_ALIB)\"; echo \"SAVE\"; echo \"END\") | armr5-none-eabi-ar -M;"

rpuexample: $(AIELIBRPU_ALIB)
	bash -c "cd ../arch/ps/psv_cortexr5_0/buildexample/; source ./build.sh"

####

$(AIE_BARMETALDATA): $(AIE_CONTROL_JSON)
	python3  $(AIE_METADATATOOL) $(AIE_CONTROL_JSON) $(AIE_PARTITION_JSON)
	mv baremetal_metadata.cpp $(BUILD)/

####

print:
	echo ${TEST_UC_C}

check_env:
	echo $(AIE_COMPILE_WORK)
	@if [ -z $(strip $(AIE_COMPILE_WORK)) ]; then echo "AIE_COMPILE_WORK not set, it should point to aie compiler output ./Work folder"; exit 1; fi
	@if [ ! -f $(strip $(AIE_CONTROL_JSON)) ]; then \
		echo "$(AIE_CONTROL_JSON) does not exist, please check AIE_COMPILE_WORK variable for ./Work folder"; \
		exit; \
	fi


.PHONY: example_lwip_echo
example_lwip_echo: $(HW_LIB_XTF_ALL) $(TARGETA72)
	CC=$(A72_CC) AR=$(A72_ARCHIEVE) make -C example_lwip_echo

cleanlib:
	rm -f $(TARGET)
	rm -f $(OBJS_CPP)
	rm -f $(OBJS_CC)
	rm -f $(SCSIM_CJSON_O)
	rm -f ./$(AIE_CONTROL_JSON_FILE)

.PHONY: deepclean

deepclean:
	find ../ \( -name "*.o" -o -name "*.d" -o -name "my_program" \) -type f -delete


.PHONY: clean
clean: alibclean cleanlib deepclean
	rm -f $(TESTTARGET)
	rm -f $(TESTSIMTARGET)
	rm -f $(AIELIB)
	rm -f $(OBJS_CPP)
	rm -f $(OBJS_CC)
	rm -f $(OBJ_TEST_UC_C)
	rm -f ${UCFIRM_O}
	rm -f ${CTRL_CODE_CC}
	rm -f $(UCFIRM_ELF)
	rm -f $(UCFIRM_APP_O)
	rm -f $(OBJ_TEST_AIE_C)
	rm -f ./build/*.o
	rm -f ./build/*.cpp
	rm -f ./build/*.bin
	rm -f ./build/*.BIN
	rm -f $(A72_EXAMPLE)
	rm -f $(TARGETA72)
	rm -f $(TARGETR53)
	rm -f $(TARGETR52)
	rm -f $(TARGET_LWIP)
	rm -f $(TARGET_LWIP_XTF)
	rm -f $(HW_LIB_XTF_ALL)
	make -C example_lwip_echo clean
	rm -rf ../src/ctrl_code_elf.cc
	rm -f ../src/include/generated_graphs.h

