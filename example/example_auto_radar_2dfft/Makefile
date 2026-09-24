# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
####################################################################################
#                            Makefile for this example
####################################################################################

TEST_BENCH               = src/graph.cpp 

DEVICE_FILE              = xcvc1902      # aie
DSP_LIB                  = dsp_lib/dsp
TAG ?= UUT
UUT_GRAPH ?= fft_ifft_dit_1ch_graph
DATA_TYPE_COL ?= cint32
DATA_TYPE_ROW ?= cint16
TWIDDLE_TYPE ?= cint16
PREPROC_ARGS ?= -DDATA_TYPE_ROW=$(DATA_TYPE_ROW) -DTWIDDLE_TYPE=$(TWIDDLE_TYPE) -DDATA_TYPE_COL=$(DATA_TYPE_COL)  
UUT_PREPROC_ARGS ?= -DUUT_GRAPH=$(UUT_GRAPH) $(PREPROC_ARGS) 
AIE_GEN ?= 2

ifeq ($(AIE_GEN), 1)
	DEVICE_FILE              = xcvc1902      # aie
else ifeq ($(AIE_GEN), 2)
	DEVICE_FILE              = xcvc2802      # aie-ml
else ifeq ($(AIE_GEN), 5)
	DEVICE_FILE              = xcve3858      # aie2ps
endif

$(info $(DEVICE_FILE) "  gen is " $(AIE_GEN))

COMPILE_OPTS             = --disable-multirate-analysis
WORK_DIR                 = Work
LOG_FILE                 = log.txt
SIM_OPTS                 =
REF_FILE                 = data/golden.txt
OUT_FILE                 = aiesimulator_output/data/output.txt

#all: clean compile sim check_op 
all: clean compile 

compile:
# aiecompiler $(TEST_BENCH) --part=$(DEVICE_FILE) -Xpreproc="$($(TAG)_PREPROC_ARGS)" $(COMPILE_OPTS) --workdir=$(WORK_DIR) --include=./src --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api --include=$(DSP_LIB)/L1/include/aie --include=$(DSP_LIB)/L1/include/hw --include=$(DSP_LIB)/L1/src/aie --include=$(DSP_LIB)/L1/src/hw --include=$(DSP_LIB)/L1/tests/aie/inc --include=$(DSP_LIB)/L1/tests/aie/src --include=$(DSP_LIB)/L2/include/aie --include=$(DSP_LIB)/L2/include/hw --include=$(DSP_LIB)/L2/tests/aie/common/inc --include=../thirdparty/aielib/aie-rt/driver/include/
	unzip src/input_golden.zip
	mv input.data src/
	mv golden.data src/
	
	aiecompiler $(TEST_BENCH) --part=$(DEVICE_FILE) -Xpreproc="$($(TAG)_PREPROC_ARGS)" $(COMPILE_OPTS) --workdir=$(WORK_DIR) --include=./src --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api --include=$(DSP_LIB)/L1/include/aie --include=$(DSP_LIB)/L1/include/hw --include=$(DSP_LIB)/L1/src/aie --include=$(DSP_LIB)/L1/src/hw --include=$(DSP_LIB)/L1/tests/aie/inc --include=$(DSP_LIB)/L1/tests/aie/src --include=$(DSP_LIB)/L2/include/aie --include=$(DSP_LIB)/L2/include/hw --include=$(DSP_LIB)/L2/tests/aie/common/inc --include=../../thirdparty/aielib/aie-rt/driver/include/ --include=../../include

compile-d:
	aiecompiler-debug $(TEST_BENCH) --part=$(DEVICE_FILE) $(COMPILE_OPTS) --workdir=$(WORK_DIR) --include=./src --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api 

sim:
	aiesimulator $(SIM_OPTS) --pkg-dir=./$(WORK_DIR)

check_op:
	diff -s $(REF_FILE) $(OUT_FILE)

compilex86:
	aiecompiler $(TEST_BENCH) --part=$(DEVICE_FILE) $(COMPILE_OPTS) --target=x86sim --workdir=$(WORK_DIR) --include=./src --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api 

simx86:
	x86simulator $(SIM_OPTS) --pkg-dir=./$(WORK_DIR)


.PHONY: clean
clean:
	@rm -rf Work Work_* $(WORK_DIR) *.lock temp
	rm -rf ???simulator_output NOC_* xnwOut .Xil xcd.log core.* 
	rm -rf log log* $(LOG_FILE) *.log 
	rm -rf libadf.a aiesimulator.log *.txt
	rm -rf .AIE* pl_* ISS_RPC_SERVER_PORT
	rm -rf data/output.txt data/aie_output.txt data/func_log.txt
	rm -rf PASS FAIL
	find . -name "*.swp" -delete; find . -name "*~" -delete;
	clear

