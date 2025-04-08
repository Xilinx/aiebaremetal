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

#include <stdio.h>
#include <adf.h>
#include "../../../include/log/klog.h"
#include<aie_api/aie.hpp>
#include<aie_api/aie_adf.hpp>
#include "square.h"

#define VECTOR_LENGTH 16
void square(adf::input_buffer<int32>& in, adf::output_buffer<int32>& out) { 

	printf("square log\n");
	if (0) {
		int32_t A[32];
		auto InIter = aie::begin_vector<VECTOR_LENGTH>(in);
		auto OutIter = aie::begin_vector<VECTOR_LENGTH>(out);
		for (unsigned w = 0; w < 32 / VECTOR_LENGTH; w++) 
			chess_prepare_for_pipelining{
				aie::vector<int32_t, VECTOR_LENGTH> temp_a = *InIter++;
				aie::store_unaligned_v<VECTOR_LENGTH>(A + (w*VECTOR_LENGTH), temp_a);
			}

		for (int i = 0; i < 32; i++) {
			A[i] *= A[i];
			printf("A[%d] is %d\n", i, A[i]);
		}

		for (unsigned z = 0; z < 32/VECTOR_LENGTH; z++) chess_prepare_for_pipelining{
			*OutIter++ = aie::load_v<VECTOR_LENGTH>(A + (z * VECTOR_LENGTH));
		}

		//printf("\nsquare log end \n");
	} else {
		auto InIter = aie::begin(in);
		auto OutIter = aie::begin(out);
		printf("square log2\n");
		for (unsigned i = 0 ; i < SQUARE_BUF_LEN; i++) {
			// wlog("loop read in %d\n", i);
			int j = *InIter++;
			if (i==0)printf("%d ", j);
			*OutIter++ = j*j;
		}
		printf("\nsquare log end \n");
	}
}
