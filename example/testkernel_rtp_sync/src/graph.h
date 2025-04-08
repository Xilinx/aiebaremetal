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
adf::port<adf::direction::in> trigger;
#ifdef ENABLE_PLIO
adf::kernel sq2;
adf::input_plio pin;
adf::output_plio pout;
#endif

  adf::input_gmio in;
  adf::output_gmio out;

	int a = 2;

  myGraph() {
    in = input_gmio::create("gmio1", 64, 1);
    out = output_gmio::create("gmio2", 64, 1);
    sq = adf::kernel::create( square ) ; 
    adf::source(sq) = "src/square.cc";
    runtime<ratio>(sq) = 0.6;
		adf::location<adf::kernel>(sq) = adf::tile(2, 0);
    adf::connect<window<128>>(in.out[0], sq.in[0]);
    adf::connect<window<128>>(sq.out[0], out.in[0]);
    //async(sq.in[1]);
    adf::connect<adf::parameter>(trigger, sq.in[1]);

#ifdef ENABLE_PLIO
		pin  = input_plio::create("plioin1", plio_64_bits, "data/input.txt", 500);
    pout = output_plio::create("plioout1", plio_64_bits, "data/output.txt", 500);
    sq2 = adf::kernel::create( square ) ; 
    adf::source(sq2) = "src/square.cc";
    runtime<ratio>(sq2) = 0.6;
		adf::location<adf::kernel>(sq2) = adf::tile(3, 0);
    adf::connect<window<128>>(pin.out[0], sq2.in[0]);
    adf::connect<window<128>>(sq2.out[0], pout.in[0]);
#endif

  }
};

