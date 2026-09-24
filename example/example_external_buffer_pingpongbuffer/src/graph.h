// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "adf.h"
#include "add.h"

using namespace adf;

class mygraph : public graph {
  public:
    kernel k1;
    kernel k2;
    external_buffer<int32> in1;
    external_buffer<int32> out1;
    external_buffer<int32> ext;

    mygraph() {
        in1 = external_buffer<int32>::create({128}, 0, 1);
        out1 = external_buffer<int32>::create({128}, 1, 0);
        ext = external_buffer<int32>::create({32}, 1, 1);
        num_buffers(ext) = 2;

        k1 = kernel::create(add);
        k2 = kernel::create(sub);
        repetition_count(k1) = 4;
        repetition_count(k2) = 4;
        source(k1) = "src/add.cpp";
        source(k2) = "src/add.cpp";
        runtime<ratio>(k1) = 0.9;
        runtime<ratio>(k2) = 0.9;

        connect<window<128>>(in1.out[0], k1.in[0]);
        connect<window<128>>(k1.out[0], ext.in[0]);
        connect<window<128>>(ext.out[0], k2.in[0]);
        connect<window<128>>(k2.out[0], out1.in[0]);

        location<dma>(ext.in[0]) = dma_channel(shim_tile, 3, 0, 0);
        location<dma>(ext.out[0]) = dma_channel(shim_tile, 3, 0, 0);

        location<dma>(out1.in[0]) = dma_channel(shim_tile, 6, 0, 0);
        location<dma>(in1.out[0]) = dma_channel(shim_tile, 6, 0, 0);

        read_access(in1.out[0]) = tiling({.buffer_dimension = {128},
                                          .tiling_dimension = {32},
                                          .offset = {0},
                                          .tile_traversal = {{.dimension = 0, .stride = 32, .wrap = 4}}});
        write_access(out1.in[0]) = tiling({.buffer_dimension = {128},
                                           .tiling_dimension = {32},
                                           .offset = {0},
                                           .tile_traversal = {{.dimension = 0, .stride = 32, .wrap = 4}}});

        read_access(ext.out[0]) = {tiling({.buffer_dimension = {32}, .tiling_dimension = {16}, .offset = {16}}),
                                   tiling({.buffer_dimension = {32}, .tiling_dimension = {16}, .offset = {0}})};
        write_access(ext.in[0]) = {tiling({.buffer_dimension = {32}, .tiling_dimension = {16}, .offset = {16}}),
                                   tiling({.buffer_dimension = {32}, .tiling_dimension = {16}, .offset = {0}})};
    }
};
