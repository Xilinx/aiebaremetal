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
* @file test_externalbufferapiimpl.cpp
*  This file contains test cases of External Buffer API functions
*
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	  Who	   Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera      05/03/2026  Initial creation
* 1.1	 Veera      20/04/2026  Added test cases for External Buffer API functions
* 1.2	 Veera      10/05/2026  updated comment section for External Buffer API functions
* 1.3	 Veera      26/05/2026  updated timing analysis for External Buffer API functions
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

#ifdef new
#undef new
#endif
#ifdef node
#undef node
#endif
#ifdef __node
#undef __node
#endif
//#define __AIESIM__ 1
#include "aeg_baremetal_api.h"
#include "aiebaremetal.h"


#include "externalbufferapiimpl.h"

#include "stubs.h"
#include "test_comparator.h"
#include <chrono>
#include <cstdint>
#include <iostream>

TEST_GROUP(ExternalBufferApiImplBasicTests) {

	std::shared_ptr<XAieMem> xaiemem;
	abr::external_buffer_config config;
	std::string testName;
	ExternalBufferApiImpl *uut;


	void setup() {
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);

		config = {};
		testName = "TestBuffer";
		XAie_DevInst devInst;

		xaiemem = std::make_shared<XAieMem>(&devInst);

		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
		XAieMem::reset_test_state();
	}


	void teardown() {
		delete uut;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
		XAieMem::reset_test_state();
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: TC_001_SetAddress_Success
*		Function Name: setAddress
*		Input: testPtr = (void *)0x12345678
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplBasicTests, TC_001_SetAddress_Success) {

	void *testPtr = (void *)0x12345678;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uut->setAddress(testPtr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setAddress function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(true);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_002_SetAddress_AlreadySet
*		Function Name: setAddress
*		Input: initialPtr = (void *)0x12345678, newPtr = (void *)0x87654321
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplBasicTests, TC_002_SetAddress_AlreadySet) {

	void *initialPtr = (void *)0x12345678;
	void *newPtr = (void *)0x87654321;
	uut->setAddress(initialPtr);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uut->setAddress(newPtr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setAddress function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_003_Wait_EmptyShimPortConfigs
*		Function Name: ExternalBufferApiImpl::wait
*		Input: port_name = "invalid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplBasicTests, TC_003_Wait_EmptyShimPortConfigs) {

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->wait("invalid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_004_Wait_ValidPortNameMM2S
*		Function Name: ExternalBufferApiImpl::wait
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplBasicTests, TC_004_Wait_ValidPortNameMM2S) {

	abr::shim_port_config portConfig;
	portConfig.portName = "valid_port";
	portConfig.column = 1;
	portConfig.S2MMOrMM2S = 1;
	portConfig.channel = 1;
	config.shimPortConfigs.push_back(portConfig);


	delete uut;
	uut = new ExternalBufferApiImpl(config, testName, xaiemem);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->wait("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_005_Wait_ValidPortNameS2MM
*		Function Name: ExternalBufferApiImpl::wait
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplBasicTests, TC_005_Wait_ValidPortNameS2MM) {

	abr::shim_port_config portConfig;
	portConfig.portName = "valid_port";
	portConfig.column = 1;
	portConfig.S2MMOrMM2S = 0;
	portConfig.channel = 1;
	config.shimPortConfigs.push_back(portConfig);


	delete uut;
	uut = new ExternalBufferApiImpl(config, testName, xaiemem);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->wait("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_006_Wait_InvalidConfig
*		Function Name: ExternalBufferApiImpl::wait
*		Input: wait port_name = "invalid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplBasicTests, TC_006_Wait_InvalidConfig) {

	abr::shim_port_config portConfig;
	portConfig.portName = "valid_port";
	portConfig.column = 1;
	portConfig.S2MMOrMM2S = -1;
	portConfig.channel = 1;
	config.shimPortConfigs.push_back(portConfig);


	delete uut;
	uut = new ExternalBufferApiImpl(config, testName, xaiemem);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->wait("invalid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_Wait_InvalidPort_WithConfiguredPorts
*		Function Name: ExternalBufferApiImpl::wait
*		Input: wait port_name = "missing_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplBasicTests, TC_007_Wait_InvalidPort_WithConfiguredPorts) {
	abr::shim_port_config portConfig;
	portConfig.portName = "configured_port";
	portConfig.column = 1;
	portConfig.S2MMOrMM2S = 0;
	portConfig.channel = 1;
	config.shimPortConfigs.push_back(portConfig);

	delete uut;
	uut = new ExternalBufferApiImpl(config, testName, xaiemem);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->wait("missing_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

TEST_GROUP(ExternalBufferApiImplPingPongTests) {

	std::shared_ptr<XAieMem> xaiemem;
	abr::external_buffer_config config;
	std::string testName;
	ExternalBufferApiImpl *uut;
	void *pingAddress;
	void *pongAddress;

	void setup() {

		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);

		config = {};
		testName = "TestBuffer";


		pingAddress = (void *)0x12345678;
		pongAddress = (void *)0x87654321;


		XAie_DevInst devInst;

		xaiemem = std::make_shared<XAieMem>(&devInst);


		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
		XAieMem::reset_test_state();
	}

	void teardown() {
		delete uut;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
		XAieMem::reset_test_state();
	}

	void setupCommonPortConfig(bool isS2MM) {
		abr::shim_port_config portConfig;
		portConfig.portName = "valid_port";
		portConfig.column = 1;
		portConfig.S2MMOrMM2S = isS2MM ? 0 : 1;
		portConfig.channel = 1;
		portConfig.taskRepetition = 1;
		portConfig.enable_task_complete_token = false;


		abr::shimBDInfo bdInfo1;
		bdInfo1.bd_id = 2;
		bdInfo1.buf_idx = 0;
		bdInfo1.offset = 0;
		bdInfo1.transaction_size = 1024;
		portConfig.shimBDInfos.push_back(bdInfo1);

		abr::shimBDInfo bdInfo2;
		bdInfo2.bd_id = 3;
		bdInfo2.buf_idx = 1;
		bdInfo2.offset = 0;
		bdInfo2.transaction_size = 1024;
		portConfig.shimBDInfos.push_back(bdInfo2);

		config.shimPortConfigs.push_back(portConfig);


		delete uut;
		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_SetupPingPongBuffers_Success
*		Function Name: setupPingPongBuffers
*		Input: ptr1 = (void *)0x12345678, ptr2 = (void *)0x87654321
*		Expectation: result = abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplPingPongTests, TC_001_SetupPingPongBuffers_Success) {

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;


	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_002_SetupPingPongBuffers_AlreadySet
*		Function Name: setupPingPongBuffers
*		Input: newPingAddress = (void *)0x11223344, newPongAddress = (void *)0x55667788
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplPingPongTests, TC_002_SetupPingPongBuffers_AlreadySet) {

	uut->setupPingPongBuffers(pingAddress, pongAddress);


	void *newPingAddress = (void *)0x11223344;
	void *newPongAddress = (void *)0x55667788;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->setupPingPongBuffers(newPingAddress, newPongAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_003_SetupPingPongBuffers_PartialNullState
*		Function Name: setupPingPongBuffers
*		Input: newPingAddress = nullptr, newPongAddress = (void *)0x87654321
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplPingPongTests, TC_003_SetupPingPongBuffers_PartialNullState) {
	(void)uut->setupPingPongBuffers(nullptr, pongAddress);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_004_SetupPingPongBuffers_Success
*		Function Name: setupPingPongBuffers
*		Input: ptr1 = (void *)0x12345678, ptr2 = (void *)0x87654321
*		Expectation: result = abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplPingPongTests, TC_004_SetupPingPongBuffers_Success) {

	setupCommonPortConfig(true);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_SetupPingPongBuffers_UpdateBDAddress_ErrorPath
*		Function Name: setupPingPongBuffers
*		Input: pingAddress = (void *)0x12345678, pongAddress = (void *)0x87654321
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplPingPongTests, TC_005_SetupPingPongBuffers_UpdateBDAddress_ErrorPath) {
	setupCommonPortConfig(true);
	g_ut_externalbuffer_force_dma_update_bd_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_externalbuffer_force_dma_update_bd_fail = 0;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

TEST_GROUP(ExternalBufferApiImplDmaTests) {
	std::shared_ptr<XAieMem> xaiemem;
	abr::external_buffer_config config;
	std::string testName;
	ExternalBufferApiImpl *uut;
	void *testAddress;

	void setup() {
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		config = {};
		testName = "TestBuffer";
		testAddress = (void *)0x12345678;

		XAie_DevInst devInst;
		xaiemem = std::make_shared<XAieMem>(&devInst);

		uut = new ExternalBufferApiImpl(config, testName, xaiemem);

		uut->setAddress(testAddress);
		XAieMem::reset_test_state();
	}

	void teardown() {
		delete uut;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
		XAieMem::reset_test_state();
	}

	void setupCommonPortConfig(bool isS2MM) {
		abr::shim_port_config portConfig;
		portConfig.portName = "valid_port";
		portConfig.column = 1;
		portConfig.S2MMOrMM2S = isS2MM ? 0 : 1;
		portConfig.channel = 1;
		portConfig.taskRepetition = 1;
		portConfig.enable_task_complete_token = false;


		abr::shimBDInfo bdInfo;
		bdInfo.bd_id = 2;
		bdInfo.buf_idx = 0;
		bdInfo.offset = 0;
		bdInfo.transaction_size = 1024;
		portConfig.shimBDInfos.push_back(bdInfo);

		config.shimPortConfigs.push_back(portConfig);


		delete uut;
		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
		uut->setAddress(testAddress);
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: TC_001_GM2AIE_NB_EmptyShimPortConfigs
*		Function Name: gm2aie_nb
*		Input: port_name = "test_port"
*		Expectation: result = abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_001_GM2AIE_NB_EmptyShimPortConfigs) {

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->gm2aie_nb("test_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_002_GM2AIE_NB_InvalidPortName
*		Function Name: gm2aie_nb
*		Input: port_name = "invalid_port"
*		Expectation: result = abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_002_GM2AIE_NB_InvalidPortName) {
	setupCommonPortConfig(true);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->gm2aie_nb("invalid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_003_GM2AIE_NB_Success
*		Function Name: gm2aie_nb
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_003_GM2AIE_NB_Success) {
	setupCommonPortConfig(false);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->gm2aie_nb("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();

	std::cout << "Time taken by gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_004_GM2AIE_NB_Failure
*		Function Name: gm2aie_nb
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_004_GM2AIE_NB_Failure) {

	setupCommonPortConfig(false);

	XAieMem::s_get_dev_addr_ret = 1;
	g_ut_externalbuffer_force_dma_enqueue_fail = 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->gm2aie_nb("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_externalbuffer_force_dma_enqueue_fail = 0;


	CHECK_EQUAL(static_cast<int>(abr::err_code::aie_driver_error), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_005_AIE2GM_NB_EmptyShimPortConfigs
*		Function Name: aie2gm_nb
*		Input: port_name = "test_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_005_AIE2GM_NB_EmptyShimPortConfigs) {

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->aie2gm_nb("test_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_006_AIE2GM_NB_InvalidPortName
*		Function Name: aie2gm_nb
*		Input: port_name = "invalid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_006_AIE2GM_NB_InvalidPortName) {
	setupCommonPortConfig(false);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->aie2gm_nb("invalid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_007_AIE2GM_NB_UpdateBDAddressSuccess
*		Function Name: aie2gm_nb
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_007_AIE2GM_NB_UpdateBDAddressSuccess) {
	setupCommonPortConfig(true);
	XAieMem::s_get_dev_addr_ret = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->aie2gm_nb("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_008_AIE2GM_NB_UpdateBDAddressFails
*		Function Name: aie2gm_nb
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_008_AIE2GM_NB_UpdateBDAddressFails) {
	setupCommonPortConfig(true);
	g_ut_externalbuffer_force_dma_enqueue_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->aie2gm_nb("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_externalbuffer_force_dma_enqueue_fail = 0;
	CHECK_EQUAL(static_cast<int>(abr::err_code::aie_driver_error), static_cast<int>(result));
}

//*********************************************************************************/
/*		Unique test case ID: TC_009_GM2AIE_NB_UpdateBDAddress_ErrorPath
*		Function Name: gm2aie_nb
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_009_GM2AIE_NB_UpdateBDAddress_ErrorPath) {
	setupCommonPortConfig(false);
	g_ut_externalbuffer_force_dma_update_bd_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->gm2aie_nb("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_externalbuffer_force_dma_update_bd_fail = 0;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*		Unique test case ID: TC_010_AIE2GM_NB_UpdateBDAddress_ErrorPath
*		Function Name: aie2gm_nb
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_010_AIE2GM_NB_UpdateBDAddress_ErrorPath) {
	setupCommonPortConfig(true);
	g_ut_externalbuffer_force_dma_update_bd_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->aie2gm_nb("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_externalbuffer_force_dma_update_bd_fail = 0;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*		Unique test case ID: TC_011_GM2AIE
*		Function Name: gm2aie
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_011_GM2AIE) {
	setupCommonPortConfig(false);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->gm2aie("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*		Unique test case ID: TC_012_AIE2GM
*		Function Name: aie2gm
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_012_AIE2GM) {
	setupCommonPortConfig(true);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->aie2gm("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*		Unique test case ID: TC_013_GM2AIE_NB_MultipleBds
*		Function Name: gm2aie_nb
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_013_GM2AIE_NB_MultipleBds) {
	abr::shim_port_config portConfig;
	portConfig.portName = "valid_port";
	portConfig.column = 1;
	portConfig.S2MMOrMM2S = 1;
	portConfig.channel = 1;
	portConfig.taskRepetition = 1;
	portConfig.enable_task_complete_token = false;
	abr::shimBDInfo bdInfo1; bdInfo1.bd_id = 2; bdInfo1.buf_idx = 0; bdInfo1.offset = 0; bdInfo1.transaction_size = 1024;
	abr::shimBDInfo bdInfo2; bdInfo2.bd_id = 3; bdInfo2.buf_idx = 0; bdInfo2.offset = 64; bdInfo2.transaction_size = 1024;
	portConfig.shimBDInfos.push_back(bdInfo1);
	portConfig.shimBDInfos.push_back(bdInfo2);
	config.shimPortConfigs.push_back(portConfig);

	delete uut;
	uut = new ExternalBufferApiImpl(config, testName, xaiemem);
	uut->setAddress(testAddress);


	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->gm2aie_nb("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_014_AIE2GM_NB_MultipleBds
*		Function Name: aie2gm_nb
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_014_AIE2GM_NB_MultipleBds) {
	abr::shim_port_config portConfig;
	portConfig.portName = "valid_port";
	portConfig.column = 1;
	portConfig.S2MMOrMM2S = 0;
	portConfig.channel = 1;
	portConfig.taskRepetition = 1;
	portConfig.enable_task_complete_token = false;
	abr::shimBDInfo bdInfo1; bdInfo1.bd_id = 2; bdInfo1.buf_idx = 0; bdInfo1.offset = 0; bdInfo1.transaction_size = 1024;
	abr::shimBDInfo bdInfo2; bdInfo2.bd_id = 3; bdInfo2.buf_idx = 0; bdInfo2.offset = 64; bdInfo2.transaction_size = 1024;
	portConfig.shimBDInfos.push_back(bdInfo1);
	portConfig.shimBDInfos.push_back(bdInfo2);
	config.shimPortConfigs.push_back(portConfig);

	delete uut;
	uut = new ExternalBufferApiImpl(config, testName, xaiemem);
	uut->setAddress(testAddress);


	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->aie2gm_nb("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_015_GM2AIE_Failure
*		Function Name: gm2aie
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_015_GM2AIE_Failure) {
	setupCommonPortConfig(false);
	g_ut_externalbuffer_force_dma_enqueue_fail = 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->gm2aie("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;

	g_ut_externalbuffer_force_dma_enqueue_fail = 0;
	CHECK_EQUAL(static_cast<int>(abr::err_code::aie_driver_error), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_016_AIE2GM_Failure
*		Function Name: aie2gm
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_016_AIE2GM_Failure) {
	setupCommonPortConfig(true);
	g_ut_externalbuffer_force_dma_enqueue_fail = 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->aie2gm("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm function: " << total_clock_enable_us << " microseconds" << std::endl;

	g_ut_externalbuffer_force_dma_enqueue_fail = 0;
	CHECK_EQUAL(static_cast<int>(abr::err_code::aie_driver_error), static_cast<int>(result));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_017_GM2AIE_NB_GetAddressFailure_Continues
*		Function Name: gm2aie_nb
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_017_GM2AIE_NB_GetAddressFailure_Continues) {
	setupCommonPortConfig(false);
	XAieMem::s_get_dev_addr_ret = 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->gm2aie_nb("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	XAieMem::s_get_dev_addr_ret = 0;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_018_AIE2GM_NB_GetAddressFailure_Continues
*		Function Name: aie2gm_nb
*		Input: port_name = "valid_port"
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_018_AIE2GM_NB_GetAddressFailure_Continues) {
	setupCommonPortConfig(true);
	XAieMem::s_get_dev_addr_ret = 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->aie2gm_nb("valid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	XAieMem::s_get_dev_addr_ret = 0;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_019_GM2AIE_NB_InvalidPort_WithConfigs
*		Function Name: gm2aie_nb
*		Input: port_name = "no_port"
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_019_GM2AIE_NB_InvalidPort_WithConfigs) {
	setupCommonPortConfig(false);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->gm2aie_nb("no_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_020_AIE2GM_NB_InvalidPort_WithConfigs
*		Function Name: aie2gm_nb
*		Input: port_name = "no_port"
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_020_AIE2GM_NB_InvalidPort_WithConfigs) {
	setupCommonPortConfig(true);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->aie2gm_nb("no_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

#if defined(__AIESIM__)
//*********************************************************************************/
/*		Unique test case ID: TC_021_Wait_Sim_DrainsGm2aieQueue_AfterGm2aieNb
*		Function Name: ExternalBufferApiImpl::wait
*		Input: gm2aie_nb("valid_port") followed by wait("valid_port")
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_021_Wait_Sim_DrainsGm2aieQueue_AfterGm2aieNb) {
	setupCommonPortConfig(false);

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(uut->gm2aie_nb("valid_port")));

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(uut->wait("valid_port")));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	mock_c()->checkExpectations();
}

//*********************************************************************************/
/*		Unique test case ID: TC_022_Wait_Sim_DrainsAie2gmQueue_AfterAie2gmNb
*		Function Name: ExternalBufferApiImpl::wait
*		Input: aie2gm_nb("valid_port") followed by wait("valid_port")
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_022_Wait_Sim_DrainsAie2gmQueue_AfterAie2gmNb) {
	setupCommonPortConfig(true);

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(uut->aie2gm_nb("valid_port")));

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(uut->wait("valid_port")));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	mock_c()->checkExpectations();
}

//*********************************************************************************/
/*		Unique test case ID: TC_023_Wait_Sim_DrainsGm2aieQueue_MultipleNb
*		Function Name: ExternalBufferApiImpl::wait
*		Input: two gm2aie_nb("valid_port") calls followed by wait("valid_port")
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplDmaTests, TC_023_Wait_Sim_DrainsGm2aieQueue_MultipleNb) {
	setupCommonPortConfig(false);

	for (int i = 0; i < 2; ++i) {
		CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(uut->gm2aie_nb("valid_port")));
	}

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(uut->wait("valid_port")));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	mock_c()->checkExpectations();
}
#endif // defined(__AIESIM__)

TEST_GROUP(ExternalBufferApiImplPingPongTestsforsim) {

	std::shared_ptr<XAieMem> xaiemem;
	abr::external_buffer_config config;
	std::string testName;
	ExternalBufferApiImpl *uut;
	void *pingAddress;
	void *pongAddress;
	void setup() {

		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);

		config = {};
		testName = "TestBuffer";


		pingAddress = (void *)0x12345678;
		pongAddress = (void *)0x87654321;

		XAie_DevInst devInst;
		xaiemem = std::make_shared<XAieMem>(&devInst);


		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
		XAieMem::reset_test_state();
	}

	void teardown() {
		delete uut;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
		XAieMem::reset_test_state();
	}

	void setupCommonPortConfig(bool isS2MM) {
		abr::shim_port_config portConfig;
		portConfig.portName = "valid_port";
		portConfig.column = 1;
		portConfig.S2MMOrMM2S = isS2MM ? 0 : 1;
		portConfig.channel = 1;
		portConfig.taskRepetition = 1;
		portConfig.enable_task_complete_token = false;


		abr::shimBDInfo bdInfo1;
		bdInfo1.bd_id = 2;
		bdInfo1.buf_idx = 0;
		bdInfo1.offset = 0;
		bdInfo1.transaction_size = 1024;
		portConfig.shimBDInfos.push_back(bdInfo1);

		abr::shimBDInfo bdInfo2;
		bdInfo2.bd_id = 3;
		bdInfo2.buf_idx = 1;
		bdInfo2.offset = 0;
		bdInfo2.transaction_size = 1024;
		portConfig.shimBDInfos.push_back(bdInfo2);

		config.shimPortConfigs.push_back(portConfig);


		delete uut;
		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
	}
};

//*********************************************************************************/
/*		Unique test case ID: TC_001_SetupPingPongBuffers_S2MM
*		Function Name: setupPingPongBuffers
*		Input: pingAddress = (void *)0x12345678, pongAddress = (void *)0x87654321
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplPingPongTestsforsim, TC_001_SetupPingPongBuffers_S2MM) {

	setupCommonPortConfig(true);


	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_002_SetupPingPongBuffers_MM2S
*		Function Name: setupPingPongBuffers
*		Input: pingAddress = (void *)0x12345678, pongAddress = (void *)0x87654321
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplPingPongTestsforsim, TC_002_SetupPingPongBuffers_MM2S) {

	setupCommonPortConfig(false);


	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;


	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_003_SetupPingPongBuffers_MM2S_Failure
*		Function Name: setupPingPongBuffers
*		Input: pingAddress = (void *)0x12345678, pongAddress = (void *)0x87654321
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplPingPongTestsforsim, TC_003_SetupPingPongBuffers_MM2S_Failure) {

	setupCommonPortConfig(false);

	XAieMem::s_get_dev_addr_ret = 1;
	g_ut_externalbuffer_force_dma_enqueue_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_externalbuffer_force_dma_enqueue_fail = 0;
	CHECK_EQUAL(static_cast<int>(abr::err_code::aie_driver_error), static_cast<int>(result));
}
//*********************************************************************************/
/*		Unique test case ID: TC_004_SetupPingPongBuffers_NoSyncCache
*		Function Name: setupPingPongBuffers
*		Input: pingAddress = (void *)0x12345678, pongAddress = (void *)0x87654321, sync_cache = false
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplPingPongTestsforsim, TC_004_SetupPingPongBuffers_NoSyncCache) {
	setupCommonPortConfig(true);


	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress, /*sync_cache=*/false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_SetupPingPong_EnqueueTaskFail
*		Function Name: setupPingPongBuffers
*		Input: pingAddress = (void *)0x12345678, pongAddress = (void *)0x87654321
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ExternalBufferApiImplPingPongTestsforsim, TC_005_SetupPingPong_EnqueueTaskFail) {
	setupCommonPortConfig(false);
	g_ut_externalbuffer_force_dma_enqueue_fail = 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;

	g_ut_externalbuffer_force_dma_enqueue_fail = 0;
	CHECK_EQUAL(static_cast<int>(abr::err_code::aie_driver_error), static_cast<int>(result));
}

/*@section Test Execution details
| Test Case ID                                          | Test Result | CR  | Impact | Test Details |
|-------------------------------------------------------|-------------|-----|--------|--------------|
| TC_001_SetAddress_Success                             | PASS        | No  | NA     | NA           |
| TC_002_SetAddress_AlreadySet                          | PASS        | No  | NA     | NA           |
| TC_003_Wait_EmptyShimPortConfigs                      | PASS        | No  | NA     | NA           |
| TC_004_Wait_ValidPortNameMM2S                         | PASS        | No  | NA     | NA           |
| TC_005_Wait_ValidPortNameS2MM                         | PASS        | No  | NA     | NA           |
| TC_006_Wait_InvalidConfig                             | PASS        | No  | NA     | NA           |
| TC_007_Wait_InvalidPort_WithConfiguredPorts           | PASS        | No  | NA     | NA           |
| TC_001_SetupPingPongBuffers_Success                   | PASS        | No  | NA     | NA           |
| TC_002_SetupPingPongBuffers_AlreadySet                | PASS        | No  | NA     | NA           |
| TC_003_SetupPingPongBuffers_PartialNullState          | PASS        | No  | NA     | NA           |
| TC_004_SetupPingPongBuffers_Success                   | PASS        | No  | NA     | NA           |
| TC_005_SetupPingPongBuffers_UpdateBDAddress_ErrorPath | PASS        | No  | NA     | NA           |
| TC_001_GM2AIE_NB_EmptyShimPortConfigs                 | PASS        | No  | NA     | NA           |
| TC_002_GM2AIE_NB_InvalidPortName                      | PASS        | No  | NA     | NA           |
| TC_003_GM2AIE_NB_Success                              | PASS        | No  | NA     | NA           |
| TC_004_GM2AIE_NB_Failure                              | PASS        | No  | NA     | NA           |
| TC_005_AIE2GM_NB_EmptyShimPortConfigs                 | PASS        | No  | NA     | NA           |
| TC_006_AIE2GM_NB_InvalidPortName                      | PASS        | No  | NA     | NA           |
| TC_007_AIE2GM_NB_UpdateBDAddressSuccess               | PASS        | No  | NA     | NA           |
| TC_008_AIE2GM_NB_UpdateBDAddressFails                 | PASS        | No  | NA     | NA           |
| TC_009_GM2AIE_NB_UpdateBDAddress_ErrorPath            | PASS        | No  | NA     | NA           |
| TC_010_AIE2GM_NB_UpdateBDAddress_ErrorPath            | PASS        | No  | NA     | NA           |
| TC_011_GM2AIE                                         | PASS        | No  | NA     | NA           |
| TC_012_AIE2GM                                         | PASS        | No  | NA     | NA           |
| TC_013_GM2AIE_NB_MultipleBds                          | PASS        | No  | NA     | NA           |
| TC_014_AIE2GM_NB_MultipleBds                          | PASS        | No  | NA     | NA           |
| TC_015_GM2AIE_Failure                                 | PASS        | No  | NA     | NA           |
| TC_016_AIE2GM_Failure                                 | PASS        | No  | NA     | NA           |
| TC_017_GM2AIE_NB_GetAddressFailure_Continues          | PASS        | No  | NA     | NA           |
| TC_018_AIE2GM_NB_GetAddressFailure_Continues          | PASS        | No  | NA     | NA           |
| TC_019_GM2AIE_NB_InvalidPort_WithConfigs              | PASS        | No  | NA     | NA           |
| TC_020_AIE2GM_NB_InvalidPort_WithConfigs              | PASS        | No  | NA     | NA           |
| TC_021_Wait_Sim_DrainsGm2aieQueue_AfterGm2aieNb       | PASS        | No  | NA     | NA           |
| TC_022_Wait_Sim_DrainsAie2gmQueue_AfterAie2gmNb       | PASS        | No  | NA     | NA           |
| TC_023_Wait_Sim_DrainsGm2aieQueue_MultipleNb          | PASS        | No  | NA     | NA           |
| TC_001_SetupPingPongBuffers_S2MM                      | PASS        | No  | NA     | NA           |
| TC_002_SetupPingPongBuffers_MM2S                      | PASS        | No  | NA     | NA           |
| TC_003_SetupPingPongBuffers_MM2S_Failure              | PASS        | No  | NA     | NA           |
| TC_004_SetupPingPongBuffers_NoSyncCache               | PASS        | No  | NA     | NA           |
| TC_005_SetupPingPong_EnqueueTaskFail                  | PASS        | No  | NA     | NA           |

/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)                | %      | Result     | CR/Status    | Impact          |
|----------------------------------------------|--------|------------|--------------|-----------------|
| ExternalBufferApiImpl::ExternalBufferApiImpl | <100>  | <Pass>     | <NA>         | No Impact       |
| ExternalBufferApiImpl::setAddress            | <100>  | <Pass>     | <NA>         | No Impact       |
| ExternalBufferApiImpl::wait                  | <100>  | <Pass>     | <NA>         | No Impact       |
| ExternalBufferApiImpl::setupPingPongBuffers  | <100>  | <Pass>     | <NA>         | No Impact       |
| ExternalBufferApiImpl::gm2aie_nb             | <100>  | <Pass>     | <NA>         | No Impact       |
| ExternalBufferApiImpl::aie2gm_nb             | <100>  | <Pass>     | <NA>         | No Impact       |
| ExternalBufferApiImpl::gm2aie                | <100>  | <Pass>     | <NA>         | No Impact       |
| ExternalBufferApiImpl::aie2gm                | <100>  | <Pass>     | <NA>         | No Impact       |
| _TransactionInfo::_TransactionInfo           | <100>  | <Pass>     | <NA>         | No Impact       |
| ExternalBufferApiImpl::getAddress            | <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)                | 100    | <Pass>     | <NA>         | No Impact       |

/*@section Uncovered details*/
/* No uncovered details */
