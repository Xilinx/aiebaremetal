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

#include "softmax.h"
#include <adf.h>

using namespace adf;
struct myGraph : public adf::graph {
public :
adf::kernel sq;

  adf::input_gmio in;
  adf::output_gmio out;
	int a = 2;

  myGraph() {
    in = input_gmio::create("gmio1", 64, 1);
    out = output_gmio::create("gmio2", 64, 1);

    sq = adf::kernel::create( softmax ) ; 
    adf::source(sq) = "src/softmax.cc";

    runtime<ratio>(sq) = 0.6;
		adf::location<adf::kernel>(sq) = adf::tile(2, 0);

    adf::connect<window<128>>(in.out[0], sq.in[0]);
    adf::connect<window<128>>(sq.out[0], out.in[0]);

  }
};

