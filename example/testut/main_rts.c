// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
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
