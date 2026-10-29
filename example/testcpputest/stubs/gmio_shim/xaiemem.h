// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef XAIEMEM_H_
#define XAIEMEM_H_

#include <cstdint>
#include <xaiengine.h>

class XAieMem {
  public:

    inline static std::int32_t s_get_dev_addr_ret = 0;
    inline static std::int32_t s_sync_for_dev_ret = 0;
    inline static std::int32_t s_sync_for_cpu_ret = 0;
    inline static std::int32_t s_free_ret          = 0;
    inline static void        *s_malloc_ret        = nullptr;

    XAie_MemCacheProp Cache;

    explicit XAieMem(XAie_DevInst * ) noexcept : Cache(XAIE_MEM_NONCACHEABLE) {}

    void *malloc(ssize_t ) const noexcept { return s_malloc_ret; }

    std::int32_t free(void * ) const noexcept { return s_free_ret; }

    std::int32_t sync_for_cpu(void * , std::uint64_t ) const noexcept {
        return s_sync_for_cpu_ret;
    }

    std::int32_t sync_for_dev(void * , std::uint64_t ) const noexcept {
        return s_sync_for_dev_ret;
    }

    std::int32_t get_dev_addr(void * , std::uint64_t * ) const noexcept {
        return s_get_dev_addr_ret;
    }

    static void reset_test_state() noexcept {
        s_get_dev_addr_ret = 0;
        s_sync_for_dev_ret = 0;
        s_sync_for_cpu_ret = 0;
        s_free_ret         = 0;
        s_malloc_ret       = nullptr;
    }
};

#endif
