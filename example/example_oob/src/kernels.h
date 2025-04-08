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

#ifndef __KERNELS__
#define __KERNELS__
#include <adf.h>
#include "kernels/config.h"
#if (__AIE_ARCH__ == AIE)
void OneInput(input_window<int32>* __restrict dataIn,
	      output_window<int32>* __restrict aOut,
	      output_window<int32>* __restrict result,
	      output_stream_acc48 *bOut);

void TwoInputs(input_window<int32>* __restrict aIn,
	       input_window<int32>* __restrict bypassResult,
	       input_stream_acc48 *bIn,
	       output_window<int32>* __restrict aOut,
	       output_window<int32>* __restrict result,
	       output_stream_acc48 *bOut);

void OneOutput(input_window<int32>* __restrict aIn,
	       input_window<int32>* __restrict bypassResult,
	       input_stream_acc48 *bIn,
	       output_window<int32>* __restrict result);
#else
void OneInput(input_window_uint32 * dataIn,
		output_window_uint32 * dataOut,
		output_window_uint32 * result);

void TwoInputs(input_window_uint32 * dataIn,
		input_window_uint32 * bypassResult,
		output_window_uint32 * dataOut,
		output_window_uint32 * result);


void OneOutput(input_window_uint32 * dataIn,
		input_window_uint32 * bypassResult,
		output_window_uint32 * result);
#endif
#endif
