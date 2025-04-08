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
#include <stdio.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>

// void square(adf::input_buffer<int32> & __restrict A, adf::output_buffer<int32> & __restrict B)
// {
// 	const int32* __restrict pA=(int32*)A.data();
// 	int32* __restrict pB=(int32*)B.data();
//     for (int i=0; i<256; i++)
//     {
//         int val = *(pA + i);
//         *(pB + i) = val*val;
//     }
// }

void square(adf::input_buffer<int32>& __restrict data1,
    adf::output_buffer<int32>& __restrict out) 
{
    auto inIter1=aie::begin(data1);
    auto outIter=aie::begin(out);
    for(int i=0;i<256;i++) {
        int32 a=*inIter1++;
        int32 c=a*a;
        *outIter++=c;
    }
}


// void square(input_window_int32* in, output_window_int32* out)
// {
//     for (int i=0; i<256; i++)
//     {
//         int val = window_readincr(in);
//         val = val * val;
//         window_writeincr(out, val);
//     }
// }


