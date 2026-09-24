// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xaiengine.h>
#include "../../include/hw_config.h"

#define aielog(fmt, ...)	{printf("%s: %d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);fflush(stdout);}

extern "C" {
int test_aie_kernel_load(XAie_DevInst *DevInst);
int test_aie_shim(XAie_DevInst *DevInst);
int test_compare_lib_load(const char* lib_path, const char* name);
int xaie_test_memalloc(XAie_DevInst *DevInst);

static inline void hexdump(void *data, ssize_t size)
{
	int i;
	u_int8_t *buf = (u_int8_t *)data;

	for (i = 0; i < size; i++) {
		if (!(i % 16)) {
			printf("\n%08x: ", i);
		}
		printf("%02x ", buf[i]);
	}
	printf("\n");
}

}

int xaie_test_xaiemem(XAie_DevInst *DevInst);

