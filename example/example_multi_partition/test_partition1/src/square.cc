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
#include<aie_api/aie.hpp>
#include<aie_api/aie_adf.hpp>
#include "config.h"

void square(adf::input_buffer<int32>& __restrict data1,
    adf::output_buffer<int32>& __restrict out) 
{
    auto inIter1=aie::begin(data1);
    auto outIter=aie::begin(out);
    for(int i=0;i<256;i++) {
        int32 a=*inIter1++;
        int32 c=a*a;
        *outIter++=c;
        //printf("a: %d  c: %d\n", a, c);
    }
}