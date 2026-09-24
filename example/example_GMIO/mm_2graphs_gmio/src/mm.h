// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef FUNCTION_KERNELS_H
#define FUNCTION_KERNELS_H

#define NBYTES_DATA 4

#define WIDTH 8
#define HEIGHT 8
#define SIZE (WIDTH * HEIGHT)

#include <adf.h>

void krnlmm(input_window_float *ina, input_window_float *inb, output_window_float *out);
#endif
