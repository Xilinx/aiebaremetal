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

#include "xaiemem.h"
#include "xaie_test_all.h"
#include <iostream>
#include <stddef.h>

#define TEST_COUNT	50
#define MEMINST_COUNT	200
#define MEMINST_MAX_SIZE	4096

extern "C" {
void XAie_SimDumpDevMem(XAie_DevInst *DevInst);
ssize_t XAie_SimDevmemFreeSize(XAie_DevInst *DevInst);
ssize_t XAie_SimDevmemUsedSize(XAie_DevInst *DevInst);

static int xaie_test_memsync(XAie_MemInst *MemInst)
{
	u_int8_t *buf;

	buf = (u_int8_t *)malloc(MemInst->Size);
	if (!buf)
		return -1;

	memset(MemInst->VAddr, 0, MemInst->Size);
	XAie_MemSyncForDev(MemInst);

	for (int i = 0; i < MemInst->Size; i++) {
		((u_int8_t *)MemInst->VAddr)[i] = rand() % 127;
	}
	XAie_MemSyncForCPU(MemInst);
	for (int i = 0; i < MemInst->Size; i++) {
		if (((u_int8_t *)MemInst->VAddr)[i]) {
			aielog("MemSync failed byte %d: 0x%x", i, ((u_int8_t *)MemInst->VAddr)[i]);
			return -1;
		}
	}

	for (int i = 0; i < MemInst->Size; i++) {
		buf[i] = rand() % 127;
	}
	memcpy(MemInst->VAddr, buf, MemInst->Size);
	XAie_MemSyncForDev(MemInst);
	memset(MemInst->VAddr, 0 ,MemInst->Size);
	XAie_MemSyncForCPU(MemInst);
	if (memcmp(buf, MemInst->VAddr, MemInst->Size)) {
		aielog("Mem sync test failed.: %d", memcmp(buf, MemInst->VAddr, MemInst->Size));
		return -1;
	}

	return 0;
}

int xaie_test_memalloc(XAie_DevInst *DevInst)
{
	for (int i = 0; i < TEST_COUNT; i++) {
		XAie_MemInst *MemInst[MEMINST_COUNT];
		ssize_t Used;
		ssize_t Free;

		for (int j = 0; j < MEMINST_COUNT; j++) {
			ssize_t Size = rand() % MEMINST_MAX_SIZE;

			Size = !Size ? MEMINST_MAX_SIZE : Size;
			MemInst[j] = XAie_MemAllocate(DevInst, Size,
						      XAIE_MEM_NONCACHEABLE);
			if (!MemInst[j]) {
				aielog("Failed to allocate MemInst.");
				return -1;
			}

		}

		for (int j = 0; j < MEMINST_COUNT; j++) {
			if (xaie_test_memsync(MemInst[j])) {
				return -1;
			}
		}

		for (int j = 0; j < MEMINST_COUNT; j++) {
			int k = rand() % MEMINST_COUNT;

			if (!MemInst[k])
				continue;
			XAie_MemFree(MemInst[k]);
			MemInst[k] = NULL;
		}

		for (int j = 0; j < MEMINST_COUNT; j++) {
			if (!MemInst[j])
				continue;
			XAie_MemFree(MemInst[j]);
		}
		XAie_SimDumpDevMem(DevInst);
		Used = XAie_SimDevmemUsedSize(DevInst);
		Free = XAie_SimDevmemFreeSize(DevInst);
		if (Used) {
			aielog("Used Size Not 0. TEST FAILED.");
			return -1;
		}
		if (Free != 0xc0000000) {
			aielog("Free size: 0x%llx != 0xc0000000. TEST FAILED.", Free);
			return -1;
		}
		aielog("TEST PASS ITERATION: %d", i);
	}

	return 0;
}

} /* extern "C" */

static int xaie_test_memsync_vaddr(XAieMem &mem, void *addr, uint64_t size)
{
	u_int8_t *buf;
	int ret = 420;

	buf = (u_int8_t *)malloc(size);

	if (!buf)
		return -1;

	ret = mem.sync_for_dev(addr, size);
	if (ret)
		goto ret;

	memset(addr, 0, size);
	if (ret)
		goto ret;
	ret = mem.sync_for_dev(addr, size);
	if (ret)
		goto ret;

	for (int i = 0; i < size; i++) {
		((u_int8_t *)addr)[i] = rand() % 127;
	}
	ret = mem.sync_for_cpu(addr, size);
	if (ret) {
		aielog("Mem Sync failed: %d\n", ret);
		goto ret;
	}

	for (int i = 0; i < size; i++) {
		if (((u_int8_t *)addr)[i]) {
			aielog("MemSync failed byte %d: 0x%x", i, ((u_int8_t *)addr)[i]);
			aielog("Addr: 0x%llx, Size: %llu\n", addr, size);
			goto ret;
		}
	}

	for (int i = 0; i < size; i++) {
		buf[i] = rand() % 127;
	}
	memcpy(addr, buf, size);
	ret = mem.sync_for_dev(addr, size);
	if (ret) {
		aielog("Mem Sync failed: %d\n", ret);
		goto ret;
	}


	memset(addr, 0 ,size);
	ret = mem.sync_for_cpu(addr, size);
	if (ret) {
		aielog("Mem Sync failed: %d\n", ret);
		goto ret;
	}

	if (memcmp(buf, addr, size)) {
		aielog("Mem sync test failed.: %d", memcmp(buf, addr, size));
		goto ret;
	}
ret:
	free(buf);
	return ret;
}

int xaie_test_xaiemem(XAie_DevInst *DevInst)
{
	XAieMem mem(DevInst);
	int ret;

	aielog("xaie_test_xaiemem:\n");

	for (int i = 0; i < TEST_COUNT; i++) {
		void *addr[MEMINST_COUNT];
		uint64_t size[MEMINST_COUNT];
		ssize_t Used;
		ssize_t Free;

		for (int j = 0; j < MEMINST_COUNT; j++) {
			ssize_t Align;

			size[j] = rand() % MEMINST_MAX_SIZE;
			size[j] = !size[j] ? MEMINST_MAX_SIZE : size[j];
			Align = size[j] % 4;
			size[j] = size[j] + (4 - Align);
			addr[j] = mem.malloc(size[j]);
			if (!addr[j]) {
				aielog("Failed to allocate MemInst.");
				return -1;
			}

		}

		for (int j = 0; j < MEMINST_COUNT; j++) {
			ret = xaie_test_memsync_vaddr(mem, addr[j], size[j]);
			if (ret) {
				aielog("xaie_test_memsync_vaddr failed.\n");
				return -1;
			}

			ret = xaie_test_memsync_vaddr(mem, addr[j], 3);
			if (!ret) {
				aielog("This test should have failed. Got success.");
				return -1;
			}

			ret = xaie_test_memsync_vaddr(mem, addr[j], size[j] + 4);
			if (!ret) {
				aielog("This test should have failed. Got success.");
				return -1;
			}

			if (!(size[j] < 256)) {
				ret = xaie_test_memsync_vaddr(mem, addr[j] + 128, size[j] - 128);
				if (ret) {
					aielog("Test failed: %d\n", ret);
					return -1;
				}
				ret = xaie_test_memsync_vaddr(mem, addr[j], 128);
				if (ret) {
					aielog("Test failed: %d\n", ret);
					return -1;
				}

				ret = xaie_test_memsync_vaddr(mem, addr[j] + size[j] - 8, 8);
				if (ret) {
					aielog("Test failed: %d\n", ret);
					return -1;
				}

				ret = xaie_test_memsync_vaddr(mem, addr[j] - 128, size[j]);
				if (!ret) {
					aielog("This test should have failed: %d\n", ret);
					return -1;
				}

			}

		}

		for (int j = 0; j < MEMINST_COUNT; j++) {
			int k = rand() % MEMINST_COUNT;
			if (!addr[k])
				continue;
			ret = mem.free(addr[k]);

			if (ret) {
				aielog("Failed to free mem: %d", ret);
				return -1;
			}
			addr[k] = NULL;
		}

		for (int j = 0; j < MEMINST_COUNT; j++) {
			if (!addr[j])
				continue;
			ret = mem.free(addr[j]);
			if (ret) {
				aielog("Failed to free mem: %d", ret);
				return -1;
			}
		}
		XAie_SimDumpDevMem(DevInst);
		Used = XAie_SimDevmemUsedSize(DevInst);
		Free = XAie_SimDevmemFreeSize(DevInst);
		if (Used) {
			aielog("Used Size Not 0. TEST FAILED.");
			return -1;
		}
		if (Free != 0xc0000000) {
			aielog("Free size: 0x%llx != 0xc0000000. TEST FAILED.", Free);
			return -1;
		}
		aielog("TEST PASS ITERATION: %d", i);
	}

	return 0;
}

