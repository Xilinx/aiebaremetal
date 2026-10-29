// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef PASSTHROUGH_MODULE_H
#define PASSTHROUGH_MODULE_H

#include "common.h"
#include <adf.h>

void passthrough(adf::input_buffer<float> &in, adf::output_buffer<float> &out);

#endif
