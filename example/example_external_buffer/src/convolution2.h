// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef CONVOLUTION_2_MODULE_H
#define CONVOLUTION_2_MODULE_H

#include "adf.h"

void conv2(adf::input_buffer<int32> &__restrict data1, adf::input_buffer<int32> &__restrict data2,
           adf::output_buffer<int32> &__restrict out);
#endif
