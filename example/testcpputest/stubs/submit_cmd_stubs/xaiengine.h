// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint32_t u32;

typedef int AieRC;

typedef struct {
    u32 Row;
    u32 Col;
} XAie_LocType;

struct XAie_DevInst;

typedef struct XAie_UcMod {
    AieRC (*Wakeup)(struct XAie_DevInst *DevInst,
                    XAie_LocType Loc,
                    const struct XAie_UcMod *Self);
    AieRC (*GetCoreStatus)(struct XAie_DevInst *DevInst,
                           XAie_LocType Loc,
                           u32 *CoreStatus,
                           const struct XAie_UcMod *Self);
} XAie_UcMod;

typedef struct {
    const XAie_UcMod *UcMod;
} XAie_DevModEntry;

typedef struct {
    u32 ColShift;
    u32 NumCols;

    XAie_DevModEntry DevMod[4];
} XAie_DevProp;

typedef struct {
    u8 (*GetTTypefromLoc)(struct XAie_DevInst *DevInst, XAie_LocType Loc);
} XAie_DevOps;

typedef struct XAie_DevInst {
    int StartCol;
    int NumCols;
    XAie_DevProp DevProp;
    XAie_DevOps *DevOps;
} XAie_DevInst;

typedef struct XAie_MemInst {
    void    *vaddr;
    uintptr_t devAddr;
    size_t   size;
} XAie_MemInst;

#define XAIE_OK               0
#define XAIE_MEM_NONCACHEABLE 0u

XAie_MemInst *XAie_MemAllocate(XAie_DevInst *DevInst, size_t size, u32 flags);
void          XAie_MemFree(XAie_MemInst *mem);
void         *XAie_MemGetVAddr(XAie_MemInst *mem);
uintptr_t     XAie_MemGetDevAddr(XAie_MemInst *mem);
AieRC         XAie_MemSyncForCPU(XAie_MemInst *mem);
AieRC         XAie_MemSyncForDev(XAie_MemInst *mem);

AieRC XAie_DataMemBlockWrite(XAie_DevInst *DevInst,
                             XAie_LocType Loc,
                             u32 Addr,
                             const void *Data,
                             size_t Size);

AieRC XAie_LoadUcMem(XAie_DevInst *DevInst,
                     XAie_LocType Loc,
                     const void *ElfData);

#ifdef __cplusplus
}
#endif
