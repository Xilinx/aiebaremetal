// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
//
// Minimal AIE kernel for the pl_only softmax example. All the actual softmax
// math (exp + sum + divide) happens in the PL kernel `softmax_pl`
// (pl_kernels/softmax_pl.cpp) — this AIE tile only exists because aiebaremetal
// requires a non-empty libadf.a / at least one AIE core in the graph. It is
// the minimal valid AIE participant: a straight copy.

#include "passthrough.h"
#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <stdio.h>

void passthrough(adf::input_buffer<float> &in, adf::output_buffer<float> &out) {
    auto InIter = aie::begin(in);
    auto OutIter = aie::begin(out);
    for (unsigned i = 0; i < TILE_LEN; i++) {
        *OutIter++ = *InIter++;
    }
}
