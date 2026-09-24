# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
include ../../../thirdparty/arch/microblaze/microblaze.mk

OUTPUT_DIR = output
SRCS = \
	platform.c \
	ucapp.c

OBJS = $(addprefix $(OUTPUT_DIR)/,$(SRCS:%.c=%.o))
CFLAGS += $(ADD_CFLAGS) -D_UCFW_

$(OUTPUT_DIR)/app.elf: $(OBJS)
	$(CC) -Wl,-T -Wl,lscript.ld $(CFLAGS) -o $@ $(OBJS) $(LFLAGS)

$(OUTPUT_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c -o $@ $(CFLAGS) $<

clean:
	rm -f ./$(OUTPUT_DIR)/*
