// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <adf.h>

void loopback(input_window_int32 *in_buf, output_window_int32 *buf) {
    int32 i;
    for (i = 0; i < 32; i++) {
        int k = window_readincr(in_buf);
        window_writeincr(buf, k);
    }
};
