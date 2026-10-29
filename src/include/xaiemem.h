/**<!--
 * Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
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
 * -->
 */
#ifndef XAIEMEM_H_
#define XAIEMEM_H_

#include <cstdint>
#include <cstdlib>
#include <xaiengine.h>
namespace xaiemem {
class XAieMem {
  public:
    using AllocHostFn = void *(*)(uint32_t size);
    using FreeHostFn  = void (*)(void *host);
    using DevAddrFn   = uint64_t (*)(const void *host);
    using WriteGmFn   = void (*)(uint64_t aie_addr, const void *data, size_t size);
    using ReadGmFn    = void (*)(uint64_t aie_addr, void *buf, size_t size);

    explicit XAieMem(XAie_DevInst *Inst) : DevInst_(Inst), ipc_(false) {}

    XAieMem(AllocHostFn alloc, FreeHostFn freefn, DevAddrFn devaddr, WriteGmFn write, ReadGmFn read)
        : DevInst_(nullptr), ipc_(true), alloc_host_(alloc), free_host_(freefn),
          dev_addr_(devaddr), write_gm_(write), read_gm_(read) {}

    void *malloc(ssize_t Size) const {
        if (ipc_) {
            return alloc_host_(static_cast<uint32_t>(Size));
        }
        XAie_MemInst *MemInst = XAie_MemAllocate(DevInst_, static_cast<std::uint64_t>(Size), Cache);
        if (!MemInst) {
            return nullptr;
        }
        return MemInst->VAddr;
    }

    std::int32_t free(void *Addr) const {
        if (ipc_) {
            free_host_(Addr);
            return 0;
        }
        return XAie_MemFreeVAddr(DevInst_, Addr);
    }

    std::int32_t sync_for_cpu(void *VAddr, std::uint64_t Size) const {
        if (ipc_) {
            uint64_t dev_addr = dev_addr_(VAddr);
            if (dev_addr == 0U) {
                return -1;
            }
            read_gm_(dev_addr, VAddr, static_cast<size_t>(Size));
            return 0;
        }
        return XAie_MemSyncForCPUVAddr(DevInst_, VAddr, Size);
    }

    std::int32_t sync_for_dev(void *VAddr, std::uint64_t Size) const {
        if (ipc_) {
            uint64_t dev_addr = dev_addr_(VAddr);
            if (dev_addr == 0U) {
                return -1;
            }
            write_gm_(dev_addr, VAddr, static_cast<size_t>(Size));
            return 0;
        }
        return XAie_MemSyncForDevVAddr(DevInst_, VAddr, Size);
    }

    std::int32_t get_dev_addr(void *VAddr, std::uint64_t *DevAddr) const {
        if (ipc_) {
            uint64_t dev_addr = dev_addr_(VAddr);
            if (dev_addr == 0U) {
                return -1;
            }
            *DevAddr = dev_addr;
            return 0;
        }
        return XAie_MemGetDevAddrFromVAddr(DevInst_, VAddr, DevAddr);
    }

  private:
    XAie_MemCacheProp Cache = XAIE_MEM_NONCACHEABLE;

    XAie_DevInst *DevInst_;
    bool          ipc_;

    AllocHostFn alloc_host_ = nullptr;
    FreeHostFn  free_host_  = nullptr;
    DevAddrFn   dev_addr_   = nullptr;
    WriteGmFn   write_gm_   = nullptr;
    ReadGmFn    read_gm_    = nullptr;
};
} // namespace xaiemem
#endif /* XAIEMEM_H_ */
