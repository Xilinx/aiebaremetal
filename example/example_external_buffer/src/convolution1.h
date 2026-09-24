// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef CONVOLUTION_1_MODULE_H
#define CONVOLUTION_1_MODULE_H

#include "adf.h"

void conv1(adf::input_buffer<int32> &__restrict data1, adf::input_buffer<int32> &__restrict data2,
           adf::output_buffer<int32> &__restrict out);
#endif
