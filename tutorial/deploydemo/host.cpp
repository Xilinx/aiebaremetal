// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <fstream>
#include "aeg_baremetal_api.h"
AIEGraph gr("gradf");
#define DATA_LENGTH 128
int main(int argc, char ** argv)
{
	printf("adfr-----\n");
	gr.init();

	int32_t* inputArray = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));
	int32_t* outputArray = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));
    int32_t* outputArray_gold = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));

    int param = 5;
	std::string port = "gradf.sq.in[1]";
	gr.update(port, reinterpret_cast<char *>(&param), sizeof (int));

	gr.run(1);
	for (int i=0; i<DATA_LENGTH; i++){
		inputArray[i] = i+1;
		outputArray_gold[i] = inputArray[i]*inputArray[i] + param ;
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
