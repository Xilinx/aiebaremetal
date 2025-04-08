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
#include "abr_baremetal_api.h"
BaremetalGraph gr("gradf");
#define DATA_LENGTH 128
int main(int argc, char ** argv)
{
	printf("adfr-----\n");
	int32_t* inputArray = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));
	int32_t* outputArray = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));
  int32_t* outputArray_gold = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));

	gr.init();
	gr.run(1);
	for (int i=0; i<DATA_LENGTH; i++){
		inputArray[i] = i+1;
		outputArray_gold[i] = inputArray[i]*inputArray[i];
	}
	gr.gm2aie_nb("gradf.in",reinterpret_cast<char*>(&inputArray[0]), DATA_LENGTH*sizeof(int32_t));
	gr.aie2gm_nb("gradf.out",reinterpret_cast<char*>(&outputArray[0]), DATA_LENGTH*sizeof(int32_t));
	//std::cout << "before gmio_wait ---" << std::endl;
	gr.gmio_wait("gradf.out"); //assuming data from gm1 are processed by the graph and output to gm2
	gr.wait();
	gr.end();

	for (int i=0; i<DATA_LENGTH; i++)
	{
		std::cout<< "value " << outputArray[i]<<", ";
		if(outputArray[i] != outputArray_gold[i]){
			std::cout<<"\n at data["<< i <<"] incorrect Test Failed \n";
			return -1;
		}
	}

	std::cout<<"\n"<<"Test Passed\n";
	
	gr.free(inputArray);
	gr.free(outputArray);
	gr.free(outputArray_gold);
  return 0;
}
