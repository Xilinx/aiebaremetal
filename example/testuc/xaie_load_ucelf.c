/******************************************************************************
* Copyright (C) 2019 - 2020 Xilinx, Inc.  All rights reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

/*****************************************************************************/
/**
* @file xaie_load_ucelf.c
* @{
*
* This file contains the test application of data memory check.
*
* This application write random integers to first 1024 bytes of data memory of
* all valid tiles and reads the data back to check the basic functioning of
* data memory.
*
******************************************************************************/

/***************************** Include Files *********************************/
#include "xaie_test.h"
#include <iostream>
//#ifndef _UCFW_
//int test_aie_load_uc_elf(XAie_DevInst *DevInst){
//	return 0;
//}
//#else
extern "C" {
#include "xaiengine/xaie_uc.h"
#include "hw_config.h"
#include "ucfirmware/ucheader.h"
/************************** Constant Definitions *****************************/
extern const char _binary___app_elf_start[];
extern const char _binary___app_elf_end[];

AieRC XAie_SimIO_Read32(void *IOInst, u64 RegOff, u32 *Data);
/************************** Constant Definitions *****************************/
int test_aie_load_uc_elf(XAie_DevInst *DevInst)
{
	if (DevInst->DevProp.DevGen <= XAIE_DEV_GEN_AIEML ||
			DevInst->DevProp.DevGen == 6/*s100*/ ||
			DevInst->DevProp.DevGen == 7/*s200*/) {
		std::cout << "test_aie_load_uc_elf only can run on aie2ps or laterversion" << std::endl;
		return 0;
	}
	std::cout << "test_aie_load_uc_elf" << std::endl;
	//printf("test_aie_load_uc_elf\n");
	AieRC RC = XAIE_OK;
	XAie_LocType Loc = {0,12}, Loc_Core = {3, 12};//(row, col)
	u8 TileType = DevInst->DevOps->GetTTypefromLoc(DevInst, Loc);
	uint32_t shared_mem_base_addr = DevInst->DevProp.DevMod[TileType].UcMod->DataMemAddr;
	uint32_t smvar1 = 1,smvar2 = 2;
	uint32_t addr = XAie_GetTileAddr(DevInst, Loc.Row, Loc.Col) + shared_mem_base_addr;
	uint32_t core_status_addr = XAie_GetTileAddr(DevInst, Loc_Core.Row, Loc_Core.Col) + 0x00038004;
	uint32_t core_status_control = XAie_GetTileAddr(DevInst, Loc_Core.Row, Loc_Core.Col) + 0x00038000;
	XAie_Write32(DevInst, addr, smvar1);
	XAie_Read32(DevInst, addr, &smvar1);
#if DEVICE == 0
	//sleep(1);
#endif
	uint32_t core_status = 0;
	XAie_Read32(DevInst, core_status_addr, &core_status);
	core_status_control = 2;
	XAie_Write32(DevInst, core_status_control, core_status_control);
	core_status_control = 0;
	XAie_Read32(DevInst, core_status_control, &core_status_control);
	//printf("core_status = %x status_control = %x \n", core_status, core_status_control);
	
	//printf("\n before uc write the value is %x in %x\n", smvar1, shared_mem_base_addr);
	const void *ElfMem = _binary___app_elf_start;
	if(ElfMem == NULL) {
		std::cout << "Error load the UC firmware data section.\n" << std::endl;
		return -1;
	} else {
		std::cout << "Elf file read from _binary___app_elf_start ";
	  std::cout <<  std::hex << reinterpret_cast<uintptr_t>(_binary___app_elf_start) << std::endl;
	}
	///*
	RC = XAie_LoadUcMem(DevInst, Loc,(const unsigned char*)ElfMem);
	std::cout << "Load UC elf. RC =  <<  RC" << std::endl;
	if (RC != 0) {
		printf("load UC elf failed\n");
		return -1;
	}
	//*/
	const XAie_UcMod *UcMod = DevInst->DevProp.DevMod[TileType].UcMod;
	u32 status1, status2;
	UcMod->GetCoreStatus(DevInst, Loc, &status1,UcMod );
	std::cout << "Get core status.%x before Wakeup "<< status1 << std::endl;
	UcMod->Wakeup(DevInst, Loc, UcMod);
	UcMod->GetCoreStatus(DevInst, Loc, &status2,UcMod );
	std::cout << "Get core status.%x after Wakeup " << status2 << std::endl;
	if (status1 == status2 && status2 !=0) {
		return -1;
	}
	//this is a highlevel api to tigger UC run
	//while (XAie_CoreWaitForDone(DevInst, Loc_Core, 0) == XAIE_CORE_STATUS_TIMEOUT) {
	//	std::cout << "XAie_CoreWaitForDone" << std::endl;
	//}
	//
	/*// this wait can trigger UC run and get log out and the UC memory change can get detected.
	uint32_t RegVal;
	while(XAie_Read32(DevInst, addr, & RegVal) != 1230) {
		std::cout << "XAie_SimIO_Read32" << std::endl;
		usleep(1);
	}
	*/
  /* there is a issue in aiesimulator, the issue is that , after load and wake up UC
		 the UC will never run util, keep do XAie_Read32, we do not know whether one XAie_Read32
		 will tigger one UC instruction run or others, but we should do a lot XAie_Read32 to
		 get the UC log print out, on XAie_Read32 never work, 
		 to make sure get UC log print out/ UC run in the simulator, we need to follow the
		 format, while(1) {XAie_Read32}, we can use usleep here, but not sleep, because tht will
		 means wait a very long time util UC log get out
	 */
	uint32_t count = 0;
	while(smvar2 != MAGIC_DATA) {
		XAie_Read32(DevInst, addr, &smvar2);
		core_status = 0;
		core_status_control = 0;
		XAie_Read32(DevInst, core_status_addr, &core_status);
		XAie_Read32(DevInst, core_status_control, &core_status_control);
		//std::cout << "svar2 = " << smvar2 << std::endl;
		//printf("uc write value %x in %x sharebase = %x core_status = %x status_control = %x \n", \
				smvar2, addr,  shared_mem_base_addr, core_status, core_status_control);
		usleep(1);
	}

	if (smvar2 != MAGIC_DATA){
		std::cout << "timeout the UC load failed" << std::endl;
		return -1;
	}
	std::cout << "uc load success!\n";
	return XAIE_OK;
}
}
