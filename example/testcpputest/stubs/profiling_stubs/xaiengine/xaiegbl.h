// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef XAIEGBL_H
#define XAIEGBL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef int64_t  s64;
typedef uint32_t XAie_Events;
typedef enum
{
    XAIE_OK            = 0,
    XAIE_GENERIC_ERROR = 1,
    XAIE_ERR           = 2
} XAie_ReturnCode;
typedef struct {
    u8 Col;
    u8 Row;
} XAie_LocType;

typedef enum
{
    XAIE_PL_MOD = 0
} XAie_ModuleType;

typedef enum
{
    XAIE_STRMSW_SLAVE  = 0,
    XAIE_STRMSW_MASTER = 1
} XAie_StrmSwPortType;

typedef enum
{
    NORTH = 0,
    EAST  = 1,
    SOUTH = 2,
    WEST  = 3
} XAie_Direction;

static inline XAie_LocType XAie_TileLoc(u8 Col, u8 Row)
{
    XAie_LocType loc;
    loc.Col = Col;
    loc.Row = Row;
    return loc;
}

typedef XAie_ReturnCode AieRC;

typedef u32 XAie_Events;

typedef struct {
	u64 BaseAddr;
	u8 StartCol;
	u8 NumRows;
	u8 NumCols;
	u8 ShimRow;
	u8 MemTileRowStart;
	u8 MemTileNumRows;
	u8 AieTileRowStart;
	u8 AieTileNumRows;
	u8 IsReady;
	u8 IsProd;
	u8 EccStatus;
} XAie_DevInst;

static const XAie_Events XAIE_EVENT_PORT_RUNNING_0_PL = 0x0000;
static const XAie_Events XAIE_EVENT_PORT_RUNNING_1_PL = 0x0001;
static const XAie_Events XAIE_EVENT_PORT_RUNNING_2_PL = 0x0002;
static const XAie_Events XAIE_EVENT_PORT_RUNNING_3_PL = 0x0003;
static const XAie_Events XAIE_EVENT_PORT_RUNNING_4_PL = 0x0004;
static const XAie_Events XAIE_EVENT_PORT_RUNNING_5_PL = 0x0005;
static const XAie_Events XAIE_EVENT_PORT_RUNNING_6_PL = 0x0006;
static const XAie_Events XAIE_EVENT_PORT_RUNNING_7_PL = 0x0007;

static const XAie_Events XAIE_EVENT_PORT_IDLE_0_PL = 0x0010;
static const XAie_Events XAIE_EVENT_PORT_IDLE_1_PL = 0x0011;
static const XAie_Events XAIE_EVENT_PORT_IDLE_2_PL = 0x0012;
static const XAie_Events XAIE_EVENT_PORT_IDLE_3_PL = 0x0013;
static const XAie_Events XAIE_EVENT_PORT_IDLE_4_PL = 0x0014;
static const XAie_Events XAIE_EVENT_PORT_IDLE_5_PL = 0x0015;
static const XAie_Events XAIE_EVENT_PORT_IDLE_6_PL = 0x0016;
static const XAie_Events XAIE_EVENT_PORT_IDLE_7_PL = 0x0017;

static const XAie_Events XAIE_EVENT_BROADCAST_A_0_PL  = 0x0100;
static const XAie_Events XAIE_EVENT_BROADCAST_A_1_PL  = 0x0101;
static const XAie_Events XAIE_EVENT_BROADCAST_A_2_PL  = 0x0102;
static const XAie_Events XAIE_EVENT_BROADCAST_A_3_PL  = 0x0103;
static const XAie_Events XAIE_EVENT_BROADCAST_A_4_PL  = 0x0104;
static const XAie_Events XAIE_EVENT_BROADCAST_A_5_PL  = 0x0105;
static const XAie_Events XAIE_EVENT_BROADCAST_A_6_PL  = 0x0106;
static const XAie_Events XAIE_EVENT_BROADCAST_A_7_PL  = 0x0107;
static const XAie_Events XAIE_EVENT_BROADCAST_A_8_PL  = 0x0108;
static const XAie_Events XAIE_EVENT_BROADCAST_A_9_PL  = 0x0109;
static const XAie_Events XAIE_EVENT_BROADCAST_A_10_PL = 0x010A;
static const XAie_Events XAIE_EVENT_BROADCAST_A_11_PL = 0x010B;
static const XAie_Events XAIE_EVENT_BROADCAST_A_12_PL = 0x010C;
static const XAie_Events XAIE_EVENT_BROADCAST_A_13_PL = 0x010D;
static const XAie_Events XAIE_EVENT_BROADCAST_A_14_PL = 0x010E;
static const XAie_Events XAIE_EVENT_BROADCAST_A_15_PL = 0x010F;

static const XAie_Events XAIE_EVENT_BROADCAST_WEST  = 0x0200;
static const XAie_Events XAIE_EVENT_BROADCAST_EAST  = 0x0201;
static const XAie_Events XAIE_EVENT_BROADCAST_NORTH = 0x0202;
static const XAie_Events XAIE_EVENT_BROADCAST_SOUTH = 0x0203;

static const XAie_Events XAIE_EVENT_PERF_CNT_1_PL   = 0x0301;
static const XAie_Events XAIE_EVENT_USER_EVENT_0_PL = 0x0400;
static const XAie_Events XAIE_EVENT_SWITCH_A        = 0x0500;
static const XAie_Events XAIE_EVENT_SWITCH_B        = 0x0501;

typedef struct {
	u8 EnDoubleBuff;
	u8 FifoMode;
	u8 EnCompression;
	u8 EnOutofOrderBdId;
	u8 TlastSuppress;
	u8 TileType;
	u8 IsReady;
} XAie_DmaDesc;

#ifdef __cplusplus
}
#endif

#endif
