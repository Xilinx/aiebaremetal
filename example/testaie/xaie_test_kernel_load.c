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

#include "xaie_test_all.h"
#include "log/klogr.h"
#include "xaiemem.h"
#include <iostream>
extern "C" {
/************************** Constant Definitions *****************************/
int test_aie_kernel_load_route(XAie_DevInst *DevInst) {
	gdb_point(0);
	uint32_t CORE_IP_MEM =  0x4000;
        uint32_t CORE_OP_MEM = 0x8000;
	XAieMem mem(DevInst);
	if (DevInst->DevProp.DevGen == XAIE_DEV_GEN_AIE) {
		//printf("test_aie_kernel_load currently only support aie1 \n");
        	CORE_OP_MEM = 0x5000;
		//return 0;
	}
	XAie_LocType core_loc = XAie_TileLoc(4,3);
	AieRC RC = XAIE_OK;
	XAie_RoutingInstance* routingInstance;

	XAie_CoreReset(DevInst, core_loc);
	XAie_CoreUnreset(DevInst, core_loc);
	//configure the core disable event, the kernel 'done' instruction can/will trigger 67U event in core moduel
  //driver need to set this event to make sure event will trigger core disable 
  //if not do this waitcore done will never success	
	XAie_CoreConfigureDone(DevInst, core_loc);

	//test code
	//XAie_Write32(DevInst,(core_loc.Row<<18) + (core_loc.Col<<23) + 0x7000, 0x1234);
	rlog(core_loc.Row, core_loc.Col, (uint64_t)DevInst);
	const char* ldpath = "../data/aiekernel";
	if(XAie_LoadElf(DevInst, core_loc, ldpath, XAIE_ENABLE)!=XAIE_OK)
	{
		std::cout << "kernel load failed" << std::endl;
		return 0;
	}

	routingInstance = XAie_InitRoutingHandler(DevInst);
	XAie_Route(routingInstance, NULL, XAie_TileLoc(2,0) /* Source*/,core_loc/* destination*/);
	XAie_Route(routingInstance, NULL, core_loc /* Source*/,XAie_TileLoc(2,0) /* destination*/);
  printf("Routing successful\n");
        /*
	 * prepare the data for data movement
	 */
	u32 mlen = 256;
        char* vmem_in =(char*) mem.malloc(mlen);
        char* vmem_out = (char*)mem.malloc(mlen);
	u64 phy, phy_out;

	mem.get_dev_addr(vmem_in, &phy);
	mem.get_dev_addr(vmem_out, &phy_out);
	for (int i = 0 ;i < mlen; i++) {
		vmem_in[i] = i;
		vmem_out[i] = 0;
	}
	mem.sync_for_dev((void *)vmem_in, mlen);
	const u32 recv_len = 256;
	std::cout << "test_aie_kernel_load" << std::endl;
	//XAie_CoreEnable(DevInst,  core_loc);
  XAie_MoveData(routingInstance,  XAie_TileLoc(2,0) /* Source*/,
		((void*)(phy)), mlen*sizeof(uint32_t),
	  (void*)(CORE_IP_MEM), core_loc /* destination*/);

  XAie_Run(routingInstance, 1);
	XAie_CoreWaitForDone(DevInst, core_loc, 1000000);
	char* slog = rlog(core_loc.Row, core_loc.Col, (uint64_t)DevInst);
	/*
	while (!slog || slog[0] == 0) {
		printf("rlog read nothing\n");
		slog = rlog(core_loc.Row, core_loc.Col, (uint64_t)DevInst);
	}
	*/
	printf("slog =%s\n",slog);
	/*while(1) {
		const uint32_t col_0_row_3 = (1<<23) + (1 << 18);
		const uint32_t core_control_addr =col_0_row_3 + 0x32000;// 0x00038000;
		const uint32_t core_status_addr = col_0_row_3 + 0x32004;//0x00038004;
		const uint32_t core_enable_event = col_0_row_3 + 0x32008;//0x00038004;
		uint32_t dc, ds, denable;
		XAie_Read32(DevInst,core_status_addr, &dc);
		XAie_Read32(DevInst,core_control_addr, &ds);
		// 14:8 is the disable event number
		XAie_Read32(DevInst,core_enable_event, &denable);
		std::cout << "core_status_addr:" << dc << "control:" << ds << "enable e" << &denable<<std::endl;
		if(denable << 15) break;
		usleep(100);
	}
	*/
	mem.free((void *)vmem_in);
	mem.free((void *)vmem_out);

	std::cout << "core done ***" << std::endl;
	return 0;
}
int test_aie_kernel_load_simple(XAie_DevInst *DevInst) {
	gdb_point(0);
	if (DevInst->DevProp.DevGen != XAIE_DEV_GEN_AIE) {
		printf("test_aie_kernel_load currently only support aie1 \n");
		return 0;
	}
	XAie_CoreReset(DevInst, XAie_TileLoc(1,1));
	XAie_CoreUnreset(DevInst, XAie_TileLoc(1,1));
	//configure the core disable event, the kernel 'done' instruction can/will trigger 67U event in core moduel
  //driver need to set this event to make sure event will trigger core disable 
  //if not do this waitcore done will never success	
	XAie_CoreConfigureDone(DevInst, XAie_TileLoc(1, 1));
	const char* ldpath = "../data/aiekernel";
	if(XAie_LoadElf(DevInst, XAie_TileLoc(1, 1), ldpath, XAIE_ENABLE)!=XAIE_OK)
	{
		std::cout << "kernel load failed" << std::endl;
		return 0;
	}
	std::cout << "test_aie_kernel_load" << std::endl;
	XAie_CoreEnable(DevInst,  XAie_TileLoc(1, 1));
	XAie_CoreWaitForDone(DevInst, XAie_TileLoc(1,1), 1000000);
	/*while(1) {
		const uint32_t col_0_row_3 = (1<<23) + (1 << 18);
		const uint32_t core_control_addr =col_0_row_3 + 0x32000;// 0x00038000;
		const uint32_t core_status_addr = col_0_row_3 + 0x32004;//0x00038004;
		const uint32_t core_enable_event = col_0_row_3 + 0x32008;//0x00038004;
		uint32_t dc, ds, denable;
		XAie_Read32(DevInst,core_status_addr, &dc);
		XAie_Read32(DevInst,core_control_addr, &ds);
		// 14:8 is the disable event number
		XAie_Read32(DevInst,core_enable_event, &denable);
		std::cout << "core_status_addr:" << dc << "control:" << ds << "enable e" << &denable<<std::endl;
		if(denable << 15) break;
		usleep(100);
	}
	*/
	std::cout << "core done ***" << std::endl;
	return 0;
}
int test_aie_kernel_load(XAie_DevInst *DevInst) {
	test_aie_kernel_load_route(DevInst);
	//test_aie_kernel_load_simple(DevInst);
}
}
