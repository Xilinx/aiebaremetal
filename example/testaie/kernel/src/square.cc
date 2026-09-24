// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <adf.h>
#include <stdio.h>
void square(input_window_int32 *win, output_window_int32 *out) {

    printf("square log\n");
    for (unsigned i = 0; i < 32; i++) {
        int j = window_readincr(win);
        if (i == 0)
            printf("%d ", j);
        window_writeincr(out, j * j);
    }
    printf("\nsquare log end \n");
}
