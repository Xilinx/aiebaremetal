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

#include "convolution2.h"
#include <adf.h>
#include <stdio.h>
#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>

#define input_width 3
#define input_height 3
#define filter_size_1 3
#define in_channels_1 3
#define out_channels_1 2
#define filter_size_2 3
#define in_channels_2 2
#define out_channels_2 1

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

void conv2(adf::input_buffer<int32>& __restrict data1,
    adf::input_buffer<int32>& __restrict data2,
    adf::output_buffer<int32>& __restrict out) 
{
    auto inIter1=aie::begin(data1);
    auto inIter2=aie::begin(data2);
    auto outIter=aie::begin(out);
    auto inIter_temp = inIter1;
    int32 c = 0;
    int32 a = 0;
    int32 b = 0;
    for(int j = 0;j < input_width*input_height; j++)
    {
        for(int k = 0 ;k < out_channels_2; k++)
        {
            c = 0;
            inIter1 = inIter_temp;
            
            for(int i=0;i< filter_size_2 * filter_size_2*in_channels_2;i++) 
            {    
                a=*inIter1++;
                b=*inIter2++;
                c += a*b;
                printf("%d  %d %d\n", a, b, c);       
            }
                //c = c/3;
                *outIter++=c;
               
            
            
        }
        inIter2=aie::begin(data2);
        inIter_temp = inIter1;
        
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


