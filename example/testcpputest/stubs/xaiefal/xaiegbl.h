// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef XAIEGBL_H
#define XAIEGBL_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

enum AieRC {
    XAIE_OK = 0,
    XAIE_ERR = 1,
    XAIE_INVALID_ARGS = 2,
    XAIE_INVALID_TILE = 3,
    XAIE_ERR_STREAM_PORT = 4,
    XAIE_FEATURE_NOT_SUPPORTED = 5
};

#undef XAIE_OK
#undef XAIE_ERR

#ifndef NULL
#ifdef __cplusplus
#define NULL                   0
#else
#define NULL                   ((void*)0)
#endif
#endif

typedef struct {
    u8 Row;
    u8 Col;
} XAie_LocType;

typedef struct XAie_DevInst XAie_DevInst;

typedef enum {
    XAIE_CORE_MOD = 0,
    XAIE_MEM_MOD = 1,
    XAIE_PL_MOD = 2
} XAie_ModuleType;

typedef u8 (*GetTTypefromLoc_t)(XAie_DevInst*, XAie_LocType);
typedef AieRC (*Wakeup_t)(XAie_DevInst*, XAie_LocType, const struct XAie_UcMod*);
typedef AieRC (*GetCoreStatus_t)(XAie_DevInst*, XAie_LocType, u32*, const struct XAie_UcMod*);

typedef struct XAie_DeviceOps {
    GetTTypefromLoc_t GetTTypefromLoc;
} XAie_DeviceOps;

typedef struct XAie_UcMod {
    Wakeup_t Wakeup;
    GetCoreStatus_t GetCoreStatus;
} XAie_UcMod;

typedef struct XAie_PerfMod {
    u32 MaxCounterVal;
} XAie_PerfMod;

typedef struct {
    const XAie_UcMod* UcMod;
    const XAie_PerfMod* PerfMod;
} XAie_DevMod;

typedef struct {
    XAie_DevMod DevMod[10];
} XAie_DeviceProp;

typedef struct XAie_MemInst {
    u64 VAddr;
    u64 DevAddr;
    u64 Size;
} XAie_MemInst;

struct XAie_DevInst {
    u64 BaseAddr;
    u32 NumRows;
    u32 NumCols;
    u8 StartCol;
    XAie_DeviceOps* DevOps;
    XAie_DeviceProp DevProp;
};

static inline XAie_LocType XAie_TileLoc(u8 Col, u8 Row)
{
    XAie_LocType loc;
    loc.Col = Col;
    loc.Row = Row;
    return loc;
}

#endif
