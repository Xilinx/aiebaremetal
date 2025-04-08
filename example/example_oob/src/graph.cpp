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
#include <vector>
#include <sstream>
#include <xaiengine.h>
#include "aiebaremetal.h"

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
#include "kernels/config.h"
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
void adf_run() {
	std::cout << " adf_run alloc " << NUM_HW_ROWS * MAT_A_CHUNK_SIZE/1024 << " kB" << std::endl;
	std::vector<int32_t *> inputArray(NUM_HW_ROWS, NULL), inputArrayB(NUM_HW_ROWS, NULL),  outputArray(NUM_HW_ROWS, NULL);
	int32_t* mem_in = (int32_t*)GMIO::malloc(NUM_HW_ROWS * MAT_A_CHUNK_SIZE);
	int32_t* mem_in_b = (int32_t*)GMIO::malloc(NUM_ELMNTS * sizeof(int32_t));

	int32_t* mem_out = (int32_t*)GMIO::malloc(NUM_HW_ROWS * MAT_A_CHUNK_SIZE);
	std::cout << "after alloc size = " << NUM_HW_ROWS * MAT_A_CHUNK_SIZE/1024 << "kb" <<std::endl;
	for (int i=0; i<NUM_HW_ROWS; i++) {
		std::cout<< " 1 i = "<< i << std::endl;
		inputArray[i] = mem_in + i * MAT_A_CHUNK;
		inputArrayB[i] = mem_in_b + i * MAT_A_CHUNK;
		outputArray[i] = mem_out + i * MAT_A_CHUNK;
		//std::cout<< " 1i = " << i << std::endl;
		for(int j = 0; j <  MAT_A_CHUNK; j++) {
			inputArray[i][j] = i + 1;
			inputArrayB[i][j] = i + 2;
			outputArray[i][j] = i + 1;
			//std::cout<< " 2. j=" << j << std::endl;
		}
	}

	std::ofstream ofs;

	//gradf.init();
	std::cout << "before run" << std::endl;
	gradf.run(1);
	std::cout << "after run" << std::endl;
	int rown = 1;
	for (int i=0; i<rown; i++)
	{
		gradf.in[i].gm2aie_nb(inputArray[i], MAT_A_CHUNK_SIZE);
		gradf.in[i].gm2aie_nb(inputArrayB[i], NUM_ELMNTS * sizeof(int32_t));
	}

	for (int i =0;i < rown; i++) {
		gradf.out[i].aie2gm_nb(outputArray[i], MAT_A_CHUNK_SIZE);
		gradf.out[i].wait(); //assuming data from gm1 are processed by the graph and output to gm2
	}

	for (int i=0; i<256; i++)
	{
		//std::cout<<outputArray[i]<<", ";
		//ofs<<outputArray[i]<<std::endl;
	}
  //load();
	//gr.init();
  std::cout << "before end" << std::endl;
	gradf.end();
	for (auto x:inputArray) {
		GMIO::free(x);
	}
	for (auto x:inputArrayB) {
		GMIO::free(x);
	}
	for (auto x:outputArray) {
		GMIO::free(x);
	}
}
#endif
//extern adf::return_code gradf_init(const std::string& work_path);
//extern bool gradf_load_elf(const std::string& work_path, std::vector<std::string>& elfInfoPath);
int main(int argc, char ** argv)
{
	//std::string str_work="/scratch/staff/huaj/baremetal/aiebaremetal/testkernel/Work";
	//std::vector<std::string> elfInfoPath;
	//gradf_load_elf(str_work, elfInfoPath);
	//gradf_init(str_work);
  //Fix me , keep this adf init to call the elf load and graph init(stream config)
	//should add the cdo load logic in baremetal init to do the work
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
	std::cout << "gradf.a  =" << gradf.a << " before init" << std::endl;
	gradf.init();
	std::cout << "after init" << gradf.a << std::endl;
	//adf_run();
	//std::cout << "after run "<< std::endl;
	//return 0;
#else
 //FIXME, should handle the sync in runtime api
	//Xil_DCacheDisable();
	//Xil_ICacheDisable();
	//get time
  XTime tStart, tEnd;
  //init_platform();
#endif
	const int rown =NUM_HW_ROWS;
 	std::cout << "1 start--08 07" << std::endl;
	std::vector<int32_t *> inputArray(NUM_HW_ROWS, NULL), inputArrayB(NUM_HW_ROWS, NULL),  outputArray(NUM_HW_ROWS, NULL);
	int32_t* mem_in = (int32_t*)gr.malloc(NUM_HW_ROWS * MAT_A_CHUNK_SIZE);
	int32_t* mem_in_b = (int32_t*)gr.malloc(NUM_ELMNTS * sizeof(int32_t));
	int32_t* mem_out = (int32_t*)gr.malloc(NUM_HW_ROWS * MAT_A_CHUNK_SIZE);
	for (int i=0; i<rown; i++) {
		std::cout<< " 1 i = "<< i << std::endl;
		inputArray[i] = mem_in + i * MAT_A_CHUNK;
		inputArrayB[i] = mem_in_b + i * MAT_A_CHUNK;
		outputArray[i] = mem_out + i * MAT_A_CHUNK;
		//std::cout<< " 1i = " << i << std::endl;
		for(int j = 0; j <  MAT_A_CHUNK; j++) {
			inputArray[i][j] = ((j)/608)+1 + i;
			inputArrayB[i][j] = 1;
			outputArray[i][j] = 2;
			//std::cout<< " 2. j=" << j << std::endl;
		}
#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)
		// to make sure we can read correct output data from ddr, we need to first flush the output data into DDR
		// WITHOUT doing this, we can not read the correct data , even if we called the invalid funciton
		// this should be the xil api bug
		//__asm__ __volatile__("dmb ish" : : : "memory");
		//Xil_DCacheFlushRange((INTPTR)outputArray[i], MAT_A_CHUNK * sizeof(uint32_t));
#endif
	}
  	std::cout << "2" << std::endl;

	gr.init();
	gr.run(1);

#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
  XTime_GetTime(&tStart);
#endif
  //	std::cout << "3" << std::endl;
 	std::ostringstream in_ostr,out_ostr;
	for (int i=0; i<rown; i++)
	{
  		std::ostringstream in_ostr;
		in_ostr << "gradf.in[" << i << "]";
		gr.gm2aie_nb((char*)in_ostr.str().c_str(), reinterpret_cast<char*>(inputArray[i]), MAT_A_CHUNK_SIZE);
		gr.gm2aie_nb((char*)in_ostr.str().c_str(),reinterpret_cast<char*>(inputArrayB[i]), NUM_ELMNTS * sizeof(int32_t));
	}
	std::vector<std::string> out_strs;
	for (int i =0;i < rown; i++) {
		//std::cout << "waiting on row " << i << std::endl;
  	std::ostringstream out_ostr;
		out_ostr << "gradf.out[" << i << "]";
		out_strs.push_back(out_ostr.str());
		std::cout << "MAT_A_CHUNK_SIZE " << MAT_A_CHUNK_SIZE << std::endl;
		gr.aie2gm_nb((char*)out_ostr.str().c_str(), reinterpret_cast<char*>(outputArray[i]), MAT_A_CHUNK_SIZE);
		/*
		for (int k = 0; k < NUM_HW_COLS; k++) {
			//printf("dump core col %d of(total col %d), row 1\n", k, NUM_HW_COLS);
			//gr.dumpcorelog(1, k);
		}*/
		//printf("dump log \n");
	}

	for (int i =0;i < rown; i++) {
		gr.gmio_wait((char*)out_strs[i].c_str()); //assuming data from gm1 are processed by the graph and output to gm2
	}
	//std::cout << "4. run2---11:10" << std::endl;
#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
  XTime_GetTime(&tEnd);

  printf("Output took %llu clock cycles.\n", 2*(tEnd - tStart));
  printf("Output took %.2f ms.\n",
           1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000));
#endif
	std::cout << "6. run--3-" << std::endl;
	gr.end();
	for (int j = 0; j< rown; j++) {
	    for (int i = 0; i < 16; i++) {
		printf("outputArray[%d][%d] value is %d\n ",j, i, outputArray[j][i]);
	    }
	}
	std::cout<<"\n"<<"OOB Test Passed\n";
	
	/*// this can trigger the UC/UC print logic
	while(1) {
		uint32_t_t v= ess_Read32(0x18338004 + 0x20000000000);
		//std::cout << "ess_Read" << std::endl;
	  usleep(1);
	};//*/
  return 0;
}
