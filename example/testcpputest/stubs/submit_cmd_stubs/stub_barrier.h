// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Stub_Barrier_Reset(void);
uint32_t Stub_Barrier_GetWmbCount(void);
uint32_t Stub_Barrier_GetReadOnceCount(void);

void smp_wmb(void);

static inline uint32_t READ_ONCE(uint32_t *p)
{
    extern void Stub_Barrier_IncReadOnce(void);
    Stub_Barrier_IncReadOnce();
    (void)p;
    return 1u;
}

#ifdef __cplusplus
}
#endif
