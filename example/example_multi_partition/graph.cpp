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

#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xtime_l.h"

#include <fstream>
#include <xaiengine.h>
#include "aiebaremetal.h"


#ifdef _RPU_
//set rpu support
uint8_t rpu = 1;
#else
uint8_t rpu = 0;
#endif

AbrGraph gr0("pr0_gradf", rpu);
AbrGraph gr1("pr1_gradf", rpu);

#define DATA_LENGTH 256

int main(int argc, char ** argv)
{
	printf("adfr--2---\n");

#ifdef _RPU_
 //RPU need to disable data cache, without cache diable app will stuck
	Xil_DCacheDisable();
	//Xil_ICacheDisable();
#endif

    Xil_DCacheDisable();
	gr0.init();
	gr1.init();
		
	int32_t* inputArray0 = (int32_t*)gr0.malloc(DATA_LENGTH*sizeof(int32_t));
	int32_t* outputArray0 = (int32_t*)gr0.malloc(DATA_LENGTH*sizeof(int32_t));

	int32_t* inputArray1 = (int32_t*)gr1.malloc(DATA_LENGTH*sizeof(int32_t));
	int32_t* outputArray1 = (int32_t*)gr1.malloc(DATA_LENGTH*sizeof(int32_t));

  	int32_t* outputArray_gold = (int32_t*)gr1.malloc(DATA_LENGTH*sizeof(int32_t));
   
	gr0.run(1);
	gr1.run(1);
	printf("************Testing both partition***************\n");
	for (int i=0; i<DATA_LENGTH; i++){
		inputArray0[i] = i+1;
		inputArray1[i] = i+1;
		outputArray_gold[i] = inputArray1[i]*inputArray1[i];
	}
	gr0.gm2aie("pr0_gradf.in",reinterpret_cast<char*>(&inputArray0[0]), DATA_LENGTH*sizeof(int32_t));
	
	gr0.aie2gm_nb("pr0_gradf.out",reinterpret_cast<char*>(&outputArray0[0]), DATA_LENGTH*sizeof(int32_t));
	gr0.gmio_wait("pr0_gradf.out"); //assuming data from gm1 are processed by the graph and output to gm2

	gr1.gm2aie_nb("pr1_gradf.in",reinterpret_cast<char*>(&inputArray1[0]), DATA_LENGTH*sizeof(int32_t));
	gr1.aie2gm_nb("pr1_gradf.out",reinterpret_cast<char*>(&outputArray1[0]), DATA_LENGTH*sizeof(int32_t));
	gr1.gmio_wait("pr1_gradf.out"); //assuming data from gm1 are processed by the graph and output to gm2
	
	gr0.wait();
	gr0.end();

	gr1.wait();
	gr1.end();
	

	for (int i=0; i<DATA_LENGTH; i++)
	{
		std::cout<< "value " << outputArray0[i]<<" "<<outputArray1[i]<<", ";
		if((outputArray0[i] != outputArray_gold[i]) || (outputArray1[i] != outputArray_gold[i])){
			std::cout<<"\n at data["<< i <<"] incorrect Test Failed \n";
			return -1;
		}
	}

	std::cout<<"\n"<<"Test Passed\n";
	
	gr0.free(inputArray0);
	gr0.free(outputArray0);
	
	gr1.free(inputArray1);
	gr1.free(outputArray1);
	gr1.free(outputArray_gold);

	

  return 0;
}
