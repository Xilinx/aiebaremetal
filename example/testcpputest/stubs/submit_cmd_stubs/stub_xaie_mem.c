// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "stub_xaie_mem.h"
#include <string.h>
#include <stdint.h>

#ifndef HSA_QUEUE_ENTRY
#define HSA_QUEUE_ENTRY 32u
#endif

typedef struct {
    uint32_t sequence[16];
    uint32_t length;
    uint32_t pos;
    uint32_t sync_count;
} comp_behaviour_t;

#ifndef HSA_COMP_SUCCESS

#define HSA_COMP_SUCCESS 1u
#endif

static uint8_t  g_queue_storage[1024];
static uint32_t g_completion[HSA_QUEUE_ENTRY];
static XAie_MemInst *g_hsa_meminst  = 0;
static XAie_MemInst *g_comp_meminst = 0;
static comp_behaviour_t g_comp_behaviour[HSA_QUEUE_ENTRY];

static XAie_MemInst g_hsa_meminst_storage;
static XAie_MemInst g_comp_meminst_storage;
static XAie_MemInst g_extra_meminst_storage;

static int g_force_ctrlcode_vaddr_null = 0;
static int g_alloc_call_count = 0;
static int g_free_call_count = 0;
static int g_fail_alloc_index = -1;
uint32_t g_stub_comp_sig_value = 0;

void Stub_XAie_ForceCtrlCodeVaddrNull(int enable)
{
    g_force_ctrlcode_vaddr_null = (enable != 0);
}

void Stub_AieMem_SetAllocateFailIndex(int index)
{
    g_fail_alloc_index = index;
}

void Stub_AieMem_ResetAll(void)
{
    g_alloc_call_count = 0;
    g_free_call_count = 0;
    g_fail_alloc_index = -1;
    g_stub_comp_sig_value = 0;
}

int Stub_AieMem_GetAllocateCallCount(void)
{
    return g_alloc_call_count;
}

int Stub_AieMem_GetFreeCallCount(void)
{
    return g_free_call_count;
}

static int g_case3_entry_fail = 0;
static int g_case3_ctrl_fail  = 0;
static int g_case4_entry_fail = 0;
static int g_case4_ctrl_fail  = 0;

void Stub_XAie_ForceCase3AllocPattern(int entry_fail, int ctrl_fail)
{
    g_case3_entry_fail = entry_fail != 0;
    g_case3_ctrl_fail  = ctrl_fail  != 0;
}

void Stub_XAie_ForceCase4AllocPattern(int entry_fail, int ctrl_fail)
{
    g_case4_entry_fail = entry_fail != 0;
    g_case4_ctrl_fail  = ctrl_fail  != 0;
}

int Stub_XAie_Case3EntryShouldFail(void) { return g_case3_entry_fail; }
int Stub_XAie_Case3CtrlShouldFail(void)  { return g_case3_ctrl_fail;  }
int Stub_XAie_Case4EntryShouldFail(void) { return g_case4_entry_fail; }
int Stub_XAie_Case4CtrlShouldFail(void)  { return g_case4_ctrl_fail;  }

void Stub_XAie_Reset(void)
{
    memset(g_queue_storage, 0, sizeof(g_queue_storage));
    memset(g_completion, 0, sizeof(g_completion));
    g_hsa_meminst  = 0;
    g_comp_meminst = 0;
    memset(g_comp_behaviour, 0, sizeof(g_comp_behaviour));

    g_force_ctrlcode_vaddr_null = 0;
    g_case3_entry_fail = 0;
    g_case3_ctrl_fail  = 0;
    g_case4_entry_fail = 0;
    g_case4_ctrl_fail  = 0;

    memset(&g_hsa_meminst_storage, 0, sizeof(g_hsa_meminst_storage));
    memset(&g_comp_meminst_storage, 0, sizeof(g_comp_meminst_storage));
    memset(&g_extra_meminst_storage, 0, sizeof(g_extra_meminst_storage));
}

void Stub_XAie_InitQueue(XAie_MemInst *hsa_meminst,
                         XAie_MemInst *comp_sig_meminst,
                         uint32_t entries)
{
    (void)entries;
    g_hsa_meminst  = hsa_meminst;
    g_comp_meminst = comp_sig_meminst;

    if (g_hsa_meminst) {
        g_hsa_meminst->vaddr   = g_queue_storage;
        g_hsa_meminst->devAddr = 0x1000u;
    }
    if (g_comp_meminst) {
        g_comp_meminst->vaddr   = g_completion;
        g_comp_meminst->devAddr = 0x2000u;
    }
}

void Stub_XAie_SetCompletionBehavior(uint32_t index,
                                     const uint32_t *sequence,
                                     uint32_t length)
{
    if (index >= HSA_QUEUE_ENTRY || length > 16u || sequence == NULL) {
        return;
    }
    g_comp_behaviour[index].length     = length;
    g_comp_behaviour[index].pos        = 0u;
    g_comp_behaviour[index].sync_count = 0u;
    for (uint32_t i = 0; i < length; ++i) {
        g_comp_behaviour[index].sequence[i] = sequence[i];
    }
}

uint32_t Stub_XAie_GetCompSyncCount(uint32_t index)
{
    if (index >= HSA_QUEUE_ENTRY) {
        return 0u;
    }
    return g_comp_behaviour[index].sync_count;
}

uint64_t Stub_XAie_GetCompletionDevAddr(uint32_t index)
{
    if (!g_comp_meminst) {
        return 0u;
    }
    return g_comp_meminst->devAddr + (uint64_t)sizeof(uint32_t) * index;
}

XAie_MemInst *Stub_XAie_GetQueueMemInst(void)
{
    return g_hsa_meminst;
}

XAie_MemInst *Stub_XAie_GetCompMemInst(void)
{
    return g_comp_meminst;
}

static void Stub_XAie_EnsureBackingStore(XAie_MemInst *mem)
{
    if (mem == NULL)
        return;

    if (mem->vaddr != NULL)
        return;

    mem->vaddr   = g_queue_storage;
    mem->devAddr = 0x3000u;
}

static uint8_t g_dummy_devinst_storage[1];

XAie_DevInst *Stub_XAie_CreateMinimalDevInst(void)
{
    static XAie_DevInst dummy_dev;
    (void)g_dummy_devinst_storage;
    return &dummy_dev;
}

void *XAie_MemGetVAddr(XAie_MemInst *mem)
{
    if (mem == NULL) {
        return NULL;
    }

    if (g_force_ctrlcode_vaddr_null &&
        mem != g_hsa_meminst && mem != g_comp_meminst) {
        return NULL;
    }

    Stub_XAie_EnsureBackingStore(mem);
    return mem->vaddr;
}

uint64_t XAie_MemGetDevAddr(XAie_MemInst *mem)
{
    if (mem == NULL) {
        return 0u;
    }
    Stub_XAie_EnsureBackingStore(mem);
    return (uint64_t)mem->devAddr;
}

int XAie_MemSyncForCPU(XAie_MemInst *mem)
{
    (void)mem;

    for (uint32_t i = 0; i < HSA_QUEUE_ENTRY; ++i) {
        if (g_comp_behaviour[i].length > 0u &&
            g_comp_behaviour[i].pos < g_comp_behaviour[i].length) {
            g_completion[i] = g_comp_behaviour[i].sequence[g_comp_behaviour[i].pos++];
            g_comp_behaviour[i].sync_count++;
        } else if (g_stub_comp_sig_value != 0) {

            g_completion[i] = g_stub_comp_sig_value;
        } else {

            g_completion[i] = HSA_COMP_SUCCESS;
        }
    }

    return 0;
}

int XAie_MemSyncForDev(XAie_MemInst *mem)
{
    (void)mem;
    return 0;
}

XAie_MemInst *XAie_MemAllocate(XAie_DevInst *DevInst, size_t size, u32 flags)
{
    (void)DevInst;
    (void)size;
    (void)flags;

    if (g_fail_alloc_index >= 0 && g_alloc_call_count == g_fail_alloc_index) {
        g_alloc_call_count++;
        return NULL;
    }

    if (g_hsa_meminst == NULL) {
        g_hsa_meminst = &g_hsa_meminst_storage;
        memset(g_hsa_meminst, 0, sizeof(*g_hsa_meminst));
        g_hsa_meminst->vaddr   = g_queue_storage;
        g_hsa_meminst->devAddr = 0x1000u;
        g_alloc_call_count++;
        return g_hsa_meminst;
    }

    if (g_comp_meminst == NULL) {
        g_comp_meminst = &g_comp_meminst_storage;
        memset(g_comp_meminst, 0, sizeof(*g_comp_meminst));
        g_comp_meminst->vaddr   = g_completion;
        g_comp_meminst->devAddr = 0x2000u;
        g_alloc_call_count++;
        return g_comp_meminst;
    }

    memset(&g_extra_meminst_storage, 0, sizeof(g_extra_meminst_storage));
    Stub_XAie_EnsureBackingStore(&g_extra_meminst_storage);
    g_alloc_call_count++;
    return &g_extra_meminst_storage;
}

void XAie_MemFree(XAie_MemInst *mem)
{
    (void)mem;
    g_free_call_count++;
}

AieRC XAie_LoadUcMem(XAie_DevInst *DevInst, XAie_LocType Loc, const void *Src)
{
    (void)DevInst;
    (void)Loc;
    (void)Src;
    return XAIE_OK;
}

AieRC XAie_DataMemBlockWrite(XAie_DevInst *DevInst,
                             XAie_LocType Loc,
                             u32 AddrOffset,
                             const void *Src,
                             size_t Size)
{
    (void)DevInst;
    (void)Loc;
    (void)AddrOffset;
    (void)Src;
    (void)Size;
    return XAIE_OK;
}
