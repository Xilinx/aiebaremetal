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

#include <iostream>
//#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <sstream>
#include "xil_printf.h"
#include "xaiengine.h"
#include "xil_io.h"
#include "xil_cache.h"
#include "../../../log/klogr.h"
#include "../../../aiebaremetal.h"
#define uint_TYPE uint32_t
//#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
class ConfigureParser {
public:
	ConfigureParser() {};
	~ConfigureParser() {};
	void Load(std::string fname) {
		try {
			//boost::property_tree::read_json(fname, pt);
		} catch(...) {
			
		}
		std::cout << " hello" << std::endl;
		//std::cout << "hw_gen is " << pt.get<std::string>("aie_metadata.driver_config.hw_gen")<< std::endl;
	}
private:
	//boost::property_tree::ptree pt;
};

#define AIE_GEN 2
#if AIE_GEN == 1
#define HW_GEN XAIE_DEV_GEN_AIE

/* AIE Device parameters */
#define XAIE_BASE_ADDR	0x20000000000
#define XAIE_NUM_ROWS		9
#define XAIE_NUM_COLS		50
#define XAIE_COL_SHIFT		23
#define XAIE_ROW_SHIFT		18
#define XAIE_SHIM_ROW		0
#define XAIE_RES_TILE_ROW_START	0
#define XAIE_RES_TILE_NUM_ROWS	0
#define XAIE_AIE_TILE_ROW_START	1
#define XAIE_AIE_TILE_NUM_ROWS	8

#elif AIE_GEN == 2
#define HW_GEN XAIE_DEV_GEN_AIEML

#define XAIE_BASE_ADDR 0x20000000000
#define XAIE_COL_SHIFT 25
#define XAIE_ROW_SHIFT 20

#define XAIE_NUM_ROWS 11
#define XAIE_NUM_COLS 38
#define XAIE_SHIM_ROW 0
#define XAIE_RES_TILE_ROW_START 1
#define XAIE_RES_TILE_NUM_ROWS 2
#define XAIE_AIE_TILE_ROW_START 3
#define XAIE_AIE_TILE_NUM_ROWS 8
#endif

extern unsigned char _binary_D_start[];
extern unsigned char _binary_D_end[];
extern unsigned int _binary_D_size;

#define CORE_IP_MEM 0x4000
#define CORE_OP_MEM 0x8000

int test_routing(XAie_DevInst *DevInst)
{
	AieRC RC = XAIE_OK;
	XAie_RoutingInstance* routingInstance;
	XAie_LocType core_loc = XAie_TileLoc(4,3);

	XAie_CoreReset(DevInst, core_loc);
	XAie_CoreUnreset(DevInst, core_loc);
	XAie_CoreConfigureDone(DevInst, core_loc);

	XAie_LoadElfMem(DevInst, core_loc, _binary_D_start);

	routingInstance = XAie_InitRoutingHandler(DevInst);
	XAie_Route(routingInstance, NULL, XAie_TileLoc(2,0) /* Source*/,
				 core_loc /* destination*/	);
	XAie_Route(routingInstance, NULL, core_loc /* Source*/,
				 XAie_TileLoc(2,0) /* destination*/	);

	printf("Routing successful\n");
	u64 phy = 0, phy_out = 0;
	u32 mlen = 256;
	const u32 recv_len = 256;
	
	//Prepare DDR data
	XAie_MemInst *in = XAie_MemAllocate(DevInst, mlen * sizeof(u32), XAIE_MEM_CACHEABLE);
	phy = (u32)XAie_MemGetDevAddr(in);
	XAie_MemInst *out = XAie_MemAllocate(DevInst, mlen * sizeof(u32), XAIE_MEM_CACHEABLE);
	phy_out = (u32)XAie_MemGetDevAddr(out);

	u64 vmem =    phy;
	u64 vmem_out = phy_out;
	//XAie_MemSyncForCPU(in);
	//XAie_MemSyncForCPU(out);
	for(int i = 0; i < mlen; i++) {
		((u32*)vmem)[i] = i * 2;
		((u32*)vmem_out)[i] = 0;
	}

	printf("aiebaremetal alloc memory \n");

	//XAie_MemSyncForCPU(InputData);

	//XAie_MemSyncForDev(InputData);

	printf("Starting to Move data\n");
	// step 3: move data to destination tile
	XAie_MoveData(routingInstance,  XAie_TileLoc(2,0) /* Source*/,
		((void*)(phy)), mlen*sizeof(uint32_t),
		(void*)(CORE_IP_MEM), core_loc /* destination*/);					


	XAie_Run(routingInstance, 1);

	//wait until core done
	u8 allDone  = 0;
	uint32_t CoreStatus = 0;
	do {
		allDone = 1; // Assume all cores are done initially
		uint32_t coreStatCharWritten = 0;
		for (int i = 0; i < 1; i++) { // Iterate over the specified tiles
			RC = XAie_CoreWaitForDone(DevInst, core_loc, 0);

			if (RC != XAIE_OK) {
				allDone = 0;
			}
		}
		break;
	} while (!allDone);

	XAie_MoveData(routingInstance,  core_loc /* Source*/,
		(void*)(CORE_OP_MEM), mlen*sizeof(uint32_t),
		(void*)(phy_out), XAie_TileLoc(2,0) /* destination*/);					

	printf("\nFinished moving data back to DDR\n");
	// step 5 validate data
	for(int i = 0; i < recv_len; i++) {
		printf(" %d ", ((u32*)vmem_out)[i]);
		if (i % 16 == 0) printf("\n");
	}
	char* slog = rlog(core_loc.Row, core_loc.Col, (uint64_t)DevInst);
	printf("slog = %s\n", slog);

	printf("\nsuccess\n");
	return 0;
}

int test_aie_shim_good(XAie_DevInst *DevInst)
{
	printf("test_aie_shim_good\n");
	const uint32_t aie_data_addr = 0x400;
	u64 phy = 0, phy_out = 0, phy_size = 0;
	u32 mlen = 256;

	//Prepare DDR data
	XAie_MemInst *in = XAie_MemAllocate(DevInst, mlen * sizeof(u32), XAIE_MEM_CACHEABLE);
	phy = (u32)XAie_MemGetDevAddr(in);
	XAie_MemInst *out = XAie_MemAllocate(DevInst, mlen * sizeof(u32), XAIE_MEM_CACHEABLE);
	phy_out = (u32)XAie_MemGetDevAddr(out);

	u64 vmem =    phy;
	u64 vmem_out = phy_out;
	//XAie_MemSyncForCPU(in);
	//XAie_MemSyncForCPU(out);
	for(int i = 0; i < mlen; i++) {
		((u32*)vmem)[i] = i * 2;
		((u32*)vmem_out)[i] = 0;
	}

	printf("aiebaremetal alloc memory \n");
	//XAie_MemSyncForDev(in);
	//XAie_MemSyncForDev(out);

	//printf("phy = %x\n", phy);
	//return 0;
	//XAieMemSyncForCPU(in);
	//routing the tile
	int scol = 2;
	int RC;
	XAie_LocType shim = XAie_TileLoc(scol, 0);
	/*
	 * From DDR to SHIM DMA and Back Will go to MUX/DEMUX, there are limited
	 * channel (fast and slow) available for SHIM, DDR->SHIM 3, 7
	 * SHIM->DDR 2,3, in practice, SHIM->DDR 3 seems not work
	 * below we use 3 for DDR to SHMie_EnableShimDmaToAieStrmPort
	 * XAie_EnableShimDmaToAieStrmPort do the MUX setting
	 * The PORT/CHANNEL related Mux is the slave port 
	 * The internal connected port can be different when doing DMA
	 */
	RC = XAie_StrmConnCctEnable(DevInst, shim, SOUTH, 3,  NORTH, 0);
	RC |= XAie_EnableShimDmaToAieStrmPort(DevInst, shim, 3);

	XAie_LocType mem_t1 = XAie_TileLoc(scol, 1);
	RC |= XAie_StrmConnCctEnable(DevInst, mem_t1, SOUTH, 0, NORTH, 0);
	XAie_LocType mem_t2 = XAie_TileLoc(scol, 2);
	RC |= XAie_StrmConnCctEnable(DevInst, mem_t2, SOUTH, 0, NORTH, 0);
	XAie_LocType aie_t1 = XAie_TileLoc(scol, 3);
	// 1, 3 receive data
	RC |= XAie_StrmConnCctEnable(DevInst, aie_t1, SOUTH,0 , DMA, 0);
	// 1, 3 send BD data out
	RC |= XAie_StrmConnCctEnable(DevInst, aie_t1, DMA, 0, SOUTH, 0);

  XAie_LocType mem_down_t2 = XAie_TileLoc(scol, 2);
	RC |= XAie_StrmConnCctEnable(DevInst, mem_down_t2, NORTH, 0, SOUTH, 0);

	if (RC != XAIE_OK) {	printf("RC FAILED 0.1\n");	return -1;}
  XAie_LocType mem_down_t1 = XAie_TileLoc(scol, 1);
	RC |= XAie_StrmConnCctEnable(DevInst, mem_down_t1, NORTH, 0, SOUTH, 0);
	if (RC != XAIE_OK) {printf("RC FAILED mem_down_t1 cct enabl\n");return -1;}

	XAie_LocType shim_3 = XAie_TileLoc(scol, 0);
	//RC |= XAie_StrmConnCctEnable(DevInst,shim_3, NORTH, 0, SOUTH, 2);
	//RC |= XAie_EnableAieToShimDmaStrmPort(DevInst, shim_3, 2);

//
	RC |= XAie_StrmConnCctEnable(DevInst,shim_3, NORTH, 0, SOUTH, 2);
	RC |= XAie_EnableAieToShimDmaStrmPort(DevInst, shim_3, 2);
	if (RC != XAIE_OK) {printf("RC FAILED 0\n");return -1;}
	//prepare the send shim BD
	XAie_DmaDesc shim_desc;
	RC |= XAie_DmaDescInit(DevInst, &shim_desc, shim);
	//RC |= XAie_DmaSetAddrLen(&shim_desc, &MemInst_in, 256*sizeof(u32));
	//RC |= XAie_DmaSetAddrLen(&shim_desc, (u64)vmem, 256 * sizeof(int));
	printf("139, XAie_DmaSetAddrLen, phy=%x\n", (u32)phy);
	RC |= XAie_DmaSetAddrLen(&shim_desc, (u64)phy, 256 * sizeof(int));
	RC |= XAie_DmaEnableBd(&shim_desc);
	RC |= XAie_DmaWriteBd(DevInst, &shim_desc,shim, 0);
	//move data from DDR to SHIM to (1,3)
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, shim, 0, DMA_MM2S, 0);
	RC |= XAie_DmaChannelEnable(DevInst,shim, 0, DMA_MM2S);
	if (RC != XAIE_OK) {printf("RC FAILED\n");return -1;}
	//wait the send transfer, but not do receive 
	u32 shim_snd_count = 2, aie1_count = 2;
	int	round = 2;
	while(shim_snd_count && (round-- >= 0)) {
		RC |= XAie_DmaGetPendingBdCount(DevInst, shim, 0, DMA_MM2S, (u8*)&shim_snd_count);
		printf("shim send count = %d\n", shim_snd_count);
		//sleep(1);
	}

	XAie_DmaDesc DmaS2MM;
	RC = XAie_DmaDescInit(DevInst, &DmaS2MM, aie_t1);

	RC |= XAie_DmaSetAddrLen(&DmaS2MM, aie_data_addr, 256 * sizeof(uint32_t));
	RC |= XAie_DmaEnableBd(&DmaS2MM);
	if(RC != XAIE_OK) {
		printf("aie t1 s2mm Failed to setup software dma descriptors.\n");
		return -1;
	}

	RC |= XAie_DmaWriteBd(DevInst, &DmaS2MM, aie_t1, 9U);

	XAie_DmaChannelDesc MemTile_S2MM_ChDesc;
	XAie_DmaChannelDescInit(DevInst, &MemTile_S2MM_ChDesc, aie_t1);
	XAie_DmaWriteChannel(DevInst, &MemTile_S2MM_ChDesc, aie_t1, 0, DMA_S2MM);

	RC |= XAie_DmaChannelPushBdToQueue(DevInst, aie_t1, 0U, DMA_S2MM, 9U);
	RC |= XAie_DmaChannelEnable(DevInst, aie_t1, 0U, DMA_S2MM);

	aie1_count = 2;
	while(aie1_count || shim_snd_count) {
		RC |= XAie_DmaGetPendingBdCount(DevInst, shim, 0, DMA_MM2S,(u8*) &shim_snd_count);
		RC |= XAie_DmaGetPendingBdCount(DevInst, aie_t1, 0, DMA_S2MM, (u8*)&aie1_count);
		printf("shim Col %d after do receive aie count = %d shim_snd_count=%d\n", shim.Col, aie1_count,shim_snd_count);
		//sleep(1);
	}
	printf("bd creive done after \n");
	u32 data2[256];
	XAie_DataMemBlockRead(DevInst, aie_t1,  aie_data_addr, data2, 256*sizeof(uint32_t));
	for (int i = 0; i < 256; i++) {
		printf(" %d ", data2[i]);
		if (i % 16 == 0) printf("\n");
		if (((u32*)vmem)[i] != data2[i]) {
			printf("shim data transfer failed!\n");
			return 1;
		}
	}
	//aie tile DMA to stream
	XAie_DmaDesc DmaMM2S;
	RC |= XAie_DmaDescInit(DevInst, &DmaMM2S, aie_t1);
	RC |= XAie_DmaSetAddrLen(&DmaMM2S, aie_data_addr, 256 * sizeof(uint32_t));
	// bd wait a lock, we can use xsdb to write the lock to release this lock
	//RC |= XAie_DmaSetLock(&DmaMM2S, XAie_LockInit(0, -1), XAie_LockInit(1, 1));

	RC |= XAie_DmaEnableBd(&DmaMM2S);
	if(RC != XAIE_OK) {
		printf("aie t1 mm2s Failed to setup software dma descriptors.\n");
		return -1;
	}
	//prepare AIE MM to stream BD
	RC |= XAie_DmaWriteBd(DevInst, &DmaMM2S, aie_t1, 0U);
	XAie_DmaChannelDesc MemTile_MM2S_ChDesc;
	XAie_DmaChannelDescInit(DevInst, &MemTile_MM2S_ChDesc, aie_t1);
	XAie_DmaWriteChannel(DevInst, &MemTile_MM2S_ChDesc, aie_t1, 0, DMA_MM2S);

	RC |= XAie_DmaChannelPushBdToQueue(DevInst, aie_t1, 0U, DMA_MM2S, 0U);
	RC |= XAie_DmaChannelEnable(DevInst, aie_t1, 0U, DMA_MM2S);

	u32 aie1_snd_count = 1;
	round = 2;
	while(aie1_snd_count && round-- >=0 ) {
		RC |= XAie_DmaGetPendingBdCount(DevInst, aie_t1, 0, DMA_MM2S,(u8*) &aie1_snd_count);
		printf("before shim receive aie_snd_count=%d\n", aie1_snd_count);
		//sleep(1);
	}
	/*
	//prepare data from aie_t2
	RC |= XAie_StrmConnCctEnable(DevInst, aie_t2, DMA, 0, EAST, 0);
	XAie_DmaDesc DmaMM2S2;
	RC |= XAie_DmaDescInit(DevInst, &DmaMM2S2, aie_t2);
	RC |= XAie_DmaSetAddrLen(&DmaMM2S2, aie_data_addr, 256 * sizeof(uint32_t));
	// bd wait a lock, we can use xsdb to write the lock to release this lock
	//RC |= XAie_DmaSetLock(&DmaMM2S2, XAie_LockInit(0, -1), XAie_LockInit(1, 1));
	RC |= XAie_DmaEnableBd(&DmaMM2S2);
	if(RC != XAIE_OK) {
		printf("aie t1 mm2s Failed to setup software dma descriptors.\n");
		return -1;
	}
	//prepare AIE MM to stream BD
	RC |= XAie_DmaWriteBd(DevInst, &DmaMM2S2, aie_t2, 0U);
	XAie_DmaChannelDesc MemTile_MM2S_ChDesc2;
	XAie_DmaChannelDescInit(DevInst, &MemTile_MM2S_ChDesc2, aie_t2);
	XAie_DmaWriteChannel(DevInst, &MemTile_MM2S_ChDesc2, aie_t2, 0, DMA_MM2S);
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, aie_t2, 0U, DMA_MM2S, 0U);
	RC |= XAie_DmaChannelEnable(DevInst, aie_t2, 0U, DMA_MM2S);
*/
	//

	//prepare the receive shim BD
	XAie_LocType shim_out = shim_3;
	//const u32 recv_len = 128;
	const u32 recv_len = 256;
	XAie_DmaDesc shim_desc_out;
	RC |= XAie_DmaDescInit(DevInst, &shim_desc_out, shim_out);
#ifdef AIE_LINUX
	RC |= XAie_DmaSetAddrOffsetLen(&shim_desc_out, &MemInst_Out, 0X0, recv_len * sizeof(int));
#else //AIE_BAREMETAL
	RC |= XAie_DmaSetAddrLen(&shim_desc_out, (u64)phy_out, recv_len * sizeof(int));
#endif
	RC |= XAie_DmaEnableBd(&shim_desc_out);
	RC |= XAie_DmaWriteBd(DevInst, &shim_desc_out,shim_out, 0);
	//move data from DDR to SHIM to (1,3)
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, shim_out, 0, DMA_S2MM, 0);
	RC |= XAie_DmaChannelEnable(DevInst,shim_out, 0, DMA_S2MM);

	//return 1;
	u32 shim_count = 1;
	while(shim_count) {
		RC |= XAie_DmaGetPendingBdCount(DevInst, shim_out, 0, DMA_S2MM, (u8*)&shim_count);
		printf("3 - after shim receive shim_recv_count=%d len = %d col=%d row = %d\n",
				shim_count, recv_len, shim_out.Col, shim_out.Row);
		//sleep(1);
	}
#ifdef AIE_BAREMETAL
	//XAie_MemSyncForCPU(in);
	//XAie_MemSyncForCPU(out);
#endif

	for(int i = 0; i < recv_len; i++) {
		printf(" %d ", ((u32*)vmem_out)[i]);
		if (i % 16 == 0) printf("\n");
	}
	//while(1) {sleep(1); printf("wait ---\n");};
	//free memory after use it
#if DEVICE != 0
	//XAie_MemFree(in);
	sys_mem_free(phy);
#endif
	
	return 0;
}
void test(XAie_DevInst DevInst) {
	///*
		XAie_LocType loc = XAie_TileLoc(12, 3);
		const uint32_t col_0_row_3 = (loc.Col<<XAIE_COL_SHIFT) + (loc.Row << XAIE_ROW_SHIFT);
//#ifdef USE_AIE2 AIE1
		const uint32_t core_control_addr =col_0_row_3 + 0x00032000;
		const uint32_t core_status_addr = col_0_row_3 + 0x00032004;
//#else
//		const uint32_t core_control_addr =col_0_row_3 + 0x00038000;
//		const uint32_t core_status_addr = col_0_row_3 + 0x00038004;
//#endif
		std::cout << "aie_driver_test 1 - 1" << std::endl;
		uint32_t d0 = 0,d1=0, d2=0;
		uint32_t bloop = 1;
		//while(bloop) {sleep(1);};
		XAie_Read32(&DevInst,core_status_addr, &d1);
		std::cout << "aie_driver_test 2" << std::endl;
		XAie_Read32(&DevInst,core_control_addr, &d0);
		XAie_Write32(&DevInst,core_control_addr, 2);
		std::cout << "aie_driver_test 3" << std::endl;
		XAie_Read32(&DevInst,core_status_addr, &d2);
		std::cout << " 1read status d0 = " << d0 << " d1 = " << d1 << " d2 = " << d2 << std::endl;
//*/
}

void test_runtime() {
	BaremetalGraph gr("gradf");

	uint32_t* inputArray = (uint32_t*)gr.malloc(256*sizeof(uint32_t));
	uint32_t* outputArray = (uint32_t*)gr.malloc(256*sizeof(uint32_t));

	gr.init();
	gr.run(8);

	for (int i=0; i<256; i++)
		inputArray[i] = i+1;
	std::cout << "before:run---" << std::endl;
	for (int i=0; i<8; i++)
	{
		gr.gm2aie_nb("gradf.in",reinterpret_cast<char*>(&inputArray[i*32]), 32*sizeof(uint32_t));
		gr.aie2gm_nb("gradf.out",reinterpret_cast<char*>(&outputArray[i*32]), 32*sizeof(uint32_t));
	}
	gr.gmio_wait("gradf.out"); //assuming data from gm1 are processed by the graph and output to gm2
	std::cout << "run---" << std::endl;
	gr.wait();
	for (int i=0; i<256; i++)
	{
		std::cout<<outputArray[i]<<", ";
	}
	
	/*// this can trigger the UC/UC print logic
	while(1) {
		uint32_t v= ess_Read32(0x18338004 + 0x20000000000);
		//std::cout << "ess_Read" << std::endl;
	  usleep(1);
	};//*/
}

int main(int argc, char* argv[]) {
	Xil_DCacheDisable();
	Xil_ICacheDisable();
	///*
	//ConfigureParser cp;
	//cp.Load("/scratch/staff/huaj/baremetal/aiebaremetal/testkernel/Work/ps/c_rts/aie_control_config.json");
	XAie_SetupConfig(ConfigPtr, HW_GEN, XAIE_BASE_ADDR,
			XAIE_COL_SHIFT, XAIE_ROW_SHIFT,
			XAIE_NUM_COLS, XAIE_NUM_ROWS, XAIE_SHIM_ROW,
			XAIE_RES_TILE_ROW_START, XAIE_RES_TILE_NUM_ROWS,
			XAIE_AIE_TILE_ROW_START, XAIE_AIE_TILE_NUM_ROWS);

	///*

	XAie_InstDeclare(DevInst, &ConfigPtr);

	AieRC RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
	if(RC != XAIE_OK) {
		printf("Driver initialization failed.\n");
		return -1;
	}
	printf("1\n");

	XAie_SetIOBackend(&DevInst, XAIE_IO_BACKEND_BAREMETAL);
	printf("1.0\n");

#if AIE_GEN >= 2
	if(DevInst.Backend->Type == XAIE_IO_BACKEND_BAREMETAL) {
		RC = XAie_UpdateNpiAddr(&DevInst, 0xF6D10000);
		if(RC != XAIE_OK) {
			printf("Failed to update NPI address\n");
			return -1;
		}
	}
	//fix in aie2 the shim dma not work issue
	RC = XAie_PartitionInitialize(&DevInst, NULL);
#else
	//fix in aie1 shimd dma not work issue
	XAie_PmRequestTiles(&DevInst, NULL, 0); 
#endif
	printf("2\n");
	//test(DevInst);
  //test_aie_shim_good(&DevInst);
  test_routing(&DevInst);
	//*/
	//test_runtime();
	return 1;
}
