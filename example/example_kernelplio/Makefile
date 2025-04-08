# Copyright (C) 2025 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may
# not use this file except in compliance with the License. A copy of the
# License is located at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

####################################################################################
#                            Makefile for this example
####################################################################################

TEST_BENCH               = src/graph.cpp

DEVICE_FILE              = xcvc1902      # aie

ifeq ($(AIE_GEN), 1)
	DEVICE_FILE              = xcvc1902      # aie
else ifeq ($(AIE_GEN), 2)
	DEVICE_FILE              = xcvc2802      # aie-ml
  #DEVICE_FILE              = xc10AIE24x5   # aie-ml IPU
  #DEVICE_FILE              = xc10AIE2P_ML  # aie2p
else ifeq ($(AIE_GEN), 5)
	#DEVICE_FILE              = xc10Turin     # aie2ps Turin
	DEVICE_FILE              = xcve3858      # aie2ps Telluride T50
endif

$(info $(DEVICE_FILE) "  gen is " $(AIE_GEN))

COMPILE_OPTS             = --disable-multirate-analysis --adf-api-log-level=3
WORK_DIR                 = Work
LOG_FILE                 = log.txt
SIM_OPTS                 =
REF_FILE                 = data/golden.txt
OUT_FILE                 = aiesimulator_output/data/output.txt

all: clean compile sim check_op 

compile:
	aiecompiler $(TEST_BENCH) --part=$(DEVICE_FILE) $(COMPILE_OPTS) --workdir=$(WORK_DIR) --include=../../thirdparty/aielib/aie-rt/driver/include/ --include=./src --include=../../include/ --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api

compile-d:
	aiecompiler-debug $(TEST_BENCH) --part=$(DEVICE_FILE) $(COMPILE_OPTS) --workdir=$(WORK_DIR) --include=./src --include=$(XILINX_VITIS_AIETOOLS)/include/aie_api --include=../../thirdparty/aielib/aie-rt/driver/include/

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
	rm -rf .AIE_SIM_CMD_LINE_OPTIONS pl_sample_counts ISS_RPC_SERVER_PORT
	rm -rf data/output.txt data/aie_output.txt data/func_log.txt
	rm -rf PASS FAIL
	rm -f Map_Report.csv sol.db
	find . -name "*.swp" -delete; find . -name "*~" -delete;
	clear

