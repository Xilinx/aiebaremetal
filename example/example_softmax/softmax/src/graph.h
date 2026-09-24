// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "softmax.h"
#include <adf.h>

using namespace adf;
struct myGraph : public adf::graph {
  public:
    adf::kernel sq;

    adf::input_gmio in;
    adf::output_gmio out;
    int a = 2;

    myGraph() {
        in = input_gmio::create("gmio1", 64, 1);
        out = output_gmio::create("gmio2", 64, 1);

        sq = adf::kernel::create(softmax);
        adf::source(sq) = "src/softmax.cc";

        runtime<ratio>(sq) = 0.6;
        adf::location<adf::kernel>(sq) = adf::tile(2, 0);

        adf::connect<window<128>>(in.out[0], sq.in[0]);
        adf::connect<window<128>>(sq.out[0], out.in[0]);
    }
};
