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
//#ifndef __BAREMETAL_HW__
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
BaremetalGraphSim gr("gradf");
#else
//#include "platform.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xtime_l.h"
BaremetalGraph gr("gradf");
#endif

#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
void adf_run() {
	int32_t* inputArray = (int32_t*)GMIO::malloc(256*sizeof(int32_t));
	int32_t* outputArray = (int32_t*)GMIO::malloc(256*sizeof(int32_t));

	for (int i=0; i<256; i++)
		inputArray[i] = i+1;

	std::ofstream ofs;

	//gradf.init();
	gradf.run(1);
	for (int i=0; i<1; i++)
	{
		gradf.in.gm2aie_nb(&inputArray[i*32], 32*sizeof(int32_t));
		gradf.out.aie2gm_nb(&outputArray[i*32], 32*sizeof(int32_t));
	}
	gradf.out.wait(); //assuming data from gm1 are processed by the graph and output to gm2

	for (int i=0; i<256; i++)
	{
		std::cout<<outputArray[i]<<", ";
		//ofs<<outputArray[i]<<std::endl;
	}
  //load();
	//gr.init();
  std::cout << "before end" << std::endl;
	gradf.end();

	GMIO::free(inputArray);
	GMIO::free(outputArray);
}
#endif

int main(int argc, char ** argv)
{
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
	std::cout << "gradf.a  =" << gradf.a << std::endl;
	gradf.init();
#else
 //FIXME, should handle the sync in runtime api
	Xil_DCacheDisable();
	Xil_ICacheDisable();

	//get time
  XTime tStart, tEnd;

  //init_platform();

  XTime_GetTime(&tStart);
#endif

    int param = 5;
	std::string port = "gradf.sq.in[1]";
	
	int32_t* inputArray = (int32_t*)gr.malloc(256*sizeof(int32_t));
	int32_t* outputArray = (int32_t*)gr.malloc(256*sizeof(int32_t));
    int32_t* outputArray_gold = (int32_t*)gr.malloc(256*sizeof(int32_t));

	gr.init();
	gr.run(8);
	for (int i=0; i<256; i++){
		inputArray[i] = i+1;
		outputArray_gold[i] = inputArray[i]*inputArray[i] + param;
	}
	std::cout << "before:run---" << std::endl;

	for (int i=0; i<8; i++)
	{
		gr.update(port, reinterpret_cast<char *>(&param), sizeof (int));
		gr.gm2aie_nb("gradf.in",reinterpret_cast<char*>(&inputArray[i*32]), 32*sizeof(int32_t));
		gr.aie2gm_nb("gradf.out",reinterpret_cast<char*>(&outputArray[i*32]), 32*sizeof(int32_t));
	}
	gr.gmio_wait("gradf.out"); 
	gr.wait();
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
  XTime_GetTime(&tEnd);

  printf("Output took %llu clock cycles.\n", 2*(tEnd - tStart));
  printf("Output took %.2f us.\n",
           1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));
#endif
	std::cout << "6. run---" << std::endl;

	for (int i=0; i<256; i++)
	{
		std::cout<<outputArray[i]<<", ";
		if(outputArray[i] != outputArray_gold[i]){
			std::cout<<"\n"<<"Test Failed \n";
			return -1;
		}
	}

	std::cout<<"\n"<<"Test Passed\n";
	
  return 0;
}
