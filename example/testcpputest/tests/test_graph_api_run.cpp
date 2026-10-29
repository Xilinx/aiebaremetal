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
* @file test_graph_api_run.cpp
*  This file contains CppUTest cases for graph_api (run, wait, end, resume, configure, shared-buffer
*  and RTP updates), GMIO API paths, config_manager::initialize, and XAie helper wrappers (RTP config,
*  DMA BD/wait/enqueue, locks). Tests use mock XAie/CppUTest (no hardware).
*
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	  Who	   Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera      05/03/2026  Initial creation (graph_api unit tests)
* 1.1	 Veera      20/04/2026  Added test cases for graph_api::run, end, update_iter, and wait operations
* 1.2	 Veera      10/05/2026  updated comment section for graph_api::run, end, update_iter, and wait operations
* 1.3	 Veera      27/05/2026  updated timing parameters for test cases
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
#include <unordered_set>
//#define __AIESIM__
#include "aeg_runtime_api.h"
#include "aeg_aie_control_api.h"
#include <iostream>
#include <chrono>
using namespace abr;
namespace abr {
	extern std::unordered_set<int> readOnlySharedBufferInitialized;

}
#include "test_comparator.h"

#include <CppUTest/TestHarness.h>
#include "CppUTest/MemoryLeakDetector.h"


class DummyConfigManager : public config_manager {
	public:
		static void setDevInst(XAie_DevInst* inst) { s_pDevInst = inst; }
		static void setBroadcastEnableCore(bool val) { s_broadcast_enable_core = val; }
		static void setNumReservedRows(size_t val) { s_num_reserved_rows = val; }
};

struct DummyXAie_DevInst {
	struct { int DevGen = 0; } DevProp;
};

struct DummyGraphConfig : public graph_config {
	DummyGraphConfig() {
		name = "dummy";
		coreColumns = {0};
		coreRows = {0};
		iterMemAddrs = {0};
		triggered = {false};
		iterMemColumns = {0};
		iterMemRows = {0};
		broadcast_column = 0;
	}
};

#include "CppUTestExt/MockSupport_c.h"

TEST_GROUP(GraphApiRun)
{
	void setup() {
		mock_c()->clear();
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
	}

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
		mock_c()->removeAllComparatorsAndCopiers();
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: TC_001_Run_Success_BroadcastEnabled
*		Function Name: run
*		Input: none
*		Expectation: res == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_001_Run_Success_BroadcastEnabled)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);

	api.configure();
	unsigned long long timerValue1 = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withIntParameters("Module", XAIE_CORE_MOD)
		->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2, "XAie_StartTransaction")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_SubmitTransaction")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_ClearCoreDisableEventOccurred")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_CoreConfigureEnableEvent")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_EventGenerate")
		->ignoreOtherParameters();
	unsigned long long timerValue2 = 303;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &timerValue2, sizeof(timerValue2))
		->andReturnIntValue(0);
	unsigned long long timerValue3 = 503;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &timerValue3, sizeof(timerValue3))
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_SubmitTransaction")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_CoreConfigureEnableEvent")
		->ignoreOtherParameters();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.run();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by run function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::ok);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_002_Run_Success_BroadcastDisabled
*		Function Name: run
*		Input: none
*		Expectation: res == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_002_Run_Success_BroadcastDisabled)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(false);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;
	u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
	api.configure();

	unsigned long long timerValue1 = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withIntParameters("Module", XAIE_CORE_MOD)
		->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_StartTransaction")
		->withPointerParameters("DevInst", &devInst)
		->withIntParameters("Flags", Flags)
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_SubmitTransaction")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters();

	mock_c()->expectOneCall("XAie_CoreEnable")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.run();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by run function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::ok);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_003_Run_Failure_NullConfig
*		Function Name: run
*		Input: none
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_003_Run_Failure_NullConfig)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(false);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig *config = nullptr;
	graph_api api(config);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.run();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by run function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::aie_driver_error);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_004_Run_Failure_CoreEnableFails
*		Function Name: run
*		Input: none
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_004_Run_Failure_CoreEnableFails)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(false);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;
	u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
	api.configure();

	unsigned long long timerValue1 = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withIntParameters("Module", XAIE_CORE_MOD)
		->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_StartTransaction")
		->withPointerParameters("DevInst", &devInst)
		->withIntParameters("Flags", Flags)
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_SubmitTransaction")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters();

	mock_c()->expectOneCall("XAie_CoreEnable")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.run();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by run function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::aie_driver_error);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_005_Run_Failure_DataMemWrWordFails
*		Function Name: run
*		Input: iterations
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_005_Run_Failure_DataMemWrWordFails)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);


	u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
	api.configure();
	int iterations = 10;
	mock_c()->expectOneCall("XAie_StartTransaction")
		->withPointerParameters("DevInst", &devInst)
		->withIntParameters("Flags", Flags)
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_SubmitTransaction")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters();

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.run(iterations);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by run function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_006_Run_Success_WithIterations
*		Function Name: run
*		Input: iterations
*		Expectation: res == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_006_Run_Success_WithIterations)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;

	u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
	api.configure();
	int iterations = 10;
	mock_c()->expectOneCall("XAie_StartTransaction")
		->withPointerParameters("DevInst", &devInst)
		->withIntParameters("Flags", Flags)
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_SubmitTransaction")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters();
	unsigned long long timerValue1 = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withIntParameters("Module", XAIE_CORE_MOD)
		->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2, "XAie_StartTransaction")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_SubmitTransaction")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_ClearCoreDisableEventOccurred")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_CoreConfigureEnableEvent")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_EventGenerate")
		->ignoreOtherParameters();
	unsigned long long timerValue2 = 303;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &timerValue2, sizeof(timerValue2))
		->andReturnIntValue(0);
	unsigned long long timerValue3 = 503;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &timerValue3, sizeof(timerValue3))
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_SubmitTransaction")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_CoreConfigureEnableEvent")
		->ignoreOtherParameters();

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.run(iterations);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by run function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_007_Run_Failure_EventGenerateFails
*		Function Name: run
*		Input: iterations
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_007_Run_Failure_EventGenerateFails)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig *config = nullptr;
	graph_api api(config);
	int iterations = 10;
	u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
	api.configure();
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.run(iterations);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by run function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::aie_driver_error);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_008_Configure_Failure_NullConfig
*		Function Name: abr::graph_api::configure
*		Input: none
*		Expectation: res == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_008_Configure_Failure_NullConfig)
{
	abr::err_code res;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig *config = nullptr;
	graph_api api(config);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	res = api.configure();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configure function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::internal_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_009_Configure_Failure_AlreadyConfigured
*		Function Name: abr::graph_api::configure
*		Input: none
*		Expectation: res == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_009_Configure_Failure_AlreadyConfigured)
{
	abr::err_code res;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	config.coreColumns = {1, 2, 3};
	config.coreRows = {1, 2};
	config.iterMemAddrs = {1, 2, 3};
	config.iterMemColumns = {1, 2, 3};
	config.iterMemRows = {1, 2, 3};

	graph_api api(&config);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	res = api.configure();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configure function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::internal_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_010_Wait_Failure_NullConfig
*		Function Name:  abr::graph_api::wait
*		Input: none
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_010_Wait_Failure_NullConfig)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	int iterations = 10;
	abr::err_code res;
	u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	res = api.wait();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::aie_driver_error);
	mock_c()->checkExpectations();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_011_Wait_Success
*		Function Name:  abr::graph_api::wait
*		Input: none
*		Expectation: res == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_011_Wait_Success)
{

	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	int iterations = 10;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;
	abr::err_code res;
	u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
	api.configure();
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_CoreDisable")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
		->andReturnIntValue(0);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	res = api.wait();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_012_Wait_Failure_CoreWaitCycleFails
*		Function Name: abr::graph_api::wait
*		Input: none
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_012_Wait_Failure_CoreWaitCycleFails)
{

	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);

	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;
	abr::err_code res;

	api.configure();
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_CoreDisable")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	res = api.wait();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_013_Wait_Failure_CoreWaitStatusFails
*		Function Name: abr::graph_api::wait
*		Input: cycleTimeout
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_013_Wait_Failure_CoreWaitStatusFails)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);

	unsigned long long cycleTimeout = 10;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.wait(cycleTimeout);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_014_Wait_Success_WithTimeout
*		Function Name: abr::graph_api::wait
*		Input: cycleTimeout
*		Expectation: res == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_014_Wait_Success_WithTimeout)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	api.configure();

	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;

	unsigned long long cycleTimeout = 0xFFFFFFFFFFFF;
	unsigned long long timerValue1 = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_WaitCycles")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.wait(cycleTimeout);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_015_Wait_Failure_TimeoutExpired
*		Function Name: abr::graph_api::wait
*		Input: cycleTimeout
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_015_Wait_Failure_TimeoutExpired)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	api.configure();

	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;

	unsigned long long cycleTimeout = 0xFFFFFFFFFFFF;
	unsigned long long timerValue1 = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withIntParameters("Module", XAIE_CORE_MOD)
		->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_WaitCycles")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.wait(cycleTimeout);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::aie_driver_error);

}
//*********************************************************************************/
/*
*		Unique test case ID: TC_016_Wait_Failure_CoreWaitDoneFails
*		Function Name: abr::graph_api::wait
*		Input: cycleTimeout
*		Expectation: res == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_016_Wait_Failure_CoreWaitDoneFails)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	api.configure();

	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;

	unsigned long long cycleTimeout = 0xFFFFFFFFFFFFF;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.wait(cycleTimeout);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::user_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_017_Resume_Failure_NullConfig
*		Function Name: resume
*		Input: none
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_017_Resume_Failure_NullConfig)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);


	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.resume();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by resume function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_018_Resume_Success
*		Function Name: resume
*		Input: none
*		Expectation: res == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_018_Resume_Success)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	api.configure();

	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;


	unsigned long long timerValue1 = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withIntParameters("Module", XAIE_CORE_MOD)
		->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreReadDoneBit")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreEnable")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
		->andReturnIntValue(0);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.resume();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by resume function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_019_Resume_Failure_CoreEnableFails
*		Function Name: resume
*		Input: none
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_019_Resume_Failure_CoreEnableFails)
{
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(true);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	api.configure();
	int iterations = 10;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;

	unsigned long long timerValue1 = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withIntParameters("Module", XAIE_CORE_MOD)
		->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreReadDoneBit")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreEnable")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = api.resume();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by resume function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_020_End_Success
*		Function Name: end
*		Input: none
*		Expectation: res == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_020_End_Success)
{

	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(false);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;
	u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
	api.configure();

	unsigned long long timerValue1 = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withIntParameters("Module", XAIE_CORE_MOD)
		->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_StartTransaction")
		->withPointerParameters("DevInst", &devInst)
		->withIntParameters("Flags", Flags)
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_SubmitTransaction")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters();

	mock_c()->expectNCalls(2,"XAie_CoreEnable")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2,"XAie_CoreWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2,"XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	abr::err_code res = api.run();
	CHECK(res == err_code::ok);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = api.end();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by end function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_021_End_Failure_CoreDisableFails
*		Function Name: end
*		Input: none
*		Expectation: res == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiRun, TC_021_End_Failure_CoreDisableFails)
{

	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	DummyConfigManager::setBroadcastEnableCore(false);
	DummyConfigManager::setNumReservedRows(0);
	DummyGraphConfig config;
	graph_api api(&config);
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;
	u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
	api.configure();

	unsigned long long timerValue1 = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withIntParameters("Module", XAIE_CORE_MOD)
		->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_StartTransaction")
		->withPointerParameters("DevInst", &devInst)
		->withIntParameters("Flags", Flags)
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_SubmitTransaction")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters();

	mock_c()->expectNCalls(2,"XAie_CoreEnable")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2,"XAie_CoreWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(1,"XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	abr::err_code res = api.run();
	CHECK(res == err_code::ok);
	mock_c()->expectNCalls(1,"XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(1);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = api.end();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by end function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::aie_driver_error);
}

TEST_GROUP(GraphApiUpdateSharedBufferTests)
{

	XAie_DevInst deviceInstance;
	abr::shared_buffer_config sharedBufferConfig;
	abr::graph_config graphConfig;
	abr::graph_api* graphApi;
	uint8_t testData[64];
	size_t testDataSize;

	void setup()
	{

		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);

		readOnlySharedBufferInitialized.clear();


		deviceInstance.DevProp.DevGen = XAIE_DEV_GEN_AIE;
		abr::config_manager::s_pDevInst = &deviceInstance;
		abr::config_manager::s_bInitialized = true;
		abr::config_manager::s_num_reserved_rows = 0;



		graphConfig.id = 1;
		graphConfig.name = "TestGraph";


		sharedBufferConfig.id = 1;
		sharedBufferConfig.name = "TestSharedBuffer";
		sharedBufferConfig.graphId = graphConfig.id;
		sharedBufferConfig.numBytes = 64;
		sharedBufferConfig.column = 1;
		sharedBufferConfig.row = 2;
		sharedBufferConfig.numInputs = 0;
		sharedBufferConfig.numOutputs = 2;
		sharedBufferConfig.elemType = abr::rtp_config::et_int32;
		sharedBufferConfig.initialized = false;
		sharedBufferConfig.addr = {0x1000};
		sharedBufferConfig.producerLocks = {1};
		sharedBufferConfig.consumerLocks = {2};
		readOnlySharedBufferInitialized.clear();

		testDataSize = 64;
		for (size_t i = 0; i < testDataSize; i++)
		{
			testData[i] = static_cast<uint8_t>(i & 0xFF);
		}


		graphApi = new abr::graph_api(&graphConfig);
	}


	void teardown()
	{
		delete graphApi;

		readOnlySharedBufferInitialized.clear();


		mock_c()->clear();
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: TC_001_Update_SharedBufferSuccess
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateSharedBufferTests, TC_001_Update_SharedBufferSuccess)
{

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockRelease")
		->withPointerParameters("DevInst", &deviceInstance)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&sharedBufferConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::ok);

	readOnlySharedBufferInitialized.clear();
	sharedBufferConfig.initialized = true;

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t2 = std::chrono::steady_clock::now();
	result = graphApi->update(&sharedBufferConfig, testData, testDataSize);
	const auto clock_enable_t3 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t3 - clock_enable_t2).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::ok);

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t4 = std::chrono::steady_clock::now();
	result = graphApi->update(&sharedBufferConfig, testData, testDataSize);
	const auto clock_enable_t5 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t5 - clock_enable_t4).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_Update_NullBufferConfig
*		Function Name: abr::graph_api::update
*		Input: &sharedBufferConfig, testData, testDataSize
*		Expectation: result != err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateSharedBufferTests, TC_002_Update_NullBufferConfig)
{

	abr::shared_buffer_config *BufferConfig = nullptr;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(BufferConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result != err_code::ok);

}
//*********************************************************************************/
/*
*		Unique test case ID: TC_003_Update_Failure
*		Function Name: abr::graph_api::update
*		Input: &sharedBufferConfig, testData, testDataSize
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateSharedBufferTests, TC_003_Update_Failure)
{


	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockRelease")
		->withPointerParameters("DevInst", &deviceInstance)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(1);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&sharedBufferConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::aie_driver_error);

}
//*********************************************************************************/
/*
*		Unique test case ID: TC_004_Update_SharedBufferWrongGraphId
*		Function Name: abr::graph_api::update
*		Input: &sharedBufferConfig, testData, testDataSize
*		Expectation: result == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateSharedBufferTests, TC_004_Update_SharedBufferWrongGraphId)
{
	sharedBufferConfig.graphId = graphConfig.id + 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&sharedBufferConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_Update_SharedBufferHasInputs
*		Function Name: abr::graph_api::update
*		Input: &sharedBufferConfig, testData, testDataSize
*		Expectation: result == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiUpdateSharedBufferTests, TC_005_Update_SharedBufferHasInputs)
{
	sharedBufferConfig.numInputs = 1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&sharedBufferConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_Update_SharedBufferNoOutputs
*		Function Name: abr::graph_api::update
*		Input: &sharedBufferConfig, testData, testDataSize - 1U
*		Expectation: result == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiUpdateSharedBufferTests, TC_006_Update_SharedBufferNoOutputs)
{
	sharedBufferConfig.numOutputs = 0;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&sharedBufferConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_Update_SharedBufferSizeMismatch
*		Function Name: abr::graph_api::update
*		Input: &sharedBufferConfig, testData, testDataSize - 1U
*		Expectation: result == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiUpdateSharedBufferTests, TC_007_Update_SharedBufferSizeMismatch)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&sharedBufferConfig, testData, testDataSize - 1U);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::user_error);
}
TEST_GROUP(GraphApiUpdateRTPTests)
{
	XAie_DevInst deviceInstance;
	abr::rtp_config rtpConfig;
	abr::graph_config graphConfig;
	abr::graph_api* graphApi;
	uint8_t testData[64];
	size_t testDataSize;

	void setup()
	{
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);

		deviceInstance.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
		abr::config_manager::s_pDevInst = &deviceInstance;
		abr::config_manager::s_bInitialized = true;
		abr::config_manager::s_num_reserved_rows = 0;


		graphConfig.id = 1;
		graphConfig.name = "TestGraph";


		rtpConfig.portId = 1;
		rtpConfig.aliasId = 2;
		rtpConfig.portName = "TestRTPPort";
		rtpConfig.aliasName = "TestRTPAlias";
		rtpConfig.graphId = graphConfig.id;
		rtpConfig.isInput = true;
		rtpConfig.isAsync = false;
		rtpConfig.isConnect = false;
		rtpConfig.numBytes = 64;
		rtpConfig.isPL = false;
		rtpConfig.hasLock = true;
		rtpConfig.selectorColumn = 1;
		rtpConfig.selectorRow = 2;
		rtpConfig.selectorAddr = 0x1000;
		rtpConfig.selectorLockId = 1;
		rtpConfig.pingColumn = 1;
		rtpConfig.pingRow = 3;
		rtpConfig.pingAddr = 0x2000;
		rtpConfig.pingLockId = 2;
		rtpConfig.pongColumn = 1;
		rtpConfig.pongRow = 4;
		rtpConfig.pongAddr = 0x3000;
		rtpConfig.pongLockId = 3;
		rtpConfig.isAsync = 0;
		rtpConfig.hasLock = 1;

		testDataSize = 64;
		for (size_t i = 0; i < testDataSize; i++)
		{
			testData[i] = static_cast<uint8_t>(i & 0xFF);
		}


		graphApi = new abr::graph_api(&graphConfig);



	}
	void teardown()
	{
		delete graphApi;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: TC_001_Update_RTPSuccess
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiUpdateRTPTests, TC_001_Update_RTPSuccess)
{

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_Update_Failure
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateRTPTests, TC_002_Update_Failure)
{
	rtpConfig.isAsync = 0x1;
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(0x1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_Update_BlockWriteFails
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result != err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateRTPTests, TC_003_Update_BlockWriteFails)
{

	abr::rtp_config *rtpConfig1 = nullptr;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(rtpConfig1, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result != err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_Update_S100DevGen
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateRTPTests, TC_004_Update_S100DevGen)
{
	deviceInstance.DevProp.DevGen = 6;

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_Update_S200DevGen
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateRTPTests, TC_005_Update_S200DevGen)
{
	deviceInstance.DevProp.DevGen = 7;

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_Update_ConnectedPLPort
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateRTPTests, TC_006_Update_ConnectedPLPort)
{
	rtpConfig.isConnect = true;
	rtpConfig.isPL = true;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_Update_ConnectedSyncRTP
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateRTPTests, TC_007_Update_ConnectedSyncRTP)
{
	rtpConfig.isConnect = true;
	rtpConfig.isAsync = false;
	rtpConfig.isPL = false;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result == err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_Update_AsyncRTPSecondUpdate_AIEML
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result1 == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateRTPTests, TC_008_Update_AsyncRTPSecondUpdate_AIEML)
{
	rtpConfig.isAsync = true;
	rtpConfig.isConnect = false;
	rtpConfig.hasLock = true;

	u32 selector = 0;

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result1 = graphApi->update(&rtpConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(result1 == err_code::ok);

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->withOutputParameterReturning("Data", &selector, sizeof(selector))
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::err_code result2 = graphApi->update(&rtpConfig, testData, testDataSize);

	CHECK(result2 == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_Update_PongBranchWithLock
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateRTPTests, TC_009_Update_PongBranchWithLock)
{
	rtpConfig.isAsync = false;
	rtpConfig.hasLock = true;
	rtpConfig.isConnect = false;

	u32 selectorValue = 0;

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->withOutputParameterReturning("Data", &selectorValue, sizeof(selectorValue))
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_010_Update_PingBranchWithLock
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateRTPTests, TC_010_Update_PingBranchWithLock)
{
	rtpConfig.isAsync = false;
	rtpConfig.hasLock = true;
	rtpConfig.isConnect = false;

	u32 selectorValue = 1;

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->withOutputParameterReturning("Data", &selectorValue, sizeof(selectorValue))
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_011_Update_HasLockFalse_NoLockOps
*		Function Name: abr::graph_api::update
*		Input: &rtpConfig, testData, testDataSize
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiUpdateRTPTests, TC_011_Update_HasLockFalse_NoLockOps)
{
	rtpConfig.isAsync = false;
	rtpConfig.hasLock = false;
	rtpConfig.isConnect = false;

	u32 selectorValue = 0;

	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->withOutputParameterReturning("Data", &selectorValue, sizeof(selectorValue))
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DataMemBlockWrite")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by update function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
}

TEST_GROUP(GraphApiEndTests)
{
	XAie_DevInst deviceInstance;
	abr::graph_config graphConfig;
	abr::graph_api* graphApi;

	void setup()
	{
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
		deviceInstance.DevProp.DevGen = XAIE_DEV_GEN_AIE;
		abr::config_manager::s_pDevInst = &deviceInstance;
		abr::config_manager::s_bInitialized = true;
		abr::config_manager::s_num_reserved_rows = 0;

		graphConfig.id = 1;
		graphConfig.name = "TestGraph";
		graphConfig.coreColumns = {1, 2};
		graphConfig.coreRows = {3, 4};
		graphConfig.iterMemColumns = {1, 2};
		graphConfig.iterMemRows = {5, 6};
		graphConfig.iterMemAddrs = {0x1000, 0x2000};
		graphConfig.triggered = {false, false};
		graphConfig.broadcast_column = 0;

		graphApi = new abr::graph_api(&graphConfig);

		graphApi->configure();
	}

	void teardown()
	{
		delete graphApi;
		mock_c()->removeAllComparatorsAndCopiers();
		abr::config_manager::s_pDevInst = nullptr;
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_End_UnconfiguredGraph
*		Function Name: end
*		Input: none
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiEndTests, TC_001_End_UnconfiguredGraph)
{
	delete graphApi;
	graphApi = new abr::graph_api(&graphConfig);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->end();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by end function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_End_WithTimeoutUnconfiguredGraph
*		Function Name: end
*		Input: timeout = 1000
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GraphApiEndTests, TC_002_End_WithTimeoutUnconfiguredGraph)
{
	delete graphApi;
	graphApi = new abr::graph_api(&graphConfig);
	unsigned long long timeout = 1000;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->end(timeout);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by end function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_End_ConfiguredGraph_Success
*		Function Name: end
*		Input: none
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiEndTests, TC_003_End_ConfiguredGraph_Success)
{
	mock_c()->expectOneCall("wait")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->end();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by end function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_End_ConfiguredGraph_CoreDisableFails
*		Function Name: end
*		Input: none
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiEndTests, TC_004_End_ConfiguredGraph_CoreDisableFails)
{
	mock_c()->expectOneCall("wait")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->end();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by end function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_End_ConfiguredGraph_EventGenerateFails
*		Function Name: end
*		Input: timeout = 0xFFFFFFFFFFFFF
*		Expectation: result != err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiEndTests, TC_005_End_ConfiguredGraph_EventGenerateFails)
{
	unsigned long long timeout = 0xFFFFFFFFFFFFF;
	mock_c()->expectOneCall("wait")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->end(timeout);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by end function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result != err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_End_ConfiguredGraph_WithTimeout
*		Function Name: end
*		Input: timeout = 1000
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiEndTests, TC_006_End_ConfiguredGraph_WithTimeout)
{
	unsigned long long timeout = 1000;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_WaitCycles")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->end(timeout);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by end function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_End_ConfiguredGraph_TimeoutWaitFails
*		Function Name: end
*		Input: timeout = 1000
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiEndTests, TC_007_End_ConfiguredGraph_TimeoutWaitFails)
{
	unsigned long long timeout = 1000;
	mock_c()->expectOneCall("XAie_WaitCycles")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->end(timeout);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by end function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::aie_driver_error);
}
TEST_GROUP(GraphApiReadTests)
{
	XAie_DevInst deviceInstance;
	abr::graph_config graphConfig;
	abr::rtp_config rtpConfig;
	abr::graph_api* graphApi;
	char buffer[128];

	void setup() override
	{

		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
		deviceInstance.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
		abr::config_manager::s_pDevInst = &deviceInstance;
		abr::config_manager::s_bInitialized = true;
		abr::config_manager::s_num_reserved_rows = 0;

		graphConfig.id = 1;
		graphConfig.name = "TestGraph";

		rtpConfig.portId = 1;
		rtpConfig.aliasId = 1;
		rtpConfig.portName = "TestOutputPort";
		rtpConfig.graphId = 1;
		rtpConfig.isInput = false;
		rtpConfig.isAsync = false;
		rtpConfig.isConnect = false;
		rtpConfig.numBytes = 16;
		rtpConfig.isPL = false;
		rtpConfig.hasLock = true;
		rtpConfig.selectorColumn = 1;
		rtpConfig.selectorRow = 2;
		rtpConfig.selectorAddr = 0x1000;
		rtpConfig.selectorLockId = 1;
		rtpConfig.pingColumn = 1;
		rtpConfig.pingRow = 3;
		rtpConfig.pingAddr = 0x2000;
		rtpConfig.pingLockId = 2;
		rtpConfig.pongColumn = 1;
		rtpConfig.pongRow = 4;
		rtpConfig.pongAddr = 0x3000;
		rtpConfig.pongLockId = 3;

		graphApi = new abr::graph_api(&graphConfig);

		graphApi->configure();

		memset(buffer, 0, sizeof(buffer));
	}

	void teardown() override
	{
		delete graphApi;
		abr::config_manager::s_pDevInst = nullptr;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_Read_UnconfiguredGraph
*		Function Name: abr::graph_api::read
*		Input: rtpConfig, buffer, numBytes
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiReadTests, TC_001_Read_UnconfiguredGraph)
{
	delete graphApi;
	graphApi = new abr::graph_api(&graphConfig);
	size_t numBytes = rtpConfig.numBytes;
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	u32 selector = 0x1;
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->ignoreOtherParameters()
		->withOutputParameterReturning("Data", &selector, sizeof(selector))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockRead")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->read(&rtpConfig, buffer, numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by read function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_Read_UnconfiguredGraph_BlockReadFails
*		Function Name: abr::graph_api::read
*		Input: rtpConfig, buffer, numBytes
*		Expectation: result == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiReadTests, TC_002_Read_UnconfiguredGraph_BlockReadFails)
{
	delete graphApi;
	rtpConfig.isAsync = true;
	graphApi = new abr::graph_api(&graphConfig);
	size_t numBytes = rtpConfig.numBytes;
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DataMemBlockRead")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->read(&rtpConfig, buffer, numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by read function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_003_Read_InvalidRTPConfig
*		Function Name: abr::graph_api::read
*		Input: rtpConfig, buffer, numBytes
*		Expectation: result == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiReadTests, TC_003_Read_InvalidRTPConfig)
{
	rtpConfig.graphId = 2;
	size_t numBytes = rtpConfig.numBytes;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->read(&rtpConfig, buffer, numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by read function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_Read_FromPingBuffer
*		Function Name: abr::graph_api::read
*		Input: rtpConfig, buffer, numBytes
*		Expectation: result == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GraphApiReadTests, TC_004_Read_FromPingBuffer)
{
	delete graphApi;
	graphApi = new abr::graph_api(&graphConfig);
	size_t numBytes = rtpConfig.numBytes;
	u32 selector = 0;

	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->ignoreOtherParameters()
		->withOutputParameterReturning("Data", &selector, sizeof(selector))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockAcquire")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockRead")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = graphApi->read(&rtpConfig, buffer, numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by read function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == err_code::ok);
}
TEST_GROUP(config_managertests)
{
	void setup() {

	}
	void teardown() {

	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_Initialize_AlreadyInitialized
*		Function Name: config_manager::initialize
*		Input: devInst, num_reserved_row=10, broadcast_enable_core=true
*		Expectation: res == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(config_managertests, TC_001_Initialize_AlreadyInitialized)
{
	abr::err_code res;
	XAie_DevInst devInst;

	size_t num_reserved_row = 10;
	config_manager::s_bInitialized = true;
	bool broadcast_enable_core = true;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	res = config_manager::initialize(&devInst,num_reserved_row,broadcast_enable_core);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by initialize function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_Initialize_Success
*		Function Name: config_manager::initialize
*		Input: devInst, num_reserved_row=10, broadcast_enable_core=true
*		Expectation: res == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(config_managertests, TC_002_Initialize_Success)
{
	abr::err_code res;
	XAie_DevInst devInst;
	config_manager::s_bInitialized = false;
	size_t num_reserved_row = 10;
	bool broadcast_enable_core = true;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	res = config_manager::initialize(&devInst,num_reserved_row,broadcast_enable_core);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by initialize function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_Initialize_NullDevInst
*		Function Name: config_manager::initialize
*		Input: devInst = nullptr, num_reserved_row=10, broadcast_enable_core=true
*		Expectation: res == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(config_managertests, TC_003_Initialize_NullDevInst)
{
	abr::err_code res;
	XAie_DevInst *devInst = nullptr;

	config_manager::s_bInitialized = false;
	size_t num_reserved_row = 10;
	bool broadcast_enable_core = true;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	res = config_manager::initialize(devInst,num_reserved_row,broadcast_enable_core);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by initialize function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(res == err_code::internal_error);
}

TEST_GROUP(Xaie_checkRTPConfigForRead)
{

	void setup()
	{
	}
	void teardown()
	{
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkRTPConfigForRead_TC1
*		Function Name: checkRTPConfigForRead
*		Input: pRTPConfig = nullptr, pGraphConfig, numBytes
*		Expectation: err == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC1)
{

	abr::err_code err;
	abr::rtp_config *pRTPConfig = nullptr;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForRead(pRTPConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForRead function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::internal_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkRTPConfigForRead_TC2
*		Function Name: checkRTPConfigForRead
*		Input: pRTPConfig, pGraphConfig, numBytes
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC2)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x2;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForRead(&pRTPConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForRead function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkRTPConfigForRead_TC3
*		Function Name: checkRTPConfigForRead
*		Input: pRTPConfig, pGraphConfig, numBytes
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC3)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pRTPConfig.isInput = 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForRead(&pRTPConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForRead function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkRTPConfigForRead_TC4
*		Function Name: checkRTPConfigForRead
*		Input: pRTPConfig, pGraphConfig, numBytes
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC4)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pRTPConfig.numBytes = sizeof(abr::graph_config) + 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForRead(&pRTPConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForRead function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkRTPConfigForRead_TC5
*		Function Name: checkRTPConfigForRead
*		Input: pRTPConfig, pGraphConfig, numBytes
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC5)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pRTPConfig.isConnect = 1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForRead(&pRTPConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForRead function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkRTPConfigForRead_TC6
*		Function Name: checkRTPConfigForRead
*		Input: pRTPConfig, pGraphConfig, numBytes
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC6)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pRTPConfig.isConnect = 0;
	pRTPConfig.numBytes = sizeof(abr::graph_config) ;
	pRTPConfig.isInput = 0;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForRead(&pRTPConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForRead function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::ok);
}
TEST_GROUP(Xaie_RTPConfigforUpdate)
{

	void setup()
	{
	}
	void teardown()
	{}
};

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_RTPConfigForUpdate_TC1
*		Function Name: checkRTPConfigForUpdate
*		Input: pRTPConfig, pGraphConfig, numBytes, isRunning
*		Expectation: err == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC1)
{

	abr::err_code err;
	abr::rtp_config *pRTPConfig = nullptr;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);
	bool isRunning = 0x0;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForUpdate(pRTPConfig,&pGraphConfig,numBytes,isRunning);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::internal_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_RTPConfigForUpdate_TC2
*		Function Name: checkRTPConfigForUpdate
*		Input: pRTPConfig, pGraphConfig, numBytes, isRunning
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC2)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x2;
	size_t numBytes = sizeof(abr::graph_config);
	bool isRunning = 0x0;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_RTPConfigForUpdate_TC3
*		Function Name: checkRTPConfigForUpdate
*		Input: pRTPConfig, pGraphConfig, numBytes, isRunning
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC3)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pRTPConfig.isInput = 0;
	size_t numBytes = sizeof(abr::graph_config);
	bool isRunning = 0x0;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_RTPConfigForUpdate_TC4
*		Function Name: checkRTPConfigForUpdate
*		Input: pRTPConfig, pGraphConfig, numBytes, isRunning
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC4)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pRTPConfig.isInput = 1;
	size_t numBytes = sizeof(abr::graph_config);
	pRTPConfig.numBytes = sizeof(abr::rtp_config);
	bool isRunning = 0x0;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_RTPConfigForUpdate_TC5
*		Function Name: checkRTPConfigForUpdate
*		Input: pRTPConfig, pGraphConfig, numBytes, isRunning
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC5)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pRTPConfig.isInput = 1;
	size_t numBytes = sizeof(abr::rtp_config);
	pRTPConfig.numBytes = sizeof(abr::rtp_config);
	pRTPConfig.isConnect = 1;
	pRTPConfig.isPL = 0x1;
	bool isRunning = 0x0;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_RTPConfigForUpdate_TC6
*		Function Name: checkRTPConfigForUpdate
*		Input: &pRTPConfig,&pGraphConfig,numBytes,isRunning
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC6)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pRTPConfig.isInput = 1;
	size_t numBytes = sizeof(abr::rtp_config);
	pRTPConfig.numBytes = sizeof(abr::rtp_config);
	pRTPConfig.isConnect = 1;
	pRTPConfig.isPL = 0x0;
	pRTPConfig.isAsync = 0x1;
	bool isRunning = 0x1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_RTPConfigForUpdate_TC7
*		Function Name: checkRTPConfigForUpdate
*		Input: &pRTPConfig,&pGraphConfig,numBytes,isRunning
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC7)
{

	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pRTPConfig.isInput = 1;
	size_t numBytes = sizeof(abr::rtp_config);
	pRTPConfig.numBytes = sizeof(abr::rtp_config);
	pRTPConfig.isConnect = 0;
	bool isRunning  = 0x0;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_RTPConfigForUpdate_TC8
*		Function Name: checkRTPConfigForUpdate
*		Input: pRTPConfig, pGraphConfig, numBytes, isRunning
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC8)
{
	abr::err_code err;
	abr::rtp_config pRTPConfig;
	abr::graph_config pGraphConfig;
	pRTPConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pRTPConfig.isInput = 1;
	size_t numBytes = sizeof(abr::rtp_config);
	pRTPConfig.numBytes = sizeof(abr::rtp_config);
	pRTPConfig.isConnect = 1;
	pRTPConfig.isPL = 0x0;
	bool isRunning  = 0x0;
	pRTPConfig.isAsync = 0x0;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkRTPConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}
TEST_GROUP(Xaie_SharedBufferConfig)
{

	void setup()
	{

	}
	void teardown()
	{
		mock_c()->clear();
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkSharedBufferConfigForUpdate_TC1
*		Function Name: checkSharedBufferConfigForUpdate
*		Input: &pSharedBufferConfig,&pGraphConfig,numBytes
*		Expectation: err == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_SharedBufferConfig,Xaie_checkSharedBufferConfigForUpdate_TC1)
{

	abr::err_code err;
	abr::shared_buffer_config *pSharedBufferConfig = nullptr;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkSharedBufferConfigForUpdate(pSharedBufferConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkSharedBufferConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::internal_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkSharedBufferConfigForUpdate_TC2
*		Function Name: checkSharedBufferConfigForUpdate
*		Input: &pSharedBufferConfig,&pGraphConfig,numBytes
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_SharedBufferConfig,Xaie_checkSharedBufferConfigForUpdate_TC2)
{

	abr::err_code err;
	abr::shared_buffer_config pSharedBufferConfig;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);
	pSharedBufferConfig.graphId = 0x1;
	pGraphConfig.id = 0x2;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkSharedBufferConfigForUpdate(&pSharedBufferConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkSharedBufferConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkSharedBufferConfigForUpdate_TC3
*		Function Name: checkSharedBufferConfigForUpdate
*		Input: &pSharedBufferConfig,&pGraphConfig,numBytes
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_SharedBufferConfig,Xaie_checkSharedBufferConfigForUpdate_TC3)
{

	abr::err_code err;
	abr::shared_buffer_config pSharedBufferConfig;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);
	pSharedBufferConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pSharedBufferConfig.numInputs = 2;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkSharedBufferConfigForUpdate(&pSharedBufferConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkSharedBufferConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkSharedBufferConfigForUpdate_TC4
*		Function Name: checkSharedBufferConfigForUpdate
*		Input: &pSharedBufferConfig,&pGraphConfig,numBytes
*		Expectation: err == err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_SharedBufferConfig,Xaie_checkSharedBufferConfigForUpdate_TC4)
{

	abr::err_code err;
	abr::shared_buffer_config pSharedBufferConfig;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);
	pSharedBufferConfig.graphId = 0x1;
	pGraphConfig.id = 0x1;
	pSharedBufferConfig.numInputs = 0;
	pSharedBufferConfig.numOutputs = 1;
	pSharedBufferConfig.numBytes = sizeof(abr::shared_buffer_config);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkSharedBufferConfigForUpdate(&pSharedBufferConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkSharedBufferConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;


	CHECK(err == abr::err_code::user_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_checkSharedBufferConfigForUpdate_TC5
*		Function Name: checkSharedBufferConfigForUpdate
*		Input: pSharedBufferConfig, pGraphConfig, numBytes
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_SharedBufferConfig,Xaie_checkSharedBufferConfigForUpdate_TC5)
{

	abr::err_code err;
	abr::shared_buffer_config pSharedBufferConfig;
	abr::graph_config pGraphConfig;
	size_t numBytes = sizeof(abr::graph_config);
	pSharedBufferConfig.graphId = 0x1;
	pSharedBufferConfig.initialized = 0;
	pGraphConfig.id = 0x1;
	pSharedBufferConfig.numInputs = 0;
	pSharedBufferConfig.numOutputs = 1;
	pSharedBufferConfig.numBytes = sizeof(abr::graph_config);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = checkSharedBufferConfigForUpdate(&pSharedBufferConfig,&pGraphConfig,numBytes);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by checkSharedBufferConfigForUpdate function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::ok);
}

TEST_GROUP(Xaie_LockApiTests) {
	void setup() {
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
	}

	void teardown() {
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_LockApiTests_initializeLock_TC1
*		Function Name: abr::lock_api::initializeLock
*		Input: tileType, column, row, lockId, initVal
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_LockApiTests,Xaie_LockApiTests_initializeLock_TC1)
{

	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	unsigned short lockId = 0x5;
	int8_t initVal = 0x0;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	XAie_Lock testLock;
	testLock.LockId = 5;
	testLock.LockVal = 0;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_LockSetValue")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withParameterOfType("XAie_Lock", "Lock", &testLock)
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = lock_api::initializeLock(tileType,column,row,lockId,initVal);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by initializeLock function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_LockApiTests_initializeLock_TC2
*		Function Name: abr::lock_api::initializeLock
*		Input: tileType, column, row, lockId, initVal
*		Expectation: err == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_LockApiTests,Xaie_LockApiTests_initializeLock_TC2)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	unsigned short lockId = 0x5;
	int8_t initVal = 0x0;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	XAie_Lock testLock;
	testLock.LockId = 5;
	testLock.LockVal = 0;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_LockSetValue")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withParameterOfType("XAie_Lock", "Lock", &testLock)
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = lock_api::initializeLock(tileType,column,row,lockId,initVal);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by initializeLock function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_LockApiTests_acquireLock_TC3
*		Function Name: abr::lock_api::acquireLock
*		Input: tileType, column, row, lockId, acqVal
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_LockApiTests,Xaie_LockApiTests_acquireLock_TC3)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	unsigned short lockId = 0x5;
	int8_t acqVal = 0x0;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	XAie_Lock testLock;
	testLock.LockId = 5;
	testLock.LockVal = 0;
	DummyXAie_DevInst devInst;
	u32 TimeOut = 0x7fffffff;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_LockAcquire")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withParameterOfType("XAie_Lock", "Lock", &testLock)
		->withUnsignedIntParameters("TimeOut", TimeOut)
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = lock_api::acquireLock(tileType,column,row,lockId,acqVal);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by acquireLock function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_LockApiTests_acquireLock_TC4
*		Function Name: abr::lock_api::acquireLock
*		Input: tileType, column, row, lockId, acqVal
*		Expectation: err == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_LockApiTests,Xaie_LockApiTests_acquireLock_TC4)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	unsigned short lockId = 0x5;
	int8_t acqVal = 0x0;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	XAie_Lock testLock;
	testLock.LockId = 5;
	testLock.LockVal = 0;
	DummyXAie_DevInst devInst;
	u32 TimeOut = 0x7fffffff;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_LockAcquire")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withParameterOfType("XAie_Lock", "Lock", &testLock)
		->withUnsignedIntParameters("TimeOut", TimeOut)
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = lock_api::acquireLock(tileType,column,row,lockId,acqVal);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by acquireLock function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_LockApiTests_releaseLock_TC5
*		Function Name: abr::lock_api::releaseLock
*		Input: tileType, column, row, lockId, acqVal
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_LockApiTests,Xaie_LockApiTests_releaseLock_TC5)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	unsigned short lockId = 0x5;
	int8_t acqVal = 0x0;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	u32 TimeOut = 0x7fffffff;
	XAie_Lock testLock;
	testLock.LockId = 5;
	testLock.LockVal = 0;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

	mock_c()->expectOneCall("XAie_LockRelease")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withParameterOfType("XAie_Lock", "Lock", &testLock)
		->withUnsignedIntParameters("TimeOut", TimeOut)
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = lock_api::releaseLock(tileType,column,row,lockId,acqVal);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by releaseLock function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_LockApiTests_releaseLock_TC6
*		Function Name: abr::lock_api::releaseLock
*		Input: tileType, column, row, lockId, acqVal
*		Expectation: err == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_LockApiTests,Xaie_LockApiTests_releaseLock_TC6)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	unsigned short lockId = 0x5;
	int8_t acqVal = 0x0;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	u32 TimeOut = 0x7fffffff;
	XAie_Lock testLock;
	testLock.LockId = 5;
	testLock.LockVal = 0;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

	mock_c()->expectOneCall("XAie_LockRelease")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withParameterOfType("XAie_Lock", "Lock", &testLock)
		->withUnsignedIntParameters("TimeOut", TimeOut)
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = lock_api::releaseLock(tileType,column,row,lockId,acqVal);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by releaseLock function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::aie_driver_error);
}
TEST_GROUP(Xaie_DMAApiTests) {
	void setup() {
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
		mock_c()->installCopier("u8", u8Copier);
	}

	void teardown() {
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_updateBDAddress_TC1
*		Function Name: abr::dma_api::updateBDAddress
*		Input: tileType=0x0, column=0x2, row=0x3, bdId, address
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiTests,Xaie_updateBDAddress_TC1)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	uint8_t bdId = 0x1;
	uint64_t address = 0x2000000000;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withUnsignedLongLongIntParameters("Addr", address)
		->withUnsignedIntParameters("BdNum", bdId)
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::updateBDAddress(tileType,column,row,bdId,address);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by updateBDAddress function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: Xaie_updateBDAddress_TC2
*		Function Name: abr::dma_api::updateBDAddress
*		Input: tileType=0x0, column=0x2, row=0x3, bdId, address
*		Expectation: err == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiTests,Xaie_updateBDAddress_TC2)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	uint8_t bdId = 0x1;
	uint64_t address = 0x2000000000;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withUnsignedLongLongIntParameters("Addr", address)
		->withUnsignedIntParameters("BdNum", bdId)
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::updateBDAddress(tileType,column,row,bdId,address);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by updateBDAddress function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_waitDMAChannelDone_TC3
*		Function Name: abr::dma_api::waitDMAChannelDone
*		Input: tileType=0x0, column=0x2, row=0x3, dir, channel, TimeOutUs
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiTests,Xaie_waitDMAChannelDone_TC3)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	int dir = 0x0;
	uint8_t channel = 0x1;
	u32 TimeOutUs = 0;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_DmaWaitForDone")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withUnsignedIntParameters("ChNum", channel)
		->withIntParameters("Dir", dir)
		->withUnsignedIntParameters("TimeOutUs", TimeOutUs)
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::waitDMAChannelDone(tileType,column,row,dir,channel);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by waitDMAChannelDone function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_waitDMAChannelTaskQueue_TC4
*		Function Name: abr::dma_api::waitDMAChannelTaskQueue
*		Input: tileType=0x0, column=0x2, row=0x3, dir, channel
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiTests,Xaie_waitDMAChannelTaskQueue_TC4)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	int dir = 0x0;
	uint8_t channel = 0x1;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;

	u8 firstCallOutput = 5;
	u8 secondCallOutput = 2;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")

		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withUnsignedIntParameters("ChNum", channel)
		->withIntParameters("Dir", dir)
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &firstCallOutput)

		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
		mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")

		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withUnsignedIntParameters("ChNum", channel)
		->withIntParameters("Dir", dir)
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &secondCallOutput)

		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::waitDMAChannelTaskQueue(tileType,column,row,dir,channel);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by waitDMAChannelTaskQueue function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_waitDMAChannelTaskQueue_TC5
*		Function Name: abr::dma_api::waitDMAChannelTaskQueue
*		Input: tileType=0x0, column=0x2, row=0x3, dir, channel, PendingBd
*		Expectation: err == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiTests,Xaie_waitDMAChannelTaskQueue_TC5)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	int dir = 0x0;
	uint8_t channel = 0x1;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;

	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

	u8 firstCallOutput = 5;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")

		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withUnsignedIntParameters("ChNum", channel)
		->withIntParameters("Dir", dir)
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &firstCallOutput)

		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")

		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withUnsignedIntParameters("ChNum", channel)
		->withIntParameters("Dir", dir)
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &secondCallOutput)

		->ignoreOtherParameters()
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::waitDMAChannelTaskQueue(tileType,column,row,dir,channel);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by waitDMAChannelTaskQueue function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::aie_driver_error);
}
//*********************************************************************************/
/*
*		Unique test case ID: Xaie_DMAApiTests_enqueueTask_TC6
*		Function Name: abr::dma_api::enqueueTask
*		Input: tileType=0x0, column=0x2, row=0x3, dir, channel, repeatCount, enableTaskCompleteToken, startBdId
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiTests,Xaie_DMAApiTests_enqueueTask_TC6)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	int dir = 0x0;
	uint8_t channel = 0x1;
	uint32_t repeatCount = 0x3;
	bool enableTaskCompleteToken = 0x0;
	uint8_t startBdId = 0x0;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	u16 BdNum = 0x0;
	u32 RepeatCount = 0x3;
	u8 EnTokenIssue = 0x0;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withUnsignedIntParameters("ChNum", channel)
		->withIntParameters("Dir", dir)
		->withUnsignedIntParameters("BdNum", BdNum)
		->withUnsignedIntParameters("RepeatCount", RepeatCount)
		->withUnsignedIntParameters("EnTokenIssue", EnTokenIssue)
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::enqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, startBdId);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by enqueueTask function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::ok);
}


//*********************************************************************************/
/*
*		Unique test case ID: Xaie_DMAApiTests_enqueueTask_TC7
*		Function Name: abr::dma_api::enqueueTask
*		Input: tileType=0x0, column=0x2, row=0x3, dir, channel, repeatCount, enableTaskCompleteToken, startBdId
*		Expectation: err == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiTests,Xaie_DMAApiTests_enqueueTask_TC7)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	int dir = 0x0;
	uint8_t channel = 0x1;
	uint32_t repeatCount = 0x3;
	bool enableTaskCompleteToken = 0x0;
	uint8_t startBdId = 0x0;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 14;
	u16 BdNum = 0x0;
	u32 RepeatCount = 0x3;
	u8 EnTokenIssue = 0x0;
	DummyXAie_DevInst devInst;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->withUnsignedIntParameters("ChNum", channel)
		->withIntParameters("Dir", dir)
		->withUnsignedIntParameters("BdNum", BdNum)
		->withUnsignedIntParameters("RepeatCount", RepeatCount)
		->withUnsignedIntParameters("EnTokenIssue", EnTokenIssue)
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::enqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, startBdId);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by enqueueTask function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::aie_driver_error);
}

TEST_GROUP(Xaie_DMAApiConfigTests) {

	void setup() {
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installComparator("XAie_Packet", XAie_PacketComparator, XAie_PacketToString);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
		mock_c()->installCopier("u8", u8Copier);

	}


	void teardown() {
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC1
*		Function Name: abr::dma_api::configureBdWaitQueueEnqueueTask
*		Input: tileType=0x0, column=0x2, row=0x3, dir=0x1, channel=0x1, repeatCount=0x1, bdParams={...}
*		Expectation: err == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiConfigTests,Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC1)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	int dir = 0x1;
	uint8_t channel = 0x1;
	uint32_t repeatCount = 0x1;
	bool enableTaskCompleteToken = true;
	std::vector<uint8_t> bdIds = {0};
	std::vector<dma_api::buffer_descriptor> bdParams = {};
	config_manager::s_bInitialized = false;
	size_t num_reserved_row = 10;
	bool broadcast_enable_core = true;
	XAie_DevInst devInst;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIE;
	config_manager::initialize(&devInst,num_reserved_row,broadcast_enable_core);

	config_manager::s_bInitialized = true;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::configureBdWaitQueueEnqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, bdIds, bdParams);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configureBdWaitQueueEnqueueTask function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::internal_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC2
*		Function Name: abr::dma_api::configureBdWaitQueueEnqueueTask
*		Input: tileType=0x0, column=0x2, row=0x3, dir=0x1, channel=0x1, repeatCount=0x1, bdParams={...}
*		Expectation: err == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiConfigTests,Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC2)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	int dir = 0x1;
	uint8_t channel = 0x1;
	uint32_t repeatCount = 0x1;
	bool enableTaskCompleteToken = true;
	std::vector<uint8_t> bdIds = {0};
	std::vector<dma_api::buffer_descriptor> bdParams = {};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::configureBdWaitQueueEnqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, bdIds, bdParams);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configureBdWaitQueueEnqueueTask function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::internal_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC3
*		Function Name: abr::dma_api::configureBdWaitQueueEnqueueTask
*		Input: tileType=0x0, column=0x2, row=0x14, dir=0x1, channel=0x1, repeatCount=0x1, bdParams={...}
*		Expectation: err == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiConfigTests,Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC3)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	int dir = 0x1;
	uint8_t channel = 0x1;
	uint32_t repeatCount = 0x1;
	bool enableTaskCompleteToken = true;
	std::vector<uint8_t> bdIds = {0};

	dma_api::buffer_descriptor bdParam1;
	bdParam1.address = 0x1000;
	bdParam1.length = 1024;

	dma_api::buffer_descriptor bdParam2;
	bdParam2.address = 0x2000;
	bdParam2.length = 2048;

	std::vector<dma_api::buffer_descriptor> bdParams = {bdParam1, bdParam2};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::configureBdWaitQueueEnqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, bdIds, bdParams);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configureBdWaitQueueEnqueueTask function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::internal_error);
}
abr::err_code waitDMAChannelTaskQueue(int tileType, uint8_t column, uint8_t row, int dir, uint8_t channel)
{
	return (err_code) mock_c()->actualCall("waitDMAChannelTaskQueue")
		->withIntParameters("tileType", tileType)
		->withIntParameters("col",column)
		->withIntParameters("row",row)
		->withIntParameters("Dir",dir)
		->withUnsignedIntParameters("Channel",channel)
		->returnIntValueOrDefault(XAIE_OK);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC4
*		Function Name: abr::dma_api::configureBdWaitQueueEnqueueTask
*		Input: tileType=0x0, column=0x2, row=0x14, dir=0x1, channel=0x1, repeatCount=0x1, bdParams={...}
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiConfigTests,Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC4)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x14;
	int dir = 0x1;
	uint8_t channel = 0x1;
	uint32_t repeatCount = 0x1;
	bool enableTaskCompleteToken = true;
	std::vector<uint8_t> bdIds = {0};

	dma_api::buffer_descriptor bdParam;
	bdParam.address = 0x1000;
	bdParam.length = 1024;
	std::vector<dma_api::buffer_descriptor> bdParams = {bdParam};
	XAie_LocType testLoc,testLoc1;
	testLoc.Col = 2;
	testLoc.Row = 6;
	testLoc1.Col = 2;
	testLoc1.Row = 4;
	DummyXAie_DevInst devInst;
	XAie_DmaDesc DmaDesc;
	DmaDesc.IsReady = 0;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()

		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	u8 firstCallOutput = 5;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")

		->ignoreOtherParameters()

		->withUnsignedIntParameters("ChNum", channel)
		->withIntParameters("Dir", dir)
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &firstCallOutput)


		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")

		->ignoreOtherParameters()

		->withUnsignedIntParameters("ChNum", channel)
		->withIntParameters("Dir", dir)
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &secondCallOutput)


		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")

		->ignoreOtherParameters()

		->withUnsignedIntParameters("ChNum", channel)
		->withIntParameters("Dir", dir)
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &secondCallOutput)


		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectNCalls(2,"waitDMAChannelTaskQueue")
		->withIntParameters("tileType", tileType)
		->withIntParameters("col",column)
		->withIntParameters("row",row)
		->withIntParameters("Dir",dir)
		->withUnsignedIntParameters("Channel",channel)
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::configureBdWaitQueueEnqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, bdIds, bdParams);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configureBdWaitQueueEnqueueTask function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::ok);
	mock_c()->clear();
}
//*********************************************************************************/
/*
*		Unique test case ID: Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC5
*		Function Name: abr::dma_api::configureBdWaitQueueEnqueueTask
*		Input: tileType=0x0, column=0x2, row=0x3, dir=0x1, channel=0x1, repeatCount=0x1, enableTaskCompleteToken=true, bdIds={0}, bdParams={...}
*		Expectation: err == err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiConfigTests,Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC5)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	int dir = 0x1;
	uint8_t channel = 0x1;
	uint32_t repeatCount = 0x1;
	bool enableTaskCompleteToken = true;
	std::vector<uint8_t> bdIds = {0};


	dma_api::buffer_descriptor bdParam;
	bdParam.address = 0x1000;
	bdParam.length = 1024;
	std::vector<dma_api::buffer_descriptor> bdParams = {bdParam};


	config_manager::s_bInitialized = false;
	size_t num_reserved_row = 10;
	bool broadcast_enable_core = true;
	XAie_DevInst devInst;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_S100;
	config_manager::initialize(&devInst, num_reserved_row, broadcast_enable_core);
	config_manager::s_bInitialized = true;


	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::configureBdWaitQueueEnqueueTask(tileType, column, row, dir, channel, repeatCount, enableTaskCompleteToken, bdIds, bdParams);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configureBdWaitQueueEnqueueTask function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::internal_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_configureBD_MemoryTile_TC6
*		Function Name: abr::dma_api::configureBD
*		Input: tileType=0x0, column=0x2, row=0x3
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiConfigTests, Xaie_configureBD_MemoryTile_TC6)
{
	abr::err_code err;
	int tileType = 2;
	uint8_t column = 0x2;
	uint8_t row = 0x3;
	uint8_t bdIds = 0;

	dma_api::buffer_descriptor bdParam;
	bdParam.address = 0x1000;
	bdParam.length = 1024;
	bdParam.stepsize = {128};
	bdParam.lock_acq_enable = false;

	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 4;

	DummyXAie_DevInst devInst;
	XAie_DmaDesc DmaDesc;
	DmaDesc.IsReady = 0;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::configureBD(tileType, column, row, bdIds, bdParam);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configureBD function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_configureBD_TC7
*		Function Name: abr::dma_api::configureBD
*		Input: tileType=0x0, column=0x0, row=0x0
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(Xaie_DMAApiConfigTests,Xaie_configureBD_TC7)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x2;
	uint8_t row = 0x3;

	uint8_t  bdIds = {0};
	dma_api::buffer_descriptor bdParam;
	bdParam.address = 0x1000;
	bdParam.length = 1024;
	bdParam.enable_packet = 1;
	bdParam.tlast_suppress = 1;
	bdParam.stepsize = {128, 256};
	bdParam.enable_compression = 1;
	bdParam.lock_acq_enable = 1;
	bdParam.use_next_bd = 1;
	XAie_LocType testLoc;
	testLoc.Col = 2;
	testLoc.Row = 4;
	DummyXAie_DevInst devInst;
	XAie_DmaDesc DmaDesc;
	DmaDesc.IsReady = 0;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetZeroPadding")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetPkt")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetOutofOrderBdId")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaTlastDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetBdIteration")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaEnableCompression")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetLock")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetNextBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::configureBD(tileType,column,row,bdIds, bdParam);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configureBD function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::ok);
}
//*********************************************************************************/
/*
*		Unique test case ID: Xaie_configureBD_TC8
*		Function Name: abr::dma_api::configureBD
*		Input: tileType=0x1, column=0x1, row=0x0
*		Expectation: err == err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(Xaie_DMAApiConfigTests,Xaie_configureBD_TC8)
{
	abr::err_code err;
	int tileType = 0x0;
	uint8_t column = 0x0;
	uint8_t row = 0x0;

	uint8_t  bdIds = {0};
	dma_api::buffer_descriptor bdParam;
	bdParam.address = 0x1000;
	bdParam.length = 1024;
	bdParam.enable_packet = 1;
	bdParam.tlast_suppress = 1;
	bdParam.stepsize = {128, 256};
	bdParam.enable_compression = 1;
	bdParam.lock_acq_enable = 1;
	bdParam.use_next_bd = 1;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 1;
	DummyXAie_DevInst devInst;
	XAie_DmaDesc DmaDesc;
	DmaDesc.IsReady = 0;
	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetZeroPadding")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetPkt")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetOutofOrderBdId")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaTlastDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetBdIteration")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaEnableCompression")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetLock")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetNextBd")
		->ignoreOtherParameters()
		->andReturnIntValue(1);

	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(1);

	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::configureBD(tileType,column,row,bdIds, bdParam);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configureBD function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == abr::err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: Xaie_configureBD_TC9
*		Function Name: abr::dma_api::configureBD
*		Input: tileType=0x1, column=0x1, row=0x0, bdIds={0}
*		Expectation: err == err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(Xaie_DMAApiConfigTests,Xaie_configureBD_TC9)
{
	abr::err_code err;
	int tileType = 0x1;
	uint8_t column = 0x1;
	uint8_t row = 0x0;

	uint8_t  bdIds = {0};
	dma_api::buffer_descriptor bdParam;
	bdParam.address = 0x1000;
	bdParam.length = 1024;
	bdParam.enable_packet = 1;
	bdParam.padding = {{4, 4}};
	bdParam.tlast_suppress = 1;
	bdParam.stepsize = {128, 256};
	bdParam.enable_compression = 1;
	bdParam.iteration_stepsize = 32;
	bdParam.lock_acq_enable = 1;
	bdParam.use_next_bd = 1;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;
	DummyXAie_DevInst devInst;
	XAie_DmaDesc DmaDesc;
	DmaDesc.IsReady = 0;

	DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetZeroPadding")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetZeroPadding")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetPkt")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetOutofOrderBdId")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaTlastDisable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetBdIteration")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaEnableCompression")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetLock")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetNextBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = dma_api::configureBD(tileType,column,row,bdIds, bdParam);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by configureBD function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(err == abr::err_code::ok);
}

TEST_GROUP(GmioApiTests) {
	XAie_DevInst devInst;
	void setup() {
		devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML - 1;
		abr::config_manager::s_pDevInst = &devInst;
		abr::config_manager::s_bInitialized = true;
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installCopier("u8", u8Copier);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
	}

	void teardown() {
		abr::config_manager::s_pDevInst = nullptr;
		abr::config_manager::s_bInitialized = false;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Constructor_ValidConfig_TC1
*		Function Name: abr::gmio_api::gmio_api
*		Input: config.type = abr::gmio_config::gm2aie; config.id = 1; config.name = "test_gmio"; config.shimColumn = 0; config.channelNum = 0; config.burstLength = 4;
*		Expectation: &gmio != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GmioApiTests, GmioApiTests_Constructor_ValidConfig_TC1) {
	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::gmio_api gmio(&config);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api constructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(&gmio != nullptr);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Configure_Failure_NullConfig_TC2
*		Function Name: abr::gmio_api::configure
*		Input: config = nullptr
*		Expectation: result == abr::err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GmioApiTests, GmioApiTests_Configure_Failure_NullConfig_TC2) {
	abr::gmio_config *config = nullptr;

	abr::gmio_api gmio(config);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.configure();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api configure function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL((int)abr::err_code::internal_error, (int)result);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Configure_GM2AIE_Success_TC3
*		Function Name: abr::gmio_api::configure
*		Input: config.type = abr::gmio_config::gm2aie; config.id = 1; config.name = "test_gmio"; config.shimColumn = 0; config.channelNum = 0; config.burstLength = 4;
*		Expectation: result == abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiTests, GmioApiTests_Configure_GM2AIE_Success_TC3) {
	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;

	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(XAIE_OK);


	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::gmio_api gmio(&config);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.configure();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api configure function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::ok, (int)result);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Configure_GM2AIE_Failure_TC4
*		Function Name: abr::gmio_api::configure
*		Input: config.type = abr::gmio_config::gm2aie; config.id = 1; config.name = "test_gmio"; config.shimColumn = 0; config.channelNum = 0; config.burstLength = 4;
*		Expectation: result == abr::err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiTests, GmioApiTests_Configure_GM2AIE_Failure_TC4) {
	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;

	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(1);


	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(1);

	abr::gmio_api gmio(&config);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.configure();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api configure function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::aie_driver_error, (int)result);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Configure_AIE2GM_Success_TC5
*		Function Name: abr::gmio_api::configure
*		Input: config.type = abr::gmio_config::aie2gm; config.id = 1; config.name = "test_gmio"; config.shimColumn = 0; config.channelNum = 0; config.burstLength = 4;
*		Expectation: result == abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiTests, GmioApiTests_Configure_AIE2GM_Success_TC5) {
	abr::gmio_config config;
	config.type = abr::gmio_config::aie2gm;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;

	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(XAIE_OK);


	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::gmio_api gmio(&config);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.configure();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api configure function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::ok, (int)result);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Configure_AIE2GM_Failure_TC6
*		Function Name: abr::gmio_api::configure
*		Input: config.type = abr::gmio_config::aie2gm; config.id = 1; config.name = "test_gmio"; config.shimColumn = 0; config.channelNum = 0; config.burstLength = 4;
*		Expectation: result == abr::err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GmioApiTests, GmioApiTests_Configure_AIE2GM_Failure_TC6) {
	abr::gmio_config config;
	config.type ;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;

	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::gmio_api gmio(&config);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.configure();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api configure function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::aie_driver_error, (int)result);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Wait_NullConfig_TC7
*		Function Name: abr::gmio_api::wait
*		Input: config = nullptr
*		Expectation: result == abr::err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(GmioApiTests, GmioApiTests_Wait_NullConfig_TC7) {
	abr::gmio_config *config = nullptr;

	abr::gmio_api gmio(config);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = gmio.wait(true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::internal_error, (int)result);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Wait_Sync_Success_TC8
*		Function Name: abr::gmio_api::wait
*		Input: true
*		Expectation: result == abr::err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiTests, GmioApiTests_Wait_Sync_Success_TC8) {

	abr::gmio_config config;
	config.type = gmio_config::gm2aie ;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;



	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;

	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(XAIE_OK);


	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::gmio_api gmio(&config);

	abr::err_code result = gmio.configure();
	config.type = gmio_config::gm2pl ;
	abr::gmio_api gmio1(&config);
	CHECK_EQUAL((int)abr::err_code::ok, (int)result);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = gmio.wait(true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::user_error, (int)res);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Wait_Async_Success_TC9
*		Function Name: abr::gmio_api::wait
*		Input: true
*		Expectation: result == abr::err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiTests, GmioApiTests_Wait_Async_Success_TC9) {

	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;

	abr::gmio_api gmio(&config);

	u32 TimeOutUs = 0;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::err_code result = gmio.configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)result);
	int dir = 0x0;
	uint8_t channel = 0x1;
	mock_c()->expectOneCall("XAie_DmaWaitForDone")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);



	abr::err_code result1 = gmio.configure();
	config.type = gmio_config::gm2pl ;
	abr::gmio_api gmio1(&config);
	CHECK_EQUAL((int)abr::err_code::ok, (int)result1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = gmio.wait(true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::user_error, (int)res);
}
//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Wait_FalseSync_TC10
*		Function Name: abr::gmio_api::wait
*		Input: false
*		Expectation: result == abr::err_code::resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiTests, GmioApiTests_Wait_FalseSync_TC10) {

	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;

	abr::gmio_api gmio(&config);

	u32 TimeOutUs = 0;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::err_code result = gmio.configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)result);
	int dir = 0x0;
	uint8_t channel = 0x1;
	mock_c()->expectOneCall("XAie_DmaWaitForDone")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = gmio.wait(false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::resource_unavailable, (int)res);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Wait_TrueSync_TC11
*		Function Name: abr::gmio_api::wait
*		Input: true
*		Expectation: result == abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiTests, GmioApiTests_Wait_TrueSync_TC11) {

	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;

	abr::gmio_api gmio(&config);

	u32 TimeOutUs = 0;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	int dir = 0x0;
	uint8_t channel = 0x1;
	mock_c()->expectOneCall("XAie_DmaWaitForDone")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);


	abr::err_code result1 = gmio.configure();

	CHECK_EQUAL((int)abr::err_code::ok, (int)result1);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = gmio.wait(true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);
}
//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Wait_FalseSync_WithMemSync_TC12
*		Function Name: abr::gmio_api::wait
*		Input: false
*		Expectation: result == abr::err_code::resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiTests, GmioApiTests_Wait_FalseSync_WithMemSync_TC12) {
	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;

	abr::gmio_api gmio(&config);

	u32 TimeOutUs = 0;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::err_code result = gmio.configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)result);

	mock_c()->expectOneCall("XAie_DmaSetAddrLen")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetLock")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	uint64_t address = 0x1000;
	size_t size = 1024;
	abr::err_code enqResult = gmio.enqueueBD(address, size);
	CHECK_EQUAL((int)abr::err_code::ok, (int)enqResult);
	int dir = 0x0;
	uint8_t channel = 0x1;
	mock_c()->expectOneCall("XAie_DmaWaitForDone")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->ignoreOtherParameters()
		->andReturnIntValue(1);

	abr::err_code result1 = gmio.configure();

	CHECK_EQUAL((int)abr::err_code::ok, (int)result1);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code res = gmio.wait(false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::resource_unavailable, (int)res);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_EnqueueBD_Success_TC13
*		Function Name: abr::gmio_api::enqueueBD
*		Input: address, size
*		Expectation: result == abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApiTests, GmioApiTests_EnqueueBD_Success_TC13) {
	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;

	abr::gmio_api gmio(&config);

	u32 TimeOutUs = 0;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::err_code result = gmio.configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)result);

	mock_c()->expectOneCall("XAie_DmaSetAddrLen")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetLock")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	uint64_t address = 0x1000;
	size_t size = 1024;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code enqResult = gmio.enqueueBD(address, size);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api enqueueBD function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::ok, (int)enqResult);
}

TEST_GROUP(GmioApi_enqueueBDTests) {
	XAie_DevInst devInst;

	void setup() {
		devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
		abr::config_manager::s_pDevInst = &devInst;
		abr::config_manager::s_bInitialized = true;
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installCopier("u8", u8Copier);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
	}

	void teardown() {
		abr::config_manager::s_pDevInst = nullptr;
		abr::config_manager::s_bInitialized = false;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};
//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_EnqueueBD_Failure_DescInit_TC01
*		Function Name: abr::gmio_api::enqueueBD
*		Input: address, size
*		Expectation: result == abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApi_enqueueBDTests, GmioApiTests_EnqueueBD_Failure_DescInit_TC01) {
	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;

	abr::gmio_api gmio(&config);

	u32 TimeOutUs = 0;
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::err_code result = gmio.configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)result);

	mock_c()->expectOneCall("XAie_DmaSetAddrLen")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetLock")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->ignoreOtherParameters()
		->andReturnIntValue(1);

	mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	uint64_t address = 0x1000;
	size_t size = 1024;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code enqResult = gmio.enqueueBD(address, size);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api enqueueBD function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::aie_driver_error, (int)enqResult);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_EnqueueBD_Failure_SetAddrLen_TC02
*		Function Name: abr::gmio_api::enqueueBD
*		Input: address, size
*		Expectation: result == abr::err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApi_enqueueBDTests, GmioApiTests_EnqueueBD_Failure_SetAddrLen_TC02) {
	abr::gmio_config *config = nullptr;
	uint64_t address = 0x1000;
	size_t size = 1024;
	abr::gmio_api gmio(config);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code enqResult = gmio.enqueueBD(address, size);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api enqueueBD function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::internal_error, (int)enqResult);
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_EnqueueBD_AIE_DevGen_TC03
*		Function Name: abr::gmio_api::enqueueBD
*		Input: address, size
*		Expectation: result == abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApi_enqueueBDTests, GmioApiTests_EnqueueBD_AIE_DevGen_TC03) {
	XAie_DevInst aieDevInst;
	aieDevInst.DevProp.DevGen = XAIE_DEV_GEN_AIE;
	abr::config_manager::s_pDevInst = &aieDevInst;
	abr::config_manager::s_bInitialized = true;

	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio_aie";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;

	abr::gmio_api gmio(&config);

	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;
	u8 queueSize = 2;

	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &aieDevInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &aieDevInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &queueSize)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &queueSize)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2, "XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::err_code result = gmio.configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)result);

	mock_c()->expectOneCall("XAie_DmaSetAddrLen")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	XAie_Lock expectedLock;
	expectedLock.LockId = 0;
	expectedLock.LockVal = XAIE_LOCK_WITH_NO_VALUE;
	mock_c()->expectOneCall("XAie_DmaSetLock")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	uint64_t address = 0x1000;
	size_t size = 1024;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code enqResult = gmio.enqueueBD(address, size);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api enqueueBD function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::ok, (int)enqResult);

	mock_c()->clear();
	abr::config_manager::s_pDevInst = nullptr;
	abr::config_manager::s_bInitialized = false;
}
//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_Destructor_Coverage_TC04
*		Function Name: abr::gmio_api
*		Input: config.type = gmio_config::gm2aie; config.id = 1; config.name = "test_gmio"; config.shimColumn = 0; config.channelNum = 0; config.burstLength = 4;
*		Expectation: gmio != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApi_enqueueBDTests, GmioApiTests_Destructor_Coverage_TC04) {
	XAie_DevInst aieDevInst;
	aieDevInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML;
	abr::config_manager::s_pDevInst = &aieDevInst;
	abr::config_manager::s_bInitialized = true;

	abr::gmio_config config;
	config.type = abr::gmio_config::gm2aie;
	config.id = 1;
	config.name = "test_gmio_destructor";
	config.shimColumn = 0;
	config.channelNum = 0;
	config.burstLength = 4;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::gmio_api *gmio = new abr::gmio_api(&config);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api destructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(gmio != nullptr);
	delete gmio;

	mock_c()->clear();
	abr::config_manager::s_pDevInst = nullptr;
	abr::config_manager::s_bInitialized = false;
}
//*********************************************************************************/
/*
*		Unique test case ID: GmioApiTests_EnqueueBD_WaitForAvailableBD_TC05
*		Function Name: abr::gmio_api::enqueueBD
*		Input: address, size
*		Expectation: result == abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GmioApi_enqueueBDTests, GmioApiTests_EnqueueBD_WaitForAvailableBD_TC05) {
	XAie_DevInst aiemlDevInst;
	aiemlDevInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML;
	abr::config_manager::s_pDevInst = &aiemlDevInst;
	abr::config_manager::s_bInitialized = true;

	abr::gmio_config config;
	config.type = abr::gmio_config::aie2gm;
	config.id = 2;
	config.name = "test_gmio_wait";
	config.shimColumn = 1;
	config.channelNum = 1;
	config.burstLength = 8;

	abr::gmio_api gmio(&config);

	XAie_LocType testLoc;
	testLoc.Col = 1;
	testLoc.Row = 0;
	u8 queueSize = 1;

	mock_c()->expectOneCall("XAie_DmaDescInit")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &queueSize)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &queueSize)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	abr::err_code result = gmio.configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)result);

	mock_c()->expectOneCall("XAie_DmaSetAddrLen")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetLock")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	uint64_t address1 = 0x1000;
	size_t size1 = 1024;
	abr::err_code enqResult1 = gmio.enqueueBD(address1, size1);
	CHECK_EQUAL((int)abr::err_code::ok, (int)enqResult1);


	u8 pendingBDs_first = 1;
	u8 pendingBDs_second = 0;

	mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &pendingBDs_first)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &pendingBDs_second)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAddrLen")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetLock")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	uint64_t address2 = 0x2000;
	size_t size2 = 2048;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code enqResult2 = gmio.enqueueBD(address2, size2);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by gmio_api enqueueBD function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL((int)abr::err_code::ok, (int)enqResult2);

	mock_c()->clear();
	abr::config_manager::s_pDevInst = nullptr;
	abr::config_manager::s_bInitialized = false;
}

TEST_GROUP(RuntimeApiTests)
{
	XAie_DevInst devInst;

	void setup()
	{
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
		mock_c()->installCopier("u8", u8Copier);
		devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML;
		abr::config_manager::s_pDevInst = &devInst;
		abr::config_manager::s_bInitialized = true;
		abr::config_manager::s_num_reserved_rows = 0;
		abr::config_manager::s_broadcast_enable_core = false;
		readOnlySharedBufferInitialized.clear();
	}

	void teardown()
	{
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
		abr::config_manager::s_pDevInst = nullptr;
		abr::config_manager::s_bInitialized = false;
		readOnlySharedBufferInitialized.clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: Configure_MismatchedIterMemAddrs
*		Function Name: abr::graph_api::configure
*		Input: cfg.coreColumns = {1, 2}; cfg.coreRows = {1, 2}; cfg.iterMemAddrs = {1}; cfg.iterMemColumns = {1, 2}; cfg.iterMemRows = {1, 2}; cfg.triggered = {false, false}; cfg.broadcast_column = 0;
*		Expectation: err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, Configure_MismatchedIterMemAddrs)
{
	double total_clock_enable_us = 0.0;

	abr::graph_config cfg;
	cfg.coreColumns = {1, 2};
	cfg.coreRows = {1, 2};
	cfg.iterMemAddrs = {1};
	cfg.iterMemColumns = {1, 2};
	cfg.iterMemRows = {1, 2};
	cfg.triggered = {false, false};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.configure() == err_code::internal_error);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api configure function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: Configure_MismatchedTriggered
*		Function Name: abr::graph_api::configure
*		Input: cfg.coreColumns = {1, 2}; cfg.coreRows = {1, 2}; cfg.iterMemAddrs = {1, 2}; cfg.iterMemColumns = {1, 2}; cfg.iterMemRows = {1, 2}; cfg.triggered = {false}; cfg.broadcast_column = 0;
*		Expectation: err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, Configure_MismatchedTriggered)
{
	double total_clock_enable_us = 0.0;

	abr::graph_config cfg;
	cfg.coreColumns = {1, 2};
	cfg.coreRows = {1, 2};
	cfg.iterMemAddrs = {1, 2};
	cfg.iterMemColumns = {1, 2};
	cfg.iterMemRows = {1, 2};
	cfg.triggered = {false};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.configure() == err_code::internal_error);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api configure function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: Configure_MismatchedCoreRows
*		Function Name: abr::graph_api::configure
*		Input: cfg.coreColumns = {1, 2}; cfg.coreRows = {1}; cfg.iterMemAddrs = {1, 2}; cfg.iterMemColumns = {1, 2}; cfg.iterMemRows = {1, 2}; cfg.triggered = {false, false}; cfg.broadcast_column = 0;
*		Expectation: err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, Configure_MismatchedCoreRows)
{
	double total_clock_enable_us = 0.0;

	abr::graph_config cfg;
	cfg.coreColumns = {1, 2};
	cfg.coreRows = {1};
	cfg.iterMemAddrs = {1, 2};
	cfg.iterMemColumns = {1, 2};
	cfg.iterMemRows = {1, 2};
	cfg.triggered = {false, false};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.configure() == err_code::internal_error);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api configure function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: Configure_MismatchedIterMemColumns
*		Function Name: abr::graph_api::configure
*		Input: cfg.coreColumns = {1, 2}; cfg.coreRows = {1, 2}; cfg.iterMemAddrs = {1, 2}; cfg.iterMemColumns = {1}; cfg.iterMemRows = {1, 2}; cfg.triggered = {false, false}; cfg.broadcast_column = 0;
*		Expectation: err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, Configure_MismatchedIterMemColumns)
{
	double total_clock_enable_us = 0.0;

	abr::graph_config cfg;
	cfg.coreColumns = {1, 2};
	cfg.coreRows = {1, 2};
	cfg.iterMemAddrs = {1, 2};
	cfg.iterMemColumns = {1};
	cfg.iterMemRows = {1, 2};
	cfg.triggered = {false, false};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.configure() == err_code::internal_error);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api configure function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: Configure_MismatchedIterMemRows
*		Function Name: abr::graph_api::configure
*		Input: cfg.coreColumns = {1, 2}; cfg.coreRows = {1, 2}; cfg.iterMemAddrs = {1, 2}; cfg.iterMemColumns = {1, 2}; cfg.iterMemRows = {1}; cfg.triggered = {false, false}; cfg.broadcast_column = 0;
*		Expectation: err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, Configure_MismatchedIterMemRows)
{
	double total_clock_enable_us = 0.0;

	abr::graph_config cfg;
	cfg.coreColumns = {1, 2};
	cfg.coreRows = {1, 2};
	cfg.iterMemAddrs = {1, 2};
	cfg.iterMemColumns = {1, 2};
	cfg.iterMemRows = {1};
	cfg.triggered = {false, false};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.configure() == err_code::internal_error);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api configure function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: RunWaitResumeWithEmptyCores
*		Function Name: abr::graph_api::wait
*		Input: cfg.coreColumns = {}; cfg.coreRows = {}; cfg.iterMemAddrs = {}; cfg.iterMemColumns = {}; cfg.iterMemRows = {}; cfg.triggered = {}; cfg.broadcast_column = 0;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, RunWaitResumeWithEmptyCores)
{
	double total_clock_enable_us = 0.0;
	abr::graph_config cfg;
	cfg.coreColumns = {};
	cfg.coreRows = {};
	cfg.iterMemAddrs = {};
	cfg.iterMemColumns = {};
	cfg.iterMemRows = {};
	cfg.triggered = {};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	CHECK(api.configure() == err_code::ok);

	mock_c()->expectOneCall("XAie_StartTransaction")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_SubmitTransaction")->ignoreOtherParameters();
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.run() == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api run function: " << total_clock_enable_us << " microseconds" << std::endl;
	const auto clock_enable_t2 = std::chrono::steady_clock::now();
	CHECK(api.wait() == err_code::ok);
	const auto clock_enable_t3 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t3 - clock_enable_t2).count();
	std::cout << "Time taken by abr graph_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	const auto clock_enable_t4 = std::chrono::steady_clock::now();
	CHECK(api.wait(static_cast<uint64_t>(100)) == err_code::ok);
	const auto clock_enable_t5 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t5 - clock_enable_t4).count();
	std::cout << "Time taken by abr graph_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
	const auto clock_enable_t6 = std::chrono::steady_clock::now();
	CHECK(api.resume() == err_code::ok);
	const auto clock_enable_t7 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t7 - clock_enable_t6).count();
	std::cout << "Time taken by abr graph_api resume function: " << total_clock_enable_us << " microseconds" << std::endl;

}
//*********************************************************************************/
/*
*		Unique test case ID: WaitWithTriggeredTrue
*		Function Name: abr::graph_api::wait
*		Input: cfg.coreColumns = {1}; cfg.coreRows = {1}; cfg.iterMemAddrs = {1}; cfg.iterMemColumns = {1}; cfg.iterMemRows = {1}; cfg.triggered = {true}; cfg.broadcast_column = 0;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, WaitWithTriggeredTrue)
{
	double total_clock_enable_us = 0.0;

	abr::graph_config cfg;
	cfg.coreColumns = {1};
	cfg.coreRows = {1};
	cfg.iterMemAddrs = {1};
	cfg.iterMemColumns = {1};
	cfg.iterMemRows = {1};
	cfg.triggered = {true};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	api.configure();
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.wait() == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: WaitLoopRetryOnTimeout
*		Function Name: abr::graph_api::wait
*		Input: cfg.coreColumns = {1}; cfg.coreRows = {1}; cfg.iterMemAddrs = {1}; cfg.iterMemColumns = {1}; cfg.iterMemRows = {1}; cfg.triggered = {false}; cfg.broadcast_column = 0;
*		Expectation: result matches expected value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, WaitLoopRetryOnTimeout)
{
	double total_clock_enable_us = 0.0;
	abr::graph_config cfg;
	cfg.coreColumns = {1};
	cfg.coreRows = {1};
	cfg.iterMemAddrs = {1};
	cfg.iterMemColumns = {1};
	cfg.iterMemRows = {1};
	cfg.triggered = {false};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	api.configure();
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_CORE_STATUS_TIMEOUT);
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.wait() == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: WaitTimeoutAlreadyElapsed
*		Function Name: abr::graph_api::wait
*		Input: cfg.coreColumns = {1}; cfg.coreRows = {1}; cfg.iterMemAddrs = {1}; cfg.iterMemColumns = {1}; cfg.iterMemRows = {1}; cfg.triggered = {false}; cfg.broadcast_column = 0; cycleTimeout = 10;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, WaitTimeoutAlreadyElapsed)
{
	double total_clock_enable_us = 0.0;
	abr::graph_config cfg;
	cfg.coreColumns = {1};
	cfg.coreRows = {1};
	cfg.iterMemAddrs = {1};
	cfg.iterMemColumns = {1};
	cfg.iterMemRows = {1};
	cfg.triggered = {false};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	api.configure();
	unsigned long long elapsed = 1000;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &elapsed, sizeof(elapsed))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.wait(static_cast<uint64_t>(10)) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: ResumeWhenCoreAlreadyDone
*		Function Name: abr::graph_api::resume
*		Input: cfg.coreColumns = {1}; cfg.coreRows = {1}; cfg.iterMemAddrs = {1}; cfg.iterMemColumns = {1}; cfg.iterMemRows = {1}; cfg.triggered = {false}; cfg.broadcast_column = 0;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, ResumeWhenCoreAlreadyDone)
{
	double total_clock_enable_us = 0.0;
	abr::graph_config cfg;
	cfg.coreColumns = {1};
	cfg.coreRows = {1};
	cfg.iterMemAddrs = {1};
	cfg.iterMemColumns = {1};
	cfg.iterMemRows = {1};
	cfg.triggered = {false};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	api.configure();
	unsigned long long timerVal = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &timerVal, sizeof(timerVal))
		->andReturnIntValue(XAIE_OK);
	u8 isDone = 1;
	mock_c()->expectOneCall("XAie_CoreReadDoneBit")
		->ignoreOtherParameters()
		->withOutputParameterReturning("DoneBit", &isDone, sizeof(isDone))
		->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.resume() == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api resume function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: EndLoopRetryOnTimeout
*		Function Name: abr::graph_api::end
*		Input: cfg.coreColumns = {1}; cfg.coreRows = {1}; cfg.iterMemAddrs = {4}; cfg.iterMemColumns = {1}; cfg.iterMemRows = {1}; cfg.triggered = {false}; cfg.broadcast_column = 0;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, EndLoopRetryOnTimeout)
{
	double total_clock_enable_us = 0.0;
	abr::graph_config cfg;
	cfg.coreColumns = {1};
	cfg.coreRows = {1};
	cfg.iterMemAddrs = {4};
	cfg.iterMemColumns = {1};
	cfg.iterMemRows = {1};
	cfg.triggered = {false};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	api.configure();

	unsigned long long startTimer = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &startTimer, sizeof(startTimer))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_StartTransaction")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreEnable")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_SubmitTransaction")->ignoreOtherParameters();
	CHECK(api.run() == err_code::ok);

	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreEnable")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_CORE_STATUS_TIMEOUT);
	mock_c()->expectOneCall("XAie_CoreWaitForDone")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreDisable")
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.end() == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api end function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: EndTriggeredCoreSkipsEndSequence
*		Function Name: abr::graph_api::end
*		Input: cfg.coreColumns = {1}; cfg.coreRows = {1}; cfg.iterMemAddrs = {4}; cfg.iterMemColumns = {1}; cfg.iterMemRows = {1}; cfg.triggered = {true}; cfg.broadcast_column = 0;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, EndTriggeredCoreSkipsEndSequence)
{
	double total_clock_enable_us = 0.0;
	abr::graph_config cfg;
	cfg.coreColumns = {1};
	cfg.coreRows = {1};
	cfg.iterMemAddrs = {4};
	cfg.iterMemColumns = {1};
	cfg.iterMemRows = {1};
	cfg.triggered = {true};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	api.configure();

	unsigned long long startTimer = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &startTimer, sizeof(startTimer))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_StartTransaction")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreEnable")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_SubmitTransaction")->ignoreOtherParameters();
	CHECK(api.run() == err_code::ok);

	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.end() == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api end function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: CheckRTPConfig_ConnectedAsyncNotRunning
*		Function Name: abr::checkRTPConfigForUpdate
*		Input: rc.isConnect = true; rc.isAsync = true; rc.isRunning = false;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, CheckRTPConfig_ConnectedAsyncNotRunning)
{
	double total_clock_enable_us = 0.0;
	abr::rtp_config rc;
	abr::graph_config gc;
	rc.graphId = 1;
	gc.id = 1;
	rc.isInput = true;
	rc.numBytes = sizeof(abr::graph_config);
	rc.isConnect = true;
	rc.isPL = false;
	rc.isAsync = true;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(checkRTPConfigForUpdate(&rc, &gc, sizeof(abr::graph_config), /*isRunning*/ false) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api run function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: ReadWithDevGenAIE
*		Function Name: abr::graph_api::read
*		Input: gc.id = 1; gc.name = "g"; rc.graphId = 1; rc.numBytes = 4; rc.isInput = false; rc.isAsync = false; rc.isConnect = false; rc.hasLock = true; rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1; rc.pingColumn = 1; rc.pingRow = 3; rc.pingAddr = 0x2000; rc.pingLockId = 2; rc.pongColumn = 1; rc.pongRow = 4; rc.pongAddr = 0x3000; rc.pongLockId = 3;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, ReadWithDevGenAIE)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIE;
	abr::graph_config gc;
	gc.id = 1;
	gc.name = "g";
	abr::rtp_config rc;
	rc.graphId = 1;
	rc.numBytes = 4;
	rc.isInput = false;
	rc.isAsync = false;
	rc.isConnect = false;
	rc.isPL = false;
	rc.hasLock = true;
	rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1;
	rc.pingColumn = 1;     rc.pingRow = 3;     rc.pingAddr = 0x2000;     rc.pingLockId = 2;
	rc.pongColumn = 1;     rc.pongRow = 4;     rc.pongAddr = 0x3000;     rc.pongLockId = 3;
	abr::graph_api api(&gc);
	api.configure();
	char buffer[16] = {0};
	u32 selector = 0;
	mock_c()->expectOneCall("XAie_LockAcquire")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->withOutputParameterReturning("Data", &selector, sizeof(selector))
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockAcquire")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockRead")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.read(&rc, buffer, rc.numBytes) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api read function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioConfigureChannelGT1
*		Function Name: abr::gmio_api::configure
*		Input: cfg.type = gm2aie; cfg.id = 7; cfg.name = "gm"; cfg.shimColumn = 0; cfg.channelNum = 2; cfg.burstLength = 4;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, GmioConfigureChannelGT1)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML - 1;
	abr::gmio_config cfg;
	cfg.type = abr::gmio_config::gm2aie;
	cfg.id = 7;
	cfg.name = "gm";
	cfg.shimColumn = 0;
	cfg.channelNum = 2;
	cfg.burstLength = 4;
	u8 qSize = 1;
	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &qSize)
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	abr::gmio_api gmio(&cfg);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gmio.configure() == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr gmio_api function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: GmioWaitSyncLoopRetry
*		Function Name: abr::gmio_api::wait
*		Input: cfg.type = gm2aie; cfg.id = 8; cfg.name = "gm"; cfg.shimColumn = 0; cfg.channelNum = 0; cfg.burstLength = 4;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, GmioWaitSyncLoopRetry)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML - 1;
	abr::gmio_config cfg;
	cfg.type = abr::gmio_config::gm2aie;
	cfg.id = 8;
	cfg.name = "gm";
	cfg.shimColumn = 0;
	cfg.channelNum = 0;
	cfg.burstLength = 4;
	u8 qSize = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &qSize)
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2, "XAie_DmaSetAxi")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	abr::gmio_api gmio(&cfg);
	CHECK(gmio.configure() == err_code::ok);

	mock_c()->expectOneCall("XAie_DmaWaitForDone")->ignoreOtherParameters()->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaWaitForDone")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gmio.wait(true) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr gmio_api function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioWaitSyncAIE2GM
*		Function Name: abr::gmio_api::wait
*		Input: cfg.type = aie2gm; cfg.id = 12; cfg.name = "aie2gm_sync"; cfg.shimColumn = 0; cfg.channelNum = 1; cfg.burstLength = 4;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, GmioWaitSyncAIE2GM)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML - 1;
	abr::gmio_config cfg;
	cfg.type = abr::gmio_config::aie2gm;
	cfg.id = 12;
	cfg.name = "aie2gm_sync";
	cfg.shimColumn = 0;
	cfg.channelNum = 1;
	cfg.burstLength = 4;
	u8 qSize = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &qSize)
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2, "XAie_DmaSetAxi")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	abr::gmio_api gmio(&cfg);
	CHECK(gmio.configure() == err_code::ok);

	mock_c()->expectOneCall("XAie_DmaWaitForDone")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gmio.wait(true) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr gmio_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioWaitPl2gmReturnsUserError
*		Function Name: abr::gmio_api::wait
*		Input: cfg.type = gm2aie -> pl2gm; cfg.id = 13; cfg.name = "pl2gm_wait"; cfg.shimColumn = 0; cfg.channelNum = 0; cfg.burstLength = 4;
*		Expectation: err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, GmioWaitPl2gmReturnsUserError)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML - 1;
	abr::gmio_config cfg;
	cfg.type = abr::gmio_config::gm2aie;
	cfg.id = 13;
	cfg.name = "pl2gm_wait";
	cfg.shimColumn = 0;
	cfg.channelNum = 0;
	cfg.burstLength = 4;
	u8 qSize = 1;
	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &qSize)
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	abr::gmio_api gmio(&cfg);
	CHECK(gmio.configure() == err_code::ok);

	cfg.type = abr::gmio_config::pl2gm;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gmio.wait(true) == err_code::user_error);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr gmio_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: WaitDMAChannelDoneLoopRetry
*		Function Name: abr::dma_api
*		Input: configured test fixture inputs
*		Expectation: result matches expected value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, WaitDMAChannelDoneLoopRetry)
{
	double total_clock_enable_us = 0.0;
	mock_c()->expectOneCall("XAie_DmaWaitForDone")->ignoreOtherParameters()->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaWaitForDone")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(abr::dma_api::waitDMAChannelDone(/*tileType*/0, /*col*/1, /*row*/1, /*dir*/0, /*ch*/0) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr dma_api function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: ConfigureBDWithWrap
*		Function Name: abr::dma_api::configureBD
*		Input: bd.address = 0x1000; bd.length = 1024; bd.stepsize = {128, 256}; bd.wrap = {16, 32}; bd.lock_acq_enable = false;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, ConfigureBDWithWrap)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML;
	abr::dma_api::buffer_descriptor bd;
	bd.address = 0x1000;
	bd.length = 1024;
	bd.stepsize = {128, 256};
	bd.wrap = {16, 32};
	bd.lock_acq_enable = false;
	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(abr::dma_api::configureBD(2, 1, 1, 0, bd) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr dma_api configureBD function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: ConfigureBDWithIterationWrap
*		Function Name: abr::dma_api::configureBD
*		Input: bd.address = 0x1000; bd.length = 1024; bd.stepsize = {128}; bd.iteration_wrap = 16; bd.lock_acq_enable = false;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, ConfigureBDWithIterationWrap)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML;
	abr::dma_api::buffer_descriptor bd;
	bd.address = 0x1000;
	bd.length = 1024;
	bd.stepsize = {128};
	bd.iteration_wrap = 16;
	bd.lock_acq_enable = false;
	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetBdIteration")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(abr::dma_api::configureBD(2, 1, 1, 0, bd) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr dma_api configureBD function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: ConfigureBDWithIterationCurrent
*		Function Name: abr::dma_api::configureBD
*		Input: bd.address = 0x1000; bd.length = 1024; bd.stepsize = {128}; bd.iteration_current = 5; bd.lock_acq_enable = false;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, ConfigureBDWithIterationCurrent)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML;
	abr::dma_api::buffer_descriptor bd;
	bd.address = 0x1000;
	bd.length = 1024;
	bd.stepsize = {128};
	bd.iteration_current = 5;
	bd.lock_acq_enable = false;
	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetBdIteration")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(abr::dma_api::configureBD(2, 1, 1, 0, bd) == err_code::ok);

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr dma_api configureBD function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: ConfigureBdWaitQueue_ConfigureBDFails
*		Function Name: abr::dma_api::configureBdWaitQueueEnqueueTask
*		Input: bdParams = {bd, bd}; bdIds = {0, 1};
*		Expectation: err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, ConfigureBdWaitQueue_ConfigureBDFails)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML;
	abr::dma_api::buffer_descriptor bd;
	bd.address = 0x1000;
	bd.length = 1024;
	bd.stepsize = {128};
	bd.lock_acq_enable = false;
	std::vector<abr::dma_api::buffer_descriptor> bdParams = {bd, bd};
	std::vector<uint8_t> bdIds = {0, 1};

	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(abr::dma_api::configureBdWaitQueueEnqueueTask(2, 1, 1, 0, 0, 1, false, bdIds, bdParams) == err_code::aie_driver_error);

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr dma_api configureBD function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioEnqueueBDWaitLoop_GM2AIE
*		Function Name: abr::gmio_api::enqueueBD
*		Input: configured test fixture inputs
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, GmioEnqueueBDWaitLoop_GM2AIE)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML;
	abr::gmio_config cfg;
	cfg.type = abr::gmio_config::gm2aie;
	cfg.id = 9;
	cfg.name = "gm2aie_loop";
	cfg.shimColumn = 0;
	cfg.channelNum = 0;
	cfg.burstLength = 4;
	u8 qSize = 1;
	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &qSize)
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	abr::gmio_api gmio(&cfg);
	CHECK(gmio.configure() == err_code::ok);
	mock_c()->expectOneCall("XAie_DmaSetAddrLen")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetLock")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gmio.enqueueBD(0x1000, 1024) == err_code::ok);

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr gmio_api enqueueBD function: " << total_clock_enable_us << " microseconds" << std::endl;
	u8 firstPending = 1;
	u8 secondPending = 0;
	mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &firstPending)
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
		->withOutputParameterOfTypeReturning("u8", "PendingBd", &secondPending)
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAddrLen")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetLock")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaEnableBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaWriteBd")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t2 = std::chrono::steady_clock::now();
	CHECK(gmio.enqueueBD(0x2000, 2048) == err_code::ok);
	const auto clock_enable_t3 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t3 - clock_enable_t2).count();
	std::cout << "Time taken by abr gmio_api enqueueBD function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioWaitAsyncReturnsOK
*		Function Name: abr::gmio_api::wait
*		Input: cfg.type = gm2aie; cfg.id = 10; cfg.name = "gm_async_ok"; cfg.shimColumn = 0; cfg.channelNum = 0; cfg.burstLength = 4;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, GmioWaitAsyncReturnsOK)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML - 1;
	abr::gmio_config cfg;
	cfg.type = abr::gmio_config::gm2aie;
	cfg.id = 10;
	cfg.name = "gm_async_ok";
	cfg.shimColumn = 0;
	cfg.channelNum = 0;
	cfg.burstLength = 4;
	u8 qSize = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &qSize)
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2, "XAie_DmaSetAxi")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	abr::gmio_api gmio(&cfg);
	CHECK(gmio.configure() == err_code::ok);

	mock_c()->expectOneCall("XAie_DmaWaitForDone")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gmio.wait(false) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr gmio_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: GmioWaitAsyncAIE2GM
*		Function Name: abr::gmio_api::wait
*		Input: cfg.type = aie2gm; cfg.id = 11; cfg.name = "aie2gm_async"; cfg.shimColumn = 0; cfg.channelNum = 1; cfg.burstLength = 4;
*		Expectation: result matches expected value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, GmioWaitAsyncAIE2GM)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML - 1;
	abr::gmio_config cfg;
	cfg.type = abr::gmio_config::aie2gm;
	cfg.id = 11;
	cfg.name = "aie2gm_async";
	cfg.shimColumn = 0;
	cfg.channelNum = 1;
	cfg.burstLength = 4;
	u8 qSize = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &qSize)
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2, "XAie_DmaSetAxi")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	abr::gmio_api gmio(&cfg);
	CHECK(gmio.configure() == err_code::ok);

	mock_c()->expectOneCall("XAie_DmaWaitForDone")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(gmio.wait(false) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr gmio_api wait function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: UpdateRTP_PingBranch_NoLock
*		Function Name: abr::graph_api::update
*		Input: gc.id = 1; gc.name = "g"; rc.graphId = 1; rc.numBytes = 4; rc.isInput = true; rc.isAsync = false; rc.isConnect = false; rc.hasLock = false; rc.isPL = false; rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1; rc.pingColumn = 1; rc.pingRow = 3; rc.pingAddr = 0x2000; rc.pingLockId = 2; rc.pongColumn = 1; rc.pongRow = 4; rc.pongAddr = 0x3000; rc.pongLockId = 3;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, UpdateRTP_PingBranch_NoLock)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
	abr::graph_config gc;
	gc.id = 1;
	gc.name = "g";
	abr::rtp_config rc;
	rc.graphId = 1;
	rc.numBytes = 4;
	rc.isInput = true;
	rc.isAsync = false;
	rc.isConnect = false;
	rc.hasLock = false;
	rc.isPL = false;
	rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1;
	rc.pingColumn = 1;     rc.pingRow = 3;     rc.pingAddr = 0x2000;     rc.pingLockId = 2;
	rc.pongColumn = 1;     rc.pongRow = 4;     rc.pongAddr = 0x3000;     rc.pongLockId = 3;
	abr::graph_api api(&gc);
	api.configure();
	uint8_t data[4] = {0};
	u32 selectorValue = 1;
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->withOutputParameterReturning("Data", &selectorValue, sizeof(selectorValue))
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.update(&rc, data, sizeof(data)) == err_code::ok);

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api update function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: UpdateAsyncSuspendedHasLock
*		Function Name: abr::graph_api::update
*		Input: gc.id = 1; gc.name = "g"; rc.graphId = 1; rc.numBytes = 4; rc.isInput = true; rc.isAsync = true; rc.isConnect = false; rc.hasLock = true; rc.isPL = false; rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1; rc.pingColumn = 1; rc.pingRow = 3; rc.pingAddr = 0x2000; rc.pingLockId = 2; rc.pongColumn = 1; rc.pongRow = 4; rc.pongAddr = 0x3000; rc.pongLockId = 3;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, UpdateAsyncSuspendedHasLock)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
	abr::graph_config gc;
	gc.id = 1;
	gc.name = "g";
	abr::rtp_config rc;
	rc.graphId = 1;
	rc.numBytes = 4;
	rc.isInput = true;
	rc.isAsync = true;
	rc.isConnect = false;
	rc.hasLock = true;
	rc.isPL = false;
	rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1;
	rc.pingColumn = 1;     rc.pingRow = 3;     rc.pingAddr = 0x2000;     rc.pingLockId = 2;
	rc.pongColumn = 1;     rc.pongRow = 4;     rc.pongAddr = 0x3000;     rc.pongLockId = 3;
	abr::graph_api api(&gc);
	api.configure();
	uint8_t data[4] = {0};
	u32 selectorValue = 1;
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->withOutputParameterReturning("Data", &selectorValue, sizeof(selectorValue))
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockWrite")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemWrWord")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.update(&rc, data, sizeof(data)) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api update function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: ReadAsyncNotRunningHasLockFalse
*		Function Name: abr::graph_api::read
*		Input: gc.id = 1; gc.name = "g"; rc.graphId = 1; rc.numBytes = 4; rc.isInput = false; rc.isAsync = true; rc.isConnect = false; rc.hasLock = false; rc.isPL = false; rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1; rc.pingColumn = 1; rc.pingRow = 3; rc.pingAddr = 0x2000; rc.pingLockId = 2; rc.pongColumn = 1; rc.pongRow = 4; rc.pongAddr = 0x3000; rc.pongLockId = 3;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, ReadAsyncNotRunningHasLockFalse)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
	abr::graph_config gc;
	gc.id = 1;
	gc.name = "g";
	abr::rtp_config rc;
	rc.graphId = 1;
	rc.numBytes = 4;
	rc.isInput = false;
	rc.isAsync = true;
	rc.isConnect = false;
	rc.hasLock = false;
	rc.isPL = false;
	rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1;
	rc.pingColumn = 1;     rc.pingRow = 3;     rc.pingAddr = 0x2000;     rc.pingLockId = 2;
	rc.pongColumn = 1;     rc.pongRow = 4;     rc.pongAddr = 0x3000;     rc.pongLockId = 3;
	abr::graph_api api(&gc);
	api.configure();
	char buffer[16] = {0};

	u32 selector = 0;
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->withOutputParameterReturning("Data", &selector, sizeof(selector))
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockRead")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.read(&rc, buffer, rc.numBytes) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api read function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: ReadSyncHasLockFalse
*		Function Name: abr::graph_api::read
*		Input: gc.id = 1; gc.name = "g"; rc.graphId = 1; rc.numBytes = 4; rc.isInput = false; rc.isAsync = false; rc.isConnect = false; rc.hasLock = false; rc.isPL = false; rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1; rc.pingColumn = 1; rc.pingRow = 3; rc.pingAddr = 0x2000; rc.pingLockId = 2; rc.pongColumn = 1; rc.pongRow = 4; rc.pongAddr = 0x3000; rc.pongLockId = 3;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, ReadSyncHasLockFalse)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
	abr::graph_config gc;
	gc.id = 1;
	gc.name = "g";
	abr::rtp_config rc;
	rc.graphId = 1;
	rc.numBytes = 4;
	rc.isInput = false;
	rc.isAsync = false;
	rc.isConnect = false;
	rc.hasLock = false;
	rc.isPL = false;
	rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1;
	rc.pingColumn = 1;     rc.pingRow = 3;     rc.pingAddr = 0x2000;     rc.pingLockId = 2;
	rc.pongColumn = 1;     rc.pongRow = 4;     rc.pongAddr = 0x3000;     rc.pongLockId = 3;
	abr::graph_api api(&gc);
	api.configure();
	char buffer[16] = {0};

	u32 selector = 0;
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->withOutputParameterReturning("Data", &selector, sizeof(selector))
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockRead")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.read(&rc, buffer, rc.numBytes) == err_code::ok);

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api read function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: ReadAsyncIsRunning_AfterRun
*		Function Name: abr::graph_api::read
*		Input: gc.id = 1; gc.name = "g"; gc.coreColumns = {1}; gc.coreRows = {1}; gc.iterMemAddrs = {1}; gc.iterMemColumns = {1}; gc.iterMemRows = {1}; gc.triggered = {false}; gc.broadcast_column = 0; rc.graphId = 1; rc.numBytes = 4; rc.isInput = false; rc.isAsync = true; rc.isConnect = false; rc.hasLock = true; rc.isPL = false; rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1; rc.pingColumn = 1; rc.pingRow = 3; rc.pingAddr = 0x2000; rc.pingLockId = 2; rc.pongColumn = 1; rc.pongRow = 4; rc.pongAddr = 0x3000; rc.pongLockId = 3;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, ReadAsyncIsRunning_AfterRun)
{
	double total_clock_enable_us = 0.0;
	devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
	abr::graph_config gc;
	gc.id = 1;
	gc.name = "g";
	gc.coreColumns = {1};
	gc.coreRows = {1};
	gc.iterMemAddrs = {1};
	gc.iterMemColumns = {1};
	gc.iterMemRows = {1};
	gc.triggered = {false};
	gc.broadcast_column = 0;
	abr::rtp_config rc;
	rc.graphId = 1;
	rc.numBytes = 4;
	rc.isInput = false;
	rc.isAsync = true;
	rc.isConnect = false;
	rc.hasLock = true;
	rc.isPL = false;
	rc.selectorColumn = 1; rc.selectorRow = 2; rc.selectorAddr = 0x1000; rc.selectorLockId = 1;
	rc.pingColumn = 1;     rc.pingRow = 3;     rc.pingAddr = 0x2000;     rc.pingLockId = 2;
	rc.pongColumn = 1;     rc.pongRow = 4;     rc.pongAddr = 0x3000;     rc.pongLockId = 3;
	abr::graph_api api(&gc);
	api.configure();

	abr::config_manager::s_broadcast_enable_core = false;
	unsigned long long t = 42;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &t, sizeof(t))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_StartTransaction")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_CoreEnable")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_SubmitTransaction")->ignoreOtherParameters();
	CHECK(api.run() == err_code::ok);
	char buffer[16] = {0};
	u32 selector = 0;
	mock_c()->expectOneCall("XAie_LockAcquire")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemRdWord")
		->withOutputParameterReturning("Data", &selector, sizeof(selector))
		->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockAcquire")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DataMemBlockRead")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_LockRelease")->ignoreOtherParameters()->andReturnIntValue(XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.read(&rc, buffer, rc.numBytes) == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api read function: " << total_clock_enable_us << " microseconds" << std::endl;
}
//*********************************************************************************/
/*
*		Unique test case ID: RunBroadcastMultiTimerIter
*		Function Name: abr::graph_api::run
*		Input: cfg.coreColumns = {0}; cfg.coreRows = {0}; cfg.iterMemAddrs = {0}; cfg.iterMemColumns = {0}; cfg.iterMemRows = {0}; cfg.triggered = {false}; cfg.broadcast_column = 0;
*		Expectation: err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RuntimeApiTests, RunBroadcastMultiTimerIter)
{
	double total_clock_enable_us = 0.0;
	abr::config_manager::s_broadcast_enable_core = true;
	abr::graph_config cfg;
	cfg.coreColumns = {0};
	cfg.coreRows = {0};
	cfg.iterMemAddrs = {0};
	cfg.iterMemColumns = {0};
	cfg.iterMemRows = {0};
	cfg.triggered = {false};
	cfg.broadcast_column = 0;
	abr::graph_api api(&cfg);
	api.configure();

	unsigned long long t_initial = 0;
	unsigned long long t_start = 0;
	unsigned long long t_cur1 = 100;
	unsigned long long t_cur2 = 300;
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &t_initial, sizeof(t_initial))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(2, "XAie_StartTransaction")->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_SubmitTransaction")->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_ClearCoreDisableEventOccurred")->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_CoreConfigureEnableEvent")->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_EventGenerate")->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &t_start, sizeof(t_start))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &t_cur1, sizeof(t_cur1))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_ReadTimer")
		->ignoreOtherParameters()
		->withOutputParameterReturning("TimerVal", &t_cur2, sizeof(t_cur2))
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_SubmitTransaction")->ignoreOtherParameters();
	mock_c()->expectOneCall("XAie_CoreConfigureEnableEvent")->ignoreOtherParameters();
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK(api.run() == err_code::ok);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by abr graph_api run function: " << total_clock_enable_us << " microseconds" << std::endl;
}

/*@section Test Execution details
| Test Case ID                                        | Test Result | CR  | Impact | Test Details |
|-----------------------------------------------------|-------------|-----|--------|--------------|
| TC_001_Run_Success_BroadcastEnabled                 | PASS        | No  | NA     | NA           |
| TC_002_Run_Success_BroadcastDisabled                | PASS        | No  | NA     | NA           |
| TC_003_Run_Failure_NullConfig                       | PASS        | No  | NA     | NA           |
| TC_004_Run_Failure_CoreEnableFails                  | PASS        | No  | NA     | NA           |
| TC_005_Run_Failure_DataMemWrWordFails               | PASS        | No  | NA     | NA           |
| TC_006_Run_Success_WithIterations                   | PASS        | No  | NA     | NA           |
| TC_007_Run_Failure_EventGenerateFails               | PASS        | No  | NA     | NA           |
| TC_008_Configure_Failure_NullConfig                 | PASS        | No  | NA     | NA           |
| TC_009_Configure_Failure_AlreadyConfigured          | PASS        | No  | NA     | NA           |
| TC_010_Wait_Failure_NullConfig                      | PASS        | No  | NA     | NA           |
| TC_011_Wait_Success                                 | PASS        | No  | NA     | NA           |
| TC_012_Wait_Failure_CoreWaitCycleFails              | PASS        | No  | NA     | NA           |
| TC_013_Wait_Failure_CoreWaitStatusFails             | PASS        | No  | NA     | NA           |
| TC_014_Wait_Success_WithTimeout                     | PASS        | No  | NA     | NA           |
| TC_015_Wait_Failure_TimeoutExpired                  | PASS        | No  | NA     | NA           |
| TC_016_Wait_Failure_CoreWaitDoneFails               | PASS        | No  | NA     | NA           |
| TC_017_Resume_Failure_NullConfig                    | PASS        | No  | NA     | NA           |
| TC_018_Resume_Success                               | PASS        | No  | NA     | NA           |
| TC_019_Resume_Failure_CoreEnableFails               | PASS        | No  | NA     | NA           |
| TC_020_End_Success                                  | PASS        | No  | NA     | NA           |
| TC_021_End_Failure_CoreDisableFails                 | PASS        | No  | NA     | NA           |
| TC_001_Update_SharedBufferSuccess                   | PASS        | No  | NA     | NA           |
| TC_002_Update_NullBufferConfig                      | PASS        | No  | NA     | NA           |
| TC_003_Update_Failure                               | PASS        | No  | NA     | NA           |
| TC_004_Update_SharedBufferWrongGraphId              | PASS        | No  | NA     | NA           |
| TC_005_Update_SharedBufferHasInputs                 | PASS        | No  | NA     | NA           |
| TC_006_Update_SharedBufferNoOutputs                 | PASS        | No  | NA     | NA           |
| TC_007_Update_SharedBufferSizeMismatch              | PASS        | No  | NA     | NA           |
| TC_001_Update_RTPSuccess                            | PASS        | No  | NA     | NA           |
| TC_002_Update_Failure                               | PASS        | No  | NA     | NA           |
| TC_003_Update_BlockWriteFails                       | PASS        | No  | NA     | NA           |
| TC_004_Update_S100DevGen                            | PASS        | No  | NA     | NA           |
| TC_005_Update_S200DevGen                            | PASS        | No  | NA     | NA           |
| TC_006_Update_ConnectedPLPort                       | PASS        | No  | NA     | NA           |
| TC_007_Update_ConnectedSyncRTP                      | PASS        | No  | NA     | NA           |
| TC_008_Update_AsyncRTPSecondUpdate_AIEML            | PASS        | No  | NA     | NA           |
| TC_009_Update_PongBranchWithLock                    | PASS        | No  | NA     | NA           |
| TC_010_Update_PingBranchWithLock                    | PASS        | No  | NA     | NA           |
| TC_011_Update_HasLockFalse_NoLockOps                | PASS        | No  | NA     | NA           |
| TC_001_End_UnconfiguredGraph                        | PASS        | No  | NA     | NA           |
| TC_002_End_WithTimeoutUnconfiguredGraph             | PASS        | No  | NA     | NA           |
| TC_003_End_ConfiguredGraph_Success                  | PASS        | No  | NA     | NA           |
| TC_004_End_ConfiguredGraph_CoreDisableFails         | PASS        | No  | NA     | NA           |
| TC_005_End_ConfiguredGraph_EventGenerateFails       | PASS        | No  | NA     | NA           |
| TC_006_End_ConfiguredGraph_WithTimeout              | PASS        | No  | NA     | NA           |
| TC_007_End_ConfiguredGraph_TimeoutWaitFails         | PASS        | No  | NA     | NA           |
| TC_001_Read_UnconfiguredGraph                       | PASS        | No  | NA     | NA           |
| TC_002_Read_UnconfiguredGraph_BlockReadFails        | PASS        | No  | NA     | NA           |
| TC_003_Read_InvalidRTPConfig                        | PASS        | No  | NA     | NA           |
| TC_004_Read_FromPingBuffer                          | PASS        | No  | NA     | NA           |
| TC_001_Initialize_AlreadyInitialized                | PASS        | No  | NA     | NA           |
| TC_002_Initialize_Success                           | PASS        | No  | NA     | NA           |
| TC_003_Initialize_NullDevInst                       | PASS        | No  | NA     | NA           |
| Xaie_checkRTPConfigForRead_TC1                      | PASS        | No  | NA     | NA           |
| Xaie_checkRTPConfigForRead_TC2                      | PASS        | No  | NA     | NA           |
| Xaie_checkRTPConfigForRead_TC3                      | PASS        | No  | NA     | NA           |
| Xaie_checkRTPConfigForRead_TC4                      | PASS        | No  | NA     | NA           |
| Xaie_checkRTPConfigForRead_TC5                      | PASS        | No  | NA     | NA           |
| Xaie_checkRTPConfigForRead_TC6                      | PASS        | No  | NA     | NA           |
| Xaie_RTPConfigForUpdate_TC1                         | PASS        | No  | NA     | NA           |
| Xaie_RTPConfigForUpdate_TC2                         | PASS        | No  | NA     | NA           |
| Xaie_RTPConfigForUpdate_TC3                         | PASS        | No  | NA     | NA           |
| Xaie_RTPConfigForUpdate_TC4                         | PASS        | No  | NA     | NA           |
| Xaie_RTPConfigForUpdate_TC5                         | PASS        | No  | NA     | NA           |
| Xaie_RTPConfigForUpdate_TC6                         | PASS        | No  | NA     | NA           |
| Xaie_RTPConfigForUpdate_TC7                         | PASS        | No  | NA     | NA           |
| Xaie_RTPConfigForUpdate_TC8                         | PASS        | No  | NA     | NA           |
| Xaie_checkSharedBufferConfigForUpdate_TC1           | PASS        | No  | NA     | NA           |
| Xaie_checkSharedBufferConfigForUpdate_TC2           | PASS        | No  | NA     | NA           |
| Xaie_checkSharedBufferConfigForUpdate_TC3           | PASS        | No  | NA     | NA           |
| Xaie_checkSharedBufferConfigForUpdate_TC4           | PASS        | No  | NA     | NA           |
| Xaie_checkSharedBufferConfigForUpdate_TC5           | PASS        | No  | NA     | NA           |
| Xaie_LockApiTests_initializeLock_TC1                | PASS        | No  | NA     | NA           |
| Xaie_LockApiTests_initializeLock_TC2                | PASS        | No  | NA     | NA           |
| Xaie_LockApiTests_acquireLock_TC3                   | PASS        | No  | NA     | NA           |
| Xaie_LockApiTests_acquireLock_TC4                   | PASS        | No  | NA     | NA           |
| Xaie_LockApiTests_releaseLock_TC5                   | PASS        | No  | NA     | NA           |
| Xaie_LockApiTests_releaseLock_TC6                   | PASS        | No  | NA     | NA           |
| Xaie_updateBDAddress_TC1                            | PASS        | No  | NA     | NA           |
| Xaie_updateBDAddress_TC2                            | PASS        | No  | NA     | NA           |
| Xaie_waitDMAChannelDone_TC3                         | PASS        | No  | NA     | NA           |
| Xaie_waitDMAChannelTaskQueue_TC4                    | PASS        | No  | NA     | NA           |
| Xaie_waitDMAChannelTaskQueue_TC5                    | PASS        | No  | NA     | NA           |
| Xaie_DMAApiTests_enqueueTask_TC6                    | PASS        | No  | NA     | NA           |
| Xaie_DMAApiTests_enqueueTask_TC7                    | PASS        | No  | NA     | NA           |
| Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC1| PASS        | No  | NA     | NA           |
| Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC2| PASS        | No  | NA     | NA           |
| Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC3| PASS        | No  | NA     | NA           |
| Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC4| PASS        | No  | NA     | NA           |
| Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC5| PASS        | No  | NA     | NA           |
| Xaie_configureBD_MemoryTile_TC6                     | PASS        | No  | NA     | NA           |
| Xaie_configureBD_TC7                                | PASS        | No  | NA     | NA           |
| Xaie_configureBD_TC8                                | PASS        | No  | NA     | NA           |
| Xaie_configureBD_TC9                                | PASS        | No  | NA     | NA           |
| GmioApiTests_Constructor_ValidConfig_TC1            | PASS        | No  | NA     | NA           |
| GmioApiTests_Configure_Failure_NullConfig_TC2       | PASS        | No  | NA     | NA           |
| GmioApiTests_Configure_GM2AIE_Success_TC3           | PASS        | No  | NA     | NA           |
| GmioApiTests_Configure_GM2AIE_Failure_TC4           | PASS        | No  | NA     | NA           |
| GmioApiTests_Configure_AIE2GM_Success_TC5           | PASS        | No  | NA     | NA           |
| GmioApiTests_Configure_AIE2GM_Failure_TC6           | PASS        | No  | NA     | NA           |
| GmioApiTests_Wait_NullConfig_TC7                    | PASS        | No  | NA     | NA           |
| GmioApiTests_Wait_Sync_Success_TC8                  | PASS        | No  | NA     | NA           |
| GmioApiTests_Wait_Async_Success_TC9                 | PASS        | No  | NA     | NA           |
| GmioApiTests_Wait_FalseSync_TC10                    | PASS        | No  | NA     | NA           |
| GmioApiTests_Wait_TrueSync_TC11                     | PASS        | No  | NA     | NA           |
| GmioApiTests_Wait_FalseSync_WithMemSync_TC12        | PASS        | No  | NA     | NA           |
| GmioApiTests_EnqueueBD_Success_TC13                 | PASS        | No  | NA     | NA           |
| GmioApiTests_EnqueueBD_Failure_DescInit_TC01        | PASS        | No  | NA     | NA           |
| GmioApiTests_EnqueueBD_Failure_SetAddrLen_TC02      | PASS        | No  | NA     | NA           |
| GmioApiTests_EnqueueBD_AIE_DevGen_TC03              | PASS        | No  | NA     | NA           |
| GmioApiTests_Destructor_Coverage_TC04               | PASS        | No  | NA     | NA           |
| GmioApiTests_EnqueueBD_WaitForAvailableBD_TC05      | PASS        | No  | NA     | NA           |
| Configure_MismatchedIterMemAddrs                                     | PASS        | No  | NA     | NA           |
| Configure_MismatchedTriggered                                        | PASS        | No  | NA     | NA           |
| Configure_MismatchedCoreRows                                         | PASS        | No  | NA     | NA           |
| Configure_MismatchedIterMemColumns                                   | PASS        | No  | NA     | NA           |
| Configure_MismatchedIterMemRows                                      | PASS        | No  | NA     | NA           |
| RunWaitResumeWithEmptyCores                                          | PASS        | No  | NA     | NA           |
| WaitWithTriggeredTrue                                                | PASS        | No  | NA     | NA           |
| WaitLoopRetryOnTimeout                                               | PASS        | No  | NA     | NA           |
| WaitTimeoutAlreadyElapsed                                            | PASS        | No  | NA     | NA           |
| ResumeWhenCoreAlreadyDone                                            | PASS        | No  | NA     | NA           |
| EndLoopRetryOnTimeout                                                | PASS        | No  | NA     | NA           |
| EndTriggeredCoreSkipsEndSequence                                     | PASS        | No  | NA     | NA           |
| CheckRTPConfig_ConnectedAsyncNotRunning                              | PASS        | No  | NA     | NA           |
| ReadWithDevGenAIE                                                    | PASS        | No  | NA     | NA           |
| GmioConfigureChannelGT1                                              | PASS        | No  | NA     | NA           |
| GmioWaitSyncLoopRetry                                                | PASS        | No  | NA     | NA           |
| GmioWaitSyncAIE2GM                                                   | PASS        | No  | NA     | NA           |
| GmioWaitPl2gmReturnsUserError                                        | PASS        | No  | NA     | NA           |
| WaitDMAChannelDoneLoopRetry                                          | PASS        | No  | NA     | NA           |
| ConfigureBDWithWrap                                                  | PASS        | No  | NA     | NA           |
| ConfigureBDWithIterationWrap                                         | PASS        | No  | NA     | NA           |
| ConfigureBDWithIterationCurrent                                      | PASS        | No  | NA     | NA           |
| ConfigureBdWaitQueue_ConfigureBDFails                                | PASS        | No  | NA     | NA           |
| GmioEnqueueBDWaitLoop_GM2AIE                                         | PASS        | No  | NA     | NA           |
| GmioWaitAsyncReturnsOK                                               | PASS        | No  | NA     | NA           |
| GmioWaitAsyncAIE2GM                                                  | PASS        | No  | NA     | NA           |
| UpdateRTP_PingBranch_NoLock                                          | PASS        | No  | NA     | NA           |
| UpdateAsyncSuspendedHasLock                                          | PASS        | No  | NA     | NA           |
| ReadAsyncNotRunningHasLockFalse                                      | PASS        | No  | NA     | NA           |
| ReadSyncHasLockFalse                                                 | PASS        | No  | NA     | NA           |
| ReadAsyncIsRunning_AfterRun                                          | PASS        | No  | NA     | NA           |
| RunBroadcastMultiTimerIter                                           | PASS        | No  | NA     | NA           |
/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)                  | %      | Result     | CR/Status    | Impact          |
|------------------------------------------------|--------|------------|--------------|-----------------|
| graph_api::configure                           | <100>  | <Pass>     | <NA>         | No Impact       |
| graph_api::run()                               | <100>  | <Pass>     | <NA>         | No Impact       |
| graph_api::run(int)                            | <100>  | <Pass>     | <NA>         | No Impact       |
| graph_api::wait()                              | <100>  | <Pass>     | <NA>         | No Impact       |
| graph_api::wait(unsigned long)                 | <100>  | <Pass>     | <NA>         | No Impact       |
| graph_api::resume                              | <100>  | <Pass>     | <NA>         | No Impact       |
| graph_api::end()                               | <100>  | <Pass>     | <NA>         | No Impact       |
| graph_api::end(unsigned long)                  | <100>  | <Pass>     | <NA>         | No Impact       |
| graph_api::update(const rtp_config*, ...)      | <100>  | <Pass>     | <NA>         | No Impact       |
| graph_api::update(const shared_buffer_config*, .)| <100>  | <Pass>     | <NA>         | No Impact       |
| graph_api::read                                | <100>  | <Pass>     | <NA>         | No Impact       |
| config_manager::initialize                     | <100>  | <Pass>     | <NA>         | No Impact       |
| gmio_api::configure                            | <100>  | <Pass>     | <NA>         | No Impact       |
| gmio_api::enqueueBD                            | <100>  | <Pass>     | <NA>         | No Impact       |
| gmio_api::wait                                 | <100>  | <Pass>     | <NA>         | No Impact       |
| checkRTPConfigForUpdate                        | <100>  | <Pass>     | <NA>         | No Impact       |
| checkRTPConfigForRead                          | <100>  | <Pass>     | <NA>         | No Impact       |
| checkSharedBufferConfigForUpdate               | <100>  | <Pass>     | <NA>         | No Impact       |
| dma_api::configureBdWaitQueueEnqueueTask       | <100>  | <Pass>     | <NA>         | No Impact       |
| dma_api::configureBD                           | <100>  | <Pass>     | <NA>         | No Impact       |
| dma_api::enqueueTask                           | <100>  | <Pass>     | <NA>         | No Impact       |
| dma_api::waitDMAChannelTaskQueue               | <100>  | <Pass>     | <NA>         | No Impact       |
| dma_api::waitDMAChannelDone                    | <92.3> | <Pass>     | <NA>         | No Impact       |
| dma_api::updateBDAddress                       | <100>  | <Pass>     | <NA>         | No Impact       |
| lock_api::initializeLock                       | <100>  | <Pass>     | <NA>         | No Impact       |
| lock_api::acquireLock                          | <100>  | <Pass>     | <NA>         | No Impact       |
| lock_api::releaseLock                          | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::frontAndPop                               | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::gmio_api::gmio_api                        | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::graph_api::graph_api                      | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::getNormalizedDevGen                       | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::relativeToAbsoluteRow                     | <100>  | <Pass>     | <NA>         | No Impact       |
| abr::convertLogicalToPhysicalDMAChNum          | <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)                  | 99.8   | <Pass>     | <NA>         | No Impact       |

/*@section Uncovered details*/
/*

 *Function: dma_api::waitDMAChannelDone
 *  Uncovered details:
		if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
			return errorMsg(err_code::aie_driver_error, "ERROR: abr::dma_api::waitDMAChannelDone: AIE driver error.");
		}
 *	Justification: driverStatus is initialised to XAIE_OK and never reassigned inside this
 *	function; the check is dead-defensive and has no impact on source code behavior.
 *
 *Function: graph_api::update
 *  Uncovered details:
 *
		bool bAcquireLock = !(pRTPConfig->isAsync && !isRunning);
 *	Justification: the (isAsync==true && isRunning==false) combo guards against a suspended-graph
 *	deadlock; UT stubs never produce that state, so the branch is unreachable and
 *	has no impact on source code behavior.
 */
