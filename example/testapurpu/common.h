// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef __COMMON__
#define __COMMON__
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include <stdio.h>
#if __AIE_ARCH__ == 22
#include "xiltimer.h"
#else
#include "xtime_l.h"
#endif

#define SECTION_SIZE (1024)
#define SECTION_COUNT (8)


#define SECTION_ADDRESS (1024 * 1024)
#define LOCAL_ADDRESS (1024 * 1024 *100)
#define SYNC_ADDRESS (1024 * 1024 * 50)

void wdata();
void rdata();
void wsync(uint32_t var);
uint32_t rsync();
void log(char* app);
void apu_disablecache();
#endif
