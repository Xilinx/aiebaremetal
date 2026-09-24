// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "square.h"
#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>
#include <stdio.h>

void square(adf::input_buffer<int32> &__restrict data1, adf::output_buffer<int32> &__restrict out) {
    auto inIter1 = aie::begin(data1);
    auto outIter = aie::begin(out);
    for (int i = 0; i < 256; i++) {
        int32 a = *inIter1++;
        int32 c = a * a;
        *outIter++ = c;
    }
}
