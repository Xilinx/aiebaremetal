# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
acc = aarch64-none-elf-gcc
rcc = armr5-none-eabi-gcc
PWD := $(shell pwd)
ARCH_DIR=$(PWD)/../../thirdparty/arch/ps/
ARCH_72_DIR=$(ARCH_DIR)/psv_cortexa72_0/
ARCH_R5_DIR=$(ARCH_DIR)/psv_cortexr5_0/
APUFLAG = -mcpu=cortex-a72 -Wl,-T -Wl,$(ARCH_72_DIR)/lscript.ld -I$(ARCH_72_DIR)/include/
RPUFLAG = -mcpu=cortex-r5 -mfpu=vfpv3-d16  -DARMR5 -Wall -Wextra -mfloat-abi=hard 
RPUFLAG += -fno-tree-loop-distribute-patterns -Dversal -fno-tree-loop-distribute-patterns
RPUFLAG += -T $(ARCH_R5_DIR)/lscript.ld -I$(ARCH_R5_DIR)/include/

APU_LD_FLAG = -L$(ARCH_72_DIR)/lib/origin -Wl,--start-group,-lxil,-lgcc,-lc,-lstdc++,--end-group
RPU_LD_FLAG = -L$(ARCH_R5_DIR)/lib/origin -Wl,--start-group,-lxil,-lgcc,-lc,-lstdc++,--end-group

all : apu rpu

apu: apu.c common.c
	$(acc) -o $@ $^ $(APUFLAG) $(APU_LD_FLAG)

rpu: rpu.c common.c
	$(rcc) -o $@ $^ $(RPUFLAG) $(RPU_LD_FLAG)

clean:
	rm apu
	rm rpu
