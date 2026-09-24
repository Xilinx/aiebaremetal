// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "add.h"
#include <adf.h>
#include <stdio.h>

void add(input_window_int32 *in, output_window_int32 *out, int param) {
    for (int i = 0; i < 32; i++) {
        int val = window_readincr(in);
        val = val + param;
        window_writeincr(out, val);
    }
}
