/**
* Copyright (C) 2024 Advanced Micro Devices, Inc.
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
#ifndef _XAIEMEM_H_
#define _XAIEMEM_H_

#include <xaiengine.h>
//#include "../include/main_rts.h"

class XAieMem {
private:
	XAie_DevInst *DevInst;

public:
	XAie_MemCacheProp Cache;

	XAieMem(XAie_DevInst *_DevInst): DevInst(_DevInst) {
		Cache = XAIE_MEM_NONCACHEABLE;
	}

	void *malloc(ssize_t Size) {
		XAie_MemInst *MemInst;

		MemInst = XAie_MemAllocate(DevInst, Size, Cache);

		if (!MemInst)
			return nullptr;
		return MemInst->VAddr;
	}

	int free(void *Addr) {
		return XAie_MemFreeVAddr(DevInst, Addr);
	}

	int sync_for_cpu(void *VAddr, uint64_t Size) {
		return XAie_MemSyncForCPUVAddr(DevInst, VAddr, Size);
	}

	int sync_for_dev(void *VAddr, uint64_t Size) {
		return XAie_MemSyncForDevVAddr(DevInst, VAddr, Size);
	}

	int get_dev_addr(void *VAddr, uint64_t *DevAddr) {
		return XAie_MemGetDevAddrFromVAddr(DevInst, VAddr, DevAddr);
	}
};

#endif /* _XAIEMEM_H_ */
