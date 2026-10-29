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
* @file test_fal_util.cpp
*  This file contains test cases for FAL utility functions
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	 Who	 Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera	 05/03/2026	 Initial creation
* 1.1	 Veera   10/05/2026  updated comment section for FAL utility functions
* 1.2	 Veera   26/05/2026  updated timing analysis for FAL utility functions
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
#include "fal_util.h"
#include "stubs.h"
#include "test_comparator.h"
#include <xaiegbl.h>
#include <chrono>
#include <iostream>

#include "profiling_stubs/xaiefal_mock.hpp"
using namespace xaiefal;

#include <memory>

TEST_GROUP(FalUtilInitializeTests){
	void setup(){mock_c()->clear();
	}

	void teardown() { mock_c()->clear(); }
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_InitializeWithNullPointer
*		Function Name: fal_util::initialize
*		Input: pDevInst = nullptr
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_001_InitializeWithNullPointer) {
	XAie_DevInst *pDevInst = nullptr;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	bool result = fal_util::initialize(pDevInst);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by initialize function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_FALSE(result);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_002_InitializeWithValidPointer
*		Function Name: fal_util::initialize
*		Input: dev = {}
*		Expectation: result == true, fal_util::s_pXAieDev != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_002_InitializeWithValidPointer) {
	XAie_DevInst dev{};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	bool result = fal_util::initialize(&dev);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by initialize function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(result);
	CHECK(fal_util::s_pXAieDev != nullptr);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_003_ReleaseNullResourceFailure
*		Function Name: fal_util::release
*		Input: resource = nullptr
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_003_ReleaseNullResourceFailure) {
	std::shared_ptr<xaiefal::XAieRsc> resource = nullptr;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	bool result = fal_util::release(resource);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by release function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_FALSE(result);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_004_ReleaseSuccess
*		Function Name: fal_util::release
*		Input: r = StubXaie_CreateGenericRsc()
*		Expectation: result == true, StubXaieRsc_GetReleaseCallCount(r) == 1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_004_ReleaseSuccess) {
	auto r = StubXaie_CreateGenericRsc();
	StubXaieRsc_SetReleaseResult(r, XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	bool result = fal_util::release(r);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by release function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(result);
	LONGS_EQUAL(1, StubXaieRsc_GetReleaseCallCount(r));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_ReleaseFailure
*		Function Name: fal_util::release
*		Input: r = StubXaie_CreateGenericRsc()
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_005_ReleaseFailure) {
	auto r = StubXaie_CreateGenericRsc();
	StubXaieRsc_SetReleaseResult(r, StubXaieRc_ErrorCode());
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	bool result = fal_util::release(r);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by release function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_FALSE(result);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_006_RequestNullResourceFailure
*		Function Name: fal_util::request
*		Input: resource = nullptr
*		Expectation: result == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_006_RequestNullResourceFailure) {
	std::shared_ptr<xaiefal::XAieRsc> resource = nullptr;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = fal_util::request(resource);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by request function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == -1);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_RequestBroadcastSuccess
*		Function Name: fal_util::request
*		Input: bc = StubXaie_CreateBroadcast()
*		Expectation: result == 5
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_007_RequestBroadcastSuccess) {
	auto bc = StubXaie_CreateBroadcast();
	StubXaieBroadcast_SetReserveResult(bc, XAIE_OK);
	StubXaieBroadcast_SetBcValue(bc, 5);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int id = fal_util::request(bc);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by request function: " << total_clock_enable_us << " microseconds" << std::endl;
	LONGS_EQUAL(5, id);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_RequestBroadcastFailure
*		Function Name: fal_util::request
*		Input: bc = StubXaie_CreateBroadcast()
*		Expectation: result == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_008_RequestBroadcastFailure) {
	auto bc = StubXaie_CreateBroadcast();
	StubXaieBroadcast_SetReserveResult(bc, StubXaieRc_ErrorCode());
	StubXaieBroadcast_SetBcValue(bc, 7);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int id = fal_util::request(bc);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by request function: " << total_clock_enable_us << " microseconds" << std::endl;
	LONGS_EQUAL(-1, id);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_009_RequestSingleTileSuccess
*		Function Name: fal_util::request
*		Input: r = StubXaie_CreateSingleTile()
*		Expectation: result == 42, StubXaieSingleTile_GetReserveCallCount(r) == 1, StubXaieSingleTile_GetGetRscIdCallCount(r) == 1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_009_RequestSingleTileSuccess) {
	auto r = StubXaie_CreateSingleTile();
	StubXaieSingleTile_SetReserveResult(r, XAIE_OK);
	StubXaieSingleTile_SetGetRscIdResult(r, XAIE_OK, 42);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int id = fal_util::request(r);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by request function: " << total_clock_enable_us << " microseconds" << std::endl;
	LONGS_EQUAL(42, id);
	LONGS_EQUAL(1, StubXaieSingleTile_GetReserveCallCount(r));
	LONGS_EQUAL(1, StubXaieSingleTile_GetGetRscIdCallCount(r));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_010_RequestSingleTileReserveFailure
*		Function Name: fal_util::request
*		Input: r = StubXaie_CreateSingleTile()
*		Expectation: result == -1, StubXaieSingleTile_GetGetRscIdCallCount(r) == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_010_RequestSingleTileReserveFailure) {
	auto r = StubXaie_CreateSingleTile();
	StubXaieSingleTile_SetReserveResult(r, StubXaieRc_ErrorCode());
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int id = fal_util::request(r);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by request function: " << total_clock_enable_us << " microseconds" << std::endl;
	LONGS_EQUAL(-1, id);
	LONGS_EQUAL(0, StubXaieSingleTile_GetGetRscIdCallCount(r));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_011_RequestSingleTileGetRscIdFailure
*		Function Name: fal_util::request
*		Input: r = StubXaie_CreateSingleTile()
*		Expectation: result == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_011_RequestSingleTileGetRscIdFailure) {
	auto r = StubXaie_CreateSingleTile();
	StubXaieSingleTile_SetReserveResult(r, XAIE_OK);
	StubXaieSingleTile_SetGetRscIdResult(r, StubXaieRc_ErrorCode(), 13);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int id = fal_util::request(r);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by request function: " << total_clock_enable_us << " microseconds" << std::endl;
	LONGS_EQUAL(-1, id);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_012_RequestUnsupportedResourceType
*		Function Name: fal_util::request
*		Input: r = StubXaie_CreateGenericRsc()
*		Expectation: result == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(FalUtilInitializeTests, TC_012_RequestUnsupportedResourceType) {
	auto r = StubXaie_CreateGenericRsc();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int id = fal_util::request(r);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by request function: " << total_clock_enable_us << " microseconds" << std::endl;
	LONGS_EQUAL(-1, id);
}

/*@section Test Execution details
| Test Case ID                              | Test Result | CR  | Impact | Test Details |
|-------------------------------------------|-------------|-----|--------|--------------|
| TC_001_InitializeWithNullPointer          | PASS        | No  | NA     | NA           |
| TC_002_InitializeWithValidPointer         | PASS        | No  | NA     | NA           |
| TC_003_ReleaseNullResourceFailure         | PASS        | No  | NA     | NA           |
| TC_004_ReleaseSuccess                     | PASS        | No  | NA     | NA           |
| TC_005_ReleaseFailure                     | PASS        | No  | NA     | NA           |
| TC_006_RequestNullResourceFailure         | PASS        | No  | NA     | NA           |
| TC_007_RequestBroadcastSuccess            | PASS        | No  | NA     | NA           |
| TC_008_RequestBroadcastFailure            | PASS        | No  | NA     | NA           |
| TC_009_RequestSingleTileSuccess           | PASS        | No  | NA     | NA           |
| TC_010_RequestSingleTileReserveFailure    | PASS        | No  | NA     | NA           |
| TC_011_RequestSingleTileGetRscIdFailure   | PASS        | No  | NA     | NA           |
| TC_012_RequestUnsupportedResourceType     | PASS        | No  | NA     | NA           |

/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)        | %      | Result     | CR/Status    | Impact          |
|--------------------------------------|--------|------------|--------------|-----------------|
| fal_util::initialize                 | <100>  | <Pass>     | <NA>         | No Impact       |
| fal_util::release                    | <100>  | <Pass>     | <NA>         | No Impact       |
| fal_util::request                    | <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)        | 100    | <Pass>     | <NA>         | No Impact       |

/*@section Uncovered details*/
/* No uncovered details */
