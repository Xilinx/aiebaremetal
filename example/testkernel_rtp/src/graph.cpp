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

/*
#define HW_GEN XAIE_DEV_GEN_AIE2PS
#define XAIE_COL_SHIFT 25
#define XAIE_ROW_SHIFT 20

#define XAIE_BASE_ADDR 0x20000000000
#define XAIE_NUM_ROWS 7
#define XAIE_NUM_COLS 36
#define XAIE_SHIM_ROW 0
#define XAIE_MEM_TILE_ROW_START 1
#define XAIE_MEM_TILE_NUM_ROWS 2
#define XAIE_AIE_TILE_ROW_START 3
#define XAIE_AIE_TILE_NUM_ROWS 4
void test() {

	std::cout << "init 2" << std::endl;
	XAie_SetupConfig(ConfigPtr, HW_GEN, XAIE_BASE_ADDR,
			XAIE_COL_SHIFT, XAIE_ROW_SHIFT,
			XAIE_NUM_COLS, XAIE_NUM_ROWS, XAIE_SHIM_ROW,
			XAIE_MEM_TILE_ROW_START, XAIE_MEM_TILE_NUM_ROWS,
			XAIE_AIE_TILE_ROW_START, XAIE_AIE_TILE_NUM_ROWS);

	XAie_InstDeclare(DevInst, &ConfigPtr);

	AieRC RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
	if(RC != XAIE_OK) {
		printf("Driver initialization failed.\n");
		return;
	}
	XAie_LocType loc = XAie_TileLoc(0, 3);
	XAie_SetIOBackend(&DevInst, XAIE_IO_BACKEND_SIM);
	const uint32_t_t col_0_row_3 = (loc.Col<<XAIE_COL_SHIFT) + (loc.Row << XAIE_ROW_SHIFT);
	const uint32_t_t core_control_addr =col_0_row_3 + 0x32000;// 0x00038000;
	const uint32_t_t core_status_addr = col_0_row_3 + 0x32004;//0x00038004;
	uint32_t_t dcontrol = 0,d1=0, d2=0;
	uint32_t_t bloop = 1;
	//while(bloop) {sleep(1);};
	XAie_LocType locs[1] = {loc, };
	//XAie_PmRequestTiles(&DevInst, locs, 1);
	XAie_PmRequestTiles(&DevInst, NULL, 0);

	XAie_Write32(&DevInst,core_control_addr, 0);
	XAie_Read32(&DevInst,core_status_addr, &d1);
	XAie_Write32(&DevInst,core_control_addr, 2);
	XAie_Read32(&DevInst,core_control_addr, &dcontrol);
	XAie_Read32(&DevInst,core_status_addr, &d2);
	printf("read statusou 3 dcontrol=%x d1 = %x d2 = %x\n", dcontrol, d1, d2);
}
*/
/*
void load() {
	int col = 12;
	int row = 0;
	uint64_t addr = 0x20000000000 | col << 25 | row << 20 | 0x00080000; // beginning of program memory for uC at col
	uint32_t_t val;
    for (int k = 0; k < app_bin_len; k += 4)
    {
        val = app_bin[k] | app_bin[k+1] << 8 | app_bin[k+2] << 16 | app_bin[k+3] << 24;
        ess_Write32(addr, val);
        addr += 4;
        if ( k % 500 == 0 )
            std::cout << ".";
    }
    std::cout << "\nFinished loading program to program memory of uC at col " << col << ".\n";
 		std::cout << "\nFinished loading program to program memory of uC at col " << col << ".\n";
		std::cout << "Waking up uC at col " << col << ".\n";
 		addr = 0x20000000000 | col << 25 | row << 20 | 0x000C0004;
 		ess_Write32(addr, 0x1);
		int num = 10;
		while(num-- >=0 ) {
			sleep(1);
			std::cout << "wait after load uc" <<std::endl;
		}
}
*/
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
	std::cout << "gradf.a  =" << gradf.a << std::endl;
	gradf.init();
	//adf_run();
	//return 0;
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
	gr.update(port, reinterpret_cast<char *>(&param), sizeof (int));
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
		gr.gm2aie_nb("gradf.in",reinterpret_cast<char*>(&inputArray[i*32]), 32*sizeof(int32_t));
		gr.aie2gm_nb("gradf.out",reinterpret_cast<char*>(&outputArray[i*32]), 32*sizeof(int32_t));
	}
	gr.gmio_wait("gradf.out"); //assuming data from gm1 are processed by the graph and output to gm2
	//std::cout << "4. run---" << std::endl;
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
	
	/*// this can trigger the UC/UC print logic
	while(1) {
		uint32_t_t v= ess_Read32(0x18338004 + 0x20000000000);
		//std::cout << "ess_Read" << std::endl;
	  usleep(1);
	};//*/
  return 0;
}
