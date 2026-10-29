/******************************************************************************
* (c) Copyright 2022 Xilinx, Inc. All rights reserved.
* (c) Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
*
* This file contains confidential and proprietary information
* of Xilinx, Inc. and is protected under U.S. and
* international copyright and other intellectual property
* laws.
*
* DISCLAIMER
* This disclaimer is not a license and does not grant any
* rights to the materials distributed herewith. Except as
* otherwise provided in a valid license issued to you by
* Xilinx, and to the maximum extent permitted by applicable
* law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
* WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
* AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
* BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
* INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
* (2) Xilinx shall not be liable (whether in contract or tort,
* including negligence, or under any other theory of
* liability) for any loss or damage of any kind or nature
* related to, arising under or in connection with these
* materials, including for any direct, or any indirect,
* special, incidental, or consequential loss or damage
* (including loss of data, profits, goodwill, or any type of
* loss or damage suffered as a result of any action brought
* by a third party) even if such damage or loss was
* reasonably foreseeable or Xilinx had been advised of the
* possibility of the same.
*
* CRITICAL APPLICATIONS
* Xilinx products are not designed or intended to be fail-
* safe, or for use in any application requiring fail-safe
* performance, such as life-support or safety devices or
* systems, Class III medical devices, nuclear facilities,
* applications related to the deployment of airbags, or any
* other applications that could lead to death, personal
* injury, or severe property or environmental damage
* (individually and collectively, "Critical
* Applications"). Customer assumes the sole risk and
* liability of any use of Xilinx products in Critical
* Applications, subject only to applicable laws and
* regulations governing limitations on product liability.
*
* THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
* PART OF THIS FILE AT ALL TIMES.
******************************************************************************/

/*****************************************************************************/
/**
*
* @file test_submit_command.cpp
*  This file contains test cases for cert_host::cert_api::submit_command (HSA queue
*  control-code submission): column validation; single-column, 2-24 column, and >24
*  column paths; allocation failures; external buffer maps; completion signaling.
*  Tests run against stub XAIE/barrier/cert ELF infrastructure (no hardware).
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	  Who	   Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera      05/03/2026  Initial creation (submit_command unit tests)
* 1.1	 Veera      10/05/2026  updated comment section for cert_host::cert_api::submit_command
* 1.2	 Veera      26/05/2026  updated timing analysis for cert_host::cert_api::submit_command
* </pre>
*
******************************************************************************/

/*****************************************************************************/
/**
*
* TEST SPECIFCIATION:
* Source code version: <2026.1>
* Verification Environment1:
	Testing Tool: LDRA
	Platform: Silicon
	Target: APU
	Compiler1: gcc
* Verification Environment2:
	Testing Tool: CPPUTEST
	Platform: Host
	Target: x86
    Compiler2: gcc
* Verification Method:By Specification of test cases
* Test Procedure: The test execution procedure is same for all the test cases.
* After the tests have been specified, the test procedure involves:
*   - building the tests cases for the selected environment
*   - execution of the binary executable in the selected environment
*   - recording the results of execution
*
/*****************************************************************************/
/***************************** Include Files *********************************/
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/MemoryLeakWarningPlugin.h"
#include "CppUTest/TestHarness.h"
#include "CppUTest/TestRegistry.h"

extern "C" {
#include "stubs/submit_cmd_stubs/stub_barrier.h"
#include "stubs/submit_cmd_stubs/stub_xaie_mem.h"
}
#include "stubs/submit_cmd_stubs/stub_cert_elf_loader.h"
#ifdef new
#undef new
#endif

#ifdef nothrow
#undef nothrow
#endif

#include <cstdint>
#include <map>
#include <string_view>
#include <vector>
#include <chrono>
#include "cert_host.h"
#include <iostream>

#ifndef HSA_COMP_ERROR_CODE
#define HSA_COMP_ERROR_CODE 5
#endif

using namespace cert_host;

extern "C" XAie_DevInst *Stub_XAie_CreateMinimalDevInst(void);

struct CertApi {

	std::map<int64_t, int64_t> external_buffer_map;
	std::map<int64_t, uint64_t> external_buffer_addr_map;


	XAie_DevInst *DevInst;
	handshake_t handshake;
	uint32_t host_args_low;
	uint32_t host_args_high;
	XAie_MemInst *host_args_meminst;
	XAie_MemInst *hsa_meminst;
	XAie_MemInst *comp_sig_meminst;
};

static void InitApiWithStubs(cert_api &api) {
	Stub_XAie_Reset();
	Stub_Barrier_Reset();
	Stub_AieMem_ResetAll();

	api.init_devinst(Stub_XAie_CreateMinimalDevInst());

	static XAie_MemInst hsa_mem_inst{};
	static XAie_MemInst comp_mem_inst{};

	Stub_XAie_InitQueue(&hsa_mem_inst, &comp_mem_inst, HSA_QUEUE_ENTRY);

	CertApi *hack = reinterpret_cast<CertApi *>(&api);
	hack->hsa_meminst = Stub_XAie_GetQueueMemInst();
	hack->comp_sig_meminst = Stub_XAie_GetCompMemInst();
}

static hsa_queue *GetQueue() {
	XAie_MemInst *q_mem = Stub_XAie_GetQueueMemInst();
	return static_cast<hsa_queue *>(XAie_MemGetVAddr(q_mem));
}

static uint32_t *GetCompletionBuf() {
	XAie_MemInst *c_mem = Stub_XAie_GetCompMemInst();
	return static_cast<uint32_t *>(XAie_MemGetVAddr(c_mem));
}
static int call_submit(cert_api &api, unsigned int cols, const std::vector<size_t> &sizes, uint32_t final_comp_sig) {
	StubCertElfLoader_SetColsLayout(cols, sizes);
	g_stub_comp_sig_value = final_comp_sig;
	char dummy[16] = {};
	std::map<std::string_view, uint64_t> sym;
	return api.submit_command(dummy, sizeof(dummy), sym);
}

TEST_GROUP(CertApiSubmitCommand) {
	cert_api *api;


	void setup() {
		api = new cert_api();
		InitApiWithStubs(*api);
	}


	void teardown() { delete api; }
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_SingleColumnNominal
*		Function Name: submit_command
*		Input: dummy = {16}, 16, sym = {},
*		Expectation: rc == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_001_SingleColumnNominal) {
	StubCertElfLoader_SetColsLayout(1, {16});
	g_stub_comp_sig_value = HSA_COMP_SUCCESS;

	char dummy[16] = {};
	std::map<std::string_view, uint64_t> sym;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = api->submit_command(dummy, sizeof(dummy), sym);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by submit_command function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_ColsZero
*		Function Name: call_submit
*		Input: cols = 0, sym = {},HSA_COMP_SUCCESS
*		Expectation: rc == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_002_ColsZero) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, 0, {}, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-1, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_ColsTooLarge
*		Function Name: call_submit
*		Input: cols = 25, sizes = {1}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_003_ColsTooLarge) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, 25, {1}, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-1, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_SingleColumnAllocFail
*		Function Name: call_submit
*		Input: cols = 1, sizes = {64}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_004_SingleColumnAllocFail) {
	Stub_AieMem_SetAllocateFailIndex(0);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, 1, {64}, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-1, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_CompletionError
*		Function Name: call_submit
*		Input: cols = 1, sizes = {64}, comp_sig = HSA_COMP_ERROR_CODE
*		Expectation: rc == HSA_COMP_ERROR_CODE
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_005_CompletionError) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, 1, {64}, HSA_COMP_ERROR_CODE);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(HSA_COMP_ERROR_CODE, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_TwoColumnNominal
*		Function Name: call_submit
*		Input: cols = 2, sizes = {32, 48}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_006_TwoColumnNominal) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, 2, {32, 48}, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_TwoColumnEntryAllocFail
*		Function Name: call_submit
*		Input: cols = 2, sizes = {32, 48}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_007_TwoColumnEntryAllocFail) {
	Stub_AieMem_SetAllocateFailIndex(0);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, 2, {32, 48}, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-1, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_TwoColumnCtrlAllocFail
*		Function Name: call_submit
*		Input: cols = 2, sizes = {32, 48}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_008_TwoColumnCtrlAllocFail) {
	Stub_AieMem_SetAllocateFailIndex(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, 2, {32, 48}, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-1, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_Case4Nominal
*		Function Name: call_submit
*		Input: cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1,sizes = {cols, 16}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_009_Case4Nominal) {
	unsigned int cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1;
	std::vector<size_t> sizes(cols, 16);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, cols, sizes, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_010_Case1WithExternalBuffer
*		Function Name: submit_command
*		Input: dummy = {0}, sizeof(dummy) = 16, sym = {}
*		Expectation: rc == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_010_Case1WithExternalBuffer) {
	api->external_buffer_map[1] = 100;
	api->external_buffer_addr_map[100] = 0x12345678ULL;

	StubCertElfLoader_SetColsLayout(1, {16});
	g_stub_comp_sig_value = HSA_COMP_SUCCESS;

	char dummy[16] = {};
	std::map<std::string_view, uint64_t> sym;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = api->submit_command(dummy, sizeof(dummy), sym);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by submit_command function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_011_Case3WithExternalBuffer
*		Function Name: call_submit
*		Input: cols = 2, sizes = {32, 48}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc = 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_011_Case3WithExternalBuffer) {
	api->external_buffer_map[1] = 200;
	api->external_buffer_addr_map[200] = 0xABCDEF00ULL;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, 2, {32, 48}, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_012_Case4WithExternalBuffer
*		Function Name: call_submit
*		Input: cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1, sizes = {cols, 16}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc = 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_012_Case4WithExternalBuffer) {
	api->external_buffer_map[1] = 300;
	api->external_buffer_addr_map[300] = 0xDEADBEEFULL;

	unsigned int cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1;
	std::vector<size_t> sizes(cols, 16);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, cols, sizes, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_013_Case4EntryAllocFail
*		Function Name: call_submit
*		Input: cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1, sizes = {cols, 16}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_013_Case4EntryAllocFail) {
	Stub_AieMem_SetAllocateFailIndex(0);
	unsigned int cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1;
	std::vector<size_t> sizes(cols, 16);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, cols, sizes, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-1, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_014_Case4CtrlCodeAllocFail
*		Function Name: call_submit
*		Input: cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1, sizes = {cols, 16}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_014_Case4CtrlCodeAllocFail) {
	Stub_AieMem_SetAllocateFailIndex(1);
	unsigned int cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1;
	std::vector<size_t> sizes(cols, 16);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, cols, sizes, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-1, rc);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_015_Case1LoopMultipleIterations
*		Function Name: call_submit
*		Input: cols = 1, sizes = {16}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == 0
*		Test Method: Unit testing
*		Methods for deriving test case: Analysis of requirement
*		Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_015_Case1LoopMultipleIterations) {
	const uint32_t seq[] = {0u, 0u, 0u};
	Stub_XAie_SetCompletionBehavior(0u, seq, 3u);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, 1, {16}, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_016_Case3LoopMultipleIterations
*		Function Name: 	call_submit
*		Input: cols = 2, sizes = {32, 48}, comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == 0
*		Test Method: Unit testing
*		Methods for deriving test case: Analysis of requirement
*		Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_016_Case3LoopMultipleIterations) {
	const uint32_t seq[] = {0u, 0u, 0u};
	Stub_XAie_SetCompletionBehavior(0u, seq, 3u);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, 2, {32, 48}, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, rc);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_017_Case4LoopMultipleIterations
*		Function Name: call_submit
*		Input: cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1, sizes = {cols, 16},
*		       comp_sig = HSA_COMP_SUCCESS
*		Expectation: rc == 0
*		Test Method: Unit testing
*		Methods for deriving test case: Analysis of requirement
*		Test Environment: 2
*/
/*********************************************************************************/

TEST(CertApiSubmitCommand, TC_017_Case4LoopMultipleIterations) {
	const uint32_t seq[] = {0u, 0u, 0u};
	Stub_XAie_SetCompletionBehavior(0u, seq, 3u);

	unsigned int cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1;
	std::vector<size_t> sizes(cols, 16);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int rc = call_submit(*api, cols, sizes, HSA_COMP_SUCCESS);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by call_submit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, rc);
}

/*@section Test Execution details
| Test Case ID                              | Test Result | CR  | Impact | Test Details |
|-------------------------------------------|-------------|-----|--------|--------------|
| TC_001_SingleColumnNominal                | PASS        | No  | NA     | NA           |
| TC_002_ColsZero                           | PASS        | No  | NA     | NA           |
| TC_003_ColsTooLarge                       | PASS        | No  | NA     | NA           |
| TC_004_SingleColumnAllocFail              | PASS        | No  | NA     | NA           |
| TC_005_CompletionError                    | PASS        | No  | NA     | NA           |
| TC_006_TwoColumnNominal                   | PASS        | No  | NA     | NA           |
| TC_007_TwoColumnEntryAllocFail            | PASS        | No  | NA     | NA           |
| TC_008_TwoColumnCtrlAllocFail             | PASS        | No  | NA     | NA           |
| TC_009_Case4Nominal                       | PASS        | No  | NA     | NA           |
| TC_010_Case1WithExternalBuffer            | PASS        | No  | NA     | NA           |
| TC_011_Case3WithExternalBuffer            | PASS        | No  | NA     | NA           |
| TC_012_Case4WithExternalBuffer            | PASS        | No  | NA     | NA           |
| TC_013_Case4EntryAllocFail                | PASS        | No  | NA     | NA           |
| TC_014_Case4CtrlCodeAllocFail             | PASS        | No  | NA     | NA           |
| TC_015_Case1LoopMultipleIterations        | PASS        | No  | NA     | NA           |
| TC_016_Case3LoopMultipleIterations        | PASS        | No  | NA     | NA           |
| TC_017_Case4LoopMultipleIterations        | PASS        | No  | NA     | NA           |
/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)        | %      | Result     | CR/Status    | Impact          |
|--------------------------------------|--------|------------|--------------|-----------------|
| cert_api::submit_command             | <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)        | 100    | <Pass>     | <NA>         | No Impact       |

/*@section Uncovered details*/
/* No uncovered details */
