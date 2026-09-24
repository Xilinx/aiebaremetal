// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <stdlib.h>
#include "aiebaremetal.h"
#include "sim.h"
int g_start_plio=0;// compatible with baremetal lib which assume simulator use this to enable gmio
int main(int argc, char* argv[]) {
	BaremetalGraphSim gr("gradf");
	//gr.init();
	gr.selftestexit();
	int32_t* inputArray = (int32_t*)gr.malloc(256*sizeof(int32_t));
	int32_t* outputArray = (int32_t*)gr.malloc(256*sizeof(int32_t));

	for (int i=0; i<256; i++)
		inputArray[i] = i+1;

	for (int i=0; i<8; i++)
	{
		gr.gm2aie_nb("gradf.in",reinterpret_cast<char*>(&inputArray[i*32]), 32*sizeof(int32_t));
		gr.aie2gm_nb("gradf.out",reinterpret_cast<char*>(&outputArray[i*32]), 32*sizeof(int32_t));
	}
	gr.gmio_wait("gradf.out"); //assuming data from gm1 are processed by the graph and output to gm2

	gr.init();
	gr.run(8);
	gr.wait();
	return 0;
}
