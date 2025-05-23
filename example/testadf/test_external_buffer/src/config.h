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

#ifndef __CONFIG__
#define __CONFIG__
//#define HW_TEST
#if defined(HW_TEST)
#define WIN_SIZE 256 // 
#define DATA_LENGTH (1200*1200)
#else
#define WIN_SIZE 32 // 
#define DATA_LENGTH 128
#endif
#endif
