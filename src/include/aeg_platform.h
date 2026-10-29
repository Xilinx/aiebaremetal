// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef AEG_PLATFORM_H
#define AEG_PLATFORM_H

#include <stdint.h>

#if defined(AEG_HARDWARE) && !defined(__ZEPHYR__) && !defined(__AIELINUX__)

#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xparameters.h"

#if __AIE_ARCH__ == 22
#include "xiltimer.h"
#else
#include "xtime_l.h"
#endif

#ifdef _RPU_
#include "xil_mpu.h"
#endif

#elif defined(__ZEPHYR__)

typedef uint64_t XTime;
#define COUNTS_PER_SECOND 1000000000ULL
static inline void XTime_GetTime(XTime *t) { *t = 0; }

#else

#include <time.h>
typedef uint64_t XTime;
#define COUNTS_PER_SECOND 1000000000ULL
static inline void XTime_GetTime(XTime *t) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    *t = (XTime)ts.tv_sec * COUNTS_PER_SECOND + (XTime)ts.tv_nsec;
}

#endif // defined(AEG_HARDWARE) && !defined(__ZEPHYR__)

#endif // AEG_PLATFORM_H
