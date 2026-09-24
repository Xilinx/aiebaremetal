// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "square.h"
#include <adf.h>

using namespace adf;
#define COL_NUM 5
struct myGraph : public adf::graph {
  public:
    adf::kernel sq[3][COL_NUM]; // 3 aie row 4 col

    adf::input_gmio in[3];
    adf::output_gmio out[3];

    myGraph() {
        for (int i = 0; i < 3; i++) {

            in[i] = input_gmio::create("gmioin" + std::to_string(i), 64, 1);
            out[i] = output_gmio::create("gmioout" + std::to_string(i), 64, 1);
            for (int j = 0; j < COL_NUM; j++) {
                sq[i][j] = adf::kernel::create(square);
                adf::source(sq[i][j]) = "src/square.cc";
                runtime<ratio>(sq[i][j]) = 0.6;
                adf::location<adf::kernel>(sq[i][j]) = adf::tile(j + 1, i); // col, row
            }
            adf::connect<window<128>>(in[i].out[0], sq[i][0].in[0]);
            for (int j = 0; j < COL_NUM - 1; j++) {
                adf::connect<window<128>>(sq[i][j].out[0], sq[i][j + 1].in[0]);
            }
            adf::connect<window<128>>(sq[i][COL_NUM - 1].out[0], out[i].in[0]);
            // adf::connect<window<128>>(sq.out[0],out.in[0]);
        }
    }
};
