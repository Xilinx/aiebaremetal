// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "kernels.h"
#include "config.h"
#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <stdio.h>

void multiply(adf::input_buffer<int32> &__restrict data1, adf::input_buffer<int32> &__restrict data2,
              adf::output_buffer<int32> &__restrict out) {
    auto inIter1 = aie::begin(data1);
    auto inIter2 = aie::begin(data2);
    auto outIter = aie::begin(out);
    for (int i = 0; i < NUM_SAMPLES; i++) {
        int32 a = *inIter1++;
        int32 b = *inIter2++;
        int32 c = a * b;
        *outIter++ = c;
        printf("a: %d  b: %d c: %d\n", a, b, c);
    }
}