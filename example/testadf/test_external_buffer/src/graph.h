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
#include "config.h"

using namespace adf;
struct myGraph : public adf::graph {
public :
adf::kernel sq;

adf::external_buffer<uint32> ddrin,ddrout;


myGraph() {

  sq = adf::kernel::create(square);
  adf::source(sq) = "square.cc";
  adf::runtime<ratio>(sq) = 0.9;
  
  ddrin = adf::external_buffer<uint32>::create({256}, 0, 1);
  ddrout = adf::external_buffer<uint32>::create({256}, 1, 0);

  // Specify the read and write access scheme for each iteration
  adf::read_access(ddrin.out[0]) = adf::tiling({.buffer_dimension={256}, .tiling_dimension={256}, .offset={0} });
  adf::write_access(ddrout.in[0]) = adf::tiling({.buffer_dimension={256}, .tiling_dimension={256}, .offset={0} });
  
  dimensions(sq.in[0])={256};
  dimensions(sq.out[0])={256};

  adf::connect(ddrin.out[0],sq.in[0]);
  adf::connect(sq.out[0], ddrout.in[0]);
  };
};