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

#ifndef ADF_GRAPH_H
#define ADF_GRAPH_H

#include <adf.h>
#include "dct.h"

using namespace adf;


class Dct : public adf::graph
{
private:
    kernel dct_out;
public:
	// Will be obsoleted
	
    input_plio in;
    output_plio out;

  Dct()
  {
    // create kernels
    dct_out = kernel::create(krnlDct_out);

    in = input_plio::create("Data_Source", adf::plio_32_bits,"data/inputSource.txt", 32);
    out = output_plio::create("Data_Sink",adf::plio_32_bits, "data/outputSink.txt", 32);

    connect< window<SIZE*NBYTES_DATA> > net_gc0 (in.out[0], dct_out.in[0]);
    connect< window<SIZE*NBYTES_DATA> > net_gc4 (dct_out.out[0], out.in[0]);



    // specify kernel sources
    source(dct_out) = "src/dct.cc";
    runtime<ratio>(dct_out) = 0.8;
  }
};
#endif

