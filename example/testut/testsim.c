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
