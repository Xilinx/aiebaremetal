// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "xaiengine.h"

void     Stub_XAie_InitQueue(XAie_MemInst *hsa_meminst, XAie_MemInst *comp_sig_meminst, uint32_t entries);
void     Stub_XAie_Reset(void);
void     Stub_XAie_SetCompletionBehavior(uint32_t index, const uint32_t *sequence, uint32_t length);
uint32_t Stub_XAie_GetCompSyncCount(uint32_t index);
uint64_t Stub_XAie_GetCompletionDevAddr(uint32_t index);

XAie_MemInst *Stub_XAie_GetQueueMemInst(void);
XAie_MemInst *Stub_XAie_GetCompMemInst(void);

void     Stub_AieMem_SetAllocateFailIndex(int index);
void     Stub_AieMem_ResetAll(void);
int      Stub_AieMem_GetAllocateCallCount(void);
int      Stub_AieMem_GetFreeCallCount(void);

extern uint32_t g_stub_comp_sig_value;

void    *XAie_MemGetVAddr(XAie_MemInst *mem);
uint64_t XAie_MemGetDevAddr(XAie_MemInst *mem);
int      XAie_MemSyncForCPU(XAie_MemInst *mem);
int      XAie_MemSyncForDev(XAie_MemInst *mem);
XAie_DevInst *Stub_XAie_CreateMinimalDevInst(void);
#ifdef __cplusplus
}
#endif
