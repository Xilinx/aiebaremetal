// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <fstream>
#include <vector>
#include <stdio.h>
#include <xaiengine.h>

#include "aeg_log.h"
#include "aiebaremetal.h"

#if defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
#include "app.h"
#endif

#include "test_error_interrupt.h"

AbrGraph gr("gradf");

static int num_err_backtracked = 0;
static int num_err_generated = 0;

static void dump_l1_intr_config(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_BroadcastSw Switch)
{
    const XAie_L1IntrMod *L1IntrMod;
    const char *sw_name;
    u64 tile_addr;
    u32 sw_off;
    u32 mask_off;
    u32 status_off;
    u32 irq_off;
    u32 irq_event_off;
    u32 bc_block_off;
    u32 mask_val = 0U;
    u32 status_val = 0U;
    u32 irq_val = 0U;
    u32 irq_event_val = 0U;
    u32 bc_block_val = 0U;
    u8 ttype;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    L1IntrMod = DevInst->DevProp.DevMod[ttype].L1IntrMod;
    if (L1IntrMod == NULL) {
        return;
    }

    sw_name = (Switch == XAIE_EVENT_SWITCH_A) ? "A" : "B";
    tile_addr = XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col);
    sw_off = (u32)Switch * (u32)L1IntrMod->SwOff;
    mask_off = L1IntrMod->BaseMaskRegOff + sw_off;
    status_off = L1IntrMod->BaseStatusRegOff + sw_off;
    irq_off = L1IntrMod->BaseIrqRegOff + sw_off;
    irq_event_off = L1IntrMod->BaseIrqEventRegOff + sw_off;
    bc_block_off = L1IntrMod->BaseBroadcastBlockRegOff + sw_off + 8;

    XAie_Read32(DevInst, tile_addr + mask_off, &mask_val);
    printf("[%d, %d]: L1IntrMod SW_%s MaskRegOff=0x%x value=0x%x\n",
           Loc.Col, Loc.Row, sw_name, mask_off, mask_val);

    XAie_Read32(DevInst, tile_addr + status_off, &status_val);
    printf("[%d, %d]: L1IntrMod SW_%s StatusRegOff=0x%x value=0x%x\n",
           Loc.Col, Loc.Row, sw_name, status_off, status_val);

    XAie_Read32(DevInst, tile_addr + irq_off, &irq_val);
    printf("[%d, %d]: L1IntrMod SW_%s IrqRegOff=0x%x value=0x%x\n",
           Loc.Col, Loc.Row, sw_name, irq_off, irq_val);

    XAie_Read32(DevInst, tile_addr + irq_event_off, &irq_event_val);
    printf("[%d, %d]: L1IntrMod SW_%s IrqEventRegOff=0x%x value=0x%x\n",
           Loc.Col, Loc.Row, sw_name, irq_event_off, irq_event_val);

    XAie_Read32(DevInst, tile_addr + bc_block_off, &bc_block_val);
    printf("[%d, %d]: L1IntrMod SW_%s BroadcastBlockRegOff=0x%x value=0x%x\n",
           Loc.Col, Loc.Row, sw_name, bc_block_off, bc_block_val);
}

static void dump_l2_intr_ctrl(XAie_DevInst *DevInst, XAie_LocType Loc)
{
    const XAie_L2IntrMod *L2IntrMod;
    const XAie_EvntMod *EvntMod;
    u64 tile_addr = 0U;
    u32 mask_val = 0U;
    u32 status_val = 0U;
    u32 irq_val = 0U;
    u8 ttype = 0U;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    L2IntrMod = DevInst->DevProp.DevMod[ttype].L2IntrMod;
    EvntMod = DevInst->DevProp.DevMod[ttype].EvntMod;
    if ((L2IntrMod == NULL) || (EvntMod == NULL)) {
        return;
    }

    tile_addr = XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col);

    XAie_Read32(DevInst, tile_addr + L2IntrMod->MaskRegOff, &mask_val);
    printf("[%d, %d]: L2IntrMod MaskRegOff=0x%x value=0x%x\n",
           Loc.Col, Loc.Row, L2IntrMod->MaskRegOff, mask_val);
    XAie_Read32(DevInst, tile_addr + L2IntrMod->StatusRegOff, &status_val);
    printf("[%d, %d]: L2IntrMod StatusRegOff=0x%x value=0x%x\n",
           Loc.Col, Loc.Row, L2IntrMod->StatusRegOff, status_val);
    XAie_Read32(DevInst, tile_addr + L2IntrMod->IrqRegOff, &irq_val);
    printf("[%d, %d]: L2IntrMod IrqRegOff=0x%x value=0x%x\n",
           Loc.Col, Loc.Row, L2IntrMod->IrqRegOff, irq_val);
}

static void dump_event_broadcast0(XAie_DevInst *DevInst, XAie_LocType Loc, XAie_ModuleType Module)
{
    u8 ttype;
    const XAie_EvntMod *EvntMod;
    u32 Broadcast0Val = 0;
    u32 GroupEventVal = 0;
    AieRC RC;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    RC = XAie_CheckModule(DevInst, Loc, Module);
    if(RC != XAIE_OK) {
        return;
    }
    if (Module == XAIE_PL_MOD) {
        EvntMod = &DevInst->DevProp.DevMod[ttype].EvntMod[0U];
    } else {
        EvntMod = &DevInst->DevProp.DevMod[ttype].EvntMod[Module];
    }

    if (EvntMod == NULL) {
        return;
    }

    XAie_Read32(DevInst, XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) + EvntMod->BaseBroadcastRegOff,
                &Broadcast0Val);
    printf("[%d, %d]: Mod: %d EvntMod BaseBroadcastRegOff=0x%x value=%d\n",
           Loc.Col, Loc.Row, static_cast<int>(Module), EvntMod->BaseBroadcastRegOff, Broadcast0Val);

    XAie_Read32(DevInst, XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) + EvntMod->BaseGroupEventRegOff,
                &GroupEventVal);
    printf("[%d, %d]: Mod: %d EvntMod BaseGroupEventRegOff=0x%x value=0x%x\n",
           Loc.Col, Loc.Row, static_cast<int>(Module), EvntMod->BaseGroupEventRegOff, GroupEventVal);
}

static void dump_intr_config(XAie_DevInst *DevInst)
{
    XAie_LocType Loc;
    uint8_t ttype;

    for (Loc.Col = 0U; Loc.Col < DevInst->NumCols; Loc.Col++) {
        for (Loc.Row = 0U; Loc.Row < DevInst->NumRows; Loc.Row++) {
            ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);

            switch (ttype) {
                case XAIEGBL_TILE_TYPE_SHIMNOC:
                    dump_l2_intr_ctrl(DevInst, Loc);
                /* Fallthrough */
                case XAIEGBL_TILE_TYPE_SHIMPL:
                    dump_l1_intr_config(DevInst, Loc, XAIE_EVENT_SWITCH_A);
                    dump_l1_intr_config(DevInst, Loc, XAIE_EVENT_SWITCH_B);
                    dump_event_broadcast0(DevInst, Loc, XAIE_PL_MOD);
                    break;
                case XAIEGBL_TILE_TYPE_MEMTILE:
                    dump_event_broadcast0(DevInst, Loc, XAIE_MEM_MOD);
                    break;
                case XAIEGBL_TILE_TYPE_AIETILE:
                    dump_event_broadcast0(DevInst, Loc, XAIE_CORE_MOD);
                    dump_event_broadcast0(DevInst, Loc, XAIE_MEM_MOD);
                    break;
                default:
                    break;
            }
        }
    }
}

static void graph_error_handling_cb(std::vector<XAie_ErrorPayload> payloads, void *priv) {
    num_err_backtracked += payloads.size();
    for (XAie_ErrorPayload pl : payloads)
        printf("CB: [%d, %d]: Mod: %d Error: %d\n", pl.Loc.Col, pl.Loc.Row,
               static_cast<int>(pl.Module), pl.EventId);
}

static void generate_shim_tile_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    uint8_t ttype;
    uint32_t u;
    uint32_t u_end;
    int32_t s_start;
    uint32_t u_start;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if ((ttype != XAIEGBL_TILE_TYPE_SHIMNOC) && (ttype != XAIEGBL_TILE_TYPE_SHIMPL)) {
        printf("Tile [%d, %d] not shim tile.\n", Loc.Col, Loc.Row);
    } else {
        u_end = (uint32_t)XAIE_EVENT_GROUP_STREAM_SWITCH_PL;
        s_start = (int32_t)XAIE_EVENT_GROUP_ERRORS_PL;
        s_start = s_start + 1;
        u_start = (uint32_t)s_start;
        for (u = u_start; u < u_end; u++) {
            AieRC RC;

            printf("[%d, %d]: Generating shim tile event: %u\n", Loc.Col, Loc.Row, u);
            RC = XAie_EventGenerate(DevInst, Loc, XAIE_PL_MOD, (XAie_Events)u);
            if (RC == XAIE_OK)
                num_err_generated++;
        }
    }
}

static void generate_aie_tile_core_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    uint8_t ttype;
    uint32_t u;
    uint32_t u_end;
    int32_t s_start;
    uint32_t u_start;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (ttype != XAIEGBL_TILE_TYPE_AIETILE) {
        printf("Tile [%d, %d] not aie tile.\n", Loc.Col, Loc.Row);
    } else {
        u_end = (uint32_t)XAIE_EVENT_GROUP_STREAM_SWITCH_CORE;
        s_start = (int32_t)XAIE_EVENT_GROUP_ERRORS_1_CORE;
        s_start = s_start + 1;
        u_start = (uint32_t)s_start;
        for (u = u_start; u < u_end; u++) {
            AieRC RC;

            printf("[%d, %d]: Generating aie tile core event: %u\n", Loc.Col, Loc.Row, u);
            RC = XAie_EventGenerate(DevInst, Loc, XAIE_CORE_MOD, (XAie_Events)u);
            if (RC == XAIE_OK)
                num_err_generated++;
        }
    }
}

static void generate_aie_tile_mem_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    uint8_t ttype;
    uint32_t u;
    uint32_t u_end;
    int32_t s_start;
    uint32_t u_start;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (ttype != XAIEGBL_TILE_TYPE_AIETILE) {
        printf("Tile [%d, %d] not aie tile.\n", Loc.Col, Loc.Row);
    } else {
        u_end = (uint32_t)XAIE_EVENT_GROUP_BROADCAST_MEM;
        s_start = (int32_t)XAIE_EVENT_GROUP_ERRORS_MEM;
        s_start = s_start + 1;
        u_start = (uint32_t)s_start;
        for (u = u_start; u < u_end; u++) {
            AieRC RC;

            printf("[%d, %d]: Generating aie tile mem event: %u\n", Loc.Col, Loc.Row, u);
            RC = XAie_EventGenerate(DevInst, Loc, XAIE_MEM_MOD, (XAie_Events)u);
            if (RC == XAIE_OK)
                num_err_generated++;
        }
    }
}

static void generate_mem_tile_event(XAie_DevInst *DevInst, XAie_LocType Loc) {
    uint8_t ttype;
    uint32_t u;
    uint32_t u_end;
    int32_t s_start;
    uint32_t u_start;

    ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
    if (ttype != XAIEGBL_TILE_TYPE_MEMTILE) {
        printf("Tile [%d, %d] not memtile.\n", Loc.Col, Loc.Row);
    } else {
        u_end = (uint32_t)XAIE_EVENT_GROUP_BROADCAST_MEM_TILE;
        s_start = (int32_t)XAIE_EVENT_GROUP_ERRORS_MEM_TILE;
        s_start = s_start + 1;
        u_start = (uint32_t)s_start;
        for (u = u_start; u < u_end; u++) {
            AieRC RC;

            printf("[%d, %d]: Generating mem tile event: %u\n", Loc.Col, Loc.Row, u);
            RC = XAie_EventGenerate(DevInst, Loc, XAIE_MEM_MOD, (XAie_Events)u);
            if (RC == XAIE_OK)
                num_err_generated++;
        }
    }
}

static void generate_events_on_col(XAie_DevInst *DevInst, int32_t col) {
    uint8_t ttype;
    XAie_LocType Loc;

    Loc.Col = (uint8_t)col;
    for (Loc.Row = 0U; Loc.Row < DevInst->NumRows; Loc.Row++) {
        ttype = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
        switch (ttype) {
        case XAIEGBL_TILE_TYPE_SHIMNOC:
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
            printf("[%d, %d]: Tiletype invalid\n", Loc.Col, Loc.Row);
            break;
        }
        }
    }
}

static void test_error_handling(AbrGraph &gr) {
    AieRC RC;
    XAie_DevInst *DevInst;

    DevInst = static_cast<XAie_DevInst *>(gr.getaiedevinst());
    switch (DevInst->DevProp.DevGen) {
        case XAIE_DEV_GEN_AIE2PS:
            static_cast<void>(XAie_UpdateNpiAddr(DevInst, static_cast<u64>(0xf6d50000U)));
            break;
        case XAIE_DEV_GEN_AIEML:
            static_cast<void>(XAie_UpdateNpiAddr(DevInst, static_cast<u64>(0xF6D10000U)));
            break;
        case XAIE_DEV_GEN_AIE:
            static_cast<void>(XAie_UpdateNpiAddr(DevInst, static_cast<u64>(0xF70A0000U)));
            break;
        default:
            AEG_ERROR("%s: DevGen not found: %d", "selftest_errorhandling",
                      static_cast<int32_t>(DevInst->DevProp.DevGen));
            break;
    }
    RC = XAie_PartitionInitialize(DevInst, NULL);
    if (RC != XAIE_OK) {
        printf("Partition initialization failed: %d\n", (int32_t)RC);
    }

    gr.error_handling_cb_init((void *)graph_error_handling_cb, (void *)&gr);

#ifndef __AIESIM__
    RC = XAie_PmRequestTiles(DevInst, NULL, 0);
    if (RC != XAIE_OK) {
        printf("XAie_PmRequestTiles failed: %d\n", (int32_t)RC);
        return;
    }
#endif /* __AIESIM__ */

    RC = XAie_ErrorHandlingInit(DevInst);
    if (RC != XAIE_OK) {
        printf("Error handling init failed: %d\n", (int32_t)RC);
        return;
    }

    /* Generate error events on all columns */
    for (uint32_t i = static_cast<int32_t>(DevInst->StartCol);
	 i < static_cast<int32_t>(DevInst->StartCol + DevInst->NumCols);
	 i++) {
	generate_events_on_col(DevInst, i);
    }

#if GRAPH_COMPILE_OR_SIM
    gr.BacktrackErrors();
    printf("errors generated: %d\n", num_err_generated);
    printf("errors backtracked: %d\n", num_err_backtracked);
#else
    if (num_err_backtracked == 0) {
        printf("Backtracking Errors failed\n");
        printf("Manually calling BacktrackErrors() to see if any errors are present\n");
        gr.BacktrackErrors();
        printf("Total errors generated: %d\n", num_err_generated);
        printf("Total errors manually backtracked: %d\n", num_err_backtracked);
        dump_intr_config(DevInst);
        return;
    }
    printf("Total errors generated: %d\n", num_err_generated);
    printf("Total errors backtracked: %d\n", num_err_backtracked);
    /* Check if we missed any errors */
    gr.BacktrackErrors();
#endif

    return;
}

int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    gr.init();

    test_error_handling(gr);
    printf("Test error handling completed. Check the log for error messages.\n");
    gr.selftestexit();
    printf("Selftest exit completed.\n");

    return 0;
}
