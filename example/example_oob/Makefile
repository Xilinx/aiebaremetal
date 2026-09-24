# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
####################################################################################
#                            Makefile for this example
####################################################################################

TEST_BENCH               = src/graph.cpp

DEVICE_FILE              = xcvc1902      # aie

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

all: clean compile sim check_op 

compile:
	aiecompiler $(TEST_BENCH) --part=$(DEVICE_FILE) $(COMPILE_OPTS) --workdir=$(WORK_DIR) --include=./src --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api --include=../thirdparty/aielib/aie-rt/driver/include/

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

