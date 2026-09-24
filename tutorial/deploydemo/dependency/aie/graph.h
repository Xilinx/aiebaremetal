// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0


#include "mysquare.h"
#include <adf.h>

using namespace adf;
#define DATA_LENGTH 128

struct myGraph : public adf::graph {

public :
  adf::kernel sq;
  adf::input_gmio in;
  adf::output_gmio out;

  myGraph() { 
    in = input_gmio::create("gmio1", 64, 1);
    out = output_gmio::create("gmio2", 64, 1);

    sq = adf::kernel::create( mysquare ) ; 
    adf::source(sq) = "./mysquare.cc";

    runtime<ratio>(sq) = 0.6;

    adf::connect<window<DATA_LENGTH*sizeof(int32)>>(in.out[0], sq.in[0]);
    adf::connect<window<DATA_LENGTH*sizeof(int32)>>(sq.out[0], out.in[0]);

  }
};

