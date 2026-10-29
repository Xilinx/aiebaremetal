// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "passthrough.h"
#include <adf.h>

using namespace adf;
struct myGraph : public adf::graph {
  public:
    adf::kernel pt;
    adf::input_plio pin;
    adf::output_plio pout;

    int a = 2;

    myGraph() {
        pin = input_plio::create("plioin1", plio_32_bits, "data/input.txt", 32);
        // aiesimulator_output/aietopl_output.txt when run under x86/AIE sim.
        pout = output_plio::create("plioout1", plio_32_bits, "aietopl_output.txt", 32);

        pt = adf::kernel::create(passthrough);
        adf::source(pt) = "src/passthrough.cc";
        runtime<ratio>(pt) = 0.6;
        adf::location<adf::kernel>(pt) = adf::tile(2, 0);

        connect(pin.out[0], pt.in[0]);
        connect(pt.out[0], pout.in[0]);

        dimensions(pt.in[0]) = {TILE_LEN};
        dimensions(pt.out[0]) = {TILE_LEN};
    }
};
