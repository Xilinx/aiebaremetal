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

#include <fstream>
#include <xaiengine.h>

#include "../../aiebaremetal.h"
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
BaremetalGraphSim gr("gradf");
#else
BaremetalGraph gr("gradf");
#endif

void adf_run() {
	int32* inputArray = (int32*)GMIO::malloc(256*sizeof(int32));
	int32* outputArray = (int32*)GMIO::malloc(256*sizeof(int32));

	for (int i=0; i<256; i++)
		inputArray[i] = i+1;

	std::ofstream ofs;

	gradf.run(1);
	for (int i=0; i<1; i++)
	{
		gradf.in.gm2aie_nb(&inputArray[i*32], 32*sizeof(int32));
		gradf.out.aie2gm_nb(&outputArray[i*32], 32*sizeof(int32));
	}
	gradf.out.wait(); //assuming data from gm1 are processed by the graph and output to gm2

	for (int i=0; i<256; i++)
	{
		std::cout<<outputArray[i]<<", ";
	}
 
  std::cout << "before end" << std::endl;
	gradf.end();

	GMIO::free(inputArray);
	GMIO::free(outputArray);
}

int main(int argc, char ** argv)
{
	std::cout << "gradf.a  =" << gradf.a << std::endl;
	gradf.init();

	float* inputArray = (float*)gr.malloc(256*sizeof(float));
	float* outputArray = (float*)gr.malloc(256*sizeof(float));
	float* outputArray_gold = (float*)gr.malloc(256*sizeof(float));

	gr.init();
	gr.run(8);

	for (int i=0; i<256; i++)
		inputArray[i] = (i+1)/500.0;
	std::cout << "before:run---" << std::endl;

	// Calculate softmax on cpu for comparison
	float exp_values[32];
	float sum = 0.0f;
	for(int count = 0 ;count < 8;count++){
        sum = 0.0;
		for (unsigned int i = 0; i < 32; i++) {
			exp_values[i] = exp(inputArray[count * 32 + i]);
			sum += exp_values[i];
		}

		for (size_t i = 0; i < 32; i++) {
			outputArray_gold[count * 32 + i] = exp_values[i]/sum;
		}
	}
	for (int i=0; i<8; i++)
	{
		gr.gm2aie_nb("gradf.in",reinterpret_cast<char*>(&inputArray[i*32]), 32*sizeof(float));
		gr.aie2gm_nb("gradf.out",reinterpret_cast<char*>(&outputArray[i*32]), 32*sizeof(float));
	}
	gr.gmio_wait("gradf.out"); //assuming data from gm1 are processed by the graph and output to gm2
	std::cout << "run---" << std::endl;
	gr.wait();
	for (int i=0; i<256; i++)
	{
		std::cout<<outputArray[i]<<", ";
		std::cout<<outputArray_gold[i]<<", ";
		if(abs(outputArray[i] - outputArray_gold[i]) > 1e-6){
			std::cout<<"\n"<<"Test Failed. Output does not match cpu reference\n";
			std::cout<<outputArray[i]<<", ";
		    std::cout<<outputArray_gold[i]<<", ";
			std::cout<<i<<"\n";
			return -1;
		}
	}
	std::cout<<"Test Passed\n";
	
  return 0;
}
