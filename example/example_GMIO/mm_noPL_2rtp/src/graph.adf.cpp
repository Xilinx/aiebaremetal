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
#include "graph.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xtime_l.h"

myGraph gradf;
void adf_run() {
	int32_t* inputArray = (int32_t*)GMIO::malloc(256*sizeof(int32_t));
	int32_t* outputArray = (int32_t*)GMIO::malloc(256*sizeof(int32_t));
  printf("1\n");
	for (int i=0; i<256; i++)
		inputArray[i] = i+1;

	std::ofstream ofs;

  XTime tStart, tEnd;
  XTime_GetTime(&tStart);
	gradf.init();
	gradf.run(1);
	for (int i=0; i<1; i++)
	{
		gradf.in.gm2aie_nb(&inputArray[i*32], 32*sizeof(int32_t));
		gradf.out.aie2gm_nb(&outputArray[i*32], 32*sizeof(int32_t));
	}
	gradf.out.wait(); //assuming data from gm1 are processed by the graph and output to gm2

  XTime_GetTime(&tEnd);

  printf("Output took %llu clock cycles.\n", 2*(tEnd - tStart));
  printf("Output took %.2f us.\n",
           1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND/1000000));

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
//#endif
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
	printf("test\n");
	adf_run();
	return 0;
}
