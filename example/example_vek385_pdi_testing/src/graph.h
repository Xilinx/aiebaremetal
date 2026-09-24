// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "add.h"
#include <adf.h>

using namespace adf;

#define N 24

struct mygraph : public adf::graph {
  public:
    kernel k[N];
    input_gmio input[N];
    output_gmio output[N];

    mygraph() {
        for (int i = 0; i < N; i++) {
            std::string gmin = "gmioIn" + std::to_string(i);
            std::string gmout = "gmioOut" + std::to_string(i);
            input[i] = input_gmio::create(gmin, 64, 1);
            output[i] = output_gmio::create(gmout, 64, 1);
            k[i] = kernel::create(add);
            source(k[i]) = "src/add.cc";
            runtime<ratio>(k[i]) = 0.9;

            connect<window<128>>(input[i].out[0], k[i].in[0]);
            connect<window<128>>(k[i].out[0], output[i].in[0]);
            async(k[i].in[1]);
        }
    }
};
