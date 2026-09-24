// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "adf.h"

void add(input_window_int32 *in, output_window_int32 *out) {
    printf("add enterd\n");
    for (int i = 0; i < 32; i++) {
        int val = window_readincr(in);
        val = val + 100;
        printf("add | %d\n", val);
        window_writeincr(out, val);
    }
}

void sub(input_window_int32 *in, output_window_int32 *out) {
    printf("sub enterd\n");
    for (int i = 0; i < 32; i++) {
        int val = window_readincr(in);
        val = val - 10;
        printf("sub | %d\n", val);
        window_writeincr(out, val);
    }
}
