// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "convolution1.h"
#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>
#include <stdio.h>

#define input_width 3
#define input_height 3
#define filter_size_1 3
#define in_channels_1 3
#define out_channels_1 2
#define filter_size_2 3
#define in_channels_2 2
#define out_channels_2 1

void conv1(adf::input_buffer<int32> &__restrict data1, adf::input_buffer<int32> &__restrict data2,
           adf::output_buffer<int32> &__restrict out) {
    auto inIter1 = aie::begin(data1);
    auto inIter2 = aie::begin(data2);
    auto outIter = aie::begin(out);
    auto inIter_temp = inIter1;
    int32 c = 0;
    int32 a = 0;
    int32 b = 0;
    for (int j = 0; j < input_width * input_height; j++) {
        for (int k = 0; k < out_channels_1; k++) {
            c = 0;
            inIter1 = inIter_temp;

            for (int i = 0; i < filter_size_1 * filter_size_1 * in_channels_1; i++) {
                a = *inIter1++;
                b = *inIter2++;
                c += a * b;
                printf("%d  %d %d\n", a, b, c);
            }
            // c = c/3;
            *outIter++ = c;
        }
        inIter2 = aie::begin(data2);
        inIter_temp = inIter1;
    }
}
