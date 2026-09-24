#include "stub_xaie_mem.h"
#include <string.h>
#include <stdint.h>

/*
 * Local queue depth for stubs (matches HSA_QUEUE_ENTRY in tests).
 */
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
/*
 * Define success as a non-zero value so that the completion polling loop in
 * cert_api::submit_command() eventually terminates instead of spinning
 * forever on zero.
 */
#define HSA_COMP_SUCCESS 1u
#endif

/* Opaque storage; production code only cares that pointers are non-null. */
static uint8_t  g_queue_storage[1024];
static uint32_t g_completion[HSA_QUEUE_ENTRY];
static XAie_MemInst *g_hsa_meminst  = 0;
static XAie_MemInst *g_comp_meminst = 0;
static comp_behaviour_t g_comp_behaviour[HSA_QUEUE_ENTRY];

/* Dedicated storage for meminst objects so hsa and completion buffers
 * are represented by distinct XAie_MemInst pointers.
 */
static XAie_MemInst g_hsa_meminst_storage;
static XAie_MemInst g_comp_meminst_storage;
static XAie_MemInst g_extra_meminst_storage;

/* Control flags/hooks used by tests to force specific behaviours. */
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

/*
 * For case-3 and case-4 allocation patterns we just record requested
 * behaviours here. Actual XAie_MemAllocate stubs can consult these flags
 * if needed.
 */
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

/* Optional getters if other stubs need to inspect the pattern flags. */
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

    /* Reset control flags as well. */
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

/* Ensure any non-NULL XAie_MemInst has a valid backing store. This helper is
 * only used for generic/extra allocations; dedicated queue/completion
 * instances are wired explicitly in XAie_MemAllocate/Stub_XAie_InitQueue.
 */
static void Stub_XAie_EnsureBackingStore(XAie_MemInst *mem)
{
    if (mem == NULL)
        return;

    if (mem->vaddr != NULL)
        return;

    /* Default backing store for generic meminst: reuse queue storage. */
    mem->vaddr   = g_queue_storage;
    mem->devAddr = 0x3000u;
}

/*
 * NEW: Minimal device instance and XAie_* API stubs required by cert_host.cc
 * and the unit tests.
 */
static uint8_t g_dummy_devinst_storage[1];

/* Minimal XAie_DevInst used by tests; fields are zeroed. */
XAie_DevInst *Stub_XAie_CreateMinimalDevInst(void)
{
    static XAie_DevInst dummy_dev;  /* zero-initialized */
    (void)g_dummy_devinst_storage;  /* suppress unused warning */
    return &dummy_dev;
}

/* == Stub implementations matching xaiengine.h prototypes == */

/* xaiengine.h: void *XAie_MemGetVAddr(XAie_MemInst *mem); */
void *XAie_MemGetVAddr(XAie_MemInst *mem)
{
    if (mem == NULL) {
        return NULL;
    }

    /* When enabled, simulate a NULL VA for any non-queue, non-completion
     * buffer. This is used by tests to drive ctrl_code VA failure paths.
     */
    if (g_force_ctrlcode_vaddr_null &&
        mem != g_hsa_meminst && mem != g_comp_meminst) {
        return NULL;
    }

    Stub_XAie_EnsureBackingStore(mem);
    return mem->vaddr;
}

/* xaiengine.h: uint64_t XAie_MemGetDevAddr(XAie_MemInst *mem); */
uint64_t XAie_MemGetDevAddr(XAie_MemInst *mem)
{
    if (mem == NULL) {
        return 0u;
    }
    Stub_XAie_EnsureBackingStore(mem);
    return (uint64_t)mem->devAddr;
}

/*
 * Synchronise completion memory for CPU: update completion slots based on
 * programmed behaviour sequences, and mark them as successful.
 *
 * Signature matches stub_xaie_mem.h: int XAie_MemSyncForCPU(XAie_MemInst *mem);
 */
int XAie_MemSyncForCPU(XAie_MemInst *mem)
{
    (void)mem; /* behaviour is global; mem is unused in this stub */

    /* Drive completion slots according to any programmed behaviour. */
    for (uint32_t i = 0; i < HSA_QUEUE_ENTRY; ++i) {
        if (g_comp_behaviour[i].length > 0u &&
            g_comp_behaviour[i].pos < g_comp_behaviour[i].length) {
            g_completion[i] = g_comp_behaviour[i].sequence[g_comp_behaviour[i].pos++];
            g_comp_behaviour[i].sync_count++;
        } else if (g_stub_comp_sig_value != 0) {
            /* If g_stub_comp_sig_value is set, use it for completion */
            g_completion[i] = g_stub_comp_sig_value;
        } else {
            /* Default to success when no specific sequence is configured. */
            g_completion[i] = HSA_COMP_SUCCESS;
        }
    }

    return 0; /* success */
}

int XAie_MemSyncForDev(XAie_MemInst *mem)
{
    (void)mem;
    return 0;
}

/* xaiengine.h: XAie_MemInst *XAie_MemAllocate(XAie_DevInst *DevInst, size_t size, u32 flags); */
XAie_MemInst *XAie_MemAllocate(XAie_DevInst *DevInst, size_t size, u32 flags)
{
    (void)DevInst;
    (void)size;
    (void)flags;

    /* Check if this allocation should fail */
    if (g_fail_alloc_index >= 0 && g_alloc_call_count == g_fail_alloc_index) {
        g_alloc_call_count++;
        return NULL;
    }

    /* First allocation: HSA queue (host queue) */
    if (g_hsa_meminst == NULL) {
        g_hsa_meminst = &g_hsa_meminst_storage;
        memset(g_hsa_meminst, 0, sizeof(*g_hsa_meminst));
        g_hsa_meminst->vaddr   = g_queue_storage;
        g_hsa_meminst->devAddr = 0x1000u;
        g_alloc_call_count++;
        return g_hsa_meminst;
    }

    /* Second allocation: completion signal buffer */
    if (g_comp_meminst == NULL) {
        g_comp_meminst = &g_comp_meminst_storage;
        memset(g_comp_meminst, 0, sizeof(*g_comp_meminst));
        g_comp_meminst->vaddr   = g_completion;
        g_comp_meminst->devAddr = 0x2000u;
        g_alloc_call_count++;
        return g_comp_meminst;
    }

    /* Any further allocations get a generic backing store. */
    memset(&g_extra_meminst_storage, 0, sizeof(g_extra_meminst_storage));
    Stub_XAie_EnsureBackingStore(&g_extra_meminst_storage);
    g_alloc_call_count++;
    return &g_extra_meminst_storage;
}

/* xaiengine.h: void XAie_MemFree(XAie_MemInst *mem); */
void XAie_MemFree(XAie_MemInst *mem)
{
    (void)mem;  /* no-op stub */
    g_free_call_count++;
}

/* xaiengine.h: AieRC XAie_LoadUcMem(XAie_DevInst *DevInst, XAie_LocType Loc, const void *Src); */
AieRC XAie_LoadUcMem(XAie_DevInst *DevInst, XAie_LocType Loc, const void *Src)
{
    (void)DevInst;
    (void)Loc;
    (void)Src;
    return XAIE_OK;  /* pretend load succeeded */
}

/* xaiengine.h: AieRC XAie_DataMemBlockWrite(XAie_DevInst *DevInst,
 *                                           XAie_LocType Loc,
 *                                           u32 AddrOffset,
 *                                           const void *Src,
 *                                           size_t Size); */
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
    return XAIE_OK;  /* pretend write succeeded */
}
