// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <cstdarg>
#include <cstdio>
#include <cstdint>

#include <xaiengine.h>

#include "xaie_generate_events.h"

extern "C" {

volatile int g_ut_generate_events_on_col_calls = 0;

void ut_stub_reset_generate_events_mock(void) { g_ut_generate_events_on_col_calls = 0; }

void generate_events_on_col(XAie_DevInst *DevInst, int32_t col) {
	(void)DevInst;
	(void)col;
	g_ut_generate_events_on_col_calls++;
}

int32_t test_aie_generate_intr(XAie_DevInst *DevInst) {
	(void)DevInst;

	return 0;
}

void AegError(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	(void)fputs("[AEG_ERROR] ", stderr);
	(void)vfprintf(stderr, fmt, ap);
	(void)fputc('\n', stderr);
	va_end(ap);
}

void AegLog(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	(void)vfprintf(stdout, fmt, ap);
	va_end(ap);
}

void AegWaitForContinue(void) {}

}
