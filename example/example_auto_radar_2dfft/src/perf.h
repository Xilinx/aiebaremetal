// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
//
// Phase timers for the radar 2D-FFT performance test.
//
// Enabled by building with `-perf` (which defines PERF); otherwise every macro
// below compiles to nothing, so the shipped image carries no instrumentation.
//
// Timing uses the ARM generic timer via xtime_l.h, which only exists in the
// baremetal BSP. x86 simulation builds therefore fall back to the no-op path.
//
// Output lines are consumed by script/perf_report.py; keep the format stable:
//   [PERF] api=<name> iters=<n>
//   [PERF] <phase>          : <ms> ms

#ifndef RADAR_PERF_H
#define RADAR_PERF_H

#if defined(PERF) && (defined(__aarch64__) || defined(__arm__)) && !defined(__AIELINUX__)

#include "xtime_l.h"
#include <stdio.h>

#define PERF_DECLARE() XTime perf_run_start = 0, perf_phase_start = 0, perf_now = 0

#define PERF_RUN_BEGIN(api_name, iters)                                                            \
    do {                                                                                           \
        printf("[PERF] api=%s iters=%u\n", (api_name), (unsigned)(iters));                         \
        XTime_GetTime(&perf_run_start);                                                            \
    } while (0)

#define PERF_TIC() XTime_GetTime(&perf_phase_start)

#define PERF_TOC(label)                                                                            \
    do {                                                                                           \
        XTime_GetTime(&perf_now);                                                                  \
        printf("[PERF] %-18s: %.2f ms\n", (label),                                                 \
               1.0 * (perf_now - perf_phase_start) / (COUNTS_PER_SECOND / 1000.0));                \
    } while (0)

#define PERF_RUN_END()                                                                             \
    do {                                                                                           \
        XTime_GetTime(&perf_now);                                                                  \
        printf("[PERF] %-18s: %.2f ms\n", "total",                                                 \
               1.0 * (perf_now - perf_run_start) / (COUNTS_PER_SECOND / 1000.0));                  \
    } while (0)

#else

#define PERF_DECLARE()                                                                             \
    do {                                                                                           \
    } while (0)
#define PERF_RUN_BEGIN(api_name, iters)                                                            \
    do {                                                                                           \
        (void)(api_name);                                                                          \
        (void)(iters);                                                                             \
    } while (0)
#define PERF_TIC()                                                                                 \
    do {                                                                                           \
    } while (0)
#define PERF_TOC(label)                                                                            \
    do {                                                                                           \
        (void)(label);                                                                             \
    } while (0)
#define PERF_RUN_END()                                                                             \
    do {                                                                                           \
    } while (0)

#endif

#endif // RADAR_PERF_H
