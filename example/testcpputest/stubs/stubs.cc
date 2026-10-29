// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "stubs.h"

extern "C" {
volatile int g_ut_externalbuffer_force_dma_update_bd_fail = 0;
volatile int g_ut_externalbuffer_force_dma_enqueue_fail = 0;
volatile int g_ut_gmio_configure_fail = 0;
volatile int g_ut_gmio_configure_force_code = 0;
volatile int g_ut_gmio_configure_force_null_argument = 0;
volatile int g_ut_gmio_wait_nb_fail = 0;
volatile int g_ut_graph_api_run_int_fail = 0;
volatile int g_ut_graph_api_update_rtp_user_error = 0;
volatile int g_ut_graph_api_update_force_code = 0;
volatile int g_ut_configure_external_buffers_force_code = 0;
volatile int g_ut_configure_rtps_force_code = 0;
volatile int g_ut_configure_shared_buffers_force_code = 0;
volatile int g_ut_configure_getters_force_fail = 0;

void ut_stub_force_gmio_configure_fail(int enable) {
	g_ut_gmio_configure_fail = enable ? 1 : 0;
}

void ut_stub_force_gmio_wait_nb_fail(int enable) {
	g_ut_gmio_wait_nb_fail = enable ? 1 : 0;
}

void ut_stub_force_graph_api_run_int_fail(int enable) {
	g_ut_graph_api_run_int_fail = enable ? 1 : 0;
}

void ut_stub_force_graph_api_update_rtp_user_error(int enable) {
	g_ut_graph_api_update_rtp_user_error = enable ? 1 : 0;
}

void ResetAegConfigureTestMocks(void)
{
	ut_stub_force_gmio_configure_fail(0);
	ut_stub_force_gmio_wait_nb_fail(0);
	ut_stub_force_graph_api_run_int_fail(0);
	ut_stub_force_graph_api_update_rtp_user_error(0);
	g_ut_externalbuffer_force_dma_update_bd_fail = 0;
	g_ut_externalbuffer_force_dma_enqueue_fail = 0;
	g_ut_configure_getters_force_fail = 0;
	g_ut_gmio_configure_force_code = 0;
	g_ut_gmio_configure_force_null_argument = 0;
	g_ut_graph_api_update_force_code = 0;
	g_ut_configure_external_buffers_force_code = 0;
	g_ut_configure_rtps_force_code = 0;
	g_ut_configure_shared_buffers_force_code = 0;
	ut_stub_reset_configure_parser_lazy_state();
}

__attribute__((weak)) void ut_stub_reset_configure_parser_lazy_state(void) {}

}

#define TTYPE_SEQ_MAX 32U
static u8 g_ttype_seq[TTYPE_SEQ_MAX];
static u32 g_ttype_seq_len;
static u32 g_ttype_seq_idx;
static u8 g_ttype_use_seq;

uint32_t* g_test_completion_ptr = NULL;
void* g_test_comp_sig_meminst = NULL;
int g_test_sync_call_count = 0;
int g_test_completion_threshold = 0;
uint32_t g_test_completion_value = 4;

u8 Stub_GetTTypefromLoc(XAie_DevInst* DevInst, XAie_LocType Loc)
{
	(void)DevInst;
	(void)Loc;

	if (g_ttype_use_seq != 0U) {
		if (g_ttype_seq_idx < g_ttype_seq_len) {
			return g_ttype_seq[g_ttype_seq_idx++];
		}
	}
	return g_TileType;
}

void Stub_XAie_SetTileType(u8 TileType)
{
	g_ttype_use_seq = 0U;
	g_ttype_seq_len = 0U;
	g_ttype_seq_idx = 0U;
	g_TileType = TileType;
}

void Stub_XAie_SetTTypeCallSequence(const u8 *types, u32 n)
{
	u32 i;
	if (n > TTYPE_SEQ_MAX) {
		n = TTYPE_SEQ_MAX;
	}
	g_ttype_use_seq = 1U;
	g_ttype_seq_len = n;
	g_ttype_seq_idx = 0U;
	for (i = 0U; i < n; i++) {
		g_ttype_seq[i] = types[i];
	}
}

void Stub_XAie_ClearTTypeCallSequence(void)
{
	g_ttype_use_seq = 0U;
	g_ttype_seq_len = 0U;
	g_ttype_seq_idx = 0U;
}

AieRC mock_Wakeup(XAie_DevInst *DevInst, XAie_LocType Loc, const struct XAie_UcMod *UcMod)
{
	(void)DevInst;
	(void)Loc;

	return (AieRC)XAIE_OK;
}

AieRC mock_Wakeup_fail(XAie_DevInst *DevInst, XAie_LocType Loc, const struct XAie_UcMod *UcMod)
{
	(void)DevInst;
	(void)Loc;

	return (AieRC)XAIE_ERR;
}

AieRC mock_GetCoreStatus(XAie_DevInst *DevInst,
		XAie_LocType Loc,
		u32 *CoreStatus,
		const struct XAie_UcMod *UcMod)
{
	(void)DevInst;
	(void)Loc;
	(void)CoreStatus;
	(void)UcMod;

	return (AieRC)XAIE_OK;
}

AieRC mock_GetCoreStatusfail(XAie_DevInst *DevInst,
		XAie_LocType Loc,
		u32 *CoreStatus,
		const struct XAie_UcMod *UcMod)
{
	(void)DevInst;
	(void)Loc;
	(void)CoreStatus;
	(void)UcMod;

	return (AieRC)XAIE_ERR;
}
