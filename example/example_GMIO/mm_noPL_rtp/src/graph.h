// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef CARDANO_GRAPH_H
#define CARDANO_GRAPH_H

#include "mm.h"
#include <adf.h>

using namespace adf;

class Mm : public adf::graph {
  private:
    kernel mm0;

  public:
    // Will be obsoleted
    input_gmio inA;
    input_gmio inB;
    output_gmio out;
    input_port increment_in;
    inout_port increment_out;

    Mm() {
        // create kernels
        mm0 = kernel::create(krnlmm);

        inA = input_gmio::create("in_source1", 64, 100);
        inB = input_gmio::create("in_source2", 64, 100);
        out = output_gmio::create("out_sink", 64, 100);

        connect<window<SIZE * NBYTES_DATA>> net_gc_inA(inA.out[0], mm0.in[0]);
        connect<window<SIZE * NBYTES_DATA>> net_gc_inB(inB.out[0], mm0.in[1]);
        connect<window<SIZE * NBYTES_DATA>> net_gc6(mm0.out[0], out.in[0]);
        connect<parameter>(increment_in, mm0.in[2]);
        connect<parameter>(sync(mm0.inout[0]), increment_out);

        // specify kernel sources
        source(mm0) = "src/mm.cc";
        runtime<ratio>(mm0) = 0.8;
    }
};
#endif
