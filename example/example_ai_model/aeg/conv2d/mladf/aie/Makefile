# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved.

####################################################################################
#                            Makefile for this example
####################################################################################
TEST_BENCH               = test_conv2d_4x4.cpp
DEVICE_FILE              =
COMPILE_OPTS             = --enable-multi-layer --multi-layer-ipu-txn-bin-sim
AIE_ARCH                 = xcve2802-vfvh1760-1LP-e-S-es1
#only AIG device will can geneate the transaction binary
WORK_DIR                 = Work
LOG_FILE                 = log.txt
SIM_OPTS                 = --profile
EXTRA_DEFINES            =
CUR_DIR	                 = ./
COM_INC                  = ../common/conv2d_4x4/L1/include/common/
COM_GRAPH_INC            = ../common/conv2d_4x4/L2/include/common/
AIE_DRIVER_INC           = /proj/xbuilds/2024.1_INT_0522_2023/installs/lin64/Vitis/2024.1/aietools/include/drivers/aiengine/
REF_FILE                 = data/ofm32_ref.txt
OUT_FILE                 = data/ofm32.txt

# Graph Parameters
NUM_COLS                 = 4
NUM_ROWS                 = 4
# Tensor sizes
IFM_HEIGHT               = 72
IFM_WIDTH                = 72
IFM_DEPTH                = 64
OFM_DEPTH                = 64
FILTER_K                 = 3
NUM_BATCH                = 1
# Sub Volume (SV) Sizes. One SV is processed at a time by one core
IFM_SV_HEIGHT            = 20
IFM_SV_WIDTH             = 18
IFM_SV_DEPTH             = 16
OFM_SV_DEPTH             = 16

OFM_HEIGHT               = 72
OFM_WIDTH                = 72
OFM_SV_HEIGHT            = 18
OFM_SV_WIDTH             = 16

# activation type fpr conv
ACT                      = LINEAR
#  full 8-bit mode writing output (no partial accum)
DTYPE_IFM                = int8
DTYPE_WTS                = int8
DTYPE_OUT                = int8


GRAPH_PARAMS             = "-Xpreproc=-DNUM_ROWS=$(NUM_ROWS)  -DNUM_COLS=$(NUM_COLS)  -DIFM_WIDTH=$(IFM_WIDTH) -DIFM_HEIGHT=$(IFM_HEIGHT) -DIFM_DEPTH=$(IFM_DEPTH) \
                            -DOFM_DEPTH=$(OFM_DEPTH) -DFILTER_K=$(FILTER_K) -DNUM_BATCH=$(NUM_BATCH) -DIFM_SV_HEIGHT=$(IFM_SV_HEIGHT) -DIFM_SV_WIDTH=$(IFM_SV_WIDTH) \
                            -DIFM_SV_DEPTH=$(IFM_SV_DEPTH) -DOFM_SV_DEPTH=$(OFM_SV_DEPTH) -DDTYPE_IFM=$(DTYPE_IFM) -DDTYPE_WTS=$(DTYPE_WTS) -DDTYPE_OUT=$(DTYPE_OUT) \
                            -DACT=$(ACT) -DOFM_WIDTH=$(OFM_WIDTH) -DOFM_HEIGHT=$(OFM_HEIGHT) -DOFM_SV_WIDTH=$(OFM_SV_WIDTH) -DOFM_SV_HEIGHT=$(OFM_SV_HEIGHT)"

all: clean compile sim check_op

compile:
	aiecompiler --target=hw $(TEST_BENCH) $(DEVICE_FILE) $(COMPILE_OPTS) --part=$(AIE_ARCH) --workdir=$(WORK_DIR) --include=$(XILINX_VITIS_AIETOOLS)/include/drivers/aiengine/ --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api --include=$(COM_INC) --include=$(COM_GRAPH_INC) --include=$(CUR_DIR) $(GRAPH_PARAMS) -Xpreproc=$(EXTRA_DEFINES) $(EXTRA_AIE_CXXFLAGS)

sim:
	aiesimulator $(SIM_OPTS) --pkg-dir=./$(WORK_DIR)

check_op:
	$(MLLIB_ROOT)/L2/test/common/check_op.sh $(REF_FILE) $(OUT_FILE)

.PHONY: clean
clean:
	@rm -rf Work Work_* $(WORK_DIR) *.lock temp
	rm -rf aiesimulator_output aiesimulator_output* NOC_* xnwOut .Xil xcd.log core.*
	rm -rf log log* $(LOG_FILE) *.log
	rm -rf libadf.a aiesimulator.log *.txt
	rm -rf .AIE* pl_* ISS_RPC_SERVER_PORT
	rm -rf data/ofm32.txt data/aie_output.txt data/func_log.txt
	rm -rf PASS FAIL
	find . -name "*.swp" -delete; find . -name "*~" -delete;
	clear

