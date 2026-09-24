// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "config.h"
#include "convolution1.h"
#include "convolution2.h"
#include <adf.h>

#define input_width 3
#define input_height 3
#define filter_size_1 3
#define in_channels_1 3
#define out_channels_1 2
#define filter_size_2 3
#define in_channels_2 2
#define out_channels_2 1

using namespace adf;
struct myGraph : public adf::graph {
  public:
    adf::kernel conv_1;
    adf::kernel conv_2;

    adf::external_buffer<uint32> ddrin, ddrout, weights_1, weights_2;
    adf::shared_buffer<uint32> input, output;

    myGraph() {

        conv_1 = adf::kernel::create(conv1);
        conv_2 = adf::kernel::create(conv2);

        adf::source(conv_1) = "src/convolution1.cc";
        adf::source(conv_2) = "src/convolution2.cc";

        adf::runtime<ratio>(conv_1) = 0.9;
        adf::runtime<ratio>(conv_2) = 0.9;

        ddrin = adf::external_buffer<uint32>::create({input_width, input_height, in_channels_1}, 0, 1);
        ddrout = adf::external_buffer<uint32>::create({input_width, input_height, out_channels_2}, 1, 0);
        input = adf::shared_buffer<uint32>::create({input_width, input_height, in_channels_1}, 1, 1);
        output = adf::shared_buffer<uint32>::create({input_width, input_height, out_channels_1}, 1, 1);

        weights_1 = adf::external_buffer<uint32>::create(
            {filter_size_1 * filter_size_1 * in_channels_1 * out_channels_1}, 0, 1); // 3x3x3x2
        weights_2 = adf::external_buffer<uint32>::create(
            {filter_size_2 * filter_size_2 * in_channels_2 * out_channels_2}, 0, 1); // 3x3x3x2

        // Specify the read and write access scheme for each iteration
        adf::read_access(ddrin.out[0]) =
            adf::tiling({.buffer_dimension = {input_width, input_height, in_channels_1},
                         .tiling_dimension = {input_width, input_height, in_channels_1},
                         .offset = {0, 0, 0},
                         .tile_traversal = {{.dimension = 0, .stride = input_width, .wrap = 1},
                                            {.dimension = 1, .stride = input_height, .wrap = 1}}});

        adf::read_access(input.out[0]) =
            adf::tiling({.buffer_dimension = {input_width, input_height, in_channels_1},
                         .tiling_dimension = {filter_size_1, filter_size_1, in_channels_1},
                         .offset = {-1, -1, 0},
                         .tile_traversal = {{.dimension = 0, .stride = 1, .wrap = input_width},
                                            {.dimension = 1, .stride = 1, .wrap = input_height}}});
        adf::write_access(input.in[0]) =
            adf::tiling({.buffer_dimension = {input_width, input_height, in_channels_1},
                         .tiling_dimension = {input_width, input_height, in_channels_1},
                         .offset = {0, 0, 0},
                         .tile_traversal = {{.dimension = 0, .stride = input_width, .wrap = 1},
                                            {.dimension = 1, .stride = input_height, .wrap = 1}}});

        adf::read_access(output.out[0]) =
            adf::tiling({.buffer_dimension = {input_width, input_height, in_channels_2},
                         .tiling_dimension = {filter_size_2, filter_size_2, in_channels_2},
                         .offset = {-1, -1, 0},
                         .tile_traversal = {{.dimension = 0, .stride = 1, .wrap = input_width},
                                            {.dimension = 1, .stride = 1, .wrap = input_height}}});
        adf::write_access(output.in[0]) =
            adf::tiling({.buffer_dimension = {input_width, input_height, out_channels_1},
                         .tiling_dimension = {1, 1, out_channels_1},
                         .offset = {0, 0, 0},
                         .tile_traversal = {{.dimension = 0, .stride = 1, .wrap = input_width},
                                            {.dimension = 1, .stride = 1, .wrap = input_height}}});

        adf::write_access(ddrout.in[0]) =
            adf::tiling({.buffer_dimension = {input_width, input_height, out_channels_2},
                         .tiling_dimension = {1, 1, out_channels_2},
                         .offset = {0, 0, 0},
                         .tile_traversal = {{.dimension = 0, .stride = 1, .wrap = input_width},
                                            {.dimension = 1, .stride = 1, .wrap = input_height}}});
        adf::read_access(weights_1.out[0]) =
            adf::tiling({.buffer_dimension = {filter_size_1 * filter_size_1 * in_channels_1 * out_channels_1},
                         .tiling_dimension = {filter_size_1 * filter_size_1 * in_channels_1 * out_channels_1},
                         .offset = {0}});
        adf::read_access(weights_2.out[0]) =
            adf::tiling({.buffer_dimension = {filter_size_2 * filter_size_2 * in_channels_2 * out_channels_2},
                         .tiling_dimension = {filter_size_2 * filter_size_2 * in_channels_2 * out_channels_2},
                         .offset = {0}});

        dimensions(conv_1.in[0]) = {input_width * input_height * filter_size_1 * filter_size_1 * in_channels_1};
        dimensions(conv_1.in[1]) = {filter_size_1 * filter_size_1 * in_channels_1 * out_channels_1};
        dimensions(conv_1.out[0]) = {input_width * input_height * out_channels_1};

        dimensions(conv_2.in[0]) = {input_width * input_height * filter_size_2 * filter_size_2 * in_channels_2};
        dimensions(conv_2.in[1]) = {filter_size_2 * filter_size_2 * in_channels_2 * out_channels_2};
        dimensions(conv_2.out[0]) = {input_width * input_height * out_channels_2};

        adf::connect(ddrin.out[0], input.in[0]);
        adf::connect(input.out[0], conv_1.in[0]);
        adf::connect(weights_1.out[0], conv_1.in[1]);
        adf::connect(conv_1.out[0], output.in[0]);
        adf::connect(output.out[0], conv_2.in[0]);
        adf::connect(weights_2.out[0], conv_2.in[1]);
        adf::connect(conv_2.out[0], ddrout.in[0]);
    };
};