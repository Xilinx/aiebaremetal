// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <stdlib.h>
#include "aiebaremetal.h"
#include "xaiemem.h"
#include "aeg_configure.h"
#include "common_layer/aeg_api_config.h"
#include "sim.h"
#include <cassert>
#include "xaiengine.h"
#include "hw_config.h"
#include "txn.h"
#define DEBUG_STR_MAX_LEN 512
uint32_t g_start_plio = 0;

void test_configparse() {
	ConfigureParser cp;

	cp.printDriverConfig();
	cp.printGraphConfig();
	cp.printGMIOsConfig();
}
 
//this is just for text not real op number
#define XAIE_IO_CUSTOM_OP_RECORD_TIMER (XAIE_IO_CUSTOM_OP_DDR_PATCH + 1)
void test_aietransaction() {
	XAie_SetupConfig(ConfigPtr, XAIE_DEV_GEN_AIE, XAIE_BASE_ADDR,
			XAIE_COL_SHIFT, XAIE_ROW_SHIFT,
			XAIE_NUM_COLS, XAIE_NUM_ROWS, XAIE_SHIM_ROW,
			XAIE_MEM_TILE_ROW_START, XAIE_MEM_TILE_NUM_ROWS,
			XAIE_AIE_TILE_ROW_START, XAIE_AIE_TILE_NUM_ROWS);

	///*

	XAie_InstDeclare(DevInst, &ConfigPtr);

	AieRC RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
	if(RC != XAIE_OK) {
		printf("Driver initialization failed.\n");
		return ;
	}
	XAie_SetIOBackend(&DevInst, XAIE_IO_BACKEND_DEBUG);
	//XAie_SetIOBackend(&DevInst, XAIE_IO_BACKEND_SIM);

  XAie_StartTransaction(&DevInst, XAIE_TRANSACTION_DISABLE_AUTO_FLUSH);

  record_timer_op_t timer_op;
  timer_op.id = 2;

  XAie_AddCustomTxnOp(&DevInst, XAIE_IO_CUSTOM_OP_RECORD_TIMER, &timer_op,
                      sizeof(timer_op));

  uint8_t *txn_ptr = XAie_ExportSerializedTransaction(&DevInst, 0, 0);
  XAie_TxnHeader *Hdr = (XAie_TxnHeader *)txn_ptr;
  auto size = Hdr->TxnSize;

  std::vector<uint8_t> txn(size, 0);
	printf("enclose are the the transaction buffer hex\n");
  memcpy((void *)txn.data(), (void *)txn_ptr, size);
	for (int i = 0;i < size; i++) {
		if (i %16==0) {
			printf("\n");
		}
		printf("0x%x ", txn[i]);
	}
	printf("\n");

  // check if there is an API to free txn pointer
  free(txn_ptr);
  XAie_Finish(&DevInst);
}

void test_mlf_txn() {
	 uint8_t *ReadPtr = ReadSerializedTransactionFromBin("../thirdparty/exmaple/conv2d/ml_txn.bin");
	 PrintSerializedTransaction(ReadPtr);
	 free(ReadPtr);
}

int main(int argc, char* argv[]) {
	test_configparse();
	test_aietransaction();
	test_mlf_txn();
	return 0;
}
