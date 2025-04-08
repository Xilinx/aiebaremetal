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
