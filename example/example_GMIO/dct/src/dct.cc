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

/* 
 * Kernel hist 
 */

#include <adf.h>
#include <stdio.h>
#include "dct.h"

void dct8x8(float *src, float *dst, int stride)
{
    float X07P = src[0 * stride] + src[7 * stride];
    float X16P = src[1 * stride] + src[6 * stride];
    float X25P = src[2 * stride] + src[5 * stride];
    float X34P = src[3 * stride] + src[4 * stride];

    float X07M = src[0 * stride] - src[7 * stride];
    float X61M = src[6 * stride] - src[1 * stride];
    float X25M = src[2 * stride] - src[5 * stride];
    float X43M = src[4 * stride] - src[3 * stride];

    float X07P34PP = X07P + X34P;
    float X07P34PM = X07P - X34P;
    float X16P25PP = X16P + X25P;
    float X16P25PM = X16P - X25P;

    dst[0 * stride] = C_norm * (X07P34PP + X16P25PP);
    dst[2 * stride] = C_norm * (C_b * X07P34PM + C_e * X16P25PM);
    dst[4 * stride] = C_norm * (X07P34PP - X16P25PP);
    dst[6 * stride] = C_norm * (C_e * X07P34PM - C_b * X16P25PM);

    dst[1 * stride] = C_norm * (C_a * X07M - C_c * X61M + C_d * X25M - C_f * X43M);
    dst[3 * stride] = C_norm * (C_c * X07M + C_f * X61M - C_a * X25M + C_d * X43M);
    dst[5 * stride] = C_norm * (C_d * X07M + C_a * X61M + C_f * X25M - C_c * X43M);
    dst[7 * stride] = C_norm * (C_f * X07M + C_d * X61M + C_c * X25M + C_a * X43M);


}

void krnlDct_out(input_window_uint32 * in, output_window_float* out)
{
    // typedef union {   // float32<->int32 convert
    //     unsigned int i;
    //     float f;
    // } unia;
    // unia val;
    uint32_t val;
    float src[BLK_SIZE];
    float dst[BLK_SIZE];

for(unsigned ii = 0; ii < SIZE/BLK_SIZE; ii++)
{
    for (unsigned i = 0; i < BLK_SIZE; i++)
    { 
        window_readincr(in, val);
        src[i] = val;
        printf("Input :%f\n", src[i]);
        //dst[i] = val.f;
    }
    
    //for (unsigned i = 0; i < BLK_DIM; i++)
    //    dct8x8(src + BLK_DIM*i, dst + BLK_DIM*i, 1);
    
    for (unsigned i = 0; i < BLK_DIM; i++)
        dct8x8(src + i, dst + i, BLK_DIM);   
    
    for (unsigned i = 0; i < BLK_SIZE; i++)
    {
        //val.f = dst[i];
        printf("Output: %f\n", dst[i]);
        window_writeincr(out, dst[i]);
    }
}
}
