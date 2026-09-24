// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include <stdio.h>
#include "common.h"

int main(int argc, char* argv[]) {
	//rpu will always disable cache, 
	// we just use rpu to generate the DDR data
	Xil_DCacheDisable();
	Xil_ICacheDisable();
	printf("rpu ---test\n");
	wdata(0);
	wsync(1);
	return 0;
}
