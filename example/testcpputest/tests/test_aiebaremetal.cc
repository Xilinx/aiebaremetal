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
* @file test_aiebaremetal.cc
*  This file contains test cases of AIEGraph API functions.
*
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	  Who	   Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera      05/03/2026  Initial creation
* 1.1	 Veera      20/04/2026  Added test cases for AIEGraph API functions
* 1.2	 Veera      10/05/2026  updated comment section for AIEGraph API functions
* 1.3	 Veera      27/05/2026  added time taken by functions in AIEGraph API functions
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
#include "aeg_baremetal_api.h"
#include "aiebaremetal.h"
#include "fal_util.h"
#include "stubs.h"
#include "test_comparator.h"
#include "gmio_shim/xaiemem.h"

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport_c.h"
#include <sys/stat.h>
#include <unistd.h>
#include <xscugic.h>
#include "event.h"
#include <cerrno>
#include <chrono>
#include <iostream>

extern "C" {
	extern uint32_t g_ut_backtrack_next_valid_calls_remaining;
	extern uint32_t g_ut_backtrack_error_count_override;
}
//#define __AIESIM__ 1
TEST_GROUP(AIEGraphTests) {

	AIEGraph *gr;

	void graphInit_mocks_ok() {
		CHECK(gr->init() == aeg_err::ok);
	}

	void setup() {
		RegisterXAieLocTypeComparator();
		mock_c()->installCopier("u8", u8Copier);
		mock_c()->expectOneCall("XAie_SetupPartitionConfig")->ignoreOtherParameters()->andReturnIntValue(0);
		mock_c()->expectOneCall("XAie_CfgInitialize")->ignoreOtherParameters()->andReturnIntValue(0);


		XAieMem::reset_test_state();

		gr = new AIEGraph("gradf2");
		g_ut_backtrack_next_valid_calls_remaining = 0U;
		g_ut_backtrack_error_count_override = 0U;
	}

	void teardown() {
		mock_c()->expectOneCall("XAie_Finish")->ignoreOtherParameters();
		delete gr;

		XAieMem::reset_test_state();
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
		mock_c()->checkExpectations();
	}
};

TEST_GROUP(AIEGraphCtorPartitionFailTests) {
	void setup() {
		RegisterXAieLocTypeComparator();
		mock_c()->installCopier("u8", u8Copier);
		XAieMem::reset_test_state();
	}

	void teardown() {
		XAieMem::reset_test_state();
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
		mock_c()->checkExpectations();
	}
};

/*********************************************************************************/
/*
*		Unique test case ID: TC_001_ConstructorSetupPartitionConfigFailure
*		Function Name: AIEGraph::AIEGraph
*		Input: gradf2
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphCtorPartitionFailTests, TC_001_ConstructorSetupPartitionConfigFailure) {
	mock_c()
		->expectOneCall("XAie_SetupPartitionConfig")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_Finish")->ignoreOtherParameters();

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	AIEGraph gr1("gradf2");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph constructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_ConstructorCfgInitializeFailure
*		Function Name: AIEGraph::AIEGraph
*		Input: gradf2
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_002_ConstructorCfgInitializeFailure) {
	XAie_DevInst devInst;
	devInst.NumCols = 1;
	devInst.NumRows = 1;
	devInst.StartCol = 0;
	XAie_Config ConfigPtr;
	ConfigPtr.NumRows = 0x5;
	mock_c()->expectOneCall("XAie_SetupPartitionConfig")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_CfgInitialize")->ignoreOtherParameters()->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_Finish")->ignoreOtherParameters();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	AIEGraph gr2("gradf2");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph constructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_ConstructorSuccess
*		Function Name: AIEGraph::AIEGraph
*		Input: gradf2
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_003_ConstructorSuccess) {
	mock_c()->expectOneCall("XAie_SetupPartitionConfig")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_CfgInitialize")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_Finish")->ignoreOtherParameters();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	AIEGraph gr("gradf2");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph constructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_ConstructorFailureforInvalidGraphName
*		Function Name: AIEGraph::AIEGraph
*		Input: graph
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_004_ConstructorFailureforInvalidGraphName) {
	mock_c()->expectOneCall("XAie_SetupPartitionConfig")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_CfgInitialize")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_Finish")->ignoreOtherParameters();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	AIEGraph gr("graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph constructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004a_ConstructorFalUtilInitializeFailure
*		Function Name: AIEGraph::AIEGraph
*		Input: valid graph name, fal_util::initialize forced to return false
*		Expectation: constructor completes; FAL utility initialization failed branch is exercised
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
extern "C" void set_fal_util_initialize_should_fail(bool fail);
TEST(AIEGraphTests, TC_004a_ConstructorFalUtilInitializeFailure) {
	mock_c()->expectOneCall("XAie_SetupPartitionConfig")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_CfgInitialize")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_Finish")->ignoreOtherParameters();
	set_fal_util_initialize_should_fail(true);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	AIEGraph gr("gradf2");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	set_fal_util_initialize_should_fail(false);
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph constructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_Init_failure
*		Function Name: AIEGraph::init
*		Input: none
*		Expectation:  aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_005_Init_failure) {
	g_ut_gmio_configure_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->init() == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph init function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_gmio_configure_fail = 0;
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_006_Init_xaiemem_null_backend_not_ready
*		Function Name: AIEGraph::init
*		Input: none
*		Expectation: aeg_err::backend_not_ready
*		Expectation: init() returns backend_not_ready; CHECK(gr->init() == aeg_err::backend_not_ready)
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_006_Init_xaiemem_null_backend_not_ready) {
	gr->reset_xaiemem();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->init() == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph init function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_007_Init_graphapi_null_backend_not_ready
*		Function Name: AIEGraph::init
*		Input: none
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_007_Init_graphapi_null_backend_not_ready) {
	gr->reset_GraphApi();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->init() == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph init function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_008_InitConfiguresSystem
*		Function Name: AIEGraph::init
*		Input: none
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_008_InitConfiguresSystem) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	graphInit_mocks_ok();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph init function: " << total_clock_enable_us << " microseconds" << std::endl;
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_008a_InitExternalBuffersResourceUnavailable
*		Function Name: AIEGraph::init
*		Input: ConfigureExternalBuffers forced to return resource_unavailable
*		Expectation: result == aeg_err::resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of non-null external buffer configuration failure
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_008a_InitExternalBuffersResourceUnavailable) {
	g_ut_configure_external_buffers_force_code = EAGAIN;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->init();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph init external buffer failure function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_configure_external_buffers_force_code = 0;
	CHECK(result == aeg_err::resource_unavailable);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_008b_InitGmiosUserError
*		Function Name: AIEGraph::init
*		Input: ConfigureGmios forced to return user_error
*		Expectation: result == aeg_err::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of GMIO failure not remapped to backend_not_ready
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_008b_InitGmiosUserError) {
	g_ut_gmio_configure_force_code = 22;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->init();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph init GMIO user error function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_gmio_configure_force_code = 0;
	CHECK(result == aeg_err::user_error);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_008e_InitGmiosInternalError
*		Function Name: AIEGraph::init
*		Input: ConfigureGmios forced to return internal_error
*		Expectation: result == aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of GMIO internal_error remapping
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_008e_InitGmiosInternalError) {
	g_ut_gmio_configure_force_code = ENOTSUP;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->init();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph init GMIO internal error function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_gmio_configure_force_code = 0;
	CHECK(result == aeg_err::backend_not_ready);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_008f_InitGmiosNullArgumentRemap
*		Function Name: AIEGraph::init
*		Input: ConfigureGmios forced to return null_argument
*		Expectation: result == aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of GMIO null_argument remapping arm
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_008f_InitGmiosNullArgumentRemap) {
	g_ut_gmio_configure_force_null_argument = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->init();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph init GMIO null_argument remap function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_gmio_configure_force_null_argument = 0;
	CHECK(result == aeg_err::backend_not_ready);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_008c_InitRtpsResourceUnavailableContinues
*		Function Name: AIEGraph::init
*		Input: ConfigureRtps forced to return resource_unavailable
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of non-null RTP configuration failure path
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_008c_InitRtpsResourceUnavailableContinues) {
	g_ut_configure_rtps_force_code = EAGAIN;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->init();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph init RTP failure function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_configure_rtps_force_code = 0;
	CHECK(result == aeg_err::ok);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_008d_InitSharedBuffersResourceUnavailable
*		Function Name: AIEGraph::init
*		Input: ConfigureSharedBuffers forced to return resource_unavailable
*		Expectation: result == aeg_err::resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of non-null shared buffer configuration failure
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_008d_InitSharedBuffersResourceUnavailable) {
	g_ut_configure_shared_buffers_force_code = EAGAIN;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->init();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph init shared buffer failure function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_configure_shared_buffers_force_code = 0;
	CHECK(result == aeg_err::resource_unavailable);
}

#if !defined(__AIESIM__)
//*********************************************************************************/
/*
*		Unique test case ID: TC_009_SelftestExitSuccess
*		Function Name: AIEGraph::selftestexit
*		Input: none
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_009_SelftestExitSuccess) {
	XAie_DevInst devInst;
	/* Match mock XAie_SetupPartitionConfig when driver blob has partition_num_cols==0 (NumCols forced to 1) */
	devInst.NumCols = 1;
	devInst.NumRows = 1;
	devInst.StartCol = 0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIE2PS;
	graphInit_mocks_ok();
	mock_c()
		->expectOneCall("XAie_PmRequestTiles")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("NumTiles", 0)
		->ignoreOtherParameters()
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()
		->expectNCalls(3, "XAie_PmSetColumnClk")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("StartCol", devInst.StartCol)
		->withUnsignedIntParameters("NumCols", devInst.NumCols)
		->withUnsignedIntParameters("Enable", 1)
		->andReturnIntValue(0);
	mock_c()
		->expectOneCall("XAie_PmSetColumnClk")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("StartCol", devInst.StartCol)
		->withUnsignedIntParameters("NumCols", devInst.NumCols)
		->withUnsignedIntParameters("Enable", 0)
		->andReturnIntValue(0);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->selftestexit();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph selftestexit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
	CHECK(true);
	mock_c()->clear();
}
#endif /* !defined(__AIESIM__) */

//*********************************************************************************/
/*
*		Unique test case ID: TC_010_SelftestExitFailure
*		Function Name: AIEGraph::selftestexit
*		Input: none
*		Expectation: result == aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_010_SelftestExitFailure) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->selftestexit();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph selftestexit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::invalid_state);
}

#if !defined(__AIESIM__)

//*********************************************************************************/
/*
*		Unique test case ID: TC_011_SelftestExit_PmRequestTilesFails
*		Function Name: AIEGraph::selftestexit
*		Input: none
*		Expectation: result == 3
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_011_SelftestExit_PmRequestTilesFails) {
	graphInit_mocks_ok();
	mock_c()->expectOneCall("XAie_PmRequestTiles")->ignoreOtherParameters()->andReturnIntValue(3);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->selftestexit();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph selftestexit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(static_cast<int32_t>(result) == 3);
	mock_c()->clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_012_SelftestExit_ErrorHandlingInitFails
*		Function Name: AIEGraph::selftestexit
*		Input: none
*		Expectation: result == 4
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_012_SelftestExit_ErrorHandlingInitFails) {
	graphInit_mocks_ok();
	mock_c()->expectOneCall("XAie_PmRequestTiles")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->ignoreOtherParameters()->andReturnIntValue(4);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->selftestexit();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph selftestexit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(static_cast<int32_t>(result) == 4);
	mock_c()->clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_013_SelftestExit_FirstClockEnableFails
*		Function Name: AIEGraph::selftestexit
*		Input: none
*		Expectation: result == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_013_SelftestExit_FirstClockEnableFails) {
	graphInit_mocks_ok();
	mock_c()->expectOneCall("XAie_PmRequestTiles")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_PmSetColumnClk")->ignoreOtherParameters()->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->selftestexit();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph selftestexit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::aie_driver_error);
	mock_c()->clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_014_SelftestExit_ClockDisableFails
*		Function Name: AIEGraph::selftestexit
*		Input: none
*		Expectation: result == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_014_SelftestExit_ClockDisableFails) {
	graphInit_mocks_ok();
	mock_c()->expectOneCall("XAie_PmRequestTiles")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_PmSetColumnClk")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_PmSetColumnClk")->ignoreOtherParameters()->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->selftestexit();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph selftestexit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::aie_driver_error);
	mock_c()->clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_015_SelftestExit_SecondClockEnableFails
*		Function Name: AIEGraph::selftestexit
*		Input: none
*		Expectation: result == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_015_SelftestExit_SecondClockEnableFails) {
	graphInit_mocks_ok();
	mock_c()->expectOneCall("XAie_PmRequestTiles")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_PmSetColumnClk")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_PmSetColumnClk")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_PmSetColumnClk")->ignoreOtherParameters()->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->selftestexit();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph selftestexit function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::aie_driver_error);
	mock_c()->clear();
}
#endif /* !defined(__AIESIM__) */

//*********************************************************************************/
/*
*		Unique test case ID: TC_016_SelftestExit_AIESIM_DevGenAIE2PS_UpdateNpiAddr
*		Function Name: AIEGraph::selftestexit
*		Input: none
*		Expectation: aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_016_SelftestExit_AIESIM_DevGenAIE2PS_UpdateNpiAddr) {
	XAie_DevInst devInst{};
	devInst.NumCols = 1;
	devInst.NumRows = 1;
	devInst.StartCol = 0;
	graphInit_mocks_ok();
	XAie_DevInst *const di = static_cast<XAie_DevInst *>(gr->getaiedevinst());
	CHECK(di != nullptr);
	di->DevProp.DevGen = XAIE_DEV_GEN_AIE2PS;

	mock_c()->expectOneCall("XAie_UpdateNpiAddr")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_UpdateNpiAddr")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()
		->expectOneCall("XAie_PmRequestTiles")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("NumTiles", 0)
		->ignoreOtherParameters()
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()
		->expectNCalls(3, "XAie_PmSetColumnClk")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("StartCol", devInst.StartCol)
		->withUnsignedIntParameters("NumCols", devInst.NumCols)
		->withUnsignedIntParameters("Enable", 1)
		->andReturnIntValue(0);
	mock_c()
		->expectOneCall("XAie_PmSetColumnClk")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("StartCol", devInst.StartCol)
		->withUnsignedIntParameters("NumCols", devInst.NumCols)
		->withUnsignedIntParameters("Enable", 0)
		->andReturnIntValue(0);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->selftestexit() == aeg_err::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph selftestexit function: " << total_clock_enable_us << " microseconds" << std::endl;
	mock_c()->clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_017_SelftestExit_AIESIM_DevGenAIE_UpdateNpiAddr
*		Function Name: AIEGraph::selftestexit
*		Input: none
*		Expectation: aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_017_SelftestExit_AIESIM_DevGenAIE_UpdateNpiAddr) {
	XAie_DevInst devInst{};
	devInst.NumCols = 1;
	devInst.NumRows = 1;
	devInst.StartCol = 0;
	graphInit_mocks_ok();
	XAie_DevInst *const di = static_cast<XAie_DevInst *>(gr->getaiedevinst());
	CHECK(di != nullptr);
	di->DevProp.DevGen = XAIE_DEV_GEN_AIE;

	mock_c()->expectOneCall("XAie_UpdateNpiAddr")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_UpdateNpiAddr")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()
		->expectOneCall("XAie_PmRequestTiles")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("NumTiles", 0)
		->ignoreOtherParameters()
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_ErrorHandlingInit")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()
		->expectNCalls(3, "XAie_PmSetColumnClk")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("StartCol", devInst.StartCol)
		->withUnsignedIntParameters("NumCols", devInst.NumCols)
		->withUnsignedIntParameters("Enable", 1)
		->andReturnIntValue(0);
	mock_c()
		->expectOneCall("XAie_PmSetColumnClk")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("StartCol", devInst.StartCol)
		->withUnsignedIntParameters("NumCols", devInst.NumCols)
		->withUnsignedIntParameters("Enable", 0)
		->andReturnIntValue(0);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->selftestexit() == aeg_err::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph selftestexit function: " << total_clock_enable_us << " microseconds" << std::endl;
	mock_c()->clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_017a_SelftestExit_AIESIM_UnknownDevGen_DefaultBranch
*		Function Name: AIEGraph::selftestexit
*		Input: none
*		Expectation: aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_017a_SelftestExit_AIESIM_UnknownDevGen_DefaultBranch) {
	graphInit_mocks_ok();
	XAie_DevInst *const di = static_cast<XAie_DevInst *>(gr->getaiedevinst());
	CHECK(di != nullptr);

	di->DevProp.DevGen = XAIE_DEV_GEN_S100;

	mock_c()->ignoreOtherCalls();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->selftestexit() == aeg_err::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph selftestexit function: " << total_clock_enable_us << " microseconds" << std::endl;
	mock_c()->clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_018_ClockEnableSuccess
*		Function Name: AIEGraph::clock_enable
*		Input: none
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_018_ClockEnableSuccess) {
	XAie_DevInst devInst;
	devInst.NumCols = 1;
	devInst.NumRows = 1;
	devInst.StartCol = 0;
	graphInit_mocks_ok();
	mock_c()
		->expectOneCall("XAie_PmRequestTiles")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("NumTiles", 0)
		->ignoreOtherParameters()
		->andReturnIntValue(0);
	mock_c()
		->expectOneCall("XAie_PmSetColumnClk")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("StartCol", devInst.StartCol)
		->withUnsignedIntParameters("NumCols", devInst.NumCols)
		->withUnsignedIntParameters("Enable", 1)
		->andReturnIntValue(0);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->clock_enable();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph clock_enable function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
	mock_c()->clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_019_ClockEnableFailure
*		Function Name: AIEGraph::clock_enable
*		Input: none
*		Expectation: result == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_019_ClockEnableFailure) {
	XAie_DevInst devInst;
	devInst.NumCols = 1;
	devInst.NumRows = 1;
	devInst.StartCol = 0;
	graphInit_mocks_ok();
	mock_c()
		->expectOneCall("XAie_PmRequestTiles")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("NumTiles", 0)
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()
		->expectOneCall("XAie_PmSetColumnClk")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("StartCol", devInst.StartCol)
		->withUnsignedIntParameters("NumCols", devInst.NumCols)
		->withUnsignedIntParameters("Enable", 1)
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->clock_enable();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph clock_enable function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == aeg_err::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_020_ClockEnableFailureInvalidState
*		Function Name: AIEGraph::clock_enable
*		Input: none
*		Expectation: result == aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_020_ClockEnableFailureInvalidState) {

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->clock_enable();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph clock_enable function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == aeg_err::invalid_state);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_021_ClockDisableSuccess
*		Function Name: AIEGraph::clock_disable
*		Input: none
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_021_ClockDisableSuccess) {
	XAie_DevInst devInst;
	devInst.NumCols = 1;
	devInst.NumRows = 1;
	devInst.StartCol = 0;
	graphInit_mocks_ok();
	mock_c()
		->expectOneCall("XAie_PmSetColumnClk")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("StartCol", devInst.StartCol)
		->withUnsignedIntParameters("NumCols", devInst.NumCols)
		->withUnsignedIntParameters("Enable", 0)
		->andReturnIntValue(0);



	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->clock_disable();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph clock_disable function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_022_ClockDisableFailure
*		Function Name: AIEGraph::clock_disable
*		Input: none
*		Expectation: result == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_022_ClockDisableFailure) {
	XAie_DevInst devInst;
	devInst.NumCols = 1;
	devInst.NumRows = 1;
	devInst.StartCol = 0;
	graphInit_mocks_ok();
	mock_c()
		->expectOneCall("XAie_PmSetColumnClk")
		->ignoreOtherParameters()
		->withUnsignedIntParameters("StartCol", devInst.StartCol)
		->withUnsignedIntParameters("NumCols", devInst.NumCols)
		->withUnsignedIntParameters("Enable", 0)
		->andReturnIntValue(1);



	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->clock_disable();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph clock_disable function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_023_ClockDisableInvalidState
*		Function Name: AIEGraph::clock_disable
*		Input: none
*		Expectation: result == aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_023_ClockDisableInvalidState) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->clock_disable();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph clock_disable function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::invalid_state);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_024_MallocInitSuccess
*		Function Name: AIEGraph::malloc
*		Input: testSize
*		Expectation: result != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_024_MallocInitSuccess) {
	// Configure the XAieMem shim to hand back a non-null VAddr from `malloc()`.
	// This replaces the previous `expectOneCall("XAie_MemAllocate")` mock_c
	// expectation - the shim short-circuits the C-level call entirely.
	char mock_buffer[64];
	XAieMem::s_malloc_ret = static_cast<void *>(mock_buffer);
	size_t testSize = 1024;
	graphInit_mocks_ok();

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	void *result = gr->malloc(testSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph malloc function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result != nullptr);
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_025_Malloc_xaiemem_null
*		Function Name: AIEGraph::malloc
*		Input: 1024U
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_025_Malloc_xaiemem_null) {
	graphInit_mocks_ok();
	gr->reset_xaiemem();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->malloc(1024U) == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph malloc function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_026_MallocFailure
*		Function Name: AIEGraph::malloc
*		Input: testSize = 0
*		Expectation: result == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_026_MallocFailure) {
	size_t testSize = 0;
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	void *result = gr->malloc(testSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph malloc function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == nullptr);
	mock_c()->clear();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_027_Malloc_CheckInitialized
*		Function Name: AIEGraph::malloc
*		Input: 32
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_027_Malloc_CheckInitialized) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->malloc(32) == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph malloc function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_028_FreeSuccess
*		Function Name: AIEGraph::free
*		Input: TestAddress = malloc(1024)
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_028_FreeSuccess){

	void *TestAddress;
	TestAddress = malloc(1024);
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->free(TestAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph free function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_029_FreeFailure
*		Function Name: AIEGraph::free
*		Input: TestAddress = nullptr
*		Expectation: result == aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_029_FreeFailure) {
	void *TestAddress;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->free(TestAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph free function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::invalid_state);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_030_FreeNullAddress
*		Function Name: AIEGraph::free
*		Input: TestAddress = nullptr
*		Expectation: result == aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_030_FreeNullAddress) {
	void *TestAddress = nullptr;
	graphInit_mocks_ok();

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->free(TestAddress);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph free function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::null_argument);
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_031_Free_xaiemem_null
*		Function Name: AIEGraph::free
*		Input: dummy
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_031_Free_xaiemem_null) {
	static char dummy;
	graphInit_mocks_ok();
	gr->reset_xaiemem();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->free(static_cast<void *>(&dummy)) == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph free function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_032_CheckInitialized_free
*		Function Name: AIEGraph::free
*		Input: buf
*		Expectation: result == aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_032_CheckInitialized_free) {
	char buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->free(static_cast<void *>(buf));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph free function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::invalid_state);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_033_Wait
*		Function Name: AIEGraph::wait
*		Input: none
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_033_Wait) {

	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->wait();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_034_Wait_GraphApi_null_backend_not_ready
*		Function Name: AIEGraph::wait
*		Input: none
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_034_Wait_GraphApi_null_backend_not_ready) {
	graphInit_mocks_ok();
	gr->reset_GraphApi();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->wait() == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_035_CheckInitialized_wait
*		Function Name: AIEGraph::wait
*		Input: none
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_035_CheckInitialized_wait) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->wait() == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_036_EndSuccess
*		Function Name: AIEGraph::end
*		Input: none
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_036_EndSuccess) {

	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->end();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph end function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == aeg_err::ok);
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_037_End_GraphApi_null_backend_not_ready
*		Function Name: AIEGraph::end
*		Input: none
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_037_End_GraphApi_null_backend_not_ready) {
	graphInit_mocks_ok();
	gr->reset_GraphApi();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->end() == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph end function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_038_CheckInitialized_end
*		Function Name: AIEGraph::end
*		Input: none
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_038_CheckInitialized_end) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->end() == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph end function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_039_RunDefaultIterations
*		Function Name: AIEGraph::run
*		Input: 5
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_039_RunDefaultIterations) {
	graphInit_mocks_ok();
	unsigned long long t_core = 0;
	unsigned long long t_pl_start = 0;
	unsigned long long t_pl_current_lo = 0;
	unsigned long long t_pl_current_exit = 0x200ULL;

	mock_c()->expectNCalls(3, "XAie_StartTransaction")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(3, "XAie_SubmitTransaction")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()
		->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &t_core, sizeof(t_core))
		->andReturnIntValue(XAIE_OK);
	mock_c()
		->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &t_pl_start, sizeof(t_pl_start))
		->andReturnIntValue(XAIE_OK);
	mock_c()
		->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &t_pl_current_lo, sizeof(t_pl_current_lo))
		->andReturnIntValue(XAIE_OK);
	mock_c()
		->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &t_pl_current_exit, sizeof(t_pl_current_exit))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_ClearCoreDisableEventOccurred")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2, "XAie_CoreConfigureEnableEvent")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_EventGenerate")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->run(5);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph run function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_040_Run_ReturnsDriverError
*		Function Name: AIEGraph::run
*		Input: 5
*		Expectation: aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_040_Run_ReturnsDriverError) {
	graphInit_mocks_ok();
	g_ut_graph_api_run_int_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->run(5) == aeg_err::aie_driver_error);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph run function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_graph_api_run_int_fail = 0;
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_041_Run_graphapi_null_backend_not_ready
*		Function Name: AIEGraph::run
*		Input: 1
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_041_Run_graphapi_null_backend_not_ready) {
	graphInit_mocks_ok();
	gr->reset_GraphApi();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->run(1) == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph run function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_042_CheckInitialized_run
*		Function Name: AIEGraph::run
*		Input: 1
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_042_CheckInitialized_run) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->run(1) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph run function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_043_Run_invalid_length_zero_iterations
*		Function Name: AIEGraph::run
*		Input: 0
*		Expectation: aeg_err::invalid_length
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_043_Run_invalid_length_zero_iterations) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->run(0) == aeg_err::invalid_length);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph run function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_043a_Run_invalid_length_below_minus_one
*		Function Name: AIEGraph::run
*		Input: -2
*		Expectation: aeg_err::invalid_length
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of (num < -1) length validation arm
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_043a_Run_invalid_length_below_minus_one) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->run(-2) == aeg_err::invalid_length);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph run function: " << total_clock_enable_us << " microseconds" << std::endl;
}

#if !defined(__AIESIM__)
//*********************************************************************************/
/*
*		Unique test case ID: TC_044_BacktrackErrors
*		Function Name: AIEGraph::BacktrackErrors
*		Input: none
*		Expectation: non-null pointer
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_044_BacktrackErrors) {
	graphInit_mocks_ok();
	mock_c()->expectOneCall("XAie_BacktrackErrorInterrupts")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	void *result = gr->BacktrackErrors();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph BacktrackErrors function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result != nullptr);
}

#endif /* !defined(__AIESIM__) */

//*********************************************************************************/
/*
*		Unique test case ID: TC_045_BacktrackErrors_InvalidPartitionCols
*		Function Name: AIEGraph::BacktrackErrors
*		Input: none
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_045_BacktrackErrors_InvalidPartitionCols) {
	graphInit_mocks_ok();
	XAie_DevInst *const di = static_cast<XAie_DevInst *>(gr->getaiedevinst());
	CHECK(di != nullptr);
	const u8 saved_cols = di->NumCols;
	di->NumCols = 0U;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->BacktrackErrors() == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph BacktrackErrors function: " << total_clock_enable_us << " microseconds" << std::endl;
	di->NumCols = saved_cols;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_046_BacktrackErrors_StartColGeNumCols
*		Function Name: AIEGraph::BacktrackErrors
*		Input: none
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_046_BacktrackErrors_StartColGeNumCols) {
	graphInit_mocks_ok();
	XAie_DevInst *const instance = static_cast<XAie_DevInst *>(gr->getaiedevinst());
	CHECK(instance != nullptr);
	const u8 saved_start = instance->StartCol;
	const u8 saved_cols = instance->NumCols;
	instance->StartCol = saved_cols;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->BacktrackErrors() == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph BacktrackErrors function: " << total_clock_enable_us << " microseconds" << std::endl;
	instance->StartCol = saved_start;
	instance->NumCols = saved_cols;
}

#if !defined(__AIESIM__)

//*********************************************************************************/
/*
*		Unique test case ID: TC_047_BacktrackErrors_DriverFailure
*		Function Name: AIEGraph::BacktrackErrors
*		Input: none
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_047_BacktrackErrors_DriverFailure) {
	graphInit_mocks_ok();
	mock_c()->expectOneCall("XAie_BacktrackErrorInterrupts")->ignoreOtherParameters()->andReturnIntValue(XAIE_ERR);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->BacktrackErrors() == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph BacktrackErrors function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_048_BacktrackErrors_InsufficientBufferEmptyCache
*		Function Name: AIEGraph::BacktrackErrors
*		Input: none
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_048_BacktrackErrors_InsufficientBufferEmptyCache) {
	graphInit_mocks_ok();
	mock_c()
		->expectOneCall("XAie_BacktrackErrorInterrupts")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_INSUFFICIENT_BUFFER_SIZE);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->BacktrackErrors() == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph BacktrackErrors function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_049_BacktrackErrors_ErrorHandlingCallbackInvoked
*		Function Name: AIEGraph::BacktrackErrors
*		Input: none
*		Expectation: non-null pointer
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_049_BacktrackErrors_ErrorHandlingCallbackInvoked) {
	graphInit_mocks_ok();
	static int cb_error_count = 0;
	cb_error_count = 0;
	void (*callback)(std::vector<XAie_ErrorPayload>, void *) = [](std::vector<XAie_ErrorPayload>, void *) {
		++cb_error_count;
	};
	CHECK(gr->error_handling_cb_init(reinterpret_cast<void *>(callback), nullptr) == aeg_err::ok);
	mock_c()->expectOneCall("XAie_BacktrackErrorInterrupts")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->BacktrackErrors() != nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph BacktrackErrors function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(cb_error_count == 1);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_050_BacktrackErrors_MaxIterations
*		Function Name: AIEGraph::BacktrackErrors
*		Input: none
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_050_BacktrackErrors_MaxIterations) {
	graphInit_mocks_ok();
	constexpr uint32_t kMax = 16384U;
	g_ut_backtrack_next_valid_calls_remaining = kMax;
	mock_c()
		->expectNCalls(static_cast<int>(kMax), "XAie_BacktrackErrorInterrupts")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->BacktrackErrors() == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph BacktrackErrors function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_051_BacktrackErrors_ErrorCountClamping
*		Function Name: AIEGraph::BacktrackErrors
*		Input: none
*		Expectation: non-null pointer
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_051_BacktrackErrors_ErrorCountClamping) {
	graphInit_mocks_ok();
	g_ut_backtrack_error_count_override = 64U;
	mock_c()->expectOneCall("XAie_BacktrackErrorInterrupts")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->BacktrackErrors() != nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph BacktrackErrors function: " << total_clock_enable_us << " microseconds" << std::endl;
}

#endif /* !defined(__AIESIM__) */

//*********************************************************************************/
/*
*		Unique test case ID: TC_052_CheckInitialized_BacktrackErrors
*		Function Name: AIEGraph::BacktrackErrors
*		Input: none
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_052_CheckInitialized_BacktrackErrors) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->BacktrackErrors() == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph BacktrackErrors function: " << total_clock_enable_us << " microseconds" << std::endl;
}

#if !defined(__AIESIM__)
//*********************************************************************************/
/*
*		Unique test case ID: TC_053_DumpCoreLog
*		Function Name: AIEGraph::dumpcorelog
*		Input: 0x2, 0x0
*		Expectation: ptr != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_053_DumpCoreLog) {

	int row = 0x2;
	int col = 0x0;
	char *ptr;
	graphInit_mocks_ok();
	mock_c()->expectOneCall("XAie_Read32")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	ptr = gr->dumpcorelog(row, col);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph dumpcorelog function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(ptr != nullptr);
}
#endif /* !defined(__AIESIM__) */
//*********************************************************************************/
/*
*		Unique test case ID: TC_054_Dumpcorelog_InvalidIndices
*		Function Name: AIEGraph::dumpcorelog
*		Input: -1, 0
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_054_Dumpcorelog_InvalidIndices) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->dumpcorelog(-1, 0) == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph dumpcorelog function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(gr->dumpcorelog(0, -1) == nullptr);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_055_CheckInitialized_dumpcorelog
*		Function Name: AIEGraph::dumpcorelog
*		Input: 0, 0
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_055_CheckInitialized_dumpcorelog) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->dumpcorelog(0, 0) == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph dumpcorelog function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_056_GetAieDevInst
*		Function Name: AIEGraph::getaiedevinst
*		Input: none
*		Expectation: ptr != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_056_GetAieDevInst) {

	void *ptr;
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	ptr = gr->getaiedevinst();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph getaiedevinst function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(ptr != nullptr);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_057_CheckInitialized_getaiedevinst
*		Function Name: AIEGraph::getaiedevinst
*		Input: none
*		Expectation: nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_057_CheckInitialized_getaiedevinst) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->getaiedevinst() == nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph getaiedevinst function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_058_GM2AIE_NB
*		Function Name: AIEGraph::gm2aie_nb
*		Input: gradf.in, input_data, len
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_058_GM2AIE_NB) {
	graphInit_mocks_ok();
	int len = 10;
	std::string gmio_name = "gradf.in";
	int32_t *input_data = (int32_t *)malloc(len * sizeof(int32_t));
	for (int i = 0; i < len; i++) {
		input_data[i] = i + 1;
	}
	aeg_err result = aeg_err::ok;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	result = gr->gm2aie_nb(&gmio_name[0], reinterpret_cast<char *>(&input_data[0]), len * sizeof(int32_t));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_059_GM2AIE_NB_DriverError
*		Function Name: AIEGraph::gm2aie_nb
*		Input: gradf.in, input_data, len
*		Expectation: result == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_059_GM2AIE_NB_DriverError) {
	graphInit_mocks_ok();
	int len = 10;
	std::string gmio_name = "gradf.in";
	int32_t *input_data = (int32_t *)malloc(len * sizeof(int32_t));
	XAieMem::s_get_dev_addr_ret = XAIE_ERR;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->gm2aie_nb(&gmio_name[0], reinterpret_cast<char *>(&input_data[0]), len * sizeof(int32_t));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_060_CheckInitialized_gm2aie_nb
*		Function Name: AIEGraph::gm2aie_nb
*		Input: gradf.in, io_buf
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_060_CheckInitialized_gm2aie_nb) {
	char gmio_name[] = "gradf.in";
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie_nb(gmio_name, io_buf, static_cast<int32_t>(sizeof(io_buf))) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_061_GM2AIE_NB_InvalidName
*		Function Name: AIEGraph::gm2aie_nb
*		Input: nullptr, io_buf, 10
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_061_GM2AIE_NB_InvalidName) {
	char *gmio_name = nullptr;
	graphInit_mocks_ok();
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie_nb(gmio_name, io_buf, 10) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_062_GM2AIE_NB_NullArgument
*		Function Name: AIEGraph::gm2aie_nb
*		Input: gradf.in, nullptr, 10
*		Expectation: aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_062_GM2AIE_NB_NullArgument) {
	char gmio_name[] = "gradf.in";
	graphInit_mocks_ok();
	char *io_buf = nullptr;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie_nb(gmio_name, io_buf, 10) == aeg_err::null_argument);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_063_GM2AIE_NB_InvalidLength
*		Function Name: AIEGraph::gm2aie_nb
*		Input: gradf.in, io_buf, 0
*		Expectation: aeg_err::invalid_length
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_063_GM2AIE_NB_InvalidLength) {
	char gmio_name[] = "gradf.in";
	graphInit_mocks_ok();
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie_nb(gmio_name, io_buf, 0) == aeg_err::invalid_length);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_064_GM2AIE_NB_Failure
*		Function Name: AIEGraph::gm2aie_nb
*		Input: gradf, input_data, len
*		Expectation: result == aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_064_GM2AIE_NB_Failure) {
	graphInit_mocks_ok();
	int len = 10;
	std::string gmio_name = "gradf";
	int32_t *input_data = (int32_t *)malloc(len * sizeof(int32_t)); // Allocate mock input data
	for (int i = 0; i < len; i++) {
		input_data[i] = i + 1;
	}
	aeg_err result = aeg_err::ok;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	result = gr->gm2aie_nb(&gmio_name[0], reinterpret_cast<char *>(&input_data[0]), len * sizeof(int32_t));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::backend_not_ready);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_065_AIE2GM_NB
*		Function Name: AIEGraph::aie2gm_nb
*		Input: gradf.out, output_data, len
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_065_AIE2GM_NB) {
	graphInit_mocks_ok();
	int len = 10;
	std::string gmio_name = "gradf.out";
	int32_t *output_data = (int32_t *)malloc(len * sizeof(int32_t));

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->aie2gm_nb(&gmio_name[0], reinterpret_cast<char *>(&output_data[0]), len * sizeof(int32_t));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_066_Check_aie2gm_nb_init_failure
*		Function Name: AIEGraph::aie2gm_nb
*		Input: gradf.in, nullptr, 10
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_066_Check_aie2gm_nb_init_failure) {
	char gmio_name[] = "gradf.in";
	char *io_buf = nullptr;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm_nb(gmio_name, nullptr, 10) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_067_AIE2GM_NB_InvalidName
*		Function Name: AIEGraph::aie2gm_nb
*		Input: nullptr, io_buf, 10
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_067_AIE2GM_NB_InvalidName) {
	char *gmio_name = nullptr;
	char io_buf[16] = {0};
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm_nb(gmio_name, io_buf, 10) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_067a_AIE2GM_NB_EmptyName
*		Function Name: AIEGraph::aie2gm_nb
*		Input: "", io_buf, 10
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of non-null empty C string validation
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_067a_AIE2GM_NB_EmptyName) {
	char gmio_name[] = "";
	char io_buf[16] = {0};
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm_nb(gmio_name, io_buf, 10) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb empty name function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_068_AIE2GM_NB_ZeroLength
*		Function Name: AIEGraph::aie2gm_nb
*		Input: gradf.in, io_buf, 0
*		Expectation: aeg_err::invalid_length
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_068_AIE2GM_NB_ZeroLength) {
	char gmio_name[] = "gradf.in";
	char io_buf[16] = {0};
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm_nb(gmio_name, io_buf, 0) == aeg_err::invalid_length);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_069_AIE2GM_NB_NullArgument
*		Function Name: AIEGraph::aie2gm_nb
*		Input: gradf.in, nullptr, 10
*		Expectation: aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_069_AIE2GM_NB_NullArgument) {
	char gmio_name[] = "gradf.in";
	char *io_buf = nullptr;
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm_nb(gmio_name, nullptr, 10) == aeg_err::null_argument);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_070_AIE2GM_NB_Failure
*		Function Name: AIEGraph::aie2gm_nb
*		Input: gradf, output_data, len
*		Expectation: result == aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_070_AIE2GM_NB_Failure) {
	graphInit_mocks_ok();
	int len = 10;
	std::string gmio_name = "gradf";
	int32_t *output_data = (int32_t *)malloc(len * sizeof(int32_t));
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->aie2gm_nb(&gmio_name[0], reinterpret_cast<char *>(&output_data[0]), len * sizeof(int32_t));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::backend_not_ready);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_071_GM2AIE
*		Function Name: AIEGraph::gm2aie
*		Input: gradf.in, input_data, len
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_071_GM2AIE) {
	graphInit_mocks_ok();
	int len = 10;
	int32_t *input_data = (int32_t *)malloc(len * sizeof(int32_t));
	for (int i = 0; i < len; i++) {
		input_data[i] = i + 1;
	}
	std::string gmio_name = "gradf.in";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->gm2aie(&gmio_name[0], reinterpret_cast<char *>(&input_data[0]), len * sizeof(int32_t));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_072_GM2AIE_DriverError
*		Function Name: AIEGraph::gm2aie
*		Input: gradf.in, input_data, len
*		Expectation: result == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_072_GM2AIE_DriverError) {
	graphInit_mocks_ok();
	int len = 10;
	int32_t *input_data = (int32_t *)malloc(len * sizeof(int32_t));
	std::string gmio_name = "gradf.in";
	XAieMem::s_get_dev_addr_ret = XAIE_ERR;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->gm2aie(&gmio_name[0], reinterpret_cast<char *>(&input_data[0]), len * sizeof(int32_t));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_073_CheckInitialized_gm2aie
*		Function Name: AIEGraph::gm2aie
*		Input: gradf.in, io_buf, 16
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_073_CheckInitialized_gm2aie) {
	char gmio_name[] = "gradf.in";
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie(gmio_name, io_buf, static_cast<int32_t>(sizeof(io_buf))) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_074_GM2AIE_InvalidName
*		Function Name: AIEGraph::gm2aie
*		Input: nullptr, io_buf, 10
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_074_GM2AIE_InvalidName) {
	char *gmio_name = nullptr;
	graphInit_mocks_ok();
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie(gmio_name, io_buf, 10) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_075_GM2AIE_NullArgument
*		Function Name: AIEGraph::gm2aie
*		Input: gradf.in, nullptr, 10
*		Expectation: aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_075_GM2AIE_NullArgument) {
	char gmio_name[] = "gradf.in";
	graphInit_mocks_ok();
	char *io_buf = nullptr;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie(gmio_name, io_buf, 10) == aeg_err::null_argument);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_076_GM2AIE_InvalidLength
*		Function Name: AIEGraph::gm2aie
*		Input: gradf.in, io_buf, 0
*		Expectation: aeg_err::invalid_length
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_076_GM2AIE_InvalidLength) {
	char gmio_name[] = "gradf.in";
	graphInit_mocks_ok();
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie(gmio_name, io_buf, 0) == aeg_err::invalid_length);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_077_GM2AIE_Failure
*		Function Name: AIEGraph::gm2aie
*		Input: gradf, input_data, len
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_077_GM2AIE_Failure) {
	graphInit_mocks_ok();
	int len = 10;
	int32_t *input_data = (int32_t *)malloc(len * sizeof(int32_t));
	for (int i = 0; i < len; i++) {
		input_data[i] = i + 1;
	}
	std::string gmio_name = "gradf";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->gm2aie(&gmio_name[0], reinterpret_cast<char *>(&input_data[0]), len * sizeof(int32_t));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::backend_not_ready);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_078_AIE2GM
*		Function Name: AIEGraph::aie2gm
*		Input: gradf.out, output_data, len
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_078_AIE2GM) {
	graphInit_mocks_ok();
	int len = 10;
	int32_t *output_data = (int32_t *)malloc(len * sizeof(int32_t));
	std::string gmio_name = "gradf.out";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->aie2gm(&gmio_name[0], reinterpret_cast<char *>(&output_data[0]), len * sizeof(int32_t));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_079_CheckInitialized_aie2gm
*		Function Name: AIEGraph::aie2gm
*		Input: gradf.in, io_buf, 16
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_079_CheckInitialized_aie2gm) {
	char gmio_name[] = "gradf.in";
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm(gmio_name, io_buf, static_cast<int32_t>(sizeof(io_buf))) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_080_AIE2GM_InvalidName
*		Function Name: AIEGraph::aie2gm
*		Input: nullptr, io_buf, 10
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_080_AIE2GM_InvalidName) {
	char *gmio_name = nullptr;
	char io_buf[16] = {0};
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm(gmio_name, io_buf, 10) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_081_AIE2GM_NullArgument
*		Function Name: AIEGraph::aie2gm
*		Input: gradf.in, nullptr, 10
*		Expectation: aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_081_AIE2GM_NullArgument) {
	char gmio_name[] = "gradf.in";
	char *io_buf = nullptr;
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm(gmio_name, nullptr, 10) == aeg_err::null_argument);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_082_AIE2GM_ZeroLength
*		Function Name: AIEGraph::aie2gm
*		Input: gradf.in, io_buf, 0
*		Expectation: aeg_err::invalid_length
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_082_AIE2GM_ZeroLength) {
	char gmio_name[] = "gradf.in";
	char io_buf[16] = {0};
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm(gmio_name, io_buf, 0) == aeg_err::invalid_length);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_083_AIE2GM_Failure
*		Function Name: AIEGraph::aie2gm
*		Input: gradf, output_data, len
*		Expectation: result == aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_083_AIE2GM_Failure) {
	graphInit_mocks_ok();
	int len = 10;
	int32_t *output_data = (int32_t *)malloc(len * sizeof(int32_t));
	std::string gmio_name = "gradf";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->aie2gm(&gmio_name[0], reinterpret_cast<char *>(&output_data[0]), len * sizeof(int32_t));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::backend_not_ready);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_084_GMIO_WAIT
*		Function Name: AIEGraph::gmio_wait
*		Input: gradf.out
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_084_GMIO_WAIT) {
	graphInit_mocks_ok();
	std::string gmio_name = "gradf.out";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->gmio_wait(gmio_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gmio_wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_085_CheckInitialized_gmio_wait
*		Function Name: AIEGraph::gmio_wait
*		Input: gradf.in, true
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_085_CheckInitialized_gmio_wait) {
	char gmio_name[] = "gradf.in";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gmio_wait(gmio_name, true) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gmio_wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_086_GMIO_WAIT_InvalidName
*		Function Name: AIEGraph::gmio_wait
*		Input: "", true
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_086_GMIO_WAIT_InvalidName) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gmio_wait(std::string(""), true) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gmio_wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_087_GMIO_WAIT_Failure
*		Function Name: AIEGraph::gmio_wait
*		Input: gradf
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_087_GMIO_WAIT_Failure) {
	graphInit_mocks_ok();
	std::string gmio_name = "gradf";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->gmio_wait(gmio_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gmio_wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::backend_not_ready);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_088_GMIO_WAIT_NonBlockingWaitFailure
*		Function Name: AIEGraph::gmio_wait
*		Input: gradf.out, false
*		Expectation: aeg_err::resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_088_GMIO_WAIT_NonBlockingWaitFailure) {
	graphInit_mocks_ok();
	std::string gmio_name = "gradf.out";
	g_ut_gmio_wait_nb_fail = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->gmio_wait(gmio_name, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gmio_wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_gmio_wait_nb_fail = 0;
	CHECK(result == aeg_err::resource_unavailable);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_089_ErrorHandlingCallbackInit
*		Function Name: AIEGraph::error_handling_cb_init
*		Input: callback, privData
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_089_ErrorHandlingCallbackInit) {

	graphInit_mocks_ok();
	void (*callback)(std::vector<XAie_ErrorPayload>, void *) = [](std::vector<XAie_ErrorPayload>, void *) {};
	void *privData = (void *)0xDEADBEEF;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->error_handling_cb_init((void *)callback, privData);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph error_handling_cb_init function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
	CHECK(true);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_090_CheckInitialized_error_handling_cb_init
*		Function Name: AIEGraph::error_handling_cb_init
*		Input: nullptr, nullptr
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_090_CheckInitialized_error_handling_cb_init) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->error_handling_cb_init(reinterpret_cast<void *>(0x1), nullptr) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph error_handling_cb_init function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_091_ErrorHandlingCallbackInit_NullCallback
*		Function Name: AIEGraph::error_handling_cb_init
*		Input: nullptr, priv
*		Expectation: aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_091_ErrorHandlingCallbackInit_NullCallback) {
	void *cb = nullptr;
	void *priv = (void *)0xDEADBEEF;
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->error_handling_cb_init(cb, priv) == aeg_err::null_argument);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph error_handling_cb_init function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_092_CheckInitialized_start_profiling_single_port
*		Function Name: AIEGraph::start_profiling
*		Input: gradf.in, 0x20, 100
*		Expectation: -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_092_CheckInitialized_start_profiling_single_port) {
	char gmio_name[] = "gradf.in";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->start_profiling(gmio_name, 0x20, 100) == -1);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_093_StartProfilingSinglePort_EmptyNameAfterInit
*		Function Name: AIEGraph::start_profiling
*		Input: "", 0x20, 100
*		Expectation: -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_093_StartProfilingSinglePort_EmptyNameAfterInit) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->start_profiling(std::string(""), 0x20, 100) == -1);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_094_StartProfilingSinglePort_InvalidOptions
*		Function Name: AIEGraph::start_profiling
*		Input: gradf.in, 0x24, 100
*		Expectation: -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_094_StartProfilingSinglePort_InvalidOptions) {
	graphInit_mocks_ok();
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->start_profiling("gradf.in", 0x19, 100) == -1);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	double total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	const auto clock_enable_t2 = std::chrono::steady_clock::now();
	CHECK(gr->start_profiling("gradf.in", 0x24, 100) == -1);
	const auto clock_enable_t3 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t3 - clock_enable_t2).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_095_StartProfilingSinglePort_EventApi_NullBackendNotReady
*		Function Name: AIEGraph::start_profiling
*		Input: gradf.in, 0x20, 100
*		Expectation: -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_095_StartProfilingSinglePort_EventApi_NullBackendNotReady) {
	graphInit_mocks_ok();
	gr->reset_EventApi();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->start_profiling("gradf.in", 0x20, 100) == -1);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif
//*********************************************************************************/
/*
*		Unique test case ID: TC_096_StartProfiling_Success_RunIdle
*		Function Name: AIEGraph::start_profiling
*		Input: test_port, 0x20, 100
*		Expectation: handle == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_096_StartProfiling_Success_RunIdle) {
	graphInit_mocks_ok();
	set_profiling_stub_mode(true);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int handle = gr->start_profiling("test_port", 0x20, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(handle == 0);
	set_profiling_stub_mode(false);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_097_StartProfiling_Success_StartBytes
*		Function Name: AIEGraph::start_profiling
*		Input: gradf.in, 0x21, 100
*		Expectation: handle == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_097_StartProfiling_Success_StartBytes) {
	graphInit_mocks_ok();
	set_profiling_stub_mode(true);


	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int handle = gr->start_profiling("gradf.in", 0x21, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(handle == 0);
	set_profiling_stub_mode(false);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_098_StartProfiling_Success_EventCount
*		Function Name: AIEGraph::start_profiling
*		Input: test_port, 0x23, 100
*		Expectation: handle == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_098_StartProfiling_Success_EventCount) {
	graphInit_mocks_ok();
	set_profiling_stub_mode(true);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int handle = gr->start_profiling("test_port", 0x23, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(handle == 0);
	set_profiling_stub_mode(false);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_099_CheckInitialized_start_profiling_two_ports
*		Function Name: AIEGraph::start_profiling
*		Input: gradf.in, gradf.out, 0x22, 100
*		Expectation: -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_099_CheckInitialized_start_profiling_two_ports) {
	char gmio_name[] = "gradf.in";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->start_profiling(gmio_name, gmio_name, 0x22, 100) == -1);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_100_StartProfilingTwoPorts_EmptyNameAfterInit
*		Function Name: AIEGraph::start_profiling
*		Input: "", "gradf.out", 0x22, 100
*		Expectation: -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_100_StartProfilingTwoPorts_EmptyNameAfterInit) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->start_profiling(std::string(""), std::string("gradf.out"), 0x22, 100) == -1);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(gr->start_profiling(std::string("gradf.in"), std::string(""), 0x22, 100) == -1);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_101_StartProfilingTwoPorts_InvalidOptions
*		Function Name: AIEGraph::start_profiling
*		Input: gradf.in, gradf.out, 0x19, 100
*		Expectation: -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_101_StartProfilingTwoPorts_InvalidOptions) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->start_profiling("gradf.in", "gradf.out", 0x19, 100) == -1);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(gr->start_profiling("gradf.in", "gradf.out", 0x24, 100) == -1);
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_102_StartProfilingTwoPort_EventApi_NullBackendNotReady
*		Function Name: AIEGraph::start_profiling
*		Input: gradf.in, gradf.out, 0x22, 100
*		Expectation: -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_102_StartProfilingTwoPort_EventApi_NullBackendNotReady) {
	graphInit_mocks_ok();
	gr->reset_EventApi();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->start_profiling("gradf.in", "gradf.out", 0x22, 100) == -1);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_103a_StartProfiling_PlioPortName_HitsPlioNonNull
*		Function Name: AIEGraph::start_profiling
*		Input: gradf2.pin, gradf2.pout, profiling options 0x20 and 0x23, threshold = 100
*		Expectation: pinHandle == 0, poutHandle == 1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_103a_StartProfiling_PlioPortName_HitsPlioNonNull) {
	double total_clock_enable_us = 0.0;

	graphInit_mocks_ok();
	set_profiling_stub_mode(true);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int pinHandle = gr->start_profiling("gradf2.pin", 0x20, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, pinHandle);
	int poutHandle = gr->start_profiling("gradf2.pout", 0x23, 100);
	CHECK_EQUAL(1, poutHandle);
	set_profiling_stub_mode(false);


}

//*********************************************************************************/
/*
*		Unique test case ID: TC_103_StartProfilingTwoPorts_Success_DiffCycles
*		Function Name: AIEGraph::start_profiling
*		Input: test_port1, test_port2, 0x22, 100
*		Expectation: handle == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_103_StartProfilingTwoPorts_Success_DiffCycles) {
	graphInit_mocks_ok();
	set_profiling_stub_mode(true);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int handle = gr->start_profiling("test_port1", "test_port2", 0x22, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(handle == 0);
	set_profiling_stub_mode(false);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_103b_StartProfiling_AmbiguousPort_GmioAndPlio
*		Function Name: AIEGraph::start_profiling
*		Input: ambiguous_port , 0x20, 100
*		Expectation: handle == 0; AEG_ERROR for ambiguous port logged
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of event::get_shim_config()
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_103b_StartProfiling_AmbiguousPort_GmioAndPlio) {
	graphInit_mocks_ok();
	set_profiling_stub_mode(true);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int handle = gr->start_profiling("ambiguous_port", 0x20, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(handle == 0);
	const auto clock_enable_t2 = std::chrono::steady_clock::now();
	int handleEvt = gr->start_profiling("ambiguous_port", 0x23, 100);
	const auto clock_enable_t3 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t3 - clock_enable_t2).count();
	std::cout << "Time taken by AIEGraph start_profiling function (second call): " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(handleEvt == 1);
	set_profiling_stub_mode(false);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_103c_StartProfiling_StartBytes_Failure
*		Function Name: AIEGraph::start_profiling
*		Input: gradf.in, 0x21, 100 with profiling stub returning failure
*		Expectation: handle == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of start_profiling_start_bytes
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_103c_StartProfiling_StartBytes_Failure) {
	graphInit_mocks_ok();
	set_profiling_stub_mode(false);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int handle = gr->start_profiling("gradf.in", 0x21, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(handle == -1);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_103d_StartProfiling_DiffCycles_Failure
*		Function Name: AIEGraph::start_profiling
*		Input: gradf.in, gradf.out, 0x22, 100 with profiling stub returning failure
*		Expectation: handle == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of start_profiling_diff_cycles
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_103d_StartProfiling_DiffCycles_Failure) {
	graphInit_mocks_ok();
	set_profiling_stub_mode(false);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int handle = gr->start_profiling("gradf.in", "gradf.out", 0x22, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(handle == -1);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_103e_StartProfiling_EventCount_Failure
*		Function Name: AIEGraph::start_profiling
*		Input: gradf.in, 0x23, 100 with profiling stub returning failure
*		Expectation: handle == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of start_profiling_event_count
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_103e_StartProfiling_EventCount_Failure) {
	graphInit_mocks_ok();
	set_profiling_stub_mode(false);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int handle = gr->start_profiling("gradf.in", 0x23, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(handle == -1);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_104_CheckInitialized_read_profiling
*		Function Name: AIEGraph::read_profiling
*		Input: 0
*		Expectation: -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_104_CheckInitialized_read_profiling) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->read_profiling(0) == static_cast<long long>(-1));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_105_ReadProfiling_EventApi_NullBackendNotReady
*		Function Name: AIEGraph::read_profiling
*		Input: 0
*		Expectation: -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_105_ReadProfiling_EventApi_NullBackendNotReady) {
	graphInit_mocks_ok();
	gr->reset_EventApi();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->read_profiling(0) == static_cast<int64_t>(-1));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_106_ReadProfiling_InvalidHandle
*		Function Name: AIEGraph::read_profiling
*		Input: handle
*		Expectation: data != 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_106_ReadProfiling_InvalidHandle) {

	graphInit_mocks_ok();
	int handle = gr->start_profiling("test_port", 0x20, 100);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	long long data = gr->read_profiling(handle);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(data != 0);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_107_ReadProfiling_Success_ValidHandle
*		Function Name: AIEGraph::read_profiling
*		Input: handle
*		Expectation: >= 1000
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_107_ReadProfiling_Success_ValidHandle) {
	graphInit_mocks_ok();
	set_profiling_stub_mode(true);
	int handle = gr->start_profiling("test_port", 0x20, 100);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	long long result = gr->read_profiling(handle);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result >= 1000);

	set_profiling_stub_mode(false);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_108_CheckInitialized_stop_profiling
*		Function Name: AIEGraph::stop_profiling
*		Input: 0
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_108_CheckInitialized_stop_profiling) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->stop_profiling(0) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph stop_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_109_StopProfiling_InvalidHandle
*		Function Name: AIEGraph::stop_profiling
*		Input: handle
*		Expectation: result == aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_109_StopProfiling_InvalidHandle) {

	graphInit_mocks_ok();
	int handle = gr->start_profiling("test_port", 0x20, 100);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->stop_profiling(handle);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph stop_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::invalid_state);
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_110_StopProfiling_EventApi_NullBackendNotReady
*		Function Name: AIEGraph::stop_profiling
*		Input: 0
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_110_StopProfiling_EventApi_NullBackendNotReady) {
	graphInit_mocks_ok();
	gr->reset_EventApi();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->stop_profiling(0) == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph stop_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_111_StopProfiling_Success_ValidHandle
*		Function Name: AIEGraph::stop_profiling
*		Input: handle
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_111_StopProfiling_Success_ValidHandle) {
	graphInit_mocks_ok();
	set_profiling_stub_mode(true);

	int handle = gr->start_profiling("test_port", 0x20, 100);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->stop_profiling(handle);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph stop_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
	CHECK(true);
	set_profiling_stub_mode(false);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_112_SetAddress_Success_ValidHandle
*		Function Name: AIEGraph::setAddress
*		Input: ext_name, testPtr
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_112_SetAddress_Success_ValidHandle) {

	void *testPtr = malloc(128);
	graphInit_mocks_ok();
	aeg_err result = aeg_err::ok;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	result = gr->setAddress((char *)"EG.ddrin", testPtr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setAddress function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
	free(testPtr);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_113_CheckInitialized_setAddress
*		Function Name: AIEGraph::setAddress
*		Input: ext_name, dummy_ptr
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_113_CheckInitialized_setAddress) {
	char ext_name[] = "EG.ddrin";
	char io_buf[16] = {0};
	void *dummy_ptr = static_cast<void *>(io_buf);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->setAddress(ext_name, dummy_ptr) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setAddress function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_114_SetAddress_InvalidName
*		Function Name: AIEGraph::setAddress
*		Input: nullptr, io_buf
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_114_SetAddress_InvalidName) {
	graphInit_mocks_ok();
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->setAddress(nullptr, static_cast<void *>(io_buf)) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setAddress function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_115_SetAddress_NullArgument
*		Function Name: AIEGraph::setAddress
*		Input: "EG.ddrin", nullptr
*		Expectation: aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_115_SetAddress_NullArgument) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->setAddress((char *)"EG.ddrin", nullptr) == aeg_err::null_argument);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setAddress function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_116_SetAddress_Failure_InvalidExternalBufferName
*		Function Name: AIEGraph::setAddress
*		Input: "EG", testPtr
*		Expectation: result == aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_116_SetAddress_Failure_InvalidExternalBufferName) {

	void *testPtr = malloc(128);
	graphInit_mocks_ok();

	aeg_err result = aeg_err::ok;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	result = gr->setAddress((char *)"EG", testPtr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setAddress function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::backend_not_ready);
	free(testPtr);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_117_WaitExternalBuffer
*		Function Name: AIEGraph::wait
*		Input: "EG.ddrin", "graph"
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_117_WaitExternalBuffer) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->wait((char *)"EG.ddrin", "graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_118_CheckInitialized_wait_external_buffer
*		Function Name: AIEGraph::wait
*		Input: ext_name, ext_port
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_118_CheckInitialized_wait_external_buffer) {
	char ext_name[] = "EG.ddrin";
	char ext_port[] = "graph";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->wait(ext_name, ext_port) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_119_WaitExternalBuffer_InvalidBufferName
*		Function Name: AIEGraph::wait
*		Input: nullptr, "graph"
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_119_WaitExternalBuffer_InvalidBufferName) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->wait(nullptr, std::string("graph")) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_120_WaitExternalBuffer_InvalidPortName
*		Function Name: AIEGraph::wait
*		Input: "EG.ddrin", ""
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_120_WaitExternalBuffer_InvalidPortName) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->wait((char *)"EG.ddrin", std::string("")) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_121_WaitExternalBuffer_Failure_InvalidExternalBufferName
*		Function Name: AIEGraph::wait
*		Input: "EG", "graph"
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_121_WaitExternalBuffer_Failure_InvalidExternalBufferName) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->wait((char *)"EG", "graph") == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_122_GM2AIE_NB_ExternalBuffer
*		Function Name: AIEGraph::gm2aie_nb
*		Input: "EG.ddrin", "graph"
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_122_GM2AIE_NB_ExternalBuffer) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->gm2aie_nb((char *)"EG.ddrin", "graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_123_CheckInitialized_gm2aie_nb_external
*		Function Name: AIEGraph::gm2aie_nb
*		Input: "EG.ddrin", "graph"
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_123_CheckInitialized_gm2aie_nb_external) {
	char ext_name[] = "EG.ddrin";
	char ext_port[] = "graph";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie_nb(ext_name, ext_port) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_124_GM2AIE_NB_ExternalBuffer_InvalidBufferName
*		Function Name: AIEGraph::gm2aie_nb
*		Input: nullptr, "graph"
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_124_GM2AIE_NB_ExternalBuffer_InvalidBufferName) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie_nb(nullptr, std::string("graph")) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_125_GM2AIE_NB_ExternalBuffer_InvalidPortName
*		Function Name: AIEGraph::gm2aie_nb
*		Input: "EG.ddrin", ""
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_125_GM2AIE_NB_ExternalBuffer_InvalidPortName) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->gm2aie_nb((char *)"EG.ddrin", std::string("")) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_126_GM2AIE_NB_ExternalBuffer_Failure_InvalidExternalBufferName
*		Function Name: AIEGraph::gm2aie_nb
*		Input: "EG", "graph"
*		Expectation: result == aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_126_GM2AIE_NB_ExternalBuffer_Failure_InvalidExternalBufferName) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->gm2aie_nb((char *)"EG", "graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph gm2aie_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::backend_not_ready);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_127_CheckInitialized_aie2gm_nb_external
*		Function Name: AIEGraph::aie2gm_nb
*		Input: "EG.ddrin", "graph"
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_127_CheckInitialized_aie2gm_nb_external) {
	char ext_name[] = "EG.ddrin";
	char ext_port[] = "graph";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm_nb(ext_name, ext_port) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_128_AIE2GM_NB_ExternalBuffer_InvalidBufferName
*		Function Name: AIEGraph::aie2gm_nb
*		Input: nullptr, "graph"
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_128_AIE2GM_NB_ExternalBuffer_InvalidBufferName) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm_nb(nullptr, std::string("graph")) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_129_AIE2GM_NB_ExternalBuffer_InvalidPortName
*		Function Name: AIEGraph::aie2gm_nb
*		Input: "EG.ddrin", ""
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_129_AIE2GM_NB_ExternalBuffer_InvalidPortName) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->aie2gm_nb((char *)"EG.ddrin", std::string("")) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_130_AIE2GM_NB_ExternalBuffer_Failure_InvalidExternalBufferName
*		Function Name: AIEGraph::aie2gm_nb
*		Input: "EG", "graph"
*		Expectation: result == aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_130_AIE2GM_NB_ExternalBuffer_Failure_InvalidExternalBufferName) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->aie2gm_nb((char *)"EG", "graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::backend_not_ready);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_131_AIE2GM_NB_ExternalBuffer
*		Function Name: AIEGraph::aie2gm_nb
*		Input: "EG.ddrin", "graph"
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_131_AIE2GM_NB_ExternalBuffer) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->aie2gm_nb((char *)"EG.ddrin", "graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph aie2gm_nb function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_131_UpdateRTP
*		Function Name: AIEGraph::update
*		Input: "gradf.sq.in[1]", testData, testDataSize
*		Expectation: result == aeg_err::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_131_UpdateRTP) {

	int testDataSize = 4;
	uint8_t testData[4];
	graphInit_mocks_ok();
	for (size_t i = 0; i < testDataSize; i++) {
		testData[i] = static_cast<uint8_t>(i & 0xFF);
	}
	g_ut_graph_api_update_rtp_user_error = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->update("gradf.sq.in[1]", testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_graph_api_update_rtp_user_error = 0;
	CHECK(result == aeg_err::user_error);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_131a_UpdateRTP_InternalError
*		Function Name: AIEGraph::update
*		Input: "gradf.sq.in[1]", buf, sizeof(buf)
*		Expectation: result == aeg_err::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_131a_UpdateRTP_InternalError) {
	double total_clock_enable_us = 0.0;

	uint8_t buf[4] = {0};
	graphInit_mocks_ok();
	g_ut_graph_api_update_force_code = ENOTSUP;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->update("gradf.sq.in[1]", buf, sizeof(buf));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_graph_api_update_force_code = 0;
	CHECK(result == aeg_err::internal_error);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_131b_UpdateRTP_AieDriverError
*		Function Name: AIEGraph::update
*		Input: "gradf.sq.in[1]", buf, sizeof(buf)
*		Expectation: result == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_131b_UpdateRTP_AieDriverError) {
	double total_clock_enable_us = 0.0;
	uint8_t buf[4] = {0};
	graphInit_mocks_ok();
	g_ut_graph_api_update_force_code = EIO;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->update("gradf.sq.in[1]", buf, sizeof(buf));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_graph_api_update_force_code = 0;
	CHECK(result == aeg_err::aie_driver_error);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_131c_UpdateRTP_ResourceUnavailable
*		Function Name: AIEGraph::update
*		Input: "gradf.sq.in[1]", buf, sizeof(buf)
*		Expectation: result == aeg_err::resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_131c_UpdateRTP_ResourceUnavailable) {
	double total_clock_enable_us = 0.0;
	uint8_t buf[4] = {0};
	graphInit_mocks_ok();
	g_ut_graph_api_update_force_code = EAGAIN;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->update("gradf.sq.in[1]", buf, sizeof(buf));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_graph_api_update_force_code = 0;
	CHECK(result == aeg_err::resource_unavailable);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_131d_UpdateRTP_UnknownErrorDefaultsToDriverError
*		Function Name: AIEGraph::update -> err_from_abr
*		Input: graph_api::update() forced to return an unmapped err_code value
*		Expectation: result == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of err_from_abr default switch arm
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_131d_UpdateRTP_UnknownErrorDefaultsToDriverError) {
	double total_clock_enable_us = 0.0;
	uint8_t buf[4] = {0};
	graphInit_mocks_ok();
	g_ut_graph_api_update_force_code = 123;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->update("gradf.sq.in[1]", buf, sizeof(buf));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update default error mapping function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_graph_api_update_force_code = 0;
	CHECK(result == aeg_err::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_132_UpdateRTP_failure
*		Function Name: AIEGraph::update
*		Input: "EG", testData, testDataSize
*		Expectation: result == aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_132_UpdateRTP_failure) {

	int testDataSize = 4;
	uint8_t testData[4];
	graphInit_mocks_ok();
	for (size_t i = 0; i < testDataSize; i++) {
		testData[i] = static_cast<uint8_t>(i & 0xFF);
	}
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->update("EG", testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::invalid_name);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_133_CheckInitialized_update
*		Function Name: AIEGraph::update
*		Input: rtp_name, update_buf, sizeof(update_buf)
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_133_CheckInitialized_update) {
	char rtp_name[] = "gradf.sq.in[1]";
	uint8_t update_buf[8] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->update(rtp_name, update_buf, sizeof(update_buf)) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_134_Update_InvalidName
*		Function Name: AIEGraph::update
*		Input: "", update_buf, sizeof(update_buf)
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_134_Update_InvalidName) {
	graphInit_mocks_ok();
	uint8_t update_buf[8] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->update(std::string(""), update_buf, sizeof(update_buf)) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_135_Update_NullArgument
*		Function Name: AIEGraph::update
*		Input: "gradf.sq.in[1]", nullptr, 8
*		Expectation: aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_135_Update_NullArgument) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->update(std::string("gradf.sq.in[1]"), nullptr, 8) == aeg_err::null_argument);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_136_Update_InvalidLength
*		Function Name: AIEGraph::update
*		Input: "gradf.sq.in[1]", update_buf, 0
*		Expectation: aeg_err::invalid_length
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_136_Update_InvalidLength) {
	graphInit_mocks_ok();
	uint8_t update_buf[8] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->update(std::string("gradf.sq.in[1]"), update_buf, 0) == aeg_err::invalid_length);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_137_Update_GraphApi_null_backend_not_ready
*		Function Name: AIEGraph::update
*		Input: "gradf.sq.in[1]", buf, sizeof(buf)
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_137_Update_GraphApi_null_backend_not_ready) {
	graphInit_mocks_ok();
	gr->reset_GraphApi();
	uint8_t buf[4] = {};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->update("gradf.sq.in[1]", buf, sizeof(buf)) == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_138_UpdateSharedBuffer
*		Function Name: AIEGraph::update
*		Input: "EG.input", testData, kNumBytes
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_138_UpdateSharedBuffer) {
	const size_t kNumBytes = 108U;
	uint8_t testData[kNumBytes];
	for (size_t i = 0; i < kNumBytes; i++) {
		testData[i] = static_cast<uint8_t>(i & 0xFFU);
	}
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->update("EG.input", testData, kNumBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_139_CheckInitialized_setupPingPongBuffers
*		Function Name: AIEGraph::setupPingPongBuffers
*		Input: ext_name, dummy_ptr, dummy_ptr
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_139_CheckInitialized_setupPingPongBuffers) {
	char ext_name[] = "EG.ddrin";
	char io_buf[16] = {0};
	void *dummy_ptr = static_cast<void *>(io_buf);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->setupPingPongBuffers(ext_name, dummy_ptr, dummy_ptr) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_140_SetupPingPongBuffers_InvalidName
*		Function Name: AIEGraph::setupPingPongBuffers
*		Input: nullptr, p, p
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_140_SetupPingPongBuffers_InvalidName) {
	graphInit_mocks_ok();
	char buf[16] = {0};
	void *p = static_cast<void *>(buf);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->setupPingPongBuffers(nullptr, p, p) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_141_SetupPingPongBuffers_NullArgument_ptr1
*		Function Name: AIEGraph::setupPingPongBuffers
*		Input: "EG.ddrin", nullptr, p
*		Expectation: aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_141_SetupPingPongBuffers_NullArgument_ptr1) {
	graphInit_mocks_ok();
	char buf[16] = {0};
	void *p = static_cast<void *>(buf);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->setupPingPongBuffers((char *)"EG.ddrin", nullptr, p) == aeg_err::null_argument);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_142_Check_setupPingPongBuffers_null_argument_ptr2
*		Function Name: AIEGraph::setupPingPongBuffers
*		Input: "EG.ddrin", p, nullptr
*		Expectation: aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_142_Check_setupPingPongBuffers_null_argument_ptr2) {
	graphInit_mocks_ok();
	char buf[16] = {0};
	void *p = static_cast<void *>(buf);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->setupPingPongBuffers((char *)"EG.ddrin", p, nullptr) == aeg_err::null_argument);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_143_SetupPingPongBuffers_valid_external_buffer_name
*		Function Name: AIEGraph::setupPingPongBuffers
*		Input: "EG.ddrin", ptr1, ptr2
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_143_SetupPingPongBuffers_valid_external_buffer_name) {

	graphInit_mocks_ok();
	void *ptr1 = malloc(128);
	void *ptr2 = malloc(128);
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->setupPingPongBuffers("EG.ddrin", ptr1, ptr2);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
	free(ptr1);
	free(ptr2);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_143_SetupPingPongBuffers_Failure_InvalidExternalBufferName
*		Function Name: AIEGraph::setupPingPongBuffers
*		Input: "EG", ptr1, ptr2
*		Expectation: result == aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_143_SetupPingPongBuffers_Failure_InvalidExternalBufferName) {

	graphInit_mocks_ok();
	void *ptr1 = malloc(128);
	void *ptr2 = malloc(128);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->setupPingPongBuffers("EG", ptr1, ptr2);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph setupPingPongBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::backend_not_ready);
	free(ptr1);
	free(ptr2);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_144_CheckInitialized_read
*		Function Name: AIEGraph::read
*		Input: rtp_name, io_buf, sizeof(io_buf)
*		Expectation: aeg_err::invalid_state
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_144_CheckInitialized_read) {
	char rtp_name[] = "gradf.sq.in[1]";
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->read(rtp_name, io_buf, static_cast<int32_t>(sizeof(io_buf))) == aeg_err::invalid_state);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_145_Read_InvalidName
*		Function Name: AIEGraph::read
*		Input: "", io_buf, sizeof(io_buf)
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_145_Read_InvalidName) {
	graphInit_mocks_ok();
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->read(std::string(""), io_buf, static_cast<int32_t>(sizeof(io_buf))) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_146_Read_NullArgument
*		Function Name: AIEGraph::read
*		Input: "gradf.sq.in[1]", nullptr, 16
*		Expectation: aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_146_Read_NullArgument) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->read(std::string("gradf.sq.in[1]"), nullptr, 16) == aeg_err::null_argument);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_147_Read_InvalidLength
*		Function Name: AIEGraph::read
*		Input: "gradf.sq.in[1]", io_buf, 0
*		Expectation: aeg_err::invalid_length
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_147_Read_InvalidLength) {
	graphInit_mocks_ok();
	char io_buf[16] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->read(std::string("gradf.sq.in[1]"), io_buf, 0) == aeg_err::invalid_length);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#ifdef __BAREMETAL_UTESTS__
//*********************************************************************************/
/*
*		Unique test case ID: TC_148_Read_GraphApi_null_backend_not_ready
*		Function Name: AIEGraph::read
*		Input: "gradf.sq.in[1]", rbuf, sizeof(rbuf)
*		Expectation: aeg_err::backend_not_ready
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_148_Read_GraphApi_null_backend_not_ready) {
	graphInit_mocks_ok();
	gr->reset_GraphApi();
	char rbuf[4] = {};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->read("gradf.sq.in[1]", rbuf, static_cast<int32_t>(sizeof(rbuf))) == aeg_err::backend_not_ready);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read function: " << total_clock_enable_us << " microseconds" << std::endl;
}
#endif  /* __BAREMETAL_UTESTS__ */
//*********************************************************************************/
/*
*		Unique test case ID: TC_149_Read_invalid_rtp_name
*		Function Name: AIEGraph::read
*		Input: "rtp_port", buf, sizeof(buf)
*		Expectation: aeg_err::invalid_name
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_149_Read_invalid_rtp_name) {
	graphInit_mocks_ok();
	char buf[8] = {};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->read("rtp_port", buf, static_cast<int32_t>(sizeof(buf))) == aeg_err::invalid_name);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_150_Read_InputRtp_ok
*		Function Name: AIEGraph::read
*		Input: "gradf.sq.in[1]", io_buf, sizeof(io_buf)
*		Expectation: result == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_150_Read_InputRtp_ok) {
	graphInit_mocks_ok();
	mock_c()->expectOneCall("XAie_DataMemRdWord")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockRead")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	char io_buf[4] = {0};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err result = gr->read(std::string("gradf.sq.in[1]"), io_buf, static_cast<int32_t>(sizeof(io_buf)));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph read function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == aeg_err::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_151_GetGraphImpl_AfterInit
*		Function Name: AIEGraph::getGraphImpl
*		Input: None
*		Expectation: non-null pointer
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_151_GetGraphImpl_AfterInit) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gr->getGraphImpl() != nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph getGraphImpl function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_152_StartProfiling_UnSuccess_EventCount
*		Function Name: AIEGraph::start_profiling
*		Input: "test_port", 0x22, 100
*		Expectation: handle == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AIEGraphTests, TC_152_StartProfiling_UnSuccess_EventCount) {
	graphInit_mocks_ok();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int handle = gr->start_profiling("test_port", 0x22, 100);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by AIEGraph start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(handle == -1);
}

TEST_GROUP(AbrInitIsrTests) {
	void setup() { mock_c()->clear(); }

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_NominalInitialization
*		Function Name: abr_init_isr
*		Input: void
*		Expectation: None
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AbrInitIsrTests, TC_001_NominalInitialization) {
// #if defined(__UTEST_ABR_ISR_COVERAGE_PATH__) || !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__BAREMETAL_UTESTS__) || defined(__ZEPHYR__))
// 	XScuGic_Config cfg = {};
// 	cfg.CpuBaseAddress = 0x1000U;

// 	mock_c()->expectOneCall("Xil_ExceptionInit");
// 	mock_c()->expectOneCall("XScuGic_DeviceInitialize")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
// 	mock_c()->expectOneCall("XScuGic_LookupConfig")->ignoreOtherParameters()->andReturnPointerValue(&cfg);
// 	mock_c()->expectOneCall("XScuGic_CfgInitialize")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
// 	mock_c()->expectOneCall("Xil_ExceptionRegisterHandler")
// 		->ignoreOtherParameters()
// 		->andReturnIntValue(XAIE_OK);
// 	mock_c()->expectOneCall("XScuGic_Connect")
// 		->ignoreOtherParameters()
// 		->andReturnIntValue(XAIE_OK);
// 	mock_c()->expectOneCall("XScuGic_Enable")
// 		->ignoreOtherParameters()
// 		->andReturnIntValue(XAIE_OK);
// #endif

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr_init_isr();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr_init_isr function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_002_NominalInitialization
*		Function Name: abr_init_isr
*		Input: void
*		Expectation: None
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AbrInitIsrTests, TC_002_NominalInitialization) {
	// #if defined(__UTEST_ABR_ISR_COVERAGE_PATH__) || !(defined(__AIESIM__) || defined(__ADF_FRONTEND__) || defined(__BAREMETAL_UTESTS__) || defined(__ZEPHYR__))
	// 	XScuGic_Config cfg = {};
	// 	cfg.CpuBaseAddress = 0x1000U;

	// 	mock_c()->expectOneCall("Xil_ExceptionInit");
	// 	mock_c()->expectOneCall("XScuGic_DeviceInitialize")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	// 	mock_c()->expectOneCall("XScuGic_LookupConfig")->ignoreOtherParameters()->andReturnPointerValue(&cfg);
	// 	mock_c()->expectOneCall("XScuGic_CfgInitialize")->ignoreOtherParameters()->andReturnIntValue(0x1);
	// 	mock_c()->expectOneCall("Xil_ExceptionRegisterHandler")
	// 		->ignoreOtherParameters()
	// 		->andReturnIntValue(XAIE_OK);
	// 	mock_c()->expectOneCall("XScuGic_Connect")
	// 		->ignoreOtherParameters()
	// 		->andReturnIntValue(XAIE_OK);
	// 	mock_c()->expectOneCall("XScuGic_Enable")
	// 		->ignoreOtherParameters()
	// 		->andReturnIntValue(XAIE_OK);
	// #endif

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr_init_isr();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr_init_isr function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

TEST_GROUP(AbrErrorIsrTests) {
	void setup() { mock_c()->clear(); }

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_AbrErrorIsr_NominalInitialization
*		Function Name: abr_error_isr
*		Input: nullptr
*		Expectation: None
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AbrErrorIsrTests, TC_001_AbrErrorIsr_NominalInitialization) {
	mock_c()->ignoreOtherCalls();
	mock_c()->expectOneCall("XAie_SetupPartitionConfig")->ignoreOtherParameters()->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_CfgInitialize")->ignoreOtherParameters()->andReturnIntValue(0);
	AIEGraph *gr = new AIEGraph("gradf2");

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr_error_isr(nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr_error_isr function: " << total_clock_enable_us << " microseconds" << std::endl;

	delete gr;
	CHECK(true);
}

TEST_GROUP(DumpUcAppTests) {
	void setup() { mock_c()->clear(); }

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_SuccessfulDump
*		Function Name: dump_uc_app
*		Input: void
*		Expectation: None
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(DumpUcAppTests, TC_001_SuccessfulDump) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	dump_uc_app();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by dump_uc_app function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

#ifdef __AIESIM__
//*********************************************************************************/
/*
*		Unique test case ID: TC_002_OpenFailure_ReturnsEarly
*		Function Name: dump_uc_app
*		Input: void
*		Expectation: None
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(DumpUcAppTests, TC_002_OpenFailure_ReturnsEarly) {
	constexpr char kOut[] = "output.bin";
	static_cast<void>(::unlink(kOut));
	static_cast<void>(::rmdir(kOut));
	CHECK(::mkdir(kOut, 0755) == 0);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	dump_uc_app();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by dump_uc_app function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(::rmdir(kOut) == 0);
}
#endif /* __AIESIM__ */

TEST_GROUP(aie_driver_tests) {
	void setup() { mock_c()->clear(); }

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: TC_001_aie_driver_test
*		Function Name: aie_driver_test
*		Input: DevInst
*		Expectation: true
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(aie_driver_tests, TC_001_aie_driver_test) {

	XAie_DevInst DevInst;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aie_driver_test(&DevInst);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by aie_driver_test function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}
//#undef __AIESIM__
/*@section Test Execution details
| Test Case ID                                                      | Test Result | CR  | Impact | Test Details |
|-------------------------------------------------------------------|-------------|-----|--------|--------------|
| TC_001_ConstructorSetupPartitionConfigFailure                     | PASS        | No  | NA     | NA           |
| TC_002_ConstructorCfgInitializeFailure                            | PASS        | No  | NA     | NA           |
| TC_003_ConstructorSuccess                                         | PASS        | No  | NA     | NA           |
| TC_004_ConstructorFailureforInvalidGraphName                      | PASS        | No  | NA     | NA           |
| TC_004a_ConstructorFalUtilInitializeFailure                       | PASS        | No  | NA     | NA           |
| TC_005_Init_failure                                               | PASS        | No  | NA     | NA           |
| TC_006_Init_xaiemem_null_backend_not_ready                        | PASS        | No  | NA     | NA           |
| TC_007_Init_graphapi_null_backend_not_ready                       | PASS        | No  | NA     | NA           |
| TC_008_InitConfiguresSystem                                       | PASS        | No  | NA     | NA           |
| TC_008a_InitExternalBuffersResourceUnavailable                    | PASS        | No  | NA     | NA           |
| TC_008b_InitGmiosUserError                                        | PASS        | No  | NA     | NA           |
| TC_008e_InitGmiosInternalError                                    | PASS        | No  | NA     | NA           |
| TC_008f_InitGmiosNullArgumentRemap                                | PASS        | No  | NA     | NA           |
| TC_008c_InitRtpsResourceUnavailableContinues                      | PASS        | No  | NA     | NA           |
| TC_008d_InitSharedBuffersResourceUnavailable                      | PASS        | No  | NA     | NA           |
| TC_009_SelftestExitSuccess                                        | PASS        | No  | NA     | NA           |
| TC_010_SelftestExitFailure                                        | PASS        | No  | NA     | NA           |
| TC_011_SelftestExit_PmRequestTilesFails                           | PASS        | No  | NA     | NA           |
| TC_012_SelftestExit_ErrorHandlingInitFails                        | PASS        | No  | NA     | NA           |
| TC_013_SelftestExit_FirstClockEnableFails                         | PASS        | No  | NA     | NA           |
| TC_014_SelftestExit_ClockDisableFails                             | PASS        | No  | NA     | NA           |
| TC_015_SelftestExit_SecondClockEnableFails                        | PASS        | No  | NA     | NA           |
| TC_016_SelftestExit_AIESIM_DevGenAIE2PS_UpdateNpiAddr             | PASS        | No  | NA     | NA           |
| TC_017_SelftestExit_AIESIM_DevGenAIE_UpdateNpiAddr                | PASS        | No  | NA     | NA           |
| TC_017a_SelftestExit_AIESIM_UnknownDevGen_DefaultBranch           | PASS        | No  | NA     | NA           |
| TC_018_ClockEnableSuccess                                         | PASS        | No  | NA     | NA           |
| TC_019_ClockEnableFailure                                         | PASS        | No  | NA     | NA           |
| TC_020_ClockEnableFailureInvalidState                             | PASS        | No  | NA     | NA           |
| TC_021_ClockDisableSuccess                                        | PASS        | No  | NA     | NA           |
| TC_022_ClockDisableFailure                                        | PASS        | No  | NA     | NA           |
| TC_023_ClockDisableInvalidState                                   | PASS        | No  | NA     | NA           |
| TC_024_MallocInitSuccess                                          | PASS        | No  | NA     | NA           |
| TC_025_Malloc_xaiemem_null                                        | PASS        | No  | NA     | NA           |
| TC_026_MallocFailure                                              | PASS        | No  | NA     | NA           |
| TC_027_Malloc_CheckInitialized                                    | PASS        | No  | NA     | NA           |
| TC_028_FreeSuccess                                                | PASS        | No  | NA     | NA           |
| TC_029_FreeFailure                                                | PASS        | No  | NA     | NA           |
| TC_030_FreeNullAddress                                            | PASS        | No  | NA     | NA           |
| TC_031_Free_xaiemem_null                                          | PASS        | No  | NA     | NA           |
| TC_032_CheckInitialized_free                                      | PASS        | No  | NA     | NA           |
| TC_033_Wait                                                       | PASS        | No  | NA     | NA           |
| TC_034_Wait_GraphApi_null_backend_not_ready                       | PASS        | No  | NA     | NA           |
| TC_035_CheckInitialized_wait                                      | PASS        | No  | NA     | NA           |
| TC_036_EndSuccess                                                 | PASS        | No  | NA     | NA           |
| TC_037_End_GraphApi_null_backend_not_ready                        | PASS        | No  | NA     | NA           |
| TC_038_CheckInitialized_end                                       | PASS        | No  | NA     | NA           |
| TC_039_RunDefaultIterations                                       | PASS        | No  | NA     | NA           |
| TC_040_Run_ReturnsDriverError                                     | PASS        | No  | NA     | NA           |
| TC_041_Run_graphapi_null_backend_not_ready                        | PASS        | No  | NA     | NA           |
| TC_042_CheckInitialized_run                                       | PASS        | No  | NA     | NA           |
| TC_043_Run_invalid_length_zero_iterations                         | PASS        | No  | NA     | NA           |
| TC_043a_Run_invalid_length_below_minus_one                        | PASS        | No  | NA     | NA           |
| TC_044_BacktrackErrors                                            | PASS        | No  | NA     | NA           |
| TC_045_BacktrackErrors_InvalidPartitionCols                       | PASS        | No  | NA     | NA           |
| TC_046_BacktrackErrors_StartColGeNumCols                          | PASS        | No  | NA     | NA           |
| TC_047_BacktrackErrors_DriverFailure                              | PASS        | No  | NA     | NA           |
| TC_048_BacktrackErrors_InsufficientBufferEmptyCache               | PASS        | No  | NA     | NA           |
| TC_049_BacktrackErrors_ErrorHandlingCallbackInvoked               | PASS        | No  | NA     | NA           |
| TC_050_BacktrackErrors_MaxIterations                              | PASS        | No  | NA     | NA           |
| TC_051_BacktrackErrors_ErrorCountClamping                         | PASS        | No  | NA     | NA           |
| TC_052_CheckInitialized_BacktrackErrors                           | PASS        | No  | NA     | NA           |
| TC_053_DumpCoreLog                                                | PASS        | No  | NA     | NA           |
| TC_054_Dumpcorelog_InvalidIndices                                 | PASS        | No  | NA     | NA           |
| TC_055_CheckInitialized_dumpcorelog                               | PASS        | No  | NA     | NA           |
| TC_056_GetAieDevInst                                              | PASS        | No  | NA     | NA           |
| TC_057_CheckInitialized_getaiedevinst                             | PASS        | No  | NA     | NA           |
| TC_058_GM2AIE_NB                                                  | PASS        | No  | NA     | NA           |
| TC_059_GM2AIE_NB_DriverError                                      | PASS        | No  | NA     | NA           |
| TC_060_CheckInitialized_gm2aie_nb                                 | PASS        | No  | NA     | NA           |
| TC_061_GM2AIE_NB_InvalidName                                      | PASS        | No  | NA     | NA           |
| TC_062_GM2AIE_NB_NullArgument                                     | PASS        | No  | NA     | NA           |
| TC_063_GM2AIE_NB_InvalidLength                                    | PASS        | No  | NA     | NA           |
| TC_064_GM2AIE_NB_Failure                                          | PASS        | No  | NA     | NA           |
| TC_065_AIE2GM_NB                                                  | PASS        | No  | NA     | NA           |
| TC_066_Check_aie2gm_nb_init_failure                               | PASS        | No  | NA     | NA           |
| TC_067_AIE2GM_NB_InvalidName                                      | PASS        | No  | NA     | NA           |
| TC_067a_AIE2GM_NB_EmptyName                                       | PASS        | No  | NA     | NA           |
| TC_068_AIE2GM_NB_ZeroLength                                       | PASS        | No  | NA     | NA           |
| TC_069_AIE2GM_NB_NullArgument                                     | PASS        | No  | NA     | NA           |
| TC_070_AIE2GM_NB_Failure                                          | PASS        | No  | NA     | NA           |
| TC_071_GM2AIE                                                     | PASS        | No  | NA     | NA           |
| TC_072_GM2AIE_DriverError                                         | PASS        | No  | NA     | NA           |
| TC_073_CheckInitialized_gm2aie                                    | PASS        | No  | NA     | NA           |
| TC_074_GM2AIE_InvalidName                                         | PASS        | No  | NA     | NA           |
| TC_075_GM2AIE_NullArgument                                        | PASS        | No  | NA     | NA           |
| TC_076_GM2AIE_InvalidLength                                       | PASS        | No  | NA     | NA           |
| TC_077_GM2AIE_Failure                                             | PASS        | No  | NA     | NA           |
| TC_078_AIE2GM                                                     | PASS        | No  | NA     | NA           |
| TC_079_CheckInitialized_aie2gm                                    | PASS        | No  | NA     | NA           |
| TC_080_AIE2GM_InvalidName                                         | PASS        | No  | NA     | NA           |
| TC_081_AIE2GM_NullArgument                                        | PASS        | No  | NA     | NA           |
| TC_082_AIE2GM_ZeroLength                                          | PASS        | No  | NA     | NA           |
| TC_083_AIE2GM_Failure                                             | PASS        | No  | NA     | NA           |
| TC_084_GMIO_WAIT                                                  | PASS        | No  | NA     | NA           |
| TC_085_CheckInitialized_gmio_wait                                 | PASS        | No  | NA     | NA           |
| TC_086_GMIO_WAIT_InvalidName                                      | PASS        | No  | NA     | NA           |
| TC_087_GMIO_WAIT_Failure                                          | PASS        | No  | NA     | NA           |
| TC_088_GMIO_WAIT_NonBlockingWaitFailure                           | PASS        | No  | NA     | NA           |
| TC_089_ErrorHandlingCallbackInit                                  | PASS        | No  | NA     | NA           |
| TC_090_CheckInitialized_error_handling_cb_init                    | PASS        | No  | NA     | NA           |
| TC_091_ErrorHandlingCallbackInit_NullCallback                     | PASS        | No  | NA     | NA           |
| TC_092_CheckInitialized_start_profiling_single_port               | PASS        | No  | NA     | NA           |
| TC_093_StartProfilingSinglePort_EmptyNameAfterInit                | PASS        | No  | NA     | NA           |
| TC_094_StartProfilingSinglePort_InvalidOptions                    | PASS        | No  | NA     | NA           |
| TC_095_StartProfilingSinglePort_EventApi_NullBackendNotReady      | PASS        | No  | NA     | NA           |
| TC_096_StartProfiling_Success_RunIdle                             | PASS        | No  | NA     | NA           |
| TC_097_StartProfiling_Success_StartBytes                          | PASS        | No  | NA     | NA           |
| TC_098_StartProfiling_Success_EventCount                          | PASS        | No  | NA     | NA           |
| TC_099_CheckInitialized_start_profiling_two_ports                 | PASS        | No  | NA     | NA           |
| TC_100_StartProfilingTwoPorts_EmptyNameAfterInit                  | PASS        | No  | NA     | NA           |
| TC_101_StartProfilingTwoPorts_InvalidOptions                      | PASS        | No  | NA     | NA           |
| TC_102_StartProfilingTwoPort_EventApi_NullBackendNotReady         | PASS        | No  | NA     | NA           |
| TC_103a_StartProfiling_PlioPortName_HitsPlioNonNull               | PASS        | No  | NA     | NA           |
| TC_103_StartProfilingTwoPorts_Success_DiffCycles                  | PASS        | No  | NA     | NA           |
| TC_103b_StartProfiling_AmbiguousPort_GmioAndPlio                  | PASS        | No  | NA     | NA           |
| TC_103c_StartProfiling_StartBytes_Failure                         | PASS        | No  | NA     | NA           |
| TC_103d_StartProfiling_DiffCycles_Failure                         | PASS        | No  | NA     | NA           |
| TC_103e_StartProfiling_EventCount_Failure                         | PASS        | No  | NA     | NA           |
| TC_104_CheckInitialized_read_profiling                            | PASS        | No  | NA     | NA           |
| TC_105_ReadProfiling_EventApi_NullBackendNotReady                 | PASS        | No  | NA     | NA           |
| TC_106_ReadProfiling_InvalidHandle                                | PASS        | No  | NA     | NA           |
| TC_107_ReadProfiling_Success_ValidHandle                          | PASS        | No  | NA     | NA           |
| TC_108_CheckInitialized_stop_profiling                            | PASS        | No  | NA     | NA           |
| TC_109_StopProfiling_InvalidHandle                                | PASS        | No  | NA     | NA           |
| TC_110_StopProfiling_EventApi_NullBackendNotReady                 | PASS        | No  | NA     | NA           |
| TC_111_StopProfiling_Success_ValidHandle                          | PASS        | No  | NA     | NA           |
| TC_112_SetAddress_Success_ValidHandle                             | PASS        | No  | NA     | NA           |
| TC_113_CheckInitialized_setAddress                                | PASS        | No  | NA     | NA           |
| TC_114_SetAddress_InvalidName                                     | PASS        | No  | NA     | NA           |
| TC_115_SetAddress_NullArgument                                    | PASS        | No  | NA     | NA           |
| TC_116_SetAddress_Failure_InvalidExternalBufferName               | PASS        | No  | NA     | NA           |
| TC_117_WaitExternalBuffer                                         | PASS        | No  | NA     | NA           |
| TC_118_CheckInitialized_wait_external_buffer                      | PASS        | No  | NA     | NA           |
| TC_119_WaitExternalBuffer_InvalidBufferName                       | PASS        | No  | NA     | NA           |
| TC_120_WaitExternalBuffer_InvalidPortName                         | PASS        | No  | NA     | NA           |
| TC_121_WaitExternalBuffer_Failure_InvalidExternalBufferName       | PASS        | No  | NA     | NA           |
| TC_122_GM2AIE_NB_ExternalBuffer                                   | PASS        | No  | NA     | NA           |
| TC_123_CheckInitialized_gm2aie_nb_external                        | PASS        | No  | NA     | NA           |
| TC_124_GM2AIE_NB_ExternalBuffer_InvalidBufferName                 | PASS        | No  | NA     | NA           |
| TC_125_GM2AIE_NB_ExternalBuffer_InvalidPortName                   | PASS        | No  | NA     | NA           |
| TC_126_GM2AIE_NB_ExternalBuffer_Failure_InvalidExternalBufferName | PASS        | No  | NA     | NA           |
| TC_127_CheckInitialized_aie2gm_nb_external                        | PASS        | No  | NA     | NA           |
| TC_128_AIE2GM_NB_ExternalBuffer_InvalidBufferName                 | PASS        | No  | NA     | NA           |
| TC_129_AIE2GM_NB_ExternalBuffer_InvalidPortName                   | PASS        | No  | NA     | NA           |
| TC_130_AIE2GM_NB_ExternalBuffer_Failure_InvalidExternalBufferName | PASS        | No  | NA     | NA           |
| TC_131_AIE2GM_NB_ExternalBuffer                                   | PASS        | No  | NA     | NA           |
| TC_131_UpdateRTP                                                  | PASS        | No  | NA     | NA           |
| TC_131a_UpdateRTP_InternalError                                   | PASS        | No  | NA     | NA           |
| TC_131b_UpdateRTP_AieDriverError                                  | PASS        | No  | NA     | NA           |
| TC_131c_UpdateRTP_ResourceUnavailable                             | PASS        | No  | NA     | NA           |
| TC_131d_UpdateRTP_UnknownErrorDefaultsToDriverError               | PASS        | No  | NA     | NA           |
| TC_132_UpdateRTP_failure                                          | PASS        | No  | NA     | NA           |
| TC_133_CheckInitialized_update                                    | PASS        | No  | NA     | NA           |
| TC_134_Update_InvalidName                                         | PASS        | No  | NA     | NA           |
| TC_135_Update_NullArgument                                        | PASS        | No  | NA     | NA           |
| TC_136_Update_InvalidLength                                       | PASS        | No  | NA     | NA           |
| TC_137_Update_GraphApi_null_backend_not_ready                     | PASS        | No  | NA     | NA           |
| TC_138_UpdateSharedBuffer                                         | PASS        | No  | NA     | NA           |
| TC_139_CheckInitialized_setupPingPongBuffers                      | PASS        | No  | NA     | NA           |
| TC_140_SetupPingPongBuffers_InvalidName                           | PASS        | No  | NA     | NA           |
| TC_141_SetupPingPongBuffers_NullArgument_ptr1                     | PASS        | No  | NA     | NA           |
| TC_142_Check_setupPingPongBuffers_null_argument_ptr2              | PASS        | No  | NA     | NA           |
| TC_143_SetupPingPongBuffers_valid_external_buffer_name            | PASS        | No  | NA     | NA           |
| TC_143_SetupPingPongBuffers_Failure_InvalidExternalBufferName     | PASS        | No  | NA     | NA           |
| TC_144_CheckInitialized_read                                      | PASS        | No  | NA     | NA           |
| TC_145_Read_InvalidName                                           | PASS        | No  | NA     | NA           |
| TC_146_Read_NullArgument                                          | PASS        | No  | NA     | NA           |
| TC_147_Read_InvalidLength                                         | PASS        | No  | NA     | NA           |
| TC_148_Read_GraphApi_null_backend_not_ready                       | PASS        | No  | NA     | NA           |
| TC_149_Read_invalid_rtp_name                                      | PASS        | No  | NA     | NA           |
| TC_150_Read_InputRtp_ok                                           | PASS        | No  | NA     | NA           |
| TC_151_GetGraphImpl_AfterInit                                     | PASS        | No  | NA     | NA           |
| TC_152_StartProfiling_UnSuccess_EventCount                        | PASS        | No  | NA     | NA           |
| TC_001_NominalInitialization                                      | PASS        | No  | NA     | NA           |
| TC_002_NominalInitialization                                      | PASS        | No  | NA     | NA           |
| TC_001_AbrErrorIsr_NominalInitialization                          | PASS        | No  | NA     | NA           |
| TC_001_SuccessfulDump                                             | PASS        | No  | NA     | NA           |
| TC_002_OpenFailure_ReturnsEarly                                   | PASS        | No  | NA     | NA           |
| TC_001_aie_driver_test                                            | PASS        | No  | NA     | NA           |

/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)                                                       | %      | Result     | CR/Status    | Impact          |
|-------------------------------------------------------------------------------------|--------|------------|--------------|-----------------|
| err_from_abr                                                                        | <57.1> | <Pass>     | <NA>         | No Impact       |
| is_valid_cstr                                                                       | <100>  | <Pass>     | <NA>         | No Impact       |
| is_valid_profiling_option                                                           | <100>  | <Pass>     | <NA>         | No Impact       |
| abr_init_isr                                                                        | <100>  | <Pass>     | <NA>         | No Impact       |
| abr_error_isr                                                                       | <100>  | <Pass>     | <NA>         | No Impact       |
| dump_uc_app                                                                         | <100>  | <Pass>     | <NA>         | No Impact       |
| aie_driver_test                                                                     | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::AIEGraph                                                            | <97.0> | <Pass>     | <NA>         | No Impact       |
| AIEGraph::~AIEGraph [D2 base-object dtor]                                     | <0>    | <Fail>     | <NA>         | No Impact       |
| AIEGraph::~AIEGraph [D0 deleting dtor]                                        | <0>    | <Fail>     | <NA>         | No Impact       |
| AIEGraph::selftestexit                                                           | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::clock_enable                                                           | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::clock_disable                                                          | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::error_handling_cb_init                                                 | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::init                                                                   | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::run                                                                    | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::aie2gm_nb(const char* gmio_name, char* data, int32_t len)              | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::aie2gm(const char* gmio_name, char* data, int32_t len)                 | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::gm2aie_nb(const char* gmio_name, char* data, int32_t len)              | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::gm2aie(const char* gmio_name, char* data, int32_t len)                 | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::setAddress                                                             | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::wait(const char* externalbuffer_name, std::string port_name)           | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::gm2aie_nb(const char* externalbuffer_name, std::string port_name)      | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::aie2gm_nb(const char* externalbuffer_name, std::string port_name)      | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::setupPingPongBuffers                                                   | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::update                                                                 | <86.7> | <Pass>     | <NA>         | No Impact       |
| AIEGraph::read                                                                   | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::dumpcorelog                                                            | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::getaiedevinst                                                          | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::wait()                                                                 | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::end                                                                    | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::gmio_wait                                                              | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::malloc                                                                 | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::free                                                                   | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::BacktrackErrors                                                        | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::start_profiling(const string& port_name, int, uint32_t)                | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::start_profiling(const string& port_name1, const string& port_name2, int, uint32_t) | <100> | <Pass> | <NA>     | No Impact       |
| AIEGraph::read_profiling                                                         | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::stop_profiling                                                         | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::reset_xaiemem                                                          | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::reset_GraphApi                                                         | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::reset_EventApi                                                         | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::selftest_errorhandling                                                 | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraphImpl::~AIEGraphImpl                   					                 | <0>    | <Fail>     | <NA>         | No Impact       |
| AIEGraphImpl::~AIEGraphImpl 					                                      | <0>    | <Fail>     | <NA>         | No Impact       |
| AIEGraph::AIEGraph                                                                  | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::~AIEGraph                                                                 | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::check_initialized_impl                                                    | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::getGraphImpl                                                              | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::selftestexit                                                              | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::clock_enable                                                              | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::clock_disable                                                             | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::error_handling_cb_init                                                    | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::init                                                                      | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::reset_xaiemem                                                             | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::reset_GraphApi                                                            | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::reset_EventApi                                                            | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::run                                                                       | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::aie2gm_nb(const char* name, char* data, int32_t len)                      | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::aie2gm(const char* name, char* data, int32_t len)                         | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::gm2aie_nb(const char* name, char* data, int32_t len)                      | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::gm2aie(const char* name, char* data, int32_t len)                         | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::wait()                                                                    | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::wait(const char* externalbuffer_name, std::string port_name)              | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::gm2aie_nb(const char* externalbuffer_name, std::string port_name)         | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::aie2gm_nb(const char* externalbuffer_name, std::string port_name)         | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::end                                                                       | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::gmio_wait                                                                 | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::update                                                                    | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::read                                                                      | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::dumpcorelog                                                               | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::getaiedevinst                                                             | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::BacktrackErrors                                                           | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::setAddress                                                                | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::setupPingPongBuffers                                                      | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::start_profiling(const string& port_name, int, uint32_t)                   | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::start_profiling(const string& port_name1, const string& port_name2, int, uint32_t)    | <100> | <Pass> | <NA>     | No Impact       |
| AIEGraph::read_profiling                                                            | <100>  | <Pass>     | <NA>         | No Impact       |
| AIEGraph::stop_profiling                                                            | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::event::event                                                                   | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::event::get_shim_config                                                         | <91.7> | <Pass>     | <NA>         | No Impact       |
| abr::event::start_profiling(const string&, int, unsigned int)                       | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::event::start_profiling(const string&, const string&, int, unsigned int)        | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::event::start_profiling_run_idle                                                | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::event::start_profiling_diff_cycles                                             | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::event::start_profiling_event_count                                             | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::event::start_profiling_start_bytes                                             | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::event::stop_profiling                                                          | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::event::read_profiling                                                          | <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall) 			                                          | 98.4   | <Pass>     | <NA>         | aeg_aiebaremetal  |
/*@section Uncovered details*/
/*

 *Function:  virtual ~AIEGraphSim()
 *  Uncovered details:
 *
		~AIEGraphSim() { XAie_Finish(&DevInst); }
 *	Justification: This destructor is only invoked at the end of the test process, in AIE sim mode and it wont create any impact on source code behavior.
 *				Due to C++ ABI name mangling, this single virtual destructor is emitted as 2 symbols
 *				(D0 - deleting variant, D2 - base-object variant), so it accounts for 2 uncovered
 *				entries in the lcov function report.

 *Function: AIEGraphSim constructor
 *  Uncovered details:
		 aiebase = RPU_AIE_BASE;
 * Justification: This branch is guarded by the ARMR5 build flag (rpu != 0) which is not
	enabled in unit tests, so the RPU base-address override is unreachable and has no impact.
 *
 *Function: AIEGraph::update
 *  Uncovered details:
		AEG_ERROR("%s: RTP config for '%s' not found", "update", port_name.c_str());
		return aeg_err::invalid_name;
 * Justification: cfg.isRTPorSharedBuffer() and cfg.get_rtp_config() are driven by the
	same config map, so a port classified as RTP always has a valid rt_config in unit tests.
 *
 *Function:  ~AIEGraphImpl()
 *  Uncovered details:
		~AIEGraphImpl() override = default;
 * Justification: This destructor is only invoked at the end of the test process, in AIE sim mode and it wont create any impact on source code behavior.
 *				Due to C++ ABI name mangling, this single virtual destructor is emitted as 2 symbols
 *				(D0 - deleting variant, D2 - base-object variant), so it accounts for 2 uncovered
 *				entries in the lcov function report.
 *
*/
