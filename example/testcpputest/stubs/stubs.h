// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <stdint.h>
#include <xaiengine.h>

#define XAIE_OK                0
#pragma pack(1)

#define XAIE_CORE_MOD 0
#define XAIE_MEM_MOD  1
#define XAIE_PL_MOD   2

#define TEST_NUM_ROWS 5
#define TEST_NUM_COLS 8
#define TEST_COL_MIN 0
#define TEST_COL_MAX (TEST_NUM_COLS - 1)

#define XAIE_EVENT_GROUP_ERRORS_1_CORE       0
#define XAIE_EVENT_GROUP_STREAM_SWITCH_CORE  100
#define XAIE_EVENT_GROUP_ERRORS_PL          0
#define XAIE_EVENT_GROUP_STREAM_SWITCH_PL   100
#define XAIE_EVENT_GROUP_ERRORS_MEM         0
#define XAIE_EVENT_GROUP_BROADCAST_MEM      100
#define XAIE_EVENT_GROUP_ERRORS_MEM_TILE    0
#define XAIE_EVENT_GROUP_BROADCAST_MEM_TILE 100

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

inline u8 g_TileType = 0;
u8 Stub_GetTTypefromLoc(XAie_DevInst* DevInst, XAie_LocType Loc);
AieRC mock_Wakeup(XAie_DevInst *DevInst, XAie_LocType Loc,const struct XAie_UcMod *UcMod);
AieRC mock_Wakeup_fail(XAie_DevInst *DevInst, XAie_LocType Loc,const struct XAie_UcMod *UcMod);
AieRC mock_GetCoreStatus(XAie_DevInst *DevInst, XAie_LocType Loc,u32 *CoreStatus, const struct XAie_UcMod *UcMod);
AieRC mock_GetCoreStatusfail(XAie_DevInst *DevInst, XAie_LocType Loc,u32 *CoreStatus, const struct XAie_UcMod *UcMod);

extern uint32_t* g_test_completion_ptr;
extern void* g_test_comp_sig_meminst;
extern int g_test_sync_call_count;
extern int g_test_completion_threshold;
extern uint32_t g_test_completion_value;

#ifdef __cplusplus
extern "C" {
#endif
extern volatile int g_ut_externalbuffer_force_dma_update_bd_fail;
extern volatile int g_ut_externalbuffer_force_dma_enqueue_fail;
extern volatile int g_ut_gmio_configure_fail;
extern volatile int g_ut_gmio_configure_force_code;
extern volatile int g_ut_gmio_configure_force_null_argument;
extern volatile int g_ut_gmio_wait_nb_fail;
extern volatile int g_ut_graph_api_run_int_fail;
extern volatile int g_ut_graph_api_update_rtp_user_error;
extern volatile int g_ut_graph_api_update_force_code;
extern volatile int g_ut_configure_external_buffers_force_code;
extern volatile int g_ut_configure_rtps_force_code;
extern volatile int g_ut_configure_shared_buffers_force_code;
extern volatile int g_ut_configure_getters_force_fail;
void ut_stub_force_gmio_configure_fail(int enable);
void ut_stub_force_gmio_wait_nb_fail(int enable);
void ut_stub_force_graph_api_run_int_fail(int enable);
void ut_stub_force_graph_api_update_rtp_user_error(int enable);
void ut_stub_reset_configure_parser_lazy_state(void);

void ResetAegConfigureTestMocks(void);
#ifdef __cplusplus
}
#endif

static XAie_DeviceOps  s_MockDevOps = {
	.GetTTypefromLoc = Stub_GetTTypefromLoc
};

static XAie_UcMod  Mock_XAie_UcMod = {
	.Wakeup = mock_Wakeup
};
static XAie_UcMod  Mock_XAie_UcModstatus = {
	.GetCoreStatus = mock_GetCoreStatus
};
static XAie_UcMod  Mock_XAie_UcModstatus_fail = {
	.GetCoreStatus = mock_GetCoreStatusfail
};
static XAie_UcMod  Mock_XAie_UcMod_fail = {
	.Wakeup = mock_Wakeup_fail
};
static XAie_UcMod  Mock_XAie_UcMod_full = {
	.Wakeup = mock_Wakeup,
	.GetCoreStatus = mock_GetCoreStatus
};
static XAie_PerfMod  Mock_XAie_PerfMod = {
	.MaxCounterVal = 1
};
void Stub_XAie_SetTileType(u8 TileType);

void Stub_XAie_SetTTypeCallSequence(const u8 *types, u32 n);
void Stub_XAie_ClearTTypeCallSequence(void);

#ifndef PROFILING_API_STUB_H
#define PROFILING_API_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

	void set_profiling_stub_mode(bool enable_success);

	void reset_profiling_counter();

#ifdef __cplusplus
}
#endif

#endif
