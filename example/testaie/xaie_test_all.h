/**
* Copyright (C) 2025 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

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

static void hexdump(void *data, ssize_t size)
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

