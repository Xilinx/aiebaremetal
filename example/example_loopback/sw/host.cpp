// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "xparameters.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "aiebaremetal.h"
AbrGraph gr("mygraph");
// SGRACE
//#include "graph.cpp"

//#define MM2S_1_BASE XPAR_XMM2S_0_S_AXI_CONTROL_BASEADDR
//#define S2MM_1_BASE XPAR_XS2MM_0_S_AXI_CONTROL_BASEADDR

#define MM2S_1_BASE XPAR_XMM2S_0_BASEADDR
#define S2MM_1_BASE XPAR_XS2MM_0_BASEADDR

#define MEM_OFFSET 0x10
#define SIZE_OFFSET 0x1C
#define CTRL_OFFSET 0x0

#define SQUARE_BUF_LEN 32


int RunTest(uint64_t mm2s_1_base ,uint64_t s2mm_1_base, int* in, int* golden,  int* out, uint32_t length)
{
	int i;
	int errCount = 0;
	uint64_t mem_in1Addr = (uint64_t)in;
	uint64_t mem_out1Addr = (uint64_t)out;


	printf("Starting test w/ cu\n");

	Xil_Out32(mm2s_1_base + MEM_OFFSET, (uint32_t) mem_in1Addr);
	uint32_t value = Xil_In32(mm2s_1_base + MEM_OFFSET);
	printf("value is %d\n", value);
	Xil_Out32(mm2s_1_base + MEM_OFFSET + 4, 0);

	Xil_Out32(s2mm_1_base + MEM_OFFSET, (uint32_t) mem_out1Addr);
	Xil_Out32(s2mm_1_base + MEM_OFFSET + 4, 0);

	Xil_Out32(mm2s_1_base + SIZE_OFFSET, length);
	Xil_Out32(s2mm_1_base + SIZE_OFFSET, length);

	Xil_Out32(mm2s_1_base + CTRL_OFFSET, 1);
	Xil_Out32(s2mm_1_base + CTRL_OFFSET, 1);

    // SGRACE
    printf("GRAPH INIT\n");
    gr.init();

    // SGRACE
    printf("GRAPH RUN\n");
    gr.run(1);

    printf("GRAPH WAIT\n");
    gr.wait();

    gr.end();

	while(1) {
		uint32_t v = Xil_In32(s2mm_1_base + CTRL_OFFSET);
		if(v & 6) {
			break;
		}
	}



    // SGRACE
    printf("PLIO IP DONE!\n");

	for(i = 0; i < 20; i++) {
		if((((int32_t*)out)[i] != ((int32_t*)golden)[i]) ) {
			printf("Error found in sample %d != to the golden %d\n", i+1, ((int32_t*)out)[i], ((int32_t*)golden)[i]);
			errCount++;
		}
		else
			printf("%d\n ",((int32_t*)out)[i]);
	}



	printf("Ending test w/ cu\n");
	return errCount;

}
int main()
{
	int i;
	int errCount;

	Xil_DCacheDisable();

	int in[SQUARE_BUF_LEN], out[SQUARE_BUF_LEN], golden[SQUARE_BUF_LEN];
	for (int i = 0; i < SQUARE_BUF_LEN; i++) {
		in[i] = i;
	out[i] = 0;
		golden[i] = i;
	}
    // SGRACE
	//init_platform();
	printf("- \n");
	printf("Beginning test\n");

	errCount = RunTest(MM2S_1_BASE, S2MM_1_BASE, in, golden, out, SQUARE_BUF_LEN);

	if(errCount == 0)
		printf("Test passed. \n");
	else
		printf("ERROR: Test failed! Error count: %d \n",errCount);

	return errCount;
}
