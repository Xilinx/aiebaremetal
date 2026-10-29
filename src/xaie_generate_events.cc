// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <xaiengine.h>

#include "aeg_log.h"
#include "xaie_generate_events.h"

#ifdef __cplusplus
extern "C" {
#endif

void AegWaitForContinue(void)
{
    #ifdef __AIESIM__
    (void)getchar();
    (void)getchar();
    #endif
}

#ifdef __cplusplus
}
#endif
namespace {

XAie_Events aeg_get_event_from_ordinal(uint32_t ord)
{
    return static_cast<XAie_Events>(ord);
}

void generate_shim_tile_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    uint8_t ttype = 0U;
    uint32_t u_end = 0U;
    int32_t s_start = 0;
    uint32_t u_start = 0U;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if ((ttype != XAIEGBL_TILE_TYPE_SHIMNOC) && (ttype != XAIEGBL_TILE_TYPE_SHIMPL)) {
        AEG_ERROR("Tile [%d, %d] not shim tile.\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row));
    } else {
        u_end = static_cast<uint32_t>(XAIE_EVENT_GROUP_STREAM_SWITCH_PL);
        s_start = static_cast<int32_t>(XAIE_EVENT_GROUP_ERRORS_PL);
        s_start = s_start + 1;
        u_start = static_cast<uint32_t>(s_start);
        for (uint32_t u = u_start; u < u_end; u++) {
            AEG_LOG("[%d, %d]: Generating shim tile event: %u\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row), u);
            (void)XAie_EventGenerate(DevInst, Loc, XAIE_PL_MOD, aeg_get_event_from_ordinal(u));
        }
    }
}

void generate_aie_tile_core_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    uint8_t ttype = 0U;
    uint32_t u_end = 0U;
    int32_t s_start = 0;
    uint32_t u_start = 0U;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (ttype != XAIEGBL_TILE_TYPE_AIETILE) {
        AEG_ERROR("Tile [%d, %d] not aie tile.\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row));
    } else {
        u_end = static_cast<uint32_t>(XAIE_EVENT_GROUP_STREAM_SWITCH_CORE);
        s_start = static_cast<int32_t>(XAIE_EVENT_GROUP_ERRORS_1_CORE);
        s_start = s_start + 1;
        u_start = static_cast<uint32_t>(s_start);
        for (uint32_t u = u_start; u < u_end; u++) {
            AEG_LOG("[%d, %d]: Generating aie tile core event: %u\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row), u);
            (void)XAie_EventGenerate(DevInst, Loc, XAIE_CORE_MOD, aeg_get_event_from_ordinal(u));
        }
    }
}

void generate_aie_tile_mem_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    uint8_t ttype = 0U;
    uint32_t u_end = 0U;
    int32_t s_start = 0;
    uint32_t u_start = 0U;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (ttype != XAIEGBL_TILE_TYPE_AIETILE) {
        AEG_ERROR("Tile [%d, %d] not aie tile.\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row));
    } else {
        u_end = static_cast<uint32_t>(XAIE_EVENT_GROUP_BROADCAST_MEM);
        s_start = static_cast<int32_t>(XAIE_EVENT_GROUP_ERRORS_MEM);
        s_start = s_start + 1;
        u_start = static_cast<uint32_t>(s_start);
        for (uint32_t u = u_start; u < u_end; u++) {
            AEG_LOG("[%d, %d]: Generating aie tile mem event: %u\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row), u);
            (void)XAie_EventGenerate(DevInst, Loc, XAIE_MEM_MOD, aeg_get_event_from_ordinal(u));
        }
    }
}

void generate_mem_tile_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    uint8_t ttype = 0U;
    uint32_t u_end = 0U;
    int32_t s_start = 0;
    uint32_t u_start = 0U;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (ttype != XAIEGBL_TILE_TYPE_MEMTILE) {
        AEG_ERROR("Tile [%d, %d] not memtile.\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row));
    } else {
        u_end = static_cast<uint32_t>(XAIE_EVENT_GROUP_BROADCAST_MEM_TILE);
        s_start = static_cast<int32_t>(XAIE_EVENT_GROUP_ERRORS_MEM_TILE);
        s_start = s_start + 1;
        u_start = static_cast<uint32_t>(s_start);
        for (uint32_t u = u_start; u < u_end; u++) {
            AEG_LOG("[%d, %d]: Generating mem tile event: %u\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row), u);
            (void)XAie_EventGenerate(DevInst, Loc, XAIE_MEM_MOD, aeg_get_event_from_ordinal(u));
        }
    }
}
}  // namespace

void generate_events_on_col(XAie_DevInst *DevInst, int32_t col) {
    uint8_t ttype = 0U;
    XAie_LocType Loc = {};
    Loc.Col = static_cast<uint8_t>(col);

    const uint32_t numRows = static_cast<uint32_t>(DevInst->NumRows);
    for (uint32_t row = 0U; row < numRows; row++) {
        Loc.Row = static_cast<uint8_t>(row);
        ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
        switch (ttype) {
        case XAIEGBL_TILE_TYPE_SHIMNOC:
        {
            generate_shim_tile_event(DevInst, Loc);
            break;
        }
        case XAIEGBL_TILE_TYPE_SHIMPL:
        {
            generate_shim_tile_event(DevInst, Loc);
            break;
        }
        case XAIEGBL_TILE_TYPE_MEMTILE:
        {
            generate_mem_tile_event(DevInst, Loc);
            break;
        }
        case XAIEGBL_TILE_TYPE_AIETILE:
        {
            generate_aie_tile_core_event(DevInst, Loc);
            generate_aie_tile_mem_event(DevInst, Loc);
            break;
        }
        default:
        {
            AEG_ERROR("[%d, %d]: Tiletype invalid\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row));
            break;
        }
        }
    }
}

namespace {
#if defined(AIE_GEN) && (AIE_GEN >= 5)
void generate_user_event1_on_col(XAie_DevInst *DevInst, int32_t col) {
    XAie_LocType Loc = {};
    Loc.Row = 0U; Loc.Col = static_cast<uint8_t>(col);

    AEG_LOG("[%d, %d]: Generating PL USER_EVENT1: %d\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row), static_cast<int>(XAIE_EVENT_USER_EVENT_1_PL));
    (void)XAie_EventGenerate(DevInst, Loc, XAIE_PL_MOD, XAIE_EVENT_USER_EVENT_1_PL);
}
#else
void generate_user_event1_on_col(const XAie_DevInst *DevInst, int32_t col) {
    (void)DevInst;
    (void)col;
}
#endif

void generate_mem_tile_checkbit_error_generation(XAie_DevInst *DevInst, XAie_LocType Loc) {
    uint8_t ttype = 0U;
    uint64_t RegOffSet = 0x00092000ULL;
    uint64_t RegAddr = 0U;
    uint32_t RegVal = 0x1U;

    if (DevInst->DevProp.DevGen > XAIE_DEV_GEN_AIEML)
    {
        ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
        if (ttype == XAIEGBL_TILE_TYPE_MEMTILE) {
            RegAddr = XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) + RegOffSet;
            AEG_LOG("[%d, %d]: Generating checkbit_erro: %u\n", static_cast<int32_t>(Loc.Col), static_cast<int32_t>(Loc.Row), RegVal);
            (void)XAie_Write32(DevInst, RegAddr, RegVal);
        }
    }
}

void generate_mem_tile_checkbit_error_generation_col(XAie_DevInst *DevInst, int32_t col) {
    XAie_LocType Loc = {};
    Loc.Col = static_cast<uint8_t>(col);

    const uint32_t memRowStart = static_cast<uint32_t>(DevInst->MemTileRowStart);
    const uint32_t memNumRows = static_cast<uint32_t>(DevInst->MemTileNumRows);
    for (uint32_t row = memRowStart; row < memNumRows; row++) {
        Loc.Row = static_cast<uint8_t>(row);
        generate_mem_tile_checkbit_error_generation(DevInst, Loc);
    }
}
}  // namespace

int32_t test_aie_generate_intr(XAie_DevInst *DevInst) {
    AieRC RC = XAIE_OK;
    int32_t ret = 0;

    AEG_LOG("Starting aie generate intr test\n");

    RC = XAie_PmRequestTiles(DevInst, nullptr, 0U);
    if (RC != XAIE_OK) {
        AEG_ERROR("XAie_PmRequestTiles failed: %d", static_cast<int32_t>(RC));
    }
    AEG_LOG("Load your CDO now.");
    AegWaitForContinue();

    /* Should already be done by CDO. Uncomment if no CDO*/
    RC = XAie_ErrorHandlingInit(DevInst);
    if (RC != XAIE_OK) {
        AEG_ERROR("Error handling init failed: %d\n", static_cast<int32_t>(RC));
        ret = -1;
    } else {
        const int32_t startCol = static_cast<int32_t>(DevInst->StartCol);
        const int32_t endCol = startCol + static_cast<int32_t>(DevInst->NumCols);

        for (int32_t i = startCol; i < endCol; i++) {
            generate_user_event1_on_col(DevInst, i);
        }

        for (int32_t i = startCol; i < endCol; i++) {
            generate_mem_tile_checkbit_error_generation_col(DevInst, i);
        }

        for (int32_t i = startCol; i < endCol; i++) {
            generate_events_on_col(DevInst, i);
        }

        AEG_LOG("End aie generate intr test\n");
    }
    return ret;
}