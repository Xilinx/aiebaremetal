/**
* Copyright (C) 2025 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include "square.h"
#include <adf.h>
//#define ENABLE_PLIO
using namespace adf;
struct myGraph : public adf::graph {
public :
	adf::kernel sq;
  adf::input_plio pin;
  adf::output_plio pout;

	int a = 2;

  myGraph() {

		pin  = input_plio::create("plioin1", plio_32_bits, "data/input.txt", 32);
		// the outpt file will not be generated under current folder, it get
		// redirect to ./aiesimulator_output/  , please check this output file in 
		// ./aiesimulator_output/aietopl_output.txt"
    pout = output_plio::create("plioout1", plio_32_bits, "aietopl_output.txt", 32);

    sq = adf::kernel::create( square ) ; 
    adf::source(sq) = "src/square.cc";
    runtime<ratio>(sq) = 0.6;
		adf::location<adf::kernel>(sq) = adf::tile(2, 0);

    // Connect buffer ports
    connect(pin.out[0], sq.in[0]);
    connect(sq.out[0], pout.in[0]);

    // Dimenstion specifies the size of the buffer
    dimensions(sq.in[0]) = {SQUARE_BUF_LEN};
    dimensions(sq.out[0]) = {SQUARE_BUF_LEN};

  }
};

