# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
ifeq ($(XILINX_VITIS_AIETOOLS),)
XILINX_VITIS_AIETOOLS:=${XILINX_VITIS}/aietools
endif
.PHONY: all clean
.NOTPARALLEL:
all: clean run
 

compile:
	mkdir -p ./generated-objects
	(${XILINX_VITIS_AIETOOLS}/tps/lnx64/gcc/bin/g++ -fPIC -fpermissive -g -std=c++17 -Wno-deprecated-declarations -Werror=return-type -D__AIE_ARCH__=20 -D__LOCK_FENCE_MODE__=0 -DAIE_OPTION_SCALAR_FLOAT_ON_VECTOR -DAIE2_FP32_EMULATION_ACCURACY_FAST -DSC_INCLUDE_DYNAMIC_PROCESSES -D__TXN_BIN__ -D__PS_INIT_AIE__ -DXAIE_DEBUG -I ../../Work/ps/c_rts/ -I Work/ps/c_rts -I ${XILINX_VITIS_AIETOOLS}/include -I ${XILINX_VITIS_AIETOOLS}/include/drivers/aiengine -o ./generated-objects/genTxnMain.out ./generated-source/gen_txn.cpp ./generated-source/gen_txn_main.cpp  -Wl,--allow-shlib-undefined -L ${XILINX_VITIS_AIETOOLS}/lib/lnx64.o -lxaiengine -ladf_api -ladf_rt_ctrl_api )

write_txn_bin: compile
	(./generated-objects/genTxnMain.out --write-txn "./ml_txn.bin")

read_txn_bin:
	(./generated-objects/genTxnMain.out --read-txn "./ml_txn.bin")

clean:
	(rm -rf generated-objects/*)
