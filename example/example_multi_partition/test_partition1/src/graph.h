// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "square.h"
#include <adf.h>
// #define ENABLE_PLIO
// #define ENABLE_SHARE_BUFFER
#include "config.h"
using namespace adf;
struct myGraph : public adf::graph {
  public:
    adf::kernel sq;

    adf::input_gmio in;
    adf::output_gmio out;

    myGraph() {
        in = input_gmio::create("gmio1", 64, 1);
        out = output_gmio::create("gmio2", 64, 1);
        sq = adf::kernel::create(square);
        adf::source(sq) = "src/square.cc";
        runtime<ratio>(sq) = 0.6;

        adf::connect(in.out[0], sq.in[0]);
        adf::connect(sq.out[0], out.in[0]);
        dimensions(sq.in[0]) = {256};
        dimensions(sq.out[0]) = {256};
    }
};
