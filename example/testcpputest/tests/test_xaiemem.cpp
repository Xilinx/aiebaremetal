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
* @file test_xaiemem.cpp
*  This file contains test cases of AEG Memory API functions
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	  Who	   Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera      05/03/2026  Initial creation
* 1.1	 Veera      20/04/2026  Added test cases for AEG Memory API functions
* 1.2	 Veera      10/05/2026  updated comment section for AEG Memory API functions
* 1.3	 Veera      26/05/2026  updated timing analysis for XAieMem API functions
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
#include "xaiemem.h"
#include <chrono>
#include <iostream>
TEST_GROUP(XAieMemTests) {

	XAie_DevInst devInst;
	XAieMem *memInstance;

	void setup() {

		devInst.NumRows = 8;
		devInst.StartCol = 0;
		devInst.NumCols = 4;


		memInstance = new XAieMem(&devInst);
	}

	void teardown() {
		delete memInstance;

		mock_c()->clear();
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: TC_001_SuccessfulMemAllocation
*		Function Name: XAieMem::malloc
*		Input: testSize = 1024
*		Expectation: result != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_001_SuccessfulMemAllocation) {
	XAie_MemInst mockMemInst;
	int Cache = 1;
	void *result = nullptr;
	size_t testSize = 1024;
	mock_c()
		->expectOneCall("XAie_MemAllocate")
		->withPointerParameters("DevInst", &devInst)
		->withUnsignedLongLongIntParameters("Size", testSize)
		->withIntParameters("Cache", Cache)
		->andReturnPointerValue((void *)&mockMemInst);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	result = memInstance->malloc(testSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by malloc function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result != nullptr);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_MemAllocationFailure
*		Function Name: XAieMem::malloc
*		Input: testSize = 0
*		Expectation: result == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_002_MemAllocationFailure) {
	XAie_MemInst mockMemInst;
	int Cache = 1;
	void *result = nullptr;
	size_t testSize = 0;

	mock_c()
		->expectOneCall("XAie_MemAllocate")
		->withPointerParameters("DevInst", &devInst)
		->withUnsignedLongLongIntParameters("Size", testSize)
		->withIntParameters("Cache", Cache)
		->andReturnPointerValue(nullptr);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	result = memInstance->malloc(testSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by malloc function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == nullptr);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_NullDevInstForMemAllocation
*		Function Name: XAieMem::malloc
*		Input: testSize = 100
*		Expectation: result == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_003_NullDevInstForMemAllocation) {
	XAie_MemInst mockMemInst;
	int Cache = 1;
	void *result = nullptr;
	size_t testSize = 100;
	memset(&devInst, 0x0, sizeof(XAie_DevInst));

	mock_c()
		->expectOneCall("XAie_MemAllocate")
		->withPointerParameters("DevInst", &devInst)
		->withUnsignedLongLongIntParameters("Size", testSize)
		->withIntParameters("Cache", Cache)
		->andReturnPointerValue(nullptr);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	result = memInstance->malloc(testSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by malloc function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == nullptr);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_SuccessfulFree
*		Function Name: XAieMem::free
*		Input: TestAddress = malloc(1024)
*		Expectation: result == XAIE_OK
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_004_SuccessfulFree) {
	void *TestAddress;
	TestAddress = malloc(1024);

	mock_c()
		->expectOneCall("XAie_MemFreeVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = memInstance->free(TestAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by free function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(XAIE_OK, result);


	free(TestAddress);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_FreeReturnValueCheck
*		Function Name: XAieMem::free
*		Input: TestAddress = malloc(1024)
*		Expectation: result == testReturnValue
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_005_FreeReturnValueCheck) {
	void *TestAddress;
	TestAddress = malloc(1024);
	int testReturnValue = 42;

	mock_c()
		->expectOneCall("XAie_MemFreeVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->andReturnIntValue(testReturnValue);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = memInstance->free(TestAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by free function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(testReturnValue, result);

	free(TestAddress);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_NullDevInstForFree
*		Function Name: XAieMem::free
*		Input: TestAddress = malloc(1024)
*		Expectation: result == XAIE_ERR
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_006_NullDevInstForFree) {
	void *TestAddress;
	TestAddress = malloc(1024);
	memset(&devInst, 0x0, sizeof(XAie_DevInst));

	mock_c()
		->expectOneCall("XAie_MemFreeVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->andReturnIntValue(XAIE_ERR);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = memInstance->free(TestAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by free function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(XAIE_ERR, result);

	free(TestAddress);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_SuccessfulSyncForCpu
*		Function Name: XAieMem::sync_for_cpu
*		Input: TestAddress = malloc(1024) , testSize = 1024
*		Expectation: result == XAIE_OK
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_007_SuccessfulSyncForCpu) {
	void *TestAddress;
	TestAddress = malloc(1024);
	uint64_t testSize = 1024;
	memset(&devInst, 0xaa, sizeof(XAie_DevInst));

	mock_c()
		->expectOneCall("XAie_MemSyncForCPUVAddr")
		->ignoreOtherParameters()
		->withPointerParameters("VAddr", TestAddress)
		->withUnsignedLongLongIntParameters("size", testSize)
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = memInstance->sync_for_cpu(TestAddress, testSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by sync_for_cpu function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(XAIE_OK, result);

	free(TestAddress);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_NullDevInstForSyncForCpu
*		Function Name: XAieMem::sync_for_cpu
*		Input: TestAddress = malloc(1024) , testSize = 1024
*		Expectation: result == XAIE_ERR
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_008_NullDevInstForSyncForCpu) {
	void *TestAddress;
	TestAddress = malloc(1024);
	uint64_t testSize = 1024;
	memset(&devInst, 0x0, sizeof(XAie_DevInst));

	mock_c()
		->expectOneCall("XAie_MemSyncForCPUVAddr")
		->ignoreOtherParameters()
		->withPointerParameters("VAddr", TestAddress)
		->withUnsignedLongLongIntParameters("size", testSize)
		->andReturnIntValue(XAIE_ERR);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = memInstance->sync_for_cpu(TestAddress, testSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by sync_for_cpu function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(XAIE_ERR, result);

	// Clean up
	free(TestAddress);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_SuccessfulSyncForDev
*		Function Name: XAieMem::sync_for_dev
*		Input: TestAddress = malloc(1024) , testSize = 1024
*		Expectation: result == XAIE_OK
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_009_SuccessfulSyncForDev) {
	void *TestAddress;
	TestAddress = malloc(1024);
	uint64_t testSize = 1024;
	memset(&devInst, 0xaa, sizeof(XAie_DevInst));

	mock_c()
		->expectOneCall("XAie_MemSyncForDevVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->withUnsignedLongLongIntParameters("size", testSize)
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = memInstance->sync_for_dev(TestAddress, testSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by sync_for_dev function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(XAIE_OK, result);

	free(TestAddress);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_010_NullDevInstForSyncForDev
*		Function Name: XAieMem::sync_for_dev
*		Input: TestAddress = malloc(1024) , testSize = 1024
*		Expectation: result == XAIE_ERR
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_010_NullDevInstForSyncForDev) {
	void *TestAddress;
	TestAddress = malloc(1024);
	uint64_t testSize = 1024;
	memset(&devInst, 0x0, sizeof(XAie_DevInst));

	mock_c()
		->expectOneCall("XAie_MemSyncForDevVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->withUnsignedLongLongIntParameters("size", testSize)
		->andReturnIntValue(XAIE_ERR);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = memInstance->sync_for_dev(TestAddress, testSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by sync_for_dev function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(XAIE_ERR, result);

	free(TestAddress);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_011_SuccessfulGetDevAddr
*		Function Name: XAieMem::get_dev_addr
*		Input: TestAddress = malloc(1024) , DevAddr = malloc(sizeof(uint64_t))
*		Expectation: result == XAIE_OK
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_011_SuccessfulGetDevAddr) {
	void *TestAddress;
	uint64_t *DevAddr;

	TestAddress = malloc(1024);
	DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	CHECK(DevAddr != nullptr);

	devInst.NumRows = 2;

	mock_c()
		->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->withPointerParameters("DevAddr", DevAddr)
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = memInstance->get_dev_addr(TestAddress, DevAddr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_dev_addr function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(XAIE_OK, result);

	free(TestAddress);
	free(DevAddr);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_012_NullDevInstForGetDevAddr
*		Function Name: XAieMem::get_dev_addr
*		Input: TestAddress = malloc(1024) , DevAddr = malloc(sizeof(uint64_t))
*		Expectation: result == XAIE_ERR
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(XAieMemTests, TC_012_NullDevInstForGetDevAddr) {
	void *TestAddress;
	uint64_t *DevAddr;

	TestAddress = malloc(1024);
	DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	CHECK(DevAddr != nullptr);

	memset(&devInst, 0x0, sizeof(XAie_DevInst));

	mock_c()
		->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->withPointerParameters("DevAddr", DevAddr) // Add OutputParam to mock
		->andReturnIntValue(XAIE_ERR);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = memInstance->get_dev_addr(TestAddress, DevAddr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_dev_addr function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(XAIE_ERR, result);

	free(TestAddress);
	free(DevAddr);
}
/*@section Test Execution details
| Test Case ID                         | Test Result | CR  | Impact |
|--------------------------------------|-------------|-----|--------|
| TC_001_SuccessfulMemAllocation       | PASS        | NA  | NA     |
| TC_002_MemAllocationFailure          | PASS        | NA  | NA     |
| TC_003_NullDevInstForMemAllocation   | PASS        | NA  | NA     |
| TC_004_SuccessfulFree                | PASS        | NA  | NA     |
| TC_005_FreeReturnValueCheck          | PASS        | NA  | NA     |
| TC_006_NullDevInstForFree            | PASS        | NA  | NA     |
| TC_007_SuccessfulSyncForCpu          | PASS        | NA  | NA     |
| TC_008_NullDevInstForSyncForCpu      | PASS        | NA  | NA     |
| TC_009_SuccessfulSyncForDev          | PASS        | NA  | NA     |
| TC_010_NullDevInstForSyncForDev      | PASS        | NA  | NA     |
| TC_011_SuccessfulGetDevAddr          | PASS        | NA  | NA     |
| TC_012_NullDevInstForGetDevAddr      | PASS        | NA  | NA     |

/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)        | %      | Result     | CR/Status    | Impact          |
|--------------------------------------|--------|------------|--------------|-----------------|
| XAieMem::malloc                      | <100>  | <Pass>     | <NA>         | No Impact       |
| XAieMem::free                        | <100>  | <Pass>     | <NA>         | No Impact       |
| XAieMem::sync_for_cpu                | <100>  | <Pass>     | <NA>         | No Impact       |
| XAieMem::sync_for_dev                | <100>  | <Pass>     | <NA>         | No Impact       |
| XAieMem::get_dev_addr                | <100>  | <Pass>     | <NA>         | No Impact       |
| XAieMem::XAieMem                     | <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)        | 100    | <Pass>     | <NA>         | No Impact       |

/*@section Uncovered details*/
/* No uncovered details */
