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

#define WIDTH 8
#define HEIGHT 8 
#define SIZE (WIDTH * HEIGHT)

void krnlmm(
    input_window_float * ina, 
    input_window_float * inb, 
    output_window_float* out,
    float increment_in,
    float &increment_out
);
#endif

