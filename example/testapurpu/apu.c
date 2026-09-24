// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include <stdio.h>
#include "common.h"

int main(int argc, char* argv[]) {
	void* baseaddr_p = aligned_alloc(16,2048);
	void* baseaddr_p1 = aligned_alloc(16,2048);
	void* baseaddr_p2 = aligned_alloc(16,2048);
	printf("a baseaddr_p = %p\n", baseaddr_p);
	printf("baseaddr_p1 = %p\n", baseaddr_p1);
	printf("baseaddr_p2 = %p\n", baseaddr_p2);
	log("apu");
	apu_disablecache();
	wsync(0);
	wdata(1);//clear the data
	while(rsync() == 0);
	rdata();
	return 0;
}
