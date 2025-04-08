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

#ifndef CARDANO_GRAPH_H
#define CARDANO_GRAPH_H

#include <adf.h>
#include "mm.h"

using namespace adf;


class Mm : public adf::graph
{
private:
    kernel mm0;

public:
	// Will be obsoleted
	
    input_gmio inA;
    input_gmio inB;
    output_gmio out;

  Mm()
  {
    // create kernels
    mm0 = kernel::create(krnlmm);

    inA = input_gmio::create("in_source1", 64, 100);
    inB = input_gmio::create("in_source2", 64, 100);
    out = output_gmio::create("out_sink", 64, 100);
    
    connect< window<SIZE*NBYTES_DATA> > net_gc_inA (inA.out[0], mm0.in[0]);
    connect< window<SIZE*NBYTES_DATA> > net_gc_inB (inB.out[0], mm0.in[1]);
    connect< window<SIZE*NBYTES_DATA> > net_gc6 (mm0.out[0], out.in[0]);



    // specify kernel sources
    source(mm0) = "src/mm.cc";
    runtime<ratio>(mm0) = 0.8;
  }
};
#endif

