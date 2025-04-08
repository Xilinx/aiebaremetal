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

#ifndef FUNCTION_KERNELS_H
#define FUNCTION_KERNELS_H

#include <adf.h>

#define NBYTES_DATA 4

#define WIDTH 16
#define HEIGHT 16
#define SIZE (WIDTH * HEIGHT)
#define BLK_DIM 8
#define BLK_SIZE (BLK_DIM * BLK_DIM)

////////////////////////////////////////////////////////////////////////////////
//// Hardcoded unrolled fast 8-point (i)DCT
//////////////////////////////////////////////////////////////////////////////////
#define C_a 1.3870398453221474618216191915664f       //a = sqrt(2) * cos(1 * pi / 16)
#define C_b 1.3065629648763765278566431734272f       //b = sqrt(2) * cos(2 * pi / 16)
#define C_c 1.1758756024193587169744671046113f       //c = sqrt(2) * cos(3 * pi / 16)
#define C_d 0.78569495838710218127789736765722f      //d = sqrt(2) * cos(5 * pi / 16)
#define C_e 0.54119610014619698439972320536639f      //e = sqrt(2) * cos(6 * pi / 16)
#define C_f 0.27589937928294301233595756366937f      //f = sqrt(2) * cos(7 * pi / 16)
#define C_norm 0.35355339059327376220042218105242f   //1 / sqrt(8)

void krnlDct_out(input_window_uint32 * in, output_window_float * out);

#endif
