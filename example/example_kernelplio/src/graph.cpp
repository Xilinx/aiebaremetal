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
#include "log/klogr.h"
#include "aiebaremetal.h"
//#ifndef __BAREMETAL_HW__
#include "common.h"
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf2;
BaremetalGraphSim gr("gradf2");
//extern int g_start_plio;
#else
//#include "platform.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "xtime_l.h"
BaremetalGraph gr("gradf2");
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
	gradf2.run(1);
	/*
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
	*/
  //load();
	//gr.init();
  std::cout << "before end" << std::endl;
	gradf2.end();

	GMIO::free(inputArray);
	GMIO::free(outputArray);
}
#endif
//extern adf::return_code gradf_init(const std::string& work_path);
//extern bool gradf_load_elf(const std::string& work_path, std::vector<std::string>& elfInfoPath);

//            AIE (vck190) / AIE-ML (vek280)
// MM2S_BASE: 0xA4050000   / 0xA4010000
// S2MM_BASE: 0xA4060000   / 0XA4020000
#define MM2S_BASE   XPAR_XMM2S_0_S_AXI_CONTROL_BASEADDR
#define S2MM_BASE   XPAR_XS2MM_0_S_AXI_CONTROL_BASEADDR

#define CTRL_OFFSET 0x0
#define SIZE_OFFSET 0x1C
#define MEM_OFFSET  0x10
void start_pl(uint32_t* in, uint32_t* out, int len) {
#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)

	  int i;
    int errCount = 0;
    uint64_t memAddr = (uint64_t)in;
    uint64_t mem_outAddr = (uint64_t)out;

    printf("Starting test w/ cu\n");
    printf("Starting mm2s & s2mm\n");
    Xil_Out32(MM2S_BASE + MEM_OFFSET, (uint32_t) memAddr);
    Xil_Out32(MM2S_BASE + MEM_OFFSET + 4, 0);
    Xil_Out32(S2MM_BASE + MEM_OFFSET, (uint32_t) mem_outAddr);
    Xil_Out32(S2MM_BASE + MEM_OFFSET + 4, 0);
    Xil_Out32(MM2S_BASE + SIZE_OFFSET, len);
    Xil_Out32(S2MM_BASE + SIZE_OFFSET, len);
    Xil_Out32(MM2S_BASE + CTRL_OFFSET, 1);
    Xil_Out32(S2MM_BASE + CTRL_OFFSET, 1);
#endif
}

void wait_s2mm() {
#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)
    while(1) {
        uint32_t v = Xil_In32(S2MM_BASE + CTRL_OFFSET);
        if(v & 6) {
            break;
        }
				printf("wait_s2mm\n");
    }
#endif
}

int main(int argc, char ** argv)
{
	//std::string str_work="/scratch/staff/huaj/baremetal/aiebaremetal/testkernel/Work";
	//std::vector<std::string> elfInfoPath;
	//gradf_load_elf(str_work, elfInfoPath);
	//gradf_init(str_work);
  //Fix me , keep this adf init to call the elf load and graph init(stream config)
	//should add the cdo load logic in baremetal init to do the work
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
	std::cout << "gradf2.a  =" << gradf2.a << std::endl;
	gradf2.init();
	//adf_run();
	//return 0;
#else
 //FIXME, should handle the sync in runtime api
	Xil_DCacheDisable();
	Xil_ICacheDisable();
	//get time
  XTime tStart, tEnd;
  //init_platform();
#endif
  uint32_t indata[SQUARE_BUF_LEN], outdata[SQUARE_BUF_LEN];
  for (int i = 0; i < SQUARE_BUF_LEN; i++) {
		indata[i] = i;
    outdata[i] = 0;
	}
	int handle0 = gr.start_profiling("gradf2.pin", io_total_stream_running_to_idle_cycles);
  start_pl(indata, outdata, SQUARE_BUF_LEN);

	//g_start_plio = 1;
	printf("main-1216----------****** handle0 = %d \n", handle0);
	//gr.gdbpoint();
	gr.init();
	printf("main------before run-----******\n");
	gr.run(1);

  wait_s2mm();
	printf("after run-----------******\n");
	gr.wait();
	//sleep(1);
	char * log = gr.dumpcorelog(3, 2);
	printf("kernel log is %s\n", log);

#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)
	for (int i = 0; i < SQUARE_BUF_LEN; i++) {
		printf("outdata[%d] is %d\n", i,  outdata[i]);
	}
	std::cout<<"\n"<<"Test Passed 4\n";
#endif

	/*// this can trigger the UC/UC print logic
	while(1) {
		uint32_t_t v= ess_Read32(0x18338004 + 0x20000000000);
		//std::cout << "ess_Read" << std::endl;
	  usleep(1);
	};//*/
	gr.end();
	if (handle0 != -1)
    {
        long long cycle_count0 = gr.read_profiling(handle0);
        gr.stop_profiling(handle0);
		printf("cycle_count0 = %lld\n", cycle_count0);
    }
  return 0;
}
