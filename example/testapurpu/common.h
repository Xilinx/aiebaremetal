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

#ifndef __COMMON__
#define __COMMON__
#include "xil_printf.h"
//#include "xaiengine.h"
#include "xil_io.h"
#include "xil_cache.h"
#include <stdio.h>
//#include <math.h>
#include "xtime_l.h"

#define SECTION_SIZE (1024)
#define SECTION_COUNT (8)


#define SECTION_ADDRESS (1024 * 1024)
#define LOCAL_ADDRESS (1024 * 1024 *100)
#define SYNC_ADDRESS (1024 * 1024 * 50)

void wdata();
void rdata();
void wsync(uint32_t var);
uint32_t rsync();
void log(char* app);
void apu_disablecache();
#endif
