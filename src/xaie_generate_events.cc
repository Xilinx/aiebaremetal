// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <xaiengine.h>

#include "xaie_generate_events.h"

void generate_shim_tile_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    u8 TileType;
    int i;

    TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (TileType != XAIEGBL_TILE_TYPE_SHIMNOC && TileType != XAIEGBL_TILE_TYPE_SHIMPL) {
        printf("Tile [%d, %d] not shim tile.\n", Loc.Col, Loc.Row);
        return;
    }

    for (i = XAIE_EVENT_GROUP_ERRORS_PL + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_PL; i++) {
        printf("[%d, %d]: Generating shim tile event: %d\n", Loc.Col, Loc.Row, i);
        XAie_EventGenerate(DevInst, Loc, XAIE_PL_MOD, static_cast<XAie_Events>(i));
    }
}

void generate_aie_tile_core_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    u8 TileType;
    int i;

    TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (TileType != XAIEGBL_TILE_TYPE_AIETILE) {
        printf("Tile [%d, %d] not aie tile.\n", Loc.Col, Loc.Row);
        return;
    }

    for (i = XAIE_EVENT_GROUP_ERRORS_1_CORE + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_CORE; i++) {
        printf("[%d, %d]: Generating aie tile core event: %d\n", Loc.Col, Loc.Row, i);
        XAie_EventGenerate(DevInst, Loc, XAIE_CORE_MOD, static_cast<XAie_Events>(i));
    }
}

void generate_aie_tile_mem_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    u8 TileType;
    int i;

    TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (TileType != XAIEGBL_TILE_TYPE_AIETILE) {
        printf("Tile [%d, %d] not aie tile.\n", Loc.Col, Loc.Row);
        return;
    }

    for (i = XAIE_EVENT_GROUP_ERRORS_MEM + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM; i++) {
        printf("[%d, %d]: Generating aie tile mem event: %d\n", Loc.Col, Loc.Row, i);
        XAie_EventGenerate(DevInst, Loc, XAIE_MEM_MOD, static_cast<XAie_Events>(i));
    }
}

void generate_mem_tile_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    u8 TileType;
    int i;

    TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (TileType != XAIEGBL_TILE_TYPE_MEMTILE) {
        printf("Tile [%d, %d] not memtile.\n", Loc.Col, Loc.Row);
        return;
    }

    for (i = XAIE_EVENT_GROUP_ERRORS_MEM_TILE + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM_TILE; i++) {
        printf("[%d, %d]: Generating mem tile event: %d\n", Loc.Col, Loc.Row, i);
        XAie_EventGenerate(DevInst, Loc, XAIE_MEM_MOD, static_cast<XAie_Events>(i));
    }
}

void generate_events_on_col(XAie_DevInst *DevInst, int col) {
    u8 TileType;
    XAie_LocType Loc;

    for (Loc.Row = 0, Loc.Col = col; Loc.Row < DevInst->NumRows; Loc.Row++) {
        TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
        switch (TileType) {
        case XAIEGBL_TILE_TYPE_SHIMNOC:
        case XAIEGBL_TILE_TYPE_SHIMPL:
            generate_shim_tile_event(DevInst, Loc);
            break;
        case XAIEGBL_TILE_TYPE_MEMTILE:
            generate_mem_tile_event(DevInst, Loc);
            break;
        case XAIEGBL_TILE_TYPE_AIETILE:
            generate_aie_tile_core_event(DevInst, Loc);
            generate_aie_tile_mem_event(DevInst, Loc);
            break;
        default:
            printf("[%d, %d]: Tiletype invalid\n", Loc.Col, Loc.Row);
        }
    }
}

#if AIE_GEN >= 5
void generate_user_event1_on_col(XAie_DevInst *DevInst, int col) {
    XAie_LocType Loc = {.Row = 0, .Col = col};

    printf("[%d, %d]: Generating PL USER_EVENT1: %d\n", Loc.Col, Loc.Row, XAIE_EVENT_USER_EVENT_1_PL);
    XAie_EventGenerate(DevInst, Loc, XAIE_PL_MOD, XAIE_EVENT_USER_EVENT_1_PL);
}
#else
void generate_user_event1_on_col(XAie_DevInst *DevInst, int col) {
    (void)DevInst;
    (void)col;
}
#endif

void generate_mem_tile_checkbit_error_generation(XAie_DevInst *DevInst, XAie_LocType Loc) {
    u8 TileType;
    u64 RegOffSet = 0x00092000;
    u64 RegAddr;
    u32 RegVal = 0x1;

    if (DevInst->DevProp.DevGen <= XAIE_DEV_GEN_AIEML)
        return;

    TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (TileType != XAIEGBL_TILE_TYPE_MEMTILE) {
        return;
    }

    RegAddr = XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) + RegOffSet;
    printf("[%d, %d]: Generating checkbit_erro: %d\n", Loc.Col, Loc.Row, RegVal);
    XAie_Write32(DevInst, RegAddr, RegVal);
}

void generate_mem_tile_checkbit_error_generation_col(XAie_DevInst *DevInst, int col) {
    XAie_LocType Loc;

    for (Loc.Col = col, Loc.Row = DevInst->MemTileRowStart; Loc.Row < DevInst->MemTileNumRows; Loc.Row++) {
        generate_mem_tile_checkbit_error_generation(DevInst, Loc);
    }
}

int test_aie_generate_intr(XAie_DevInst *DevInst) {
    AieRC RC = XAIE_OK;
    int i;

    printf("Starting aie generate intr test\n");

    RC = XAie_PmRequestTiles(DevInst, NULL, 0);
    if (RC != XAIE_OK) {
        printf("XAie_PmRequestTiles failed: %d", RC);
        // continue
        RC = XAIE_OK;
    }
    printf("Load your CDO now.");
    getchar();
    getchar();

    /* Should already be done by CDO. Uncomment if no CDO*/
    RC = XAie_ErrorHandlingInit(DevInst);
    if (RC != XAIE_OK) {
        printf("Error handling init failed: %d\n", RC);
        return -1;
    }

    for (i = DevInst->StartCol; i < DevInst->StartCol + DevInst->NumCols; i++) {
        generate_user_event1_on_col(DevInst, i);
    }

    for (i = DevInst->StartCol; i < DevInst->StartCol + DevInst->NumCols; i++) {
        generate_mem_tile_checkbit_error_generation_col(DevInst, i);
    }

    for (i = DevInst->StartCol; i < DevInst->StartCol + DevInst->NumCols; i++) {
        generate_events_on_col(DevInst, i);
    }

    printf("End aie generate intr test\n");
    return 0;
}
