# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
CC?=aarch64-none-elf-gcc
AR?=aarch64-none-elf-ar

ARCH_DIR		?=thirdparty/arch/ps/psv_cortexa72_0/
MCPU			?=cortex-a72
LIB_AIEBAREMETAL_NAME	?=aiebaremetala72

C_FILES:=$(wildcard *.c)
O_FILES:=$(C_FILES:%.c=%.o)
INCLUDE_DIR:=	-I../../thirdparty/xtf/hw/export/hw/sw/hw/mybsp/bspinclude/include	\
		-I../../thirdparty/embeddedsw/ThirdParty/sw_services/lwip220/src/lwip-2.2.0/contrib/ports/xilinx/include/ \
		-I../../thirdparty/embeddedsw/ThirdParty/sw_services/lwip220/src/lwip-2.2.0/src/include	\
		-I../../thirdparty/embeddedsw/XilinxProcessorIPLib/drivers/ttcps/src/	\
		-I../../thirdparty/xtf/app/src/ \
		-I../../src/include/common/net/
APP?=./example_lwip_echo.elf

.PHONY: all
all: $(APP)
	
.PHONY:	$(APP)
$(APP):
	$(CC) -mcpu=${MCPU} -Wl,-T -Wl,$(ARCH_DIR)/lscript.ld $(DEFINES) $(INCLUDE_DIR) -L$(ARCH_DIR)/lib/ -L../build/ -o $(APP) $(C_FILES) -Wl,--start-group,-lxtflwipall,-l${LIB_AIEBAREMETAL_NAME},-lxil,-lgcc,-lc,-lstdc++,--end-group

.PHONY:
clean:
	rm -rf *.o *.a *.elf
