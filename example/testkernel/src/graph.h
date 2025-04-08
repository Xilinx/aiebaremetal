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
//#define ENABLE_SHARE_BUFFER
#include "config.h"
using namespace adf;
struct myGraph : public adf::graph {
public :
adf::kernel sq;

  adf::input_gmio in;
  adf::output_gmio out;
#if (__AIE_ARCH__ != AIE)
  adf::shared_buffer<int32> mem_buff;
#endif

	int a = 2;

  myGraph() {
    in = input_gmio::create("gmio1", 64, 1);
    out = output_gmio::create("gmio2", 64, 1);
    sq = adf::kernel::create( square ) ; 
    adf::source(sq) = "src/square.cc";
    runtime<ratio>(sq) = 0.6;
		adf::location<adf::kernel>(sq) = adf::tile(4, 1);
#if (__AIE_ARCH__ != AIE && defined(ENABLE_SHARE_BUFFER))
    mem_buff = adf::shared_buffer<int32>::create({1024, 48},1,1);
		num_buffers(mem_buff) = 2;
    // col 1, mem tile row 0(abs row 1), share_buffer 0x0 addr start
    location<buffer>(mem_buff) = {address((2+(2*0)-1),1,0),address((2+(2*0)-1),1,262144)};
    adf::connect<window<1024>>(in.out[0], mem_buff.in[0]);
    adf::connect<window<128>>(mem_buff.out[0], sq.in[0]);
    adf::connect<window<128>>(sq.out[0], out.in[0]);

		tiling_parameters memtile1_in = { .buffer_dimension = { 1024,48},.tiling_dimension = { 1024,48},.offset = { 0, 0 } };
		tiling_parameters memtile1_out = { .buffer_dimension = { 1024, 48 },.tiling_dimension = { 1, 48},.offset = { 0, 0 },.tile_traversal = {{ .dimension = 0,.stride = 1,.wrap = 1024 },{ .dimension = 1,.stride = 1,.wrap = 1 }} };
		write_access(mem_buff.in[0]) = tiling(memtile1_in);
		read_access(mem_buff.out[0]) = tiling(memtile1_out);
#else
    adf::connect<window<WIN_SIZE * sizeof(int32)>>(in.out[0], async(sq.in[0]));
    adf::connect<window<WIN_SIZE * sizeof(int32)>>(async(sq.out[0]), out.in[0]);

#endif
  }
};

