// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "convolution1.h"
// #include "convolution2.h"
#include "config.h"
#include <adf.h>
///*
#define pading 3

// #define input_height 224
// #define filter_size_1 IFM_K_SIZE
// #define in_channels_1 3
// #define out_channels_1 64

// #define output_height OFM_W_SIZE
// #define output_width  OFM_W_SIZE

// #define block_size 16
//*/

using namespace adf;
struct myGraph : public adf::graph {
  public:
    input_port wgtcount_port[P_COL_NUM_USED][CORE_NUM_PER_COL]; //,wgtcount_port_2;
    adf::kernel conv[P_COL_NUM_USED][CORE_NUM_PER_COL];
    // adf::kernel conv_2;

    // 12 ddrin , each one broadcast to 8 core of whole col
    adf::external_buffer<uint32> ddrin[P_COL_NUM_USED], ddrout[P_COL_NUM_USED];
    adf::shared_buffer<uint32> input[P_COL_NUM_USED], output[P_COL_NUM_USED];
    // 8 weights each one broadcast to whole row 12 item
    adf::external_buffer<uint32> ddr_weights[CORE_NUM_PER_COL];
    adf::shared_buffer<uint32> input_weight[CORE_NUM_PER_COL];

    pktmerge<CORE_NUM_PER_COL> output_merge[P_COL_NUM_USED];

#define WEIGHT

    myGraph() {

        // conv_1 = adf::kernel::create(conv1);
        // conv_1_2 = adf::kernel::create(conv1);
        // adf::source(conv_1) = "src/convolution1.cc";
        // adf::source(conv_1_2) = "src/convolution1.cc";
        // adf::location<adf::kernel>(conv_1) = adf::tile(0, 0);//column row
        // adf::location<adf::kernel>(conv_1_2) = adf::tile(0, 1);//column row

        // adf::runtime<ratio>(conv_1) = 0.9;
        // adf::runtime<ratio>(conv_1_2) = 0.9;

        // 8 ddrin_weight, each one broadcast into 12 column core of one row
        for (int j = 0; j < CORE_NUM_PER_COL; j++) {
            ddr_weights[j] = adf::external_buffer<uint32>::create({IFM_K_SIZE * IFM_K_SIZE * 3 * 8}, 0, 1);
            input_weight[j] = adf::shared_buffer<uint32>::create({IFM_K_SIZE, IFM_K_SIZE, 3 * 8}, 1, 1);
            adf::read_access(ddr_weights[j].out[0]) =
                adf::tiling({.buffer_dimension = {IFM_K_SIZE, IFM_K_SIZE, 3 * 8},
                             .tiling_dimension = {IFM_K_SIZE, IFM_K_SIZE, 3 * 8},
                             .offset = {0, 0, 0},
                             .tile_traversal = {{.dimension = 0, .stride = IFM_K_SIZE, .wrap = 1},
                                                {.dimension = 1, .stride = IFM_K_SIZE, .wrap = 1}}});

            adf::write_access(input_weight[j].in[0]) =
                adf::tiling({.buffer_dimension = {IFM_K_SIZE, IFM_K_SIZE, 3 * 8},
                             .tiling_dimension = {IFM_K_SIZE, IFM_K_SIZE, 3 * 8},
                             .offset = {0, 0, 0},
                             .tile_traversal = {{.dimension = 0, .stride = IFM_K_SIZE, .wrap = 1},
                                                {.dimension = 1, .stride = IFM_K_SIZE, .wrap = 1}}});

            adf::read_access(input_weight[j].out[0]) =
                adf::tiling({.buffer_dimension = {IFM_K_SIZE, IFM_K_SIZE, 3 * 8},
                             .tiling_dimension = {IFM_K_SIZE, IFM_K_SIZE, 3},
                             .offset = {0, 0, 0},
                             .tile_traversal = {{.dimension = 0, .stride = IFM_K_SIZE, .wrap = 1},
                                                {.dimension = 1, .stride = IFM_K_SIZE, .wrap = 1},
                                                {.dimension = 2, .stride = 3, .wrap = 8}}});
        }
        // 12 ddrin , each one broadcast into 8 core in same column
        for (int j = 0; j < P_COL_NUM_USED; j++) {
            uint32_t block_len_in_height = TILE_BLOCK_H; // 3 + 14 + 3;
            ddrin[j] = adf::external_buffer<uint32>::create({IFM_W_AFTER_PAD_SIZE, block_len_in_height, 3}, 0, 1);
            ddrout[j] = adf::external_buffer<uint32>::create({OFM_W_SIZE, 7 /*OFM_W_SIZE/16*/, 64}, 1, 0);
            input[j] = adf::shared_buffer<uint32>::create({IFM_W_AFTER_PAD_SIZE, block_len_in_height, 3}, 1, 1);

            // output = adf::shared_buffer<uint32>::create({OFM_W_SIZE, 6,8}, 1, 1);
            // ddr_weights[j] = adf::external_buffer<uint32>::create({IFM_K_SIZE*IFM_K_SIZE*3*8}, 0, 1); //
            // IFM_K_SIZE*IFM_K_SIZE*3*64

            output_merge[j] = adf::pktmerge<CORE_NUM_PER_COL>::create();

            // Specify the read and write access scheme for each iteration
            adf::read_access(ddrin[j].out[0]) =
                adf::tiling({.buffer_dimension = {IFM_W_AFTER_PAD_SIZE, block_len_in_height, 3},
                             .tiling_dimension = {IFM_W_AFTER_PAD_SIZE, block_len_in_height, 3},
                             .offset = {0, 0, 0},
                             .tile_traversal = {{.dimension = 0, .stride = IFM_W_AFTER_PAD_SIZE, .wrap = 1},
                                                {.dimension = 1, .stride = block_len_in_height, .wrap = 1}}});

            adf::write_access(input[j].in[0]) =
                adf::tiling({.buffer_dimension = {IFM_W_AFTER_PAD_SIZE, block_len_in_height, 3},
                             .tiling_dimension = {IFM_W_AFTER_PAD_SIZE, block_len_in_height, 3},
                             .offset = {0, 0, 0},
                             .tile_traversal = {{.dimension = 0, .stride = IFM_W_AFTER_PAD_SIZE, .wrap = 1},
                                                {.dimension = 1, .stride = block_len_in_height, .wrap = 1}}});

            adf::read_access(input[j].out[0]) =
                adf::tiling({.buffer_dimension = {IFM_W_AFTER_PAD_SIZE, block_len_in_height, 3},
                             .tiling_dimension = {IFM_K_SIZE, IFM_K_SIZE, 3},
                             .offset = {0, 0, 0},
                             .tile_traversal = {{.dimension = 0, .stride = 2, .wrap = OFM_W_SIZE},
                                                {.dimension = 1, .stride = 2, .wrap = IFM_K_SIZE}}});

            /*
              adf::write_access(output.in[0]) = adf::tiling({.buffer_dimension={OFM_W_SIZE * 3 * 8*2},
                                                            .tiling_dimension={OFM_W_SIZE * 3 * 8*2},
                                                            .offset={0},
                                                            .tile_traversal = {{.dimension=0, .stride=OFM_W_SIZE*3*8*2,
              .wrap=1} }});


              adf::read_access(output.out[0]) = adf::tiling({.buffer_dimension={OFM_W_SIZE*3*8*2},
                                                            .tiling_dimension={OFM_W_SIZE*3*8*2},
                                                            .offset={0},
                                                            .tile_traversal = {{.dimension=0, .stride=OFM_W_SIZE*3*8*2,
              .wrap=1}} });

            */

            adf::write_access(ddrout[j].in[0]) =
                adf::tiling({.buffer_dimension = {OFM_W_SIZE, 7, 8 * 8},
                             .tiling_dimension = {OFM_W_SIZE, 7, 8 * 8},
                             .offset = {0, 0, 0},
                             .tile_traversal = {{.dimension = 0, .stride = OFM_W_SIZE * 7 * 8 * 8, .wrap = 1}}});

            // dimensions(conv_1.in[0])={IFM_K_SIZE*IFM_K_SIZE*3};
            // dimensions(conv_1.in[1])={IFM_K_SIZE*IFM_K_SIZE*3};
            // dimensions(conv_1.out[0])={8};

            // dimensions(conv_1_2.in[0])={IFM_K_SIZE*IFM_K_SIZE*3};
            // dimensions(conv_1_2.in[1])={IFM_K_SIZE*IFM_K_SIZE*3};
            // dimensions(conv_1_2.out[0])={8};

            adf::connect(ddrin[j].out[0], input[j].in[0]);
            // adf::connect(ddr_weights[j].out[0], input_weight[j].in[0]);
            //
            for (int i = 0; i < CORE_NUM_PER_COL; i++) {
                // only create ddr with share when do first col 8 core weight connect
                // then other col just need to broadcast from first time create sharebuffer
                conv[j][i] = adf::kernel::create(conv1);
                adf::source(conv[j][i]) = "src/convolution1.cc";
                adf::location<adf::kernel>(conv[j][i]) = adf::tile(j, i); // column row
                adf::runtime<ratio>(conv[j][i]) = 0.9;

                if (j == 0) {
                    adf::connect(ddr_weights[i].out[0], input_weight[i].in[0]);
                }
                adf::connect(input_weight[i].out[0], conv[j][i].in[1]);
                // ddrin share memory data
                adf::connect(input[j].out[0], conv[j][i].in[0]);

                connect<parameter>(wgtcount_port[j][i], async(conv[j][i].in[2]));
                adf::connect(conv[j][i].out[0], output_merge[j].in[i]);

                dimensions(conv[j][i].in[0]) = {IFM_K_SIZE * IFM_K_SIZE * 3};
                dimensions(conv[j][i].in[1]) = {IFM_K_SIZE * IFM_K_SIZE * 3};
                dimensions(conv[j][i].out[0]) = {8};
            }
            //
            /*
            adf::connect(input.out[0],conv_1.in[0]);
            adf::connect(input_weight.out[0],conv_1.in[1]);
            connect<parameter>(wgtcount_port, async(conv_1.in[2]));
            adf::connect(conv_1.out[0], output_merge[0].in[0]);
            //
            adf::connect(input.out[0],conv_1_2.in[0]);
            adf::connect(input_weight.out[0],conv_1_2.in[1]);
            connect<parameter>(wgtcount_port_2, async(conv_1_2.in[2]));
            adf::connect(conv_1_2.out[0], output_merge[0].in[1]);
            */
            //----
            // adf::connect(output_merge[0].out[0], output.in[0]);
            adf::connect(output_merge[j].out[0], ddrout[j].in[0]);
            // disable_out_of_order_mode(ddrout[j].in[0]);
            //

            // adf::connect(output.out[0], ddrout[0].in[0]);
        }
    };
};