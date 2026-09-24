# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
####################################################################################
#                            Makefile for this example
####################################################################################

TEST_BENCH               = src/graph.cpp

XILINX_VITIS_AIETOOLS  ?= $(XILINX_VITIS)/aietools

DEVICE_FILE              = xcvc1902-vsva2197-2MP-e-S

ifeq ($(AIE_GEN), 1)
	DEVICE_FILE              = xcvc1902-vsva2197-2MP-e-S
else ifeq ($(AIE_GEN), 2)
	DEVICE_FILE              = xcve2802-vsvh1760-2MP-e-S
else ifeq ($(AIE_GEN), 5)
	DEVICE_FILE              = xc2ve3858-ssva2112-2MP-e-S
endif

$(info $(DEVICE_FILE) "  gen is " $(AIE_GEN))

COMPILE_OPTS             = --disable-multirate-analysis --adf-api-log-level=3
WORK_DIR                 = Work
LOG_FILE                 = log.txt
SIM_OPTS                 =
REF_FILE                 = data/golden.txt
OUT_FILE                 = aiesimulator_output/data/output.txt

all: clean compile sim check_op 

.PHONY: check_vitis_for_aie
check_vitis_for_aie:
	@test -n "$(strip $(XILINX_VITIS))" || (echo 'ERROR: XILINX_VITIS is not set. Source ../../script/settings.sh or Vitis settings64.sh).' >&2; exit 1)

compile: check_vitis_for_aie
	aiecompiler $(TEST_BENCH) --part=$(DEVICE_FILE) $(COMPILE_OPTS) --workdir=$(WORK_DIR) --include=../../thirdparty/aielib/aie-rt/driver/include/ --include=./src --include=../../src/include/ --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api --include=$(XILINX_VITIS_AIETOOLS)/include/drivers/aiengine

compile-d: check_vitis_for_aie
	aiecompiler-debug $(TEST_BENCH) --part=$(DEVICE_FILE) $(COMPILE_OPTS) --workdir=$(WORK_DIR) --include=./src --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api --include=$(XILINX_VITIS_AIETOOLS)/include/drivers/aiengine --include=../../thirdparty/aielib/aie-rt/driver/include/

sim:
	aiesimulator $(SIM_OPTS) --pkg-dir=./$(WORK_DIR)

check_op:
	diff -s $(REF_FILE) $(OUT_FILE)

compilex86: check_vitis_for_aie
	aiecompiler $(TEST_BENCH) --part=$(DEVICE_FILE) $(COMPILE_OPTS) --target=x86sim --workdir=$(WORK_DIR) --include=./src --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api --include=$(XILINX_VITIS_AIETOOLS)/include/drivers/aiengine

simx86:
	x86simulator $(SIM_OPTS) --pkg-dir=./$(WORK_DIR)


.PHONY: clean
clean:
	@rm -rf Work Work_* $(WORK_DIR) *.lock temp
	rm -rf ???simulator_output NOC_* xnwOut .Xil xcd.log core.* 
	rm -rf log log* $(LOG_FILE) *.log 
	rm -rf libadf.a aiesimulator.log *.txt
	rm -rf .AIE_SIM_CMD_LINE_OPTIONS pl_sample_counts ISS_RPC_SERVER_PORT
	rm -rf data/output.txt data/aie_output.txt data/func_log.txt
	rm -rf PASS FAIL
	rm -f Map_Report.csv sol.db
	find . -name "*.swp" -delete; find . -name "*~" -delete;
	clear

