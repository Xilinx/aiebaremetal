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
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
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

// Forward declaration of the self-contained CDO parser. Its implementation
// lives in ../src/aeg_cdo_parser.cc (linked into libaiebaremetal) but its
// header (../src/aeg_cdo_parser.h) is outside this test's include path, so we
// declare the entry point here to match aeg_cdo_parser.h.
namespace aeg {
namespace cdo {
aeg_err parse_and_apply(const void *data, size_t len, XAie_DevInst *dev,
                        std::uint64_t cdo_base);
}
}

// CDOv2 header/command constants (mirrors aeg_cdo_parser.cc).
#define CDO_HDR_IDN_WRD 0x004F4443U // "CDO\0" identification word
#define CDO_CMD_WRITE64 8U          // WRITE64 opcode
#define CDO_CMD_NOP     17U         // NOP (padding) opcode
#define CDO_CMD_LEN_SHIFT 16U       // short-command payload-length shift

void test_load_cdo() {
	// Set up a device instance using the debug backend so that register
	// writes issued by the parser are printed rather than sent to hardware.
	XAie_SetupConfig(ConfigPtr, XAIE_DEV_GEN_AIE, XAIE_BASE_ADDR,
			XAIE_COL_SHIFT, XAIE_ROW_SHIFT,
			XAIE_NUM_COLS, XAIE_NUM_ROWS, XAIE_SHIM_ROW,
			XAIE_MEM_TILE_ROW_START, XAIE_MEM_TILE_NUM_ROWS,
			XAIE_AIE_TILE_ROW_START, XAIE_AIE_TILE_NUM_ROWS);

	XAie_InstDeclare(DevInst, &ConfigPtr);

	AieRC RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
	if(RC != XAIE_OK) {
		printf("test_load_cdo: driver initialization failed.\n");
		return;
	}
	XAie_SetIOBackend(&DevInst, XAIE_IO_BACKEND_DEBUG);

	// Build the command stream first so we know the payload length (in words).
	const uint64_t base = DevInst.BaseAddr;
	std::vector<uint32_t> cmds;
	// WRITE64: payload = [addr_hi, addr_lo, value] (3 words). Target the device
	// base address so the parser's register offset (addr - BaseAddr) is 0.
	cmds.push_back(CDO_CMD_WRITE64 | (3U << CDO_CMD_LEN_SHIFT));
	cmds.push_back(static_cast<uint32_t>(base >> 32));
	cmds.push_back(static_cast<uint32_t>(base & 0xFFFFFFFFU));
	cmds.push_back(0xDEADBEEFU);
	// NOP: zero-length padding command; exercises the NOP decode path.
	cmds.push_back(CDO_CMD_NOP | (0U << CDO_CMD_LEN_SHIFT));

	const uint32_t buf_len = static_cast<uint32_t>(cmds.size());

	// Assemble the 5-word header: [version, magic, reserved, buf_len, checksum].
	std::vector<uint32_t> blob;
	uint32_t hdr[5];
	hdr[0] = 0x00000100U;                 // arbitrary version word
	hdr[1] = CDO_HDR_IDN_WRD;             // identification magic
	hdr[2] = 0x00000000U;                 // reserved
	hdr[3] = buf_len;                     // payload length in words
	hdr[4] = ~(hdr[0] + hdr[1] + hdr[2] + hdr[3]); // checksum
	blob.insert(blob.end(), hdr, hdr + 5);
	blob.insert(blob.end(), cmds.begin(), cmds.end());

	const size_t blob_bytes = blob.size() * sizeof(uint32_t);

	// Positive case: a well-formed blob must parse and apply successfully.
	aeg_err rc_ok = aeg::cdo::parse_and_apply(blob.data(), blob_bytes, &DevInst,
			DevInst.BaseAddr);
	printf("test_load_cdo: valid blob -> %d (expect %d)\n",
			static_cast<int>(rc_ok), static_cast<int>(aeg_err::ok));
	assert(rc_ok == aeg_err::ok);

	// Negative case: corrupt the identification word; parser must reject it.
	blob[1] = 0x0BADC0DEU;
	aeg_err rc_bad = aeg::cdo::parse_and_apply(blob.data(), blob_bytes, &DevInst,
			DevInst.BaseAddr);
	printf("test_load_cdo: bad-magic blob -> %d (expect %d)\n",
			static_cast<int>(rc_bad), static_cast<int>(aeg_err::user_error));
	assert(rc_bad == aeg_err::user_error);

	XAie_Finish(&DevInst);
	printf("test_load_cdo: PASS\n");
}

// Load real CDO blobs from disk and apply them through the self-contained
// parser using a debug-backend device instance (register writes are printed,
// not sent to hardware). Pass a specific path to load a single file, or
// nullptr to load the default testkernel CDO set. Paths are relative to the
// example/ directory (the unit test's working directory).
void test_load_cdo_from_file(const char *filename) {
	// Set up a debug-backend device instance for the parser to write through.
	XAie_SetupConfig(ConfigPtr, XAIE_DEV_GEN_AIE, XAIE_BASE_ADDR,
			XAIE_COL_SHIFT, XAIE_ROW_SHIFT,
			XAIE_NUM_COLS, XAIE_NUM_ROWS, XAIE_SHIM_ROW,
			XAIE_MEM_TILE_ROW_START, XAIE_MEM_TILE_NUM_ROWS,
			XAIE_AIE_TILE_ROW_START, XAIE_AIE_TILE_NUM_ROWS);

	XAie_InstDeclare(DevInst, &ConfigPtr);

	AieRC RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
	if(RC != XAIE_OK) {
		printf("test_load_cdo_from_file: driver initialization failed.\n");
		return;
	}
	XAie_SetIOBackend(&DevInst, XAIE_IO_BACKEND_DEBUG);

	std::vector<std::string> cdos;
	if (filename != nullptr) {
		cdos.push_back(filename);
	} else {
		cdos = {
			"testkernel/Work/ps/cdo/aie_cdo_reset.bin",
			//"testkernel/Work/ps/cdo/aie_cdo_clock_gating.bin",
			//"testkernel/Work/ps/cdo/aie_cdo_error_handling.bin",
			//"testkernel/Work/ps/cdo/aie_cdo_elfs.bin",
			//"testkernel/Work/ps/cdo/aie_cdo_init.bin",
		};
	}

	for (const auto &path : cdos) {
		std::ifstream cdo(path, std::ios::binary | std::ios::ate);
		if (!cdo.is_open()) {
			std::cerr << "test_load_cdo_from_file: failed to open CDO file: "
			          << path << std::endl;
			continue;
		}
		const std::streamsize size = cdo.tellg();
		cdo.seekg(0, std::ios::beg);
		std::vector<char> buffer(static_cast<size_t>(size));
		if (!cdo.read(buffer.data(), size)) {
			std::cerr << "test_load_cdo_from_file: failed to read CDO file: "
			          << path << std::endl;
			continue;
		}

		aeg_err rc = aeg::cdo::parse_and_apply(
				buffer.data(), static_cast<size_t>(size), &DevInst,
				DevInst.BaseAddr);
		printf("test_load_cdo_from_file: %s (%lld bytes) -> %d (expect %d)\n",
				path.c_str(), static_cast<long long>(size),
				static_cast<int>(rc), static_cast<int>(aeg_err::ok));
		assert(rc == aeg_err::ok);
	}

	XAie_Finish(&DevInst);
	printf("test_load_cdo_from_file: PASS\n");
}

int main(int argc, char* argv[]) {
	test_configparse();
	test_aietransaction();
	test_mlf_txn();
	//test_load_cdo();
	test_load_cdo_from_file(nullptr);
	return 0;
}
