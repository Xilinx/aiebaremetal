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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
///proj/xbuilds/9999.0_INT_1220_2214/installs/lin64/Vitis/2024.1/aietools/include/adf/window/types.h
//typedef uint32_t uint;
//typedef unsigned long long uint64;
extern "C" {
	void ess_Write32(unsigned long long Addr, u_int32_t Data){
		printf("strong ess_Write32_\n");
	}
	u_int32_t ess_Read32(unsigned long long Addr) {
		printf("strong ess_Read32_\n");
	}

	void ess_WriteCmd(unsigned char Command, unsigned char ColId, unsigned char RowId, unsigned int CmdWd0, unsigned int CmdWd1, unsigned char *CmdStr) {
		printf("strong ess_WriteCmd\n");
	}
}
