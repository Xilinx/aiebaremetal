// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef SQUARE_MODULE_H
#define SQUARE_MODULE_H

#include "common.h"
#include <adf.h>
// #define SQUARE_BUF_LEN 128

void square(adf::input_buffer<int32> &in, adf::output_buffer<int32> &out);

#endif
