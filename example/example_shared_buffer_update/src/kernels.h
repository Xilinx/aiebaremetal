// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef SQUARE_MODULE_H
#define SQUARE_MODULE_H
#define NUM_SAMPLES 32

#include <adf.h>

void multiply(adf::input_buffer<int32> &__restrict data1, adf::input_buffer<int32> &__restrict data2,
              adf::output_buffer<int32> &__restrict out);

#endif
