# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved.

####################################################################################
#                            Makefile for this example
####################################################################################
TEST_BENCH               = resnet18_top.cpp
DEVICE_FILE              = 
COMPILE_OPTS             = --disable-multirate-analysis --enable-multi-layer --heapsize=1792 --stacksize=1400 --adf-api-log-level=3 --multi-layer-opt=2 --max-layer-ctrl-param-size=256 --Xelfgen="-j16" --print-timer-multi-layer-ctrl=1 --multi-layer-pipelining #--frontend-only
AIE_ARCH                 = aie-ml
WORK_DIR                 = Work
LOG_FILE                 = log.txt
SIM_OPTS                 =
#COM_INC                  = $(MLLIB_ROOT)/L1/include/common
#COM_GRAPH_INC            = $(MLLIB_ROOT)/L2/include/common

COM_INC                  = ../../mllib/L1/include/common/
COM_INC2                 = ../../mllib/L2/include/common/
COM_GRAPH_INC            = ../../mllib/L2/include/

AIE_DRIVER_INC           = $(XILINX_VITIS_AIETOOLS)/include/drivers/aiengine/
#XILINX_VITIS_AIETOOLS    = /proj/xbuilds/2024.1_INT_0522_2023/installs/lin64/Vitis/2024.1/aietools/

CUR_DIR	                 = ./
REF_FILE                 = data/ofm32_ref.txt
OUT_FILE                 = data/ofm32.txt
NUM_DMA_CH               = 4
COMPILE_FLAG             =
EXTRA_FLAG               =
DEVICE                   = xcve2802-vfvh1760-1LP-e-S-es1

all: clean compile sim check_op 

compile:
	aiecompiler $(TEST_BENCH) $(DEVICE_FILE) $(COMPILE_OPTS) $(COMPILE_FLAG) -Xpreproc=$(EXTRA_FLAG) --workdir=$(WORK_DIR) --include=$(AIE_DRIVER_INC) --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api --include=$(COM_INC2) --include=$(COM_INC) --include=$(COM_GRAPH_INC) --include=$(CUR_DIR) --part=$(DEVICE) $(EXTRA_AIE_CXXFLAGS)

sim:
	aiesimulator --disable-unused-mem-tiles $(SIM_OPTS) --pkg-dir=./$(WORK_DIR)

check_op:
	$(MLLIB_ROOT)/L1/regression/common/func_status.sh $(REF_FILE) $(OUT_FILE) data

run_hw_emu:
	cd hw_emu; ./run.sh
 
.PHONY: clean
clean:
	@rm -rf Work Work_* $(WORK_DIR) *.lock temp
	rm -rf aiesimulator_output aiesimulator_output* NOC_* xnwOut .Xil xcd.log core.* 
	rm -rf log log* $(LOG_FILE) *.log 
	rm -rf libadf.a aiesimulator.log *.txt Map_Report.csv sol.db
	rm -rf .AIE* pl_* ISS_RPC_SERVER_PORT
	rm -rf data/ofm32.txt data/aie_output.txt data/func_log.txt
	rm -rf PASS FAIL
	find -r . -name "*.swp" -delete; find . -name "*~" -delete;
	rm -rf hw_emu/vivado_prj vivdo*
	clear

