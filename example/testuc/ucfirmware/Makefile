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

include $(ME_ROOT)/microblaze/microblaze.mk

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
