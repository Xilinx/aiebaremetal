// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include "xil_cache.h"
#include "aeg_baremetal_api.h"
AIEGraph gr("gradf");
#define DATA_LENGTH 128
int main(int argc, char ** argv)
{
	Xil_DCacheDisable();
	Xil_ICacheDisable();
	printf("adfr-3-22---\n");
	///*
	gr.init();
	std::cout << "after ini ---" << std::endl;
	int32_t* inputArray = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));
	int32_t* outputArray = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));
    int32_t* outputArray_gold = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));

    int param = 5;
	std::string port = "gradf.sq.in[1]";
	std::cout << "1 ---" << std::endl;
	gr.update(port, reinterpret_cast<char *>(&param), sizeof (int));
	std::cout << "2 ---" << std::endl;

	gr.run(1);
	std::cout << "3 ---" << std::endl;
	for (int i=0; i<DATA_LENGTH; i++){
		inputArray[i] = i+1;
		outputArray_gold[i] = inputArray[i]*inputArray[i] + param ;
	}
	std::cout << "4 ---" << std::endl;
	gr.gm2aie_nb("gradf.in",reinterpret_cast<char*>(&inputArray[0]), DATA_LENGTH*sizeof(int32_t));
	gr.aie2gm_nb("gradf.out",reinterpret_cast<char*>(&outputArray[0]), DATA_LENGTH*sizeof(int32_t));
	std::cout << "before gmio_wait ---" << std::endl;
	gr.gmio_wait("gradf.out"); //assuming data from gm1 are processed by the graph and output to gm2
	std::cout << "before gmio_wait ---end" << std::endl;
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
	//*/
  return 0;
}
