/**
* Copyright (C) 2024 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/
#ifndef HW_CONFIG_H
#define HW_CONFIG_H
#include "common.h"
//proj/xbuilds/2024.1_INT_daily_latest/installs/lin64/Vitis/2024.1/aietools/data/aie2ps/devices/xc10T50.json
//T50
#if AIE_GEN == 1
#define HW_GEN XAIE_DEV_GEN_AIE
#define XAIE_BASE_ADDR 0x20000000000
#define XAIE_COL_SHIFT 23
#define XAIE_ROW_SHIFT 18

#define XAIE_NUM_ROWS 9
#define XAIE_NUM_COLS 50
#define XAIE_SHIM_ROW 0
#define XAIE_MEM_TILE_ROW_START 0
#define XAIE_MEM_TILE_NUM_ROWS 0
#define XAIE_AIE_TILE_ROW_START 1
#define XAIE_AIE_TILE_NUM_ROWS 8

#elif AIE_GEN == 2

#define HW_GEN XAIE_DEV_GEN_AIEML
#define XAIE_BASE_ADDR 0x20000000000
#define XAIE_COL_SHIFT 25
#define XAIE_ROW_SHIFT 20

#define XAIE_BASE_ADDR 0x20000000000
#define XAIE_NUM_ROWS 7
#define XAIE_NUM_COLS 36
#define XAIE_SHIM_ROW 0
#define XAIE_MEM_TILE_ROW_START 1
#define XAIE_MEM_TILE_NUM_ROWS 2
#define XAIE_AIE_TILE_ROW_START 3
#define XAIE_AIE_TILE_NUM_ROWS 4

#elif AIE_GEN == 5

#define HW_GEN XAIE_DEV_GEN_AIE2PS
#define XAIE_COL_SHIFT 25
#define XAIE_ROW_SHIFT 20

#define XAIE_BASE_ADDR 0x20000000000
#define XAIE_NUM_ROWS 7
#define XAIE_NUM_COLS 36
#define XAIE_SHIM_ROW 0
#define XAIE_MEM_TILE_ROW_START 1
#define XAIE_MEM_TILE_NUM_ROWS 2
#define XAIE_AIE_TILE_ROW_START 3
#define XAIE_AIE_TILE_NUM_ROWS 4
#endif
#endif /* HW_CONFIG_H */
