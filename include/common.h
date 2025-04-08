/**
* Copyright (C) 2024 Advanced Micro Devices, Inc.
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
#include <unistd.h>
#include <stdio.h>
#include <iostream>
inline void gdb_point(uint8_t bdebug) {
		while(bdebug) {
			std::cout << " getpid = "<< getpid() << std::endl;
			usleep(1000);
		}
}
#endif
