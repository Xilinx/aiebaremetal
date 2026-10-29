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
* @file test_generate_event.cpp
*  This file contains test cases of AIE event generation functions
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	  Who	   Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera      05/03/2026  Initial creation
* 1.1	 Veera      20/04/2026  Added test cases for AIE event generation functions
* 1.2	 Veera      10/05/2026  updated comment section for AIE event generation functions
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
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport_c.h"
#include "stubs.h"
#include <chrono>
#include "test_comparator.h"
#include "xaie_generate_events.h"
#include <iostream>
TEST_GROUP(GenerateEventsOnColTests) {

	XAie_DevInst mockDevInst;
	XAie_LocType testLoc;

	void setup() {
		RegisterXAieLocTypeComparator();
		mockDevInst.DevOps = &s_MockDevOps;
		mockDevInst.NumRows = 1;
		mockDevInst.StartCol = 0;
		mockDevInst.NumCols = 4;
		mockDevInst.MemTileRowStart = 1;
		mockDevInst.MemTileNumRows = 2;

		testLoc.Col = 3;
		testLoc.Row = 0;
	}

	void teardown() {
		Stub_XAie_ClearTTypeCallSequence();
		Stub_XAie_SetTileType(0);
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_AllShimTiles
*		Function Name: generate_events_on_col
*		Input: mockDevInst, testCol = 3
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GenerateEventsOnColTests, TC_001_AllShimTiles) {
	const int testCol = 3;
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	for (int i = XAIE_EVENT_GROUP_ERRORS_PL + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_PL; i++) {
		mock_c()->expectOneCall("XAie_EventGenerate")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	}
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	generate_events_on_col(&mockDevInst, testCol);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by generate_events_on_col function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_AllMemTiles
*		Function Name: generate_events_on_col
*		Input: mockDevInst, testCol = 4
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GenerateEventsOnColTests, TC_002_AllMemTiles) {
	const int testCol = 4;
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_MEMTILE);
	for (int i = XAIE_EVENT_GROUP_ERRORS_MEM + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM; i++) {
		mock_c()->expectOneCall("XAie_EventGenerate")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	}
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	generate_events_on_col(&mockDevInst, testCol);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by generate_events_on_col function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_AllAIETiles
*		Function Name: generate_events_on_col
*		Input: mockDevInst, testCol = 5
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GenerateEventsOnColTests, TC_003_AllAIETiles) {
	const int testCol = 5;
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_AIETILE);
	for (int i = XAIE_EVENT_GROUP_ERRORS_1_CORE + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_CORE; i++) {
		mock_c()
			->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}
	for (int i = XAIE_EVENT_GROUP_ERRORS_MEM + 1; i < XAIE_EVENT_GROUP_BROADCAST_MEM; i++) {
		mock_c()
			->expectOneCall("XAie_EventGenerate")
			->withPointerParameters("DevInst", &mockDevInst)
			->ignoreOtherParameters()
			->andReturnIntValue(XAIE_OK);
	}
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	generate_events_on_col(&mockDevInst, testCol);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by generate_events_on_col function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_AllInvalidTileTypes
*		Function Name: generate_events_on_col
*		Input: mockDevInst, testCol = 6
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GenerateEventsOnColTests, TC_004_AllInvalidTileTypes) {
	const int testCol = 6;
	Stub_XAie_SetTileType(99);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	generate_events_on_col(&mockDevInst, testCol);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by generate_events_on_col function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_AllShimPLTiles
*		Function Name: generate_events_on_col
*		Input: mockDevInst, testCol = 3
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GenerateEventsOnColTests, TC_005_AllShimPLTiles) {
	const int testCol = 3;
	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMPL);
	for (int i = XAIE_EVENT_GROUP_ERRORS_PL + 1; i < XAIE_EVENT_GROUP_STREAM_SWITCH_PL; i++) {
		mock_c()->expectOneCall("XAie_EventGenerate")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	}
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	generate_events_on_col(&mockDevInst, testCol);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by generate_events_on_col function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_ShimHelperTileTypeMismatch
*		Function Name: generate_events_on_col
*		Input: mockDevInst, testCol = 3
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GenerateEventsOnColTests, TC_006_ShimHelperTileTypeMismatch) {
	const u8 seq[] = {(u8)XAIEGBL_TILE_TYPE_SHIMNOC, (u8)XAIEGBL_TILE_TYPE_AIETILE};
	Stub_XAie_SetTTypeCallSequence(seq, 2U);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	generate_events_on_col(&mockDevInst, 3);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by generate_events_on_col function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}
/*********************************************************************************/
/*
*		Unique test case ID: TC_007_AieCoreAndMemHelperTileTypeMismatch
*		Function Name: generate_events_on_col
*		Input: mockDevInst, testCol = 5
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GenerateEventsOnColTests, TC_007_AieCoreAndMemHelperTileTypeMismatch) {
	const u8 seq[] = {(u8)XAIEGBL_TILE_TYPE_AIETILE, (u8)XAIEGBL_TILE_TYPE_SHIMNOC, 99};
	Stub_XAie_SetTTypeCallSequence(seq, 3U);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	generate_events_on_col(&mockDevInst, 5);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by generate_events_on_col function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_008_MemHelperTileTypeMismatch
*		Function Name: generate_events_on_col
*		Input: mockDevInst, testCol = 4
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GenerateEventsOnColTests, TC_008_MemHelperTileTypeMismatch) {
	const u8 seq[] = {(u8)XAIEGBL_TILE_TYPE_MEMTILE, (u8)XAIEGBL_TILE_TYPE_SHIMNOC};
	Stub_XAie_SetTTypeCallSequence(seq, 2U);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	generate_events_on_col(&mockDevInst, 4);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by generate_events_on_col function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}
TEST_GROUP(AieGenerateIntrTests) {
	XAie_DevInst *devInst = new XAie_DevInst;
	XAie_LocType testLoc;

	void setup() {
		RegisterXAieLocTypeComparator();

		devInst->NumCols = 1;
		devInst->NumRows = 4;
		devInst->StartCol = 0;
		devInst->MemTileRowStart = 1;
		devInst->MemTileNumRows = 2;
		devInst->DevProp.DevGen = XAIE_DEV_GEN_AIE;
		devInst->DevOps = &s_MockDevOps;
		testLoc.Col = 0;
		testLoc.Row = 0;
	}
	void teardown() {
		Stub_XAie_ClearTTypeCallSequence();
		Stub_XAie_SetTileType(0);
		delete devInst;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_FailureCases
*		Function Name: test_aie_generate_intr
*		Input: devInst
*		Expectation: result != 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AieGenerateIntrTests, TC_001_FailureCases) {
	mock_c()
		->expectOneCall("XAie_PmRequestTiles")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("NumTiles", 0)
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->withPointerParameters("DevInst", devInst)->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = test_aie_generate_intr(devInst);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by test_aie_generate_intr function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result != 0);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_002_SuccessWithInvalidTileType
*		Function Name: test_aie_generate_intr
*		Input: devInst
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AieGenerateIntrTests, TC_002_SuccessWithInvalidTileType) {
	Stub_XAie_SetTileType(99);
	mock_c()->expectOneCall("XAie_PmRequestTiles")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->withPointerParameters("DevInst", devInst)->andReturnIntValue(0);
	mock_c()->expectNCalls(2, "XAie_EventGenerate")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = test_aie_generate_intr(devInst);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by test_aie_generate_intr function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == 0);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_CheckbitOnPostAIEML
*		Function Name: test_aie_generate_intr
*		Input: devInst
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AieGenerateIntrTests, TC_003_CheckbitOnPostAIEML) {
	const u8 seq[] = {(u8)XAIEGBL_TILE_TYPE_MEMTILE, 99, 99, 99, 99};

	Stub_XAie_SetTTypeCallSequence(seq, 5U);
	devInst->DevProp.DevGen = (u8)XAIE_DEV_GEN_AIE2IPU;
	devInst->DevProp.RowShift = 0U;
	devInst->DevProp.ColShift = 0U;
	mock_c()->expectOneCall("XAie_PmRequestTiles")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->withPointerParameters("DevInst", devInst)->andReturnIntValue(0);
	mock_c()->expectNCalls(1, "XAie_EventGenerate")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()
		->expectOneCall("XAie_Write32")
		->withPointerParameters("DevInst", devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = test_aie_generate_intr(devInst);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by test_aie_generate_intr function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == 0);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_CheckbitOnPostAIEML_NonMemTile
*		Function Name: test_aie_generate_intr
*		Input: devInst
*		Expectation: result == 0
*		Test Method: Unit testing
*		Methods for deriving test case: Analysis of requirement
*		Test Environment: 2
*/
/*********************************************************************************/
TEST(AieGenerateIntrTests, TC_004_CheckbitOnPostAIEML_NonMemTile) {
	const u8 seq[] = {(u8)XAIEGBL_TILE_TYPE_SHIMPL, 99, 99, 99, 99};

	Stub_XAie_SetTTypeCallSequence(seq, 5U);
	devInst->DevProp.DevGen = (u8)XAIE_DEV_GEN_AIE2IPU;
	devInst->DevProp.RowShift = 0U;
	devInst->DevProp.ColShift = 0U;
	mock_c()->expectOneCall("XAie_PmRequestTiles")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->withPointerParameters("DevInst", devInst)->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_EventGenerate")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = test_aie_generate_intr(devInst);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by test_aie_generate_intr function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == 0);
}

/*@section Test Execution details
| Test Case ID                               | Test Result | CR  | Impact | Test Details |
|--------------------------------------------|-------------|-----|--------|--------------|
| TC_001_AllShimTiles                        | PASS        | No  | NA     | NA           |
| TC_002_AllMemTiles                         | PASS        | No  | NA     | NA           |
| TC_003_AllAIETiles                         | PASS        | No  | NA     | NA           |
| TC_004_AllInvalidTileTypes                 | PASS        | No  | NA     | NA           |
| TC_005_AllShimPLTiles                      | PASS        | No  | NA     | NA           |
| TC_006_ShimHelperTileTypeMismatch          | PASS        | No  | NA     | NA           |
| TC_007_AieCoreAndMemHelperTileTypeMismatch | PASS        | No  | NA     | NA           |
| TC_008_MemHelperTileTypeMismatch           | PASS        | No  | NA     | NA           |
| TC_001_FailureCases                        | PASS        | No  | NA     | NA           |
| TC_002_SuccessWithInvalidTileType          | PASS        | No  | NA     | NA           |
| TC_003_CheckbitOnPostAIEML                 | PASS        | No  | NA     | NA           |
| TC_004_CheckbitOnPostAIEML_NonMemTile      | PASS        | No  | NA     | NA           |

/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)        | %      | Result     | CR/Status    | Impact          |
|--------------------------------------|--------|------------|--------------|-----------------|
| generate_events_on_col               | <100>  | <Pass>     | <NA>         | No Impact       |
| test_aie_generate_intr               | <100>  | <Pass>     | <NA>         | No Impact       |
| AegError                             | <100>  | <Pass>     | <NA>         | No Impact       |
| AegLog                               | <100>  | <Pass>     | <NA>         | No Impact       |
| AegWaitForContinue                   | <100>  | <Pass>     | <NA>         | No Impact       |
| generate_mem_tile_event              | <100>  | <Pass>     | <NA>         | No Impact       |
| generate_shim_tile_event             | <100>  | <Pass>     | <NA>         | No Impact       |
| aeg_get_event_from_ordinal           | <100>  | <Pass>     | <NA>         | No Impact       |
| generate_aie_tile_mem_event          | <100>  | <Pass>     | <NA>         | No Impact       |
| generate_user_event1_on_col          | <100>  | <Pass>     | <NA>         | No Impact       |
| generate_aie_tile_core_event         | <100>  | <Pass>     | <NA>         | No Impact       |
| generate_mem_tile_checkbit_error_generation| <100>  | <Pass>     | <NA>         | No Impact       |
| generate_mem_tile_checkbit_error_generation_col| <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)        | 100    | <Pass>     | <NA>         | No Impact       |

/*@section Uncovered details*/
/* No uncovered details */
