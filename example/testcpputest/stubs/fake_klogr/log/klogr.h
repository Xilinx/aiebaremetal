// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef _KLOG_R_
#define _KLOG_R_

#include <stdint.h>
#include <stdio.h>

#include "kcommon.h"

static char rlog_mock_buffer[16] = {0};

static inline char *rlog(int r, int c, uint64_t inst) {
    (void)r;
    (void)c;
    (void)inst;
    printf("r log address = %lx\n", static_cast<unsigned long>(AIEML_CORE_LOG_DM_REALADDR_AIE2));
    printf("rlog data = 0\n");
    rlog_mock_buffer[0] = '\0';
    return rlog_mock_buffer;
}

#endif
