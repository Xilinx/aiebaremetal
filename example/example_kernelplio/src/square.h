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

#ifndef SQUARE_MODULE_H
#define SQUARE_MODULE_H


#include <adf.h>
#include "common.h"
//#define SQUARE_BUF_LEN 128

void square(adf::input_buffer<int32>& in, adf::output_buffer<int32>& out);

#endif
