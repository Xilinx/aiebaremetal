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

#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
myGraph gradf;
#else
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xtime_l.h"
#endif

#include <fstream>
#include <xaiengine.h>
#include "aiebaremetal.h"
#include "config.h"

#ifdef _RPU_
//set rpu support
uint8_t rpu = 1;
#else
uint8_t rpu = 0;
#endif

AbrGraph gr("gradf", rpu);

int main(int argc, char ** argv)
{
	printf("adfr--2---\n");
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
 gradf.init();
#else
#ifdef _RPU_
 //RPU need to disable data cache, without cache diable app will stuck
	Xil_DCacheDisable();
	//Xil_ICacheDisable();
#endif
#endif

	gr.getaiedevinst();
	int32_t* inputArray = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));
	int32_t* outputArray = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));
  	int32_t* outputArray_gold = (int32_t*)gr.malloc(DATA_LENGTH*sizeof(int32_t));
	int handle0 = gr.start_profiling("gradf.in", io_total_stream_running_to_idle_cycles);
	printf("handle0 = 0x%p\n",handle0);
	gr.init();
	printf("1");
	gr.run(1);
	printf("2");
	for (int i=0; i<DATA_LENGTH; i++){
		inputArray[i] = i+1;
		outputArray_gold[i] = inputArray[i]*inputArray[i];
	}

#if not ((defined(__AIESIM__) || defined(__ADF_FRONTEND__)))
	XTime tStart, tEnd;
  	XTime_GetTime(&tStart);
#endif

	gr.gm2aie_nb("gradf.in",reinterpret_cast<char*>(&inputArray[0]), DATA_LENGTH*sizeof(int32_t));
	gr.aie2gm_nb("gradf.out",reinterpret_cast<char*>(&outputArray[0]), DATA_LENGTH*sizeof(int32_t));
	//std::cout << "before gmio_wait ---" << std::endl;
	gr.gmio_wait("gradf.out"); //assuming data from gm1 are processed by the graph and output to gm2
	//printf("3\n");
	gr.wait();
	gr.end();

#if not ((defined(__AIESIM__) || defined(__ADF_FRONTEND__)))
	XTime_GetTime(&tEnd);
	printf("Output took %llu clock cycles.\n", 2*(tEnd - tStart));
  	printf("Output took %.2f us.\n",
           1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));
#endif

	if (handle0 != -1)
    {
        long long cycle_count0 = gr.read_profiling(handle0);
        gr.stop_profiling(handle0);
		printf("cycle_count0 = %lld\n", cycle_count0);
    }

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
