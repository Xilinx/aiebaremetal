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
#include "xaie_test_all.h"
#include "xaiemem.h"
#include <iostream>
//#ifndef _UCFW_
//int test_aie_load_uc_elf(XAie_DevInst *DevInst){
//	return 0;
//}
//#else
extern "C" {
/************************** Constant Definitions *****************************/
int test_aie_shim(XAie_DevInst *DevInst) 
{
	gdb_point(0);
	XAieMem mem(DevInst);

	int driverStatus;
	//enhancement not fix any bug
	for (int i = 1; i <= 3; i++) {
		driverStatus = XAie_CoreReset(DevInst, XAie_TileLoc(1, i));
		driverStatus |= XAie_CoreUnreset(DevInst, XAie_TileLoc(1, i));
		driverStatus |= XAie_CoreReset(DevInst, XAie_TileLoc(2, i));
		driverStatus |= XAie_CoreUnreset(DevInst, XAie_TileLoc(2, i));
		driverStatus |= XAie_CoreReset(DevInst, XAie_TileLoc(3, i));
	}

	const uint32_t aie_data_addr = 0x400;
	u64 phy = 0,phy_out = 0, phy_size = 0;
	u32 mlen = 32;
	//Prepare DDR data
	//XAie_MemInst *in = XAie_MemAllocate(DevInst, mlen * sizeof(u32), XAIE_MEM_CACHEABLE);
	//phy = (u32*)XAie_MemGetDevAddr(in);
	//XAie_MemInst *out = XAie_MemAllocate(DevInst, mlen * sizeof(u32), XAIE_MEM_CACHEABLE);
	//phy_out = (u32*)XAie_MemGetDevAddr(out);
	u32* vmem = (u32*)mem.malloc(mlen * sizeof(u32));
	u32* vmem_out = (u32*)mem.malloc(mlen * sizeof(u32));

	mem.get_dev_addr(reinterpret_cast<void *>(vmem), &phy);
	mem.get_dev_addr(reinterpret_cast<void *>(vmem_out), &phy_out);
	//XAie_MemSyncForCPU(in);
	//XAie_MemSyncForCPU(out);
	for(int i = 0; i < mlen; i++) {
		vmem[i] = i * 2;
		vmem_out[i] = 0;
	}

	printf("aiebaremetal alloc memory \n");
	mem.sync_for_dev(reinterpret_cast<void *>(vmem), mlen * sizeof(u32));
	mem.sync_for_dev(reinterpret_cast<void *>(vmem_out), mlen * sizeof(u32));
	//XAie_MemSyncForDev(in);
	//XAie_MemSyncForDev(out);

	//printf("phy = %x\n", phy);
	//return 0;
	//XAieMemSyncForCPU(in);
	//routing the tile
	int RC;
	XAie_LocType shim = XAie_TileLoc(2, 0);
	/*
	 * From DDR to SHIM DMA and Back Will go to MUX/DEMUX, there are limited
	 * channel (fast and slow) available for SHIM, DDR->SHIM 3, 7
	 * SHIM->DDR 2,3, in practice, SHIM->DDR 3 seems not work
	 * below we use 3 for DDR to SHMie_EnableShimDmaToAieStrmPort
	 * XAie_EnableShimDmaToAieStrmPort do the MUX setting
	 * The PORT/CHANNEL related Mux is the slave port 
	 * The internal connected port can be different when doing DMA
	 */
	RC = XAie_StrmConnCctEnable(DevInst, shim, SOUTH, 3,  WEST, 0);
	RC |= XAie_EnableShimDmaToAieStrmPort(DevInst, shim, 3);

	XAie_LocType shim_t1 = XAie_TileLoc(1, 0);
	RC |= XAie_StrmConnCctEnable(DevInst,shim_t1, EAST, 0, NORTH, 0);
	XAie_LocType mem_t1 = XAie_TileLoc(1, 1);
	RC |= XAie_StrmConnCctEnable(DevInst, mem_t1, SOUTH, 0, NORTH, 0);
	XAie_LocType mem_t2 = XAie_TileLoc(1, 2);
	RC |= XAie_StrmConnCctEnable(DevInst, mem_t2, SOUTH, 0, NORTH, 0);
	XAie_LocType aie_t1 = XAie_TileLoc(1, 3);
	// 1, 3 receive data
	RC |= XAie_StrmConnCctEnable(DevInst, aie_t1, SOUTH,0 , DMA, 0);
	// 1, 3 send BD data out
	RC |= XAie_StrmConnCctEnable(DevInst, aie_t1, DMA, 0, EAST, 0);

	if (RC != XAIE_OK) {printf("RC FAILED 1\n");return -1;}
  XAie_LocType aie_t2 = XAie_TileLoc(2, 3);
	RC |= XAie_StrmConnCctEnable(DevInst, aie_t2, WEST, 0, EAST, 0);

	if (RC != XAIE_OK) {printf("RC FAILED 0.2\n");return -1;}
  XAie_LocType aie_t3 = XAie_TileLoc(3, 3);
	RC |= XAie_StrmConnCctEnable(DevInst, aie_t3, WEST, 0, SOUTH, 0);

  XAie_LocType mem_down_t2 = XAie_TileLoc(3, 2);
	RC |= XAie_StrmConnCctEnable(DevInst, mem_down_t2, NORTH, 0, SOUTH, 0);

	if (RC != XAIE_OK) {	printf("RC FAILED 0.1\n");	return -1;}
  XAie_LocType mem_down_t1 = XAie_TileLoc(3, 1);
	RC |= XAie_StrmConnCctEnable(DevInst, mem_down_t1, NORTH, 0, SOUTH, 0);
	if (RC != XAIE_OK) {printf("RC FAILED mem_down_t1 cct enabl\n");return -1;}

	XAie_LocType shim_3 = XAie_TileLoc(3, 0);
	RC |= XAie_StrmConnCctEnable(DevInst,shim_3, NORTH, 0, SOUTH, 2);
	RC |= XAie_EnableAieToShimDmaStrmPort(DevInst, shim_3, 2);

	if (RC != XAIE_OK) {printf("RC FAILED 0\n");return -1;}
	//prepare the send shim BD
	XAie_DmaDesc shim_desc;
	RC |= XAie_DmaDescInit(DevInst, &shim_desc, shim);
	//RC |= XAie_DmaSetAddrLen(&shim_desc, &MemInst_in, mlen*sizeof(u32));
	//RC |= XAie_DmaSetAddrLen(&shim_desc, (u64)vmem, mlen * sizeof(int));
	RC |= XAie_DmaSetAddrLen(&shim_desc, (u64)phy, mlen * sizeof(int));
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
		usleep(100);
	}

	XAie_DmaDesc DmaS2MM;
	RC = XAie_DmaDescInit(DevInst, &DmaS2MM, aie_t1);

	RC |= XAie_DmaSetAddrLen(&DmaS2MM, aie_data_addr, mlen * sizeof(uint32_t));
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
		RC |= XAie_DmaGetPendingBdCount(DevInst, shim, 0, DMA_MM2S, (u8*)&shim_snd_count);
		RC |= XAie_DmaGetPendingBdCount(DevInst, aie_t1, 0, DMA_S2MM,(u8*) &aie1_count);
		printf("---after do receive aie count = %d shim_snd_count=%d\n", aie1_count,shim_snd_count);
		usleep(100);
	}
	printf("bd creive done after \n");
	u32 data2[256];
	XAie_DataMemBlockRead(DevInst, aie_t1,  aie_data_addr, data2, mlen*sizeof(uint32_t));
	for (int i = 0; i < mlen; i++) {
		printf(" %d ", data2[i]);
		if (i % 16 == 0) printf("\n");
		if (vmem[i] != data2[i]) {
			printf("shim data transfer failed!\n");
			return 1;
		}
	}
	//aie tile DMA to stream
	XAie_DmaDesc DmaMM2S;
	RC |= XAie_DmaDescInit(DevInst, &DmaMM2S, aie_t1);
	RC |= XAie_DmaSetAddrLen(&DmaMM2S, aie_data_addr, mlen * sizeof(uint32_t));
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
		RC |= XAie_DmaGetPendingBdCount(DevInst, aie_t1, 0, DMA_MM2S, (u8*)&aie1_snd_count);
		printf("before shim receive aie_snd_count=%d\n", aie1_snd_count);
		sleep(1);
	}
	//prepare the receive shim BD
	XAie_LocType shim_out = shim_3;
	//const u32 recv_len = 128;
	const u32 recv_len = mlen;
	XAie_DmaDesc shim_desc_out;
	RC |= XAie_DmaDescInit(DevInst, &shim_desc_out, shim_out);
	RC |= XAie_DmaSetAddrLen(&shim_desc_out, (u64)phy_out, recv_len * sizeof(int));
	RC |= XAie_DmaEnableBd(&shim_desc_out);
	RC |= XAie_DmaWriteBd(DevInst, &shim_desc_out,shim_out, 0);
	//move data from DDR to SHIM to (1,3)
	RC |= XAie_DmaChannelPushBdToQueue(DevInst, shim_out, 0, DMA_S2MM, 0);
	RC |= XAie_DmaChannelEnable(DevInst,shim_out, 0, DMA_S2MM);

	u32 shim_count = 1;
	while(shim_count) {
		RC |= XAie_DmaGetPendingBdCount(DevInst, shim_out, 0, DMA_S2MM,(u8*) &shim_count);
		printf("after shim receive shim_recv_count=%d len = %d col=%d row = %d\n",
				shim_count, recv_len, shim_out.Col, shim_out.Row);
		usleep(100);
	}
	//XAie_MemSyncForCPU(in);
	//XAie_MemSyncForCPU(out);
	mem.sync_for_cpu((void *)vmem_out, mlen * sizeof(u32));

	for(int i = 0; i < recv_len; i++) {
		printf(" %d ", vmem_out[i]);
		if (i % 16 == 0) printf("\n");
	}
	//add done log
	printf("shim test done----------\n");
	//while(1) {sleep(1); printf("wait ---\n");};
	//free memory after use it
	return 0;
}
}
