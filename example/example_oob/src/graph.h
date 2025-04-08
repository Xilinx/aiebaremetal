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

#include "kernels.h"
#include <string>
//#define ENABLE_PLIO
using namespace adf;
#define InitKernel(KERNEL, FUNC, FILEN, ROW, COL) { \
			KERNEL = adf::kernel::create( FUNC ) ; \
			adf::source(KERNEL) = FILEN;\
			adf::location<adf::kernel>(KERNEL) = tile(COL, ROW);\
			runtime<ratio>(KERNEL) = 0.9; \
		}
#if (__AIE_ARCH__ == AIE)
#define STREAM_CONNECT(OUT, IN) adf::connect<cascade>(OUT, IN);
#else
#define STREAM_CONNECT(OUT, IN)
#endif
struct myGraph : public adf::graph {
	public :
		int a = 2;
		adf::kernel g_kernel[NUM_HW_ROWS][NUM_HW_COLS];
		adf::input_gmio in[NUM_HW_ROWS];
		adf::output_gmio out[NUM_HW_ROWS];
		void ConnectSystolicArray_Row_LToR(int r,
				                   int has_third_stream_out,
				                    adf::kernel* kernel,
				                    adf::input_gmio& in,
						    adf::output_gmio& out,
						    const char* start_kernel_file,
						    const char* middle_kernel_file,
						    const char* end_kernel_file) {
			InitKernel(kernel[0], OneInput, start_kernel_file, r, 0);
			adf::connect<window<WIN_SIZE_BYTES>>(in.out[0], async(kernel[0].in[0]));
			for (int i = 1 ; i < NUM_HW_COLS - 1; i++) {
				InitKernel(kernel[i], TwoInputs, middle_kernel_file, r, i);
				adf::connect<window<WIN_SIZE_BYTES>>(async(kernel[i-1].out[0]), async(kernel[i].in[0]));
				adf::connect<window<WIN_SIZE_BYTES>>(async(kernel[i-1].out[1]), async(kernel[i].in[1]));
				if (has_third_stream_out) {
					STREAM_CONNECT(kernel[i - 1].out[2], kernel[i].in[2]);
				}
			}
			InitKernel(kernel[NUM_HW_COLS -1], OneOutput, end_kernel_file, r, NUM_HW_COLS -1);
			adf::connect<window<WIN_SIZE_BYTES>>(async(kernel[NUM_HW_COLS - 2].out[0]), async(kernel[NUM_HW_COLS-1].in[0]));
			adf::connect<window<WIN_SIZE_BYTES>>(async(kernel[NUM_HW_COLS - 2].out[1]), async(kernel[NUM_HW_COLS-1].in[1]));
			if (has_third_stream_out) {
				STREAM_CONNECT(kernel[NUM_HW_COLS - 2].out[2], kernel[NUM_HW_COLS - 1].in[2]);
			}

			adf::connect<window<WIN_SIZE_BYTES>>(async(kernel[NUM_HW_COLS - 1].out[0]), out.in[0]);
		}
		void ConnectSystolicArray_Row_RToL(int r,
						    int has_third_stream_out,
				                    adf::kernel* kernel,
				                    adf::input_gmio& in,
						    adf::output_gmio& out,
						    const char* start_kernel_file,
						    const char* middle_kernel_file,
						    const char* end_kernel_file) {
			InitKernel(kernel[NUM_HW_COLS -1], OneInput, start_kernel_file, r, NUM_HW_COLS -1);
			adf::connect<window<WIN_SIZE_BYTES>>(in.out[0], async(kernel[NUM_HW_COLS -1].in[0]));
			for (int i = NUM_HW_COLS - 2; i > 0; i--) {
				InitKernel(kernel[i], TwoInputs, middle_kernel_file, r, i);
				adf::connect<window<WIN_SIZE_BYTES>>(async(kernel[i+1].out[0]), async(kernel[i].in[0]));
				adf::connect<window<WIN_SIZE_BYTES>>(async(kernel[i+1].out[1]), async(kernel[i].in[1]));
				if (has_third_stream_out) {
					STREAM_CONNECT(kernel[i+1].out[2], kernel[i].in[2]);
				}
			}
			InitKernel(kernel[0], OneOutput, end_kernel_file, r, 0);
			adf::connect<window<WIN_SIZE_BYTES>>(async(kernel[1].out[0]), async(kernel[0].in[0]));
			adf::connect<window<WIN_SIZE_BYTES>>(async(kernel[1].out[1]), async(kernel[0].in[1]));
			if (has_third_stream_out) {
				STREAM_CONNECT(kernel[1].out[2], kernel[0].in[2]);
			}
			adf::connect<window<WIN_SIZE_BYTES>>(async(kernel[0].out[0]), out.in[0]);
		}
		myGraph() {
			std::ostringstream in_name, out_name;
			for (int r  = 0; r < NUM_HW_ROWS; r++) {
				std::ostringstream in_name, out_name;
				in_name << "gmioin" << r;
				out_name << "gmioout" << r;
				in[r] = input_gmio::create(in_name.str(), 64, 1);
				out[r] = output_gmio::create(out_name.str(), 64, 1);
#if (__AIE_ARCH__ == AIE)
				if ((r & 0x1) == 0) {//even
					ConnectSystolicArray_Row_LToR(r,
							1,
							&g_kernel[r][0],
							in[r],
							out[r],
							"./src/kernels/one_input_aie.cc",
							"./src/kernels/two_inputs_aie.cc",
							"./src/kernels/one_output_aie.cc");
				} else {
					ConnectSystolicArray_Row_RToL(r,
							1,
							&g_kernel[r][0],
							in[r],
							out[r],
							"./src/kernels/one_input_aie.cc",
							"./src/kernels/two_inputs_aie.cc",
							"./src/kernels/one_output_aie.cc");
				}
#else
				ConnectSystolicArray_Row_LToR(r,
							       0,
						               &g_kernel[r][0],
							       in[r],
							       out[r],
							       "./src/kernels/one_input.cc",
							       "./src/kernels/two_inputs.cc",
							       "./src/kernels/one_output.cc");

#endif
			}
		}
};

