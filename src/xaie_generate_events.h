// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef _XAIE_GENERATE_EVENTS_H_
#define _XAIE_GENERATE_EVENTS_H_

void generate_aie_tile_core_event(XAie_DevInst *DevInst, XAie_LocType Loc);
void generate_events_on_col(XAie_DevInst *DevInst, int col);
void generate_shim_tile_event(XAie_DevInst *DevInst, XAie_LocType Loc);
void generate_aie_tile_mem_event(XAie_DevInst *DevInst, XAie_LocType Loc);
void generate_mem_tile_event(XAie_DevInst *DevInst, XAie_LocType Loc);
void generate_user_event1_on_col(XAie_DevInst *DevInst, int col);
void generate_mem_tile_checkbit_error_generation(XAie_DevInst *DevInst, XAie_LocType Loc);
void generate_mem_tile_checkbit_error_generation_col(XAie_DevInst *DevInst, int col);
int test_aie_generate_intr(XAie_DevInst *DevInst);


#endif /* _XAIE_GENERATE_EVENTS_H_ */
