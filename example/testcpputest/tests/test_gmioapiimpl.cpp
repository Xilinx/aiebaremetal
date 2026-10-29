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
* @file test_gmioapiimpl.cpp
*  This file contains test cases of GMIO API functions
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	  Who	   Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera      05/03/2026  Initial creation
* 1.1	 Veera      20/04/2026  Added test cases for GMIO API functions
* 1.2	 Veera      10/05/2026  updated comment section for GMIO API functions
* 1.3	 Veera      26/05/2026  updated timing analysis for GMIO API functions
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
// #define __AIESIM__
#include "gmioapiimpl.h"
#include "stubs.h"
#include "test_comparator.h"
#include <chrono>
#include <iostream>
using namespace abr;
TEST_GROUP(GmioApiImplTests) {

	std::shared_ptr<abr::gmio_api> api;
	std::shared_ptr<XAieMem> mem;
	abr::gmio_config gmioconfig;
	GmioApiImpl *gmio;
	XAie_DevInst devInst;

	void setup() {
		gmioconfig.name = "gradf.in";
		gmioconfig.type = abr::gmio_config::gm2aie;
		gmioconfig.id = 1;
		gmioconfig.shimColumn = 0;
		gmioconfig.channelNum = 0;
		gmioconfig.burstLength = 4;
		devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML - 1;

		api = std::make_shared<abr::gmio_api>(&gmioconfig);
		mem = std::make_shared<XAieMem>(&devInst);

		(void)api->configure();
		gmio = new GmioApiImpl(api, gmioconfig, mem, "test_gmio");
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installCopier("u8", u8Copier);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
		XAieMem::reset_test_state();
	}

	void teardown() {
		delete gmio;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
		XAieMem::reset_test_state();
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: TC_001_ConstructorInitialization
*		Function Name: GmioApiImpl::GmioApiImpl
*		Input: configured gmio_api, gmio_config name = "test_gmio", mem = XAieMem instance
*		Expectation: gmio != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 1
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_001_ConstructorInitialization) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	bool gmio_not_null = (gmio != nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by GmioApiImpl constructor check: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(gmio_not_null);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_002_WaitCallsGmioApiWait
*		Function Name: GmioApiImpl::wait
*		Input: syncmode = true
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 1
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_002_WaitCallsGmioApiWait) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = gmio->wait(true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, result);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_003_Gm2aie_NB_Success
*		Function Name: AIEGraph::gm2aie
*		Input: buffer = {0}, len = 100
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_003_Gm2aie_NB_Success) {
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::gm2aie;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio->gm2aie_nb(buffer, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_004_Gm2aie_NB_GetAddressFailure
*		Function Name: AIEGraph::gm2aie
*		Input:  buffer = {0}, len = 100
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_004_Gm2aie_NB_GetAddressFailure) {
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::gm2aie;
	XAieMem::s_get_dev_addr_ret = 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio->gm2aie_nb(buffer, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_005_Aie2gm_NB_Success
*		Function Name: aeg::detail::OutputGMIO::aie2gm
*		Input: buffer = {0}, len = 100
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_005_Aie2gm_NB_Success) {

	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::aie2gm;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio->aie2gm_nb(buffer, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_006_Aie2gm_NB_GetAddressFailure
*		Function Name: aeg::detail::OutputGMIO::aie2gm
*		Input:  buffer = {0}, len = 100
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_006_Aie2gm_NB_GetAddressFailure) {

	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::aie2gm;
	XAieMem::s_get_dev_addr_ret = 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio->aie2gm_nb(buffer, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_007_Gm2aie_Success
*		Function Name: AIEGraph::gm2aie
*		Input: buffer = {0}, len = 100
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_007_Gm2aie_Success) {
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::gm2aie;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio->gm2aie(buffer, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
}

#if defined(__AIESIM__)
//*********************************************************************************/
/*
*		Unique test case ID: TC_008_Aie2gm_Nb_SimQueueThenWait
*		Function Name: GmioApiImpl::aie2gm_nb / wait
*		Input: buffer = {0}, len = 100 under __AIESIM__, then wait(syncmode = true)
*		Expectation: nb_result == err_code::ok, result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_008_Aie2gm_Nb_SimQueueThenWait) {
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::aie2gm;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code nb_result = gmio->aie2gm_nb(buffer, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(nb_result == err_code::ok);

	CHECK_EQUAL(0, gmio->wait(true));
}
#endif

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_GmioSyncNb_EnqueueBDFailure
*		Function Name: GmioApiImpl::gmio_sync_nb
*		Input: dir = GM2AIE, data = 0x1000, len = 100
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_009_GmioSyncNb_EnqueueBDFailure) {
	gmioconfig.type = abr::gmio_config::gm2aie;
	auto local_api = std::make_shared<abr::gmio_api>(&gmioconfig);
	CHECK(local_api->configure() == err_code::ok);
	GmioApiImpl gmio(local_api, gmioconfig, mem, "test_gmio");

	g_ut_externalbuffer_force_dma_enqueue_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.gmio_sync_nb(AIE_GMIO_DIR::GM2AIE, 0x1000, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_sync_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_externalbuffer_force_dma_enqueue_fail = 0;

	CHECK(result == err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_010_Wait_NonBlockingFailure
*		Function Name: GmioApiImpl::wait
*		Input: syncmode = false
*		Expectation: result == static_cast<int>(err_code::resource_unavailable)
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_010_Wait_NonBlockingFailure) {
	g_ut_gmio_wait_nb_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = gmio->wait(false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(err_code::resource_unavailable), result);
	g_ut_gmio_wait_nb_fail = 0;
}
/*********************************************************************************/
/*
*		Unique test case ID: TC_011_GmioSyncNb_NotConfigured
*		Function Name: GmioApiImpl::gmio_sync_nb
*		Input: dir = GM2AIE, data = 0x1000, len = 100
*		Expectation: result == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplTests, TC_011_GmioSyncNb_NotConfigured) {
	gmioconfig.type = abr::gmio_config::gm2aie;
	auto unconfigured_api = std::make_shared<abr::gmio_api>(&gmioconfig);
	GmioApiImpl gmio(unconfigured_api, gmioconfig, mem, "test_gmio");

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.gmio_sync_nb(AIE_GMIO_DIR::GM2AIE, 0x1000, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_sync_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::internal_error);
}

TEST_GROUP(GmioApiImplsyncTests) {
	std::shared_ptr<abr::gmio_api> api;
	std::shared_ptr<XAieMem> mem;
	abr::gmio_config gmioconfig;
	XAie_DevInst devInst;

	void setup() {
		devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
		gmioconfig.name = "gradf.in";
		gmioconfig.id = 1;
		gmioconfig.name = "test_gmio";
		gmioconfig.shimColumn = 0;
		gmioconfig.channelNum = 0;
		gmioconfig.burstLength = 4;
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installCopier("u8", u8Copier);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
		mem = std::make_shared<XAieMem>(&devInst);
		XAieMem::reset_test_state();
	}

	void teardown() {
		mock_c()->removeAllComparatorsAndCopiers();
		mem.reset();
		mock_c()->clear();
		mock_c()->checkExpectations();
		XAieMem::reset_test_state();
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: TC_001_GmioSyncNb_AIE2GM_Success
*		Function Name: GmioApiImpl::gmio_sync_nb
*		Input: dir = AIE_GMIO_DIR::AIE2GM, data = 0x1000, len = 100
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplsyncTests, TC_001_GmioSyncNb_AIE2GM_Success) {

	gmioconfig.type = abr::gmio_config::gm2aie;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.gmio_sync_nb(AIE_GMIO_DIR::AIE2GM, 0x1000, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_sync_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_002_GmioSyncNb_GM2AIE_Success
*		Function Name: GmioApiImpl::gmio_sync_nb
*		Input: dir = AIE_GMIO_DIR::GM2AIE, data = 0x1000, len = 100
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplsyncTests, TC_002_GmioSyncNb_GM2AIE_Success) {
	gmioconfig.type = abr::gmio_config::aie2gm;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.gmio_sync_nb(AIE_GMIO_DIR::GM2AIE, 0x1000, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_sync_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_003_GmioSyncNb_InvalidDirection
*		Function Name: GmioApiImpl::gmio_sync_nb
*		Input: dir = static_cast<AIE_GMIO_DIR>(99), data = 0x1000, len = 100
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplsyncTests, TC_003_GmioSyncNb_InvalidDirection) {
	gmioconfig.type = abr::gmio_config::gm2aie;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.gmio_sync_nb(static_cast<AIE_GMIO_DIR>(99), 0x1000, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_sync_nb function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_004_Aie2gm_Success
*		Function Name: aeg::detail::OutputGMIO::aie2gm
*		Input: buffer = {0}, len = 100
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplsyncTests, TC_004_Aie2gm_Success) {
	char buffer[100] = {0};

	gmioconfig.type = abr::gmio_config::gm2aie;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.aie2gm(buffer, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::ok);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_005_Aie2gm_GetAddressFailure
*		Function Name: aeg::detail::OutputGMIO::aie2gm
*		Input: buffer = {0}, len = 100
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplsyncTests, TC_005_Aie2gm_GetAddressFailure) {
	char buffer[100] = {0};
	XAieMem::s_get_dev_addr_ret = 1;

	gmioconfig.type = abr::gmio_config::gm2aie;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.aie2gm(buffer, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::aie_driver_error);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_006_Gm2aie_GetAddressFailure
*		Function Name: AIEGraph::gm2aie
*		Input: buffer = {0}, len = 100
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplsyncTests, TC_006_Gm2aie_GetAddressFailure) {
	char buffer[100] = {0};
	XAieMem::s_get_dev_addr_ret = 1;

	gmioconfig.type = abr::gmio_config::gm2aie;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.gm2aie(buffer, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::aie_driver_error);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_007_GmioSyncNb_AIE2GM_MatchingType
*		Function Name: GmioApiImpl::gmio_sync_nb
*		Input: dir = AIE_GMIO_DIR::AIE2GM, data = 0x1000, len = 100
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplsyncTests, TC_007_GmioSyncNb_AIE2GM_MatchingType) {
	gmioconfig.type = abr::gmio_config::aie2gm;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.gmio_sync_nb(AIE_GMIO_DIR::AIE2GM, 0x1000, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_sync_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::ok);
	mock_c()->checkExpectations();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_GmioSyncNb_TypeMismatch_Gm2aieDir
*		Function Name: GmioApiImpl::gmio_sync_nb
*		Input: dir = GM2AIE, data = 0x1000, len = 100
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplsyncTests, TC_008_GmioSyncNb_TypeMismatch_Gm2aieDir) {
	gmioconfig.type = abr::gmio_config::aie2gm;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(api->configure()));

	GmioApiImpl gmio_sync(api, gmioconfig, mem, "test_gmio");
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio_sync.gmio_sync_nb(AIE_GMIO_DIR::GM2AIE, 0x1000, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_sync_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_Aie2gm_Nb_EnqueueBDFailure
*		Function Name: GmioApiImpl::aie2gm_nb
*		Input: buffer = {0}, len = 100
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiImplsyncTests, TC_009_Aie2gm_Nb_EnqueueBDFailure) {
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::aie2gm;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(api->configure()));

	GmioApiImpl gmio_sync(api, gmioconfig, mem, "test_gmio");
	g_ut_externalbuffer_force_dma_enqueue_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio_sync.aie2gm_nb(buffer, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_externalbuffer_force_dma_enqueue_fail = 0;
	CHECK(result == err_code::aie_driver_error);
}

/*@section Test Execution details
| Test Case ID                             | Test Result | CR  | Impact | Test Details |
|------------------------------------------|-------------|-----|--------|--------------|
| TC_001_ConstructorInitialization         | PASS        | No  | NA     | NA           |
| TC_002_WaitCallsGmioApiWait              | PASS        | No  | NA     | NA           |
| TC_003_Gm2aie_NB_Success                 | PASS        | No  | NA     | NA           |
| TC_004_Gm2aie_NB_GetAddressFailure       | PASS        | No  | NA     | NA           |
| TC_005_Aie2gm_NB_Success                 | PASS        | No  | NA     | NA           |
| TC_006_Aie2gm_NB_GetAddressFailure       | PASS        | No  | NA     | NA           |
| TC_007_Gm2aie_Success                    | PASS        | No  | NA     | NA           |
| TC_008_Aie2gm_Nb_SimQueueThenWait        | PASS        | No  | NA     | NA           |
| TC_009_GmioSyncNb_EnqueueBDFailure       | PASS        | No  | NA     | NA           |
| TC_010_Wait_NonBlockingFailure           | PASS        | No  | NA     | NA           |
| TC_011_GmioSyncNb_NotConfigured          | PASS        | No  | NA     | NA           |
| TC_001_GmioSyncNb_AIE2GM_Success         | PASS        | No  | NA     | NA           |
| TC_002_GmioSyncNb_GM2AIE_Success         | PASS        | No  | NA     | NA           |
| TC_003_GmioSyncNb_InvalidDirection       | PASS        | No  | NA     | NA           |
| TC_004_Aie2gm_Success                    | PASS        | No  | NA     | NA           |
| TC_005_Aie2gm_GetAddressFailure          | PASS        | No  | NA     | NA           |
| TC_006_Gm2aie_GetAddressFailure          | PASS        | No  | NA     | NA           |
| TC_007_GmioSyncNb_AIE2GM_MatchingType    | PASS        | No  | NA     | NA           |
| TC_008_GmioSyncNb_TypeMismatch_Gm2aieDir | PASS        | No  | NA     | NA           |
| TC_009_Aie2gm_Nb_EnqueueBDFailure        | PASS        | No  | NA     | NA           |

/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)        | %      | Result     | CR/Status    | Impact          |
|--------------------------------------|--------|------------|--------------|-----------------|
| GmioApiImpl::GmioApiImpl             | <100>  | <Pass>     | <NA>         | No Impact       |
| GmioApiImpl::wait                    | <100>  | <Pass>     | <NA>         | No Impact       |
| GmioApiImpl::gm2aie_nb               | <100>  | <Pass>     | <NA>         | No Impact       |
| GmioApiImpl::aie2gm_nb               | <100>  | <Pass>     | <NA>         | No Impact       |
| GmioApiImpl::gm2aie                  | <100>  | <Pass>     | <NA>         | No Impact       |
| GmioApiImpl::gmio_sync_nb            | <100>  | <Pass>     | <NA>         | No Impact       |
| GmioApiImpl::aie2gm                  | <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)        | 100    | <Pass>     | <NA>         | No Impact       |

/*@section Uncovered details*/
/* No uncovered details */
