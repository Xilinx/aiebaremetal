// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef ADF_GRAPH_H
#define ADF_GRAPH_H

#include "dct.h"
#include <adf.h>

using namespace adf;

class Dct : public adf::graph {
  private:
    kernel dct_out;

  public:
    // Will be obsoleted

    input_plio in;
    output_plio out;

    Dct() {
        // create kernels
        dct_out = kernel::create(krnlDct_out);

        in = input_plio::create("Data_Source", adf::plio_32_bits, "data/inputSource.txt", 32);
        out = output_plio::create("Data_Sink", adf::plio_32_bits, "data/outputSink.txt", 32);

        connect<window<SIZE * NBYTES_DATA>> net_gc0(in.out[0], dct_out.in[0]);
        connect<window<SIZE * NBYTES_DATA>> net_gc4(dct_out.out[0], out.in[0]);

        // specify kernel sources
        source(dct_out) = "src/dct.cc";
        runtime<ratio>(dct_out) = 0.8;
    }
};
#endif
