// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <adf.h>
#include "kernels.h"

using namespace adf;

namespace testcase { 

  class two_node_graph : public adf::graph {

  private:
	kernel loop;
  public:
  input_plio in;
  output_plio out;
    
    two_node_graph() { 
	
	in  = input_plio::create("DataIn", adf::plio_32_bits, "data/input.txt");
	out = output_plio::create("DataOut", adf::plio_32_bits, "data/output.txt");

	
	  loop = kernel::create(loopback);

	  location<kernel>(loop) = tile(8,0);

      connect< window<128> > net0 (in.out[0], loop.in[0]);
      connect< window<128> >(loop.out[0], out.in[0]);

      source(loop) = "loopback.cc";

      runtime<ratio>(loop) = 0.2;

    };

  };

};
