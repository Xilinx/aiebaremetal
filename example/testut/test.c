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

#include <stdio.h>
#include <stdlib.h>
#include "aiebaremetal.h"
#include "xaiemem.h"
#include "abr_configure.h"
#include "common_layer/abr_api_config.h"
#include "sim.h"
#include <cassert>
#include "xaiengine.h"
#include "hw_config.h"
#include "txn.h"
#define DEBUG_STR_MAX_LEN 512
uint32_t g_start_plio = 0;


// void test_driverconfig(const adf::driver_config& dconfig_1, const adf::driver_config& dconfig_2) {
// 	assert(dconfig_1.hw_gen == dconfig_2.hw_gen);
// 	assert(dconfig_1.base_address == dconfig_2.base_address);
// 	assert(dconfig_1.column_shift == dconfig_2.column_shift);
// 	assert(dconfig_1.row_shift == dconfig_2.row_shift);
// 	assert(dconfig_1.num_columns == dconfig_2.num_columns);
// 	assert(dconfig_1.num_rows == dconfig_2.num_rows);
// 	assert(dconfig_1.shim_row == dconfig_2.shim_row);
// 	assert(dconfig_1.mem_row_start == dconfig_2.mem_row_start);
// 	assert(dconfig_1.mem_num_rows == dconfig_2.mem_num_rows);
// 	assert(dconfig_1.aie_tile_row_start == dconfig_2.aie_tile_row_start);
// 	assert(dconfig_1.aie_tile_num_rows == dconfig_2.aie_tile_num_rows);
// 	// assert(dconfig_1.partition_num_cols == dconfig_2.partition_num_cols);
// 	// assert(dconfig_1.partition_overlay_start_cols == dconfig_2.partition_overlay_start_cols);	
// }

// void test_graphconfig(const adf::graph_config& gconfig_1, const adf::graph_config& gconfig_2) {
// 	assert(gconfig_1.id == gconfig_2.id);
// 	assert(gconfig_1.name == gconfig_2.name);
// 	assert(gconfig_1.coreColumns == gconfig_2.coreColumns);
// 	assert(gconfig_1.coreRows == gconfig_2.coreRows);
// 	assert(gconfig_1.iterMemColumns == gconfig_2.iterMemColumns);
// 	assert(gconfig_1.iterMemRows == gconfig_2.iterMemRows);
// 	assert(gconfig_1.iterMemAddrs == gconfig_2.iterMemAddrs);
// 	assert(gconfig_1.triggered == gconfig_2.triggered);
// }

// void test_gmiosconfig(const adf::gmio_config& gmconfig_1, const adf::gmio_config& gmconfig_2) {
// 	assert(gmconfig_1.id == gmconfig_2.id);
// 	assert(gmconfig_1.name == gmconfig_2.name);
// 	assert(gmconfig_1.logicalName == gmconfig_2.logicalName);
// 	assert(gmconfig_1.type == gmconfig_2.type);
// 	assert(gmconfig_1.shimColumn == gmconfig_2.shimColumn);
// 	assert(gmconfig_1.channelNum == gmconfig_2.channelNum);
// 	assert(gmconfig_1.streamId == gmconfig_2.streamId);
// 	assert(gmconfig_1.burstLength == gmconfig_2.burstLength);

// }

// void test_config_parser(ConfigureParser& cp, ConfigureStaticParser& csp) {
// 	//test driver config
// 	adf::driver_config dconfig_1 = cp.get_driver_config();
// 	adf::driver_config dconfig_2 = csp.get_driver_config();
// 	test_driverconfig(dconfig_1, dconfig_2);

// 	//test graph configs
// 	const char* graphs[1] = {"gradf"};
// 	for (const auto& name: graphs) {
// 		adf::graph_config gconfig_1 = cp.get_graph_config(name);
// 		adf::graph_config gconfig_2 = csp.get_graph_config(name);
// 		test_graphconfig(gconfig_1, gconfig_2);
// 	}

// 	const char* gmios[2] = {"gradf.in", "gradf.out"};
// 	for (const auto& name: gmios) {
// 		adf::gmio_config gmconfig_1 = cp.get_gmio_config(name);
// 		adf::gmio_config gmconfig_2 = csp.get_gmio_config(name);
// 		test_gmiosconfig(gmconfig_1, gmconfig_2);
// 	}

// }

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
	//BaremetalGraph binst("gradf");
	//binst.init();
	//binst.wait();

/*
	SimMem smem;
	auto d1 = smem.malloc(36);
	auto d2 = smem.malloc(40);
	auto d3 = smem.malloc(60);

	auto fd = smem.find_start_mem(d2+20) ;
	assert(smem.find_start_mem(d2+20) == d2);
	assert(smem.find_start_mem(d2) == d2);
	assert(smem.find_start_mem(d3+20) == d3);
	assert(smem.find_start_mem(d3+80) == NULL);
	smem.free(d1);
	smem.free(d2);
	smem.free(d3);
*/
	return 0;
}
