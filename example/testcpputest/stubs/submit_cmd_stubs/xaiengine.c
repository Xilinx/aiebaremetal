// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "xaiengine.h"
#include <stdlib.h>
#include <string.h>

static u8 Stub_GetTTypefromLoc(XAie_DevInst *DevInst, XAie_LocType Loc)
{
    (void)DevInst;
    (void)Loc;

    return (u8)0;
}

static AieRC Stub_UcWakeup(XAie_DevInst *DevInst,
                           XAie_LocType Loc,
                           const XAie_UcMod *UcMod)
{
    (void)DevInst;
    (void)Loc;
    (void)UcMod;
    return XAIE_OK;
}

static AieRC Stub_UcGetCoreStatus(XAie_DevInst *DevInst,
                                  XAie_LocType Loc,
                                  u32 *CoreStatus,
                                  const XAie_UcMod *UcMod)
{
    (void)DevInst;
    (void)Loc;
    (void)UcMod;
    if (CoreStatus)
        *CoreStatus = 0u;
    return XAIE_OK;
}
