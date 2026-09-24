// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "config.h"
#include "square.h"
#include <adf.h>

using namespace adf;
struct myGraph : public adf::graph {
  public:
    adf::kernel sq;

    adf::external_buffer<uint32> ddrin, ddrout;

    myGraph() {

        sq = adf::kernel::create(square);
        adf::source(sq) = "square.cc";
        adf::runtime<ratio>(sq) = 0.9;

        ddrin = adf::external_buffer<uint32>::create({256}, 0, 1);
        ddrout = adf::external_buffer<uint32>::create({256}, 1, 0);

        // Specify the read and write access scheme for each iteration
        adf::read_access(ddrin.out[0]) =
            adf::tiling({.buffer_dimension = {256}, .tiling_dimension = {256}, .offset = {0}});
        adf::write_access(ddrout.in[0]) =
            adf::tiling({.buffer_dimension = {256}, .tiling_dimension = {256}, .offset = {0}});

        dimensions(sq.in[0]) = {256};
        dimensions(sq.out[0]) = {256};

        adf::connect(ddrin.out[0], sq.in[0]);
        adf::connect(sq.out[0], ddrout.in[0]);
    };
};