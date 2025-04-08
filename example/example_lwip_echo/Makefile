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

CC?=aarch64-none-elf-gcc
AR?=aarch64-none-elf-ar

ARCH_DIR?=../arch/
KERNEL_DIR?=$(ARCH_DIR)/kernels/
ARCH_72_DIR?=$(ARCH_DIR)/psv_cortexa72_0/
C_FILES:=$(wildcard *.c)
O_FILES:=$(C_FILES:%.c=%.o)
INCLUDE_DIR:=	-I../thirdparty/xtf/hw/export/hw/sw/hw/mybsp/bspinclude/include	\
		-I../thirdparty/embeddedsw/ThirdParty/sw_services/lwip220/src/lwip-2.2.0/contrib/ports/xilinx/include/ \
		-I../thirdparty/embeddedsw/ThirdParty/sw_services/lwip220/src/lwip-2.2.0/src/include	\
		-I../thirdparty/embeddedsw/XilinxProcessorIPLib/drivers/ttcps/src/	\
		-I../thirdparty/xtf/app/src/
APP?=./testlwip_echo.elf

.PHONY: all
all: $(APP)
	
.PHONY:	$(APP)
$(APP):
	$(CC) -mcpu=cortex-a72 -Wl,-T -Wl,$(ARCH_72_DIR)/lscript.ld $(DEFINES) $(INCLUDE_DIR) -L$(ARCH_72_DIR)/lib/ -L../build/ -o $(APP) $(C_FILES) -Wl,--start-group,-lxtflwipall,-laiebaremetala72,-lxil,-lgcc,-lc,-lstdc++,--end-group

.PHONY:
clean:
	rm -rf *.o *.a *.elf
