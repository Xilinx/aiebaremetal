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
* @file test_aeg_profiling_api.cpp
*  This file contains test cases of AEG Profiling API functions
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	  Who	   Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera      05/03/2026  Initial creation
* 1.1	 Veera      20/04/2026  Added test cases for AEG Profiling API functions
* 1.2	 Veera      10/05/2026  updated comment section for AEG Profiling API functions
* 1.3	 Veera      26/05/2026  updated timing analysis for AEG Profiling API functions
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
#include "aeg_profiling_api.h"
#include "profiling_stubs/fal_stubs.h"
#include "profiling_stubs/xaiengine/xaiegbl.h"
#include <algorithm>
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "CppUTestExt/MockSupport_c.h"
#include <chrono>
#include <iostream>
using namespace abr;
using namespace stub_controls;


static shim_config make_nominal_cfg(int shimCol = 1, int streamPort = 2, u8 slaveOrMaster = 0) {
	shim_config cfg{};
	cfg.shimColumn = shimCol;
	cfg.streamPortId = streamPort;
	cfg.slaveOrMaster = slaveOrMaster;
	return cfg;
}

TEST_GROUP(ShimConfigTests){
	void setup(){}
	void teardown(){mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_DefaultConstructor
*		Function Name: shim_config config
*		Input: None
*		Expectation: config.shimColumn == -1, config.slaveOrMaster == 0, config.streamPortId == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ShimConfigTests, TC_001_DefaultConstructor) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	shim_config config;
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by shim_config default constructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-1, config.shimColumn);
	CHECK_EQUAL(0, config.slaveOrMaster);
	CHECK_EQUAL(-1, config.streamPortId);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_GmioConstructorAie2gm
*		Function Name: shim_config config
*		Input: gmioConfig.shimColumn = 5, gmioConfig.type = gmio_config::aie2gm, gmioConfig.streamId = 3
*		Expectation: config.shimColumn == 5, config.slaveOrMaster == 1, config.streamPortId == 3
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ShimConfigTests, TC_002_GmioConstructorAie2gm) {
	gmio_config gmioConfig;
	gmioConfig.shimColumn = 5;
	gmioConfig.type = gmio_config::aie2gm;
	gmioConfig.streamId = 3;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	shim_config config(&gmioConfig);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by shim_config constructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(5, config.shimColumn);
	CHECK_EQUAL(1, config.slaveOrMaster);
	CHECK_EQUAL(3, config.streamPortId);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_GmioConstructorGm2aie
*		Function Name: shim_config config
*		Input: gmioConfig.shimColumn = 5, gmioConfig.type = gmio_config::gm2aie, gmioConfig.streamId = 3
*		Expectation: config.shimColumn == 5, config.slaveOrMaster == 0, config.streamPortId == 3
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ShimConfigTests, TC_003_GmioConstructorGm2aie) {
	gmio_config gmioConfig;
	gmioConfig.shimColumn = 5;
	gmioConfig.type = gmio_config::gm2aie;
	gmioConfig.streamId = 3;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	shim_config config(&gmioConfig);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by shim_config constructor: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(5, config.shimColumn);
	CHECK_EQUAL(0, config.slaveOrMaster);
	CHECK_EQUAL(3, config.streamPortId);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_PlioConstructor
*		Function Name: shim_config config
*		Input: plioConfig.shimColumn = 5, plioConfig.slaveOrMaster = 1, plioConfig.streamId = 3
*		Expectation: config.shimColumn == 5, config.slaveOrMaster == 1, config.streamPortId == 3
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ShimConfigTests, TC_004_PlioConstructor) {
	plio_config plioConfig;
	plioConfig.shimColumn = 5;
	plioConfig.slaveOrMaster = 1;
	plioConfig.streamId = 3;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	shim_config config(&plioConfig);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by shim_config constructor: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(5, config.shimColumn);
	CHECK_EQUAL(1, config.slaveOrMaster);
	CHECK_EQUAL(3, config.streamPortId);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_NullPointers
*		Function Name: shim_config configGmio, configPlio
*		Input: nullptr
*		Expectation: config.shimColumn == -1, config.slaveOrMaster == 0, config.streamPortId == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ShimConfigTests, TC_005_NullPointers) {
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	shim_config configGmio((gmio_config *)nullptr);
	shim_config configPlio((plio_config *)nullptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by shim_config constructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-1, configGmio.shimColumn);
	CHECK_EQUAL(0, configGmio.slaveOrMaster);
	CHECK_EQUAL(-1, configGmio.streamPortId);
	CHECK_EQUAL(-1, configPlio.shimColumn);
	CHECK_EQUAL(0, configPlio.slaveOrMaster);
	CHECK_EQUAL(-1, configPlio.streamPortId);
}

TEST_GROUP(ProfileStreamRunningToIdleCyclesTests){
	void setup(){ResetAll();
		static fal_util::DevWrapper devWrapper;
		fal_util::s_pXAieDev = &devWrapper;
		abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
	}

	void teardown() {
		fal_util::s_pXAieDev = nullptr;
		abr::config_manager::s_pDevInst = nullptr;
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_InvalidTileLocation
*		Function Name: profile_stream_running_to_idle_cycles
*		Input: config.shimColumn = -1, config.slaveOrMaster = 0, config.streamPortId = 3 , acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamRunningToIdleCyclesTests, TC_001_InvalidTileLocation) {
	shim_config config;
	config.shimColumn = -1;
	config.slaveOrMaster = 0;
	config.streamPortId = 3;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code result = profiling::profile_stream_running_to_idle_cycles(config, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_to_idle_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(err_code::internal_error), static_cast<int>(result));
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_NominalSuccess
*		Function Name: profile_stream_running_to_idle_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: rc == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamRunningToIdleCyclesTests, TC_002_NominalSuccess) {
	shim_config cfg{};
	cfg.shimColumn = 1;
	cfg.streamPortId = 2;
	cfg.slaveOrMaster = 0;

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_to_idle_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::ok);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_StreamSwitchRequestFails
*		Function Name: profile_stream_running_to_idle_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: rc == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamRunningToIdleCyclesTests, TC_003_StreamSwitchRequestFails) {
	shim_config cfg{};
	cfg.shimColumn = 1;
	cfg.streamPortId = 2;
	cfg.slaveOrMaster = 0;

	FalUtil_SetRequestResults({-1});

	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_to_idle_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::resource_unavailable);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_PerfCounterRequestFails
*		Function Name: profile_stream_running_to_idle_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: rc == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamRunningToIdleCyclesTests, TC_004_PerfCounterRequestFails) {
	shim_config cfg{};
	cfg.shimColumn = 1;
	cfg.streamPortId = 2;
	cfg.slaveOrMaster = 1;

	FalUtil_SetRequestResults({0, -1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_to_idle_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::resource_unavailable);
	acquiredResources.clear();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_005_EventSelectFails
*		Function Name: profile_stream_running_to_idle_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: rc == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamRunningToIdleCyclesTests, TC_005_EventSelectFails) {
	shim_config cfg{};
	cfg.shimColumn = 1;
	cfg.streamPortId = 2;
	cfg.slaveOrMaster = 0;

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(1);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_to_idle_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::aie_driver_error);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_PerfCounterControlFails
*		Function Name: profile_stream_running_to_idle_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamRunningToIdleCyclesTests, TC_006_PerfCounterControlFails) {
	shim_config cfg{};
	cfg.shimColumn = 1;
	cfg.streamPortId = 2;
	cfg.slaveOrMaster = 1;

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(2);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_to_idle_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::aie_driver_error);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_NominalSuccessMaster
*		Function Name: profile_stream_running_to_idle_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 1, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamRunningToIdleCyclesTests, TC_007_NominalSuccessMaster) {
	double total_clock_enable_us = 0.0;

	shim_config cfg = make_nominal_cfg(1, 2, 1);

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_stream_running_to_idle_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::ok);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_NegativeShimColumnIsolated
*		Function Name: profile_stream_running_to_idle_cycles
*		Input: cfg.shimColumn = -2, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamRunningToIdleCyclesTests, TC_008_NegativeShimColumnIsolated) {
	double total_clock_enable_us = 0.0;

	shim_config cfg{}; cfg.shimColumn = -2; cfg.streamPortId = 2; cfg.slaveOrMaster = 0;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_stream_running_to_idle_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_NegativeStreamPortIdIsolated
*		Function Name: profile_stream_running_to_idle_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = -1, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamRunningToIdleCyclesTests, TC_009_NegativeStreamPortIdIsolated) {
	double total_clock_enable_us = 0.0;

	shim_config cfg{}; cfg.shimColumn = 1; cfg.streamPortId = -1; cfg.slaveOrMaster = 0;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_stream_running_to_idle_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
}

TEST_GROUP(ProfilingStreamRunningEventCount){

	void setup(){ResetAll();

		static fal_util::DevWrapper devWrapper;
		fal_util::s_pXAieDev = &devWrapper;
		abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
	}
	void teardown() {
		fal_util::s_pXAieDev = nullptr;
		abr::config_manager::s_pDevInst = nullptr;
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: EventCount_TC_001_NominalSuccess
*		Function Name: profile_stream_running_event_count
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: rc == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStreamRunningEventCount, EventCount_TC_001_NominalSuccess) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_event_count(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_event_count function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: EventCount_TC_002_InvalidStreamPortId
*		Function Name: profile_stream_running_event_count
*		Input: cfg.shimColumn = 1, cfg.streamPortId = -1, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: rc == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStreamRunningEventCount, EventCount_TC_002_InvalidStreamPortId) {
	abr::shim_config shimConfig;
	shimConfig.shimColumn = 1;
	shimConfig.streamPortId = -1;
	shimConfig.slaveOrMaster = 0;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result = abr::profiling::profile_stream_running_event_count(shimConfig, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_event_count function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(static_cast<int>(abr::err_code::internal_error), static_cast<int>(result));
}

//*********************************************************************************/
/*
*		Unique test case ID: EventCount_TC_003_StreamSwitchRequestFails
*		Function Name: profile_stream_running_event_count
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: rc == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStreamRunningEventCount, EventCount_TC_003_StreamSwitchRequestFails) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);

	FalUtil_SetRequestResults({-1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_event_count(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_event_count function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::resource_unavailable);
}

//*********************************************************************************/
/*
*		Unique test case ID: EventCount_TC_004_PerfCounterRequestFails
*		Function Name: profile_stream_running_event_count
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: rc == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStreamRunningEventCount, EventCount_TC_004_PerfCounterRequestFails) {
	shim_config cfg = make_nominal_cfg(1, 2, 1);

	FalUtil_SetRequestResults({0, -1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_event_count(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_event_count function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::resource_unavailable);
}
//*********************************************************************************/
/*
*		Unique test case ID: EventCount_TC_005_EventSelectFails
*		Function Name: profile_stream_running_event_count
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: rc == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStreamRunningEventCount, EventCount_TC_005_EventSelectFails) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(1);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_event_count(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_event_count function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: EventCount_TC_006_PerfCounterControlFails
*		Function Name: profile_stream_running_event_count
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStreamRunningEventCount, EventCount_TC_006_PerfCounterControlFails) {
	shim_config cfg = make_nominal_cfg(1, 2, 1);

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(3);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_event_count(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_running_event_count function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::aie_driver_error);
}

//*********************************************************************************/
/*
*		Unique test case ID: EventCount_TC_007_NominalSuccessMaster
*		Function Name: profile_stream_running_event_count
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 1, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStreamRunningEventCount, EventCount_TC_007_NominalSuccessMaster) {
	double total_clock_enable_us = 0.0;

	shim_config cfg = make_nominal_cfg(1, 2, 1);

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_running_event_count(cfg, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_stream_running_event_count function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::ok);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: EventCount_TC_008_NegativeShimColumnIsolated
*		Function Name: profile_stream_running_event_count
*		Input: cfg.shimColumn = -2, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStreamRunningEventCount, EventCount_TC_008_NegativeShimColumnIsolated) {
	double total_clock_enable_us = 0.0;

	shim_config cfg{}; cfg.shimColumn = -2; cfg.streamPortId = 2; cfg.slaveOrMaster = 0;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_stream_running_event_count(cfg, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_stream_running_event_count function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: EventCount_TC_009_ShimColumnMinusOneTriggersColInvalid
*		Function Name: profile_stream_running_event_count
*		Input: cfg.shimColumn = -1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStreamRunningEventCount, EventCount_TC_009_ShimColumnMinusOneTriggersColInvalid) {
	double total_clock_enable_us = 0.0;

	shim_config cfg{}; cfg.shimColumn = -1; cfg.streamPortId = 2; cfg.slaveOrMaster = 0;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_stream_running_event_count(cfg, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_stream_running_event_count function: " << total_clock_enable_us << " microseconds" << std::endl;
}

TEST_GROUP(ProfilingStartTimeDifferenceBtw2Streams){
	void setup(){ResetAll();
		static fal_util::DevWrapper devWrapper;
		fal_util::s_pXAieDev = &devWrapper;
		abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
	}
	void teardown() {
		fal_util::s_pXAieDev = nullptr;
		abr::config_manager::s_pDevInst = nullptr;
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_InvalidShimColumn1
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: shimConfig1.shimColumn = -1, shimConfig1.streamPortId = 0, shimConfig1.slaveOrMaster = 0, shimConfig2.shimColumn = 5, shimConfig2.streamPortId = 1, shimConfig2.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_001_InvalidShimColumn1) {
	abr::shim_config shimConfig1;
	shimConfig1.shimColumn = -1;
	shimConfig1.streamPortId = 0;
	shimConfig1.slaveOrMaster = 0;

	abr::shim_config shimConfig2;
	shimConfig2.shimColumn = 5;
	shimConfig2.streamPortId = 1;
	shimConfig2.slaveOrMaster = 1;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code result =	abr::profiling::profile_start_time_difference_btw_two_streams(shimConfig1, shimConfig2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL((int)abr::err_code::internal_error, (int)result);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_SameTileNominalSuccess
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 1, cfg2.streamPortId = 3, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_002_SameTileNominalSuccess) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, 3, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::ok);
	CHECK_EQUAL(4U, acquiredResources.size());
	for (size_t i = 0; i < acquiredResources.size(); ++i)
		CHECK_TRUE(acquiredResources[i] != nullptr);

	CHECK_EQUAL(4, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(2, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(2, XAie_GetPerfCounterControlCallCount());
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_DifferentTileNominalSuccess
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_003_DifferentTileNominalSuccess) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3, 4});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::ok);
	CHECK_EQUAL(5U, acquiredResources.size());
	for (size_t i = 0; i < acquiredResources.size(); ++i)
		CHECK_TRUE(acquiredResources[i] != nullptr);

	CHECK_EQUAL(5, FalUtil_GetRequestCallCount());
	CHECK_TRUE(XAie_GetEventSelectCallCount() >= 2);
	CHECK_TRUE(XAie_GetPerfCounterControlCallCount() >= 2);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_InvalidSecondConfig
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: shimConfig1.shimColumn = -1, shimConfig1.streamPortId = 0, shimConfig1.slaveOrMaster = 0, shimConfig2.shimColumn = 1, shimConfig2.streamPortId = -1, shimConfig2.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_004_InvalidSecondConfig) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, -1, 1);

	FalUtil_SetRequestResults({});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::internal_error);
	CHECK_EQUAL(0, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(0U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_StreamSwitch1RequestFails
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 1, cfg2.streamPortId = 3, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_005_StreamSwitch1RequestFails) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, 3, 1);

	FalUtil_SetRequestResults({-1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::resource_unavailable);
	CHECK_EQUAL(1, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(0U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_PerfCounter1RequestFails
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 1, cfg2.streamPortId = 3, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_006_PerfCounter1RequestFails) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, 3, 1);

	FalUtil_SetRequestResults({0, -1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::resource_unavailable);
	CHECK_EQUAL(2, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(0U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_StreamSwitch2RequestFails
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 1, cfg2.streamPortId = 3, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_007_StreamSwitch2RequestFails) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, 3, 1);

	FalUtil_SetRequestResults({0, 1, -1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::resource_unavailable);
	CHECK_EQUAL(3, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(0U, acquiredResources.size());
}

	//*********************************************************************************/
	/*
	*		Unique test case ID: TC_008_PerfCounter2RequestFails
*		Function Name: profile_start_time_difference_btw_two_streams
	*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 1, cfg2.streamPortId = 3, cfg2.slaveOrMaster = 1, acquiredResources
	*		Expectation: rc == resource_unavailable
	*		Test Method: Unit testing
	*     	Methods for deriving test case: Analysis of requirement
	*     	Test Environment: 2
	*/
	/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_008_PerfCounter2RequestFails) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, 3, 1);

	FalUtil_SetRequestResults({0, 1, 2, -1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::resource_unavailable);
	CHECK_EQUAL(4, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(0U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_BroadcastRequestFails
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, broadcast column = 0, acquiredResources
*		Expectation: rc == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_009_BroadcastRequestFails) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3, -1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::resource_unavailable);
	CHECK_EQUAL(5, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
}


//*********************************************************************************/
/*
*		Unique test case ID: TC_010_BroadcastRequestReleaseFails
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, broadcast column = 0, acquiredResources
*		Expectation: rc == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_010_BroadcastRequestReleaseFails) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3, -1});
	FalUtil_SetReleaseResult(false);
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::resource_unavailable);
	CHECK_EQUAL(5, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
}


//*********************************************************************************/
/*
*		Unique test case ID: TC_011_EventSelectFailureSameTile
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 1, cfg2.streamPortId = 3, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_011_EventSelectFailureSameTile) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, 3, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3});
	XAie_SetEventSelectResult(1);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::aie_driver_error);
	CHECK_EQUAL(4, FalUtil_GetRequestCallCount());
	CHECK_TRUE(XAie_GetEventSelectCallCount() >= 1);
	CHECK_TRUE(XAie_GetPerfCounterControlCallCount() >= 2);
	CHECK_EQUAL(4U, acquiredResources.size());
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_012_PerfCounterControlFailureDifferentTiles
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 1, cfg2.streamPortId = 3, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: rc == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_012_PerfCounterControlFailureDifferentTiles) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3, 4});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(1);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::aie_driver_error);
	CHECK_EQUAL(5, FalUtil_GetRequestCallCount());
	CHECK_TRUE(XAie_GetEventSelectCallCount() >= 2);
	CHECK_TRUE(XAie_GetPerfCounterControlCallCount() >= 2);
	CHECK_EQUAL(5U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_013_SameTileBothMaster
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 1, cfg2.shimColumn = 1, cfg2.streamPortId = 3, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_013_SameTileBothMaster) {
	double total_clock_enable_us = 0.0;

	shim_config cfg1 = make_nominal_cfg(1, 2, 1);
	shim_config cfg2 = make_nominal_cfg(1, 3, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::ok);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_014_DifferentTileBothMasterReversedColumns
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 3, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 1, cfg2.shimColumn = 1, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_014_DifferentTileBothMasterReversedColumns) {
	double total_clock_enable_us = 0.0;

	shim_config cfg1 = make_nominal_cfg(3, 2, 1);
	shim_config cfg2 = make_nominal_cfg(1, 4, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3, 4});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::ok);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_015_Stream1NegativeShimColumnIsolated
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = -2, cfg1.streamPortId = 0, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_015_Stream1NegativeShimColumnIsolated) {
	double total_clock_enable_us = 0.0;

	shim_config cfg1{}; cfg1.shimColumn = -2; cfg1.streamPortId = 0; cfg1.slaveOrMaster = 0;
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_016_Stream1NegativeStreamPortIdIsolated
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = -1, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_016_Stream1NegativeStreamPortIdIsolated) {
	double total_clock_enable_us = 0.0;

	shim_config cfg1{}; cfg1.shimColumn = 1; cfg1.streamPortId = -1; cfg1.slaveOrMaster = 0;
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_017_Stream2InvalidCol
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = -1, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_017_Stream2InvalidCol) {
	double total_clock_enable_us = 0.0;

	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2{}; cfg2.shimColumn = -1; cfg2.streamPortId = 4; cfg2.slaveOrMaster = 1;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_018_Stream2NegativeShimColumnIsolated
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = -2, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_018_Stream2NegativeShimColumnIsolated) {
	double total_clock_enable_us = 0.0;
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2{}; cfg2.shimColumn = -2; cfg2.streamPortId = 4; cfg2.slaveOrMaster = 1;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_019_Stream2NegativeStreamPortIdIsolated
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = -1, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_019_Stream2NegativeStreamPortIdIsolated) {
	double total_clock_enable_us = 0.0;
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2{}; cfg2.shimColumn = 3; cfg2.streamPortId = -1; cfg2.slaveOrMaster = 1;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_020_BroadcastFailThirdReleaseFails
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_020_BroadcastFailThirdReleaseFails) {
	double total_clock_enable_us = 0.0;

	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);
	FalUtil_SetRequestResults({0, 1, 2, 3, -1});
	FalUtil_SetReleaseResults({true, true, false, true});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	(void)profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_021_BroadcastFailFourthReleaseFails
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_021_BroadcastFailFourthReleaseFails) {
	double total_clock_enable_us = 0.0;
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);
	FalUtil_SetRequestResults({0, 1, 2, 3, -1});
	FalUtil_SetReleaseResults({true, true, true, false});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	(void)profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_022_BroadcastRequestFailsAllCleanupReleases
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_022_BroadcastRequestFailsAllCleanupReleases) {
	double total_clock_enable_us = 0.0;
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);
	FalUtil_SetRequestResults({0, 1, 2, 3, -1});
	FalUtil_SetReleaseResults({true, true, true, true});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::resource_unavailable);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_023_BroadcastFailSecondReleaseFails
*		Function Name: profile_start_time_difference_btw_two_streams
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_023_BroadcastFailSecondReleaseFails) {
	double total_clock_enable_us = 0.0;
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);
	FalUtil_SetRequestResults({0, 1, 2, 3, -1});
	FalUtil_SetReleaseResults({true, false, true, true});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_start_time_difference_btw_two_streams function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::resource_unavailable);
}

TEST_GROUP(ProfileStreamStartToTransferComplete){void setup(){ResetAll();

	static fal_util::DevWrapper devWrapper;
	fal_util::s_pXAieDev = &devWrapper;

	abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
}

void teardown() {
	fal_util::s_pXAieDev = nullptr;
	abr::config_manager::s_pDevInst = nullptr;
	mock_c()->clear();
}
};

//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_001_InvalidShimColumn
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = -1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources, numBytes
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_001_InvalidShimColumn) {
	abr::shim_config shimConfig;
	shimConfig.shimColumn = -1;
	shimConfig.streamPortId = 2;
	shimConfig.slaveOrMaster = 0;

	uint32_t numBytes = 1024;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code result =
		abr::profiling::profile_stream_start_to_transfer_complete_cycles(shimConfig, numBytes, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(abr::err_code::internal_error), static_cast<int>(result));
}

//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_002_InvalidStreamPortId
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = -1, cfg.slaveOrMaster = 0, acquiredResources, numBytes
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_002_InvalidStreamPortId) {
	shim_config cfg = make_nominal_cfg(1, -1, 0);
	uint32_t numBytes = 256;

	FalUtil_SetRequestResults({});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(cfg, numBytes, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::internal_error);
	CHECK_EQUAL(0, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(0U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_003_StreamSwitchRequestFails
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources, numBytes
*		Expectation: result == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_003_StreamSwitchRequestFails) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);
	uint32_t numBytes = 512;

	FalUtil_SetRequestResults({-1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(cfg, numBytes, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::resource_unavailable);
	CHECK_EQUAL(1, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(0U, acquiredResources.size());
}


//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_004_FirstPerfCounterRequestFails
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 1, acquiredResources, numBytes
*		Expectation: result == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_004_FirstPerfCounterRequestFails) {
	shim_config cfg = make_nominal_cfg(1, 2, 1);
	uint32_t numBytes = 512;

	FalUtil_SetRequestResults({0, -1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(cfg, numBytes, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::resource_unavailable);
	CHECK_EQUAL(2, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(0U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_005_SecondPerfCounterRequestFails
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 1, acquiredResources, numBytes
*		Expectation: result == resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_005_SecondPerfCounterRequestFails) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);
	uint32_t numBytes = 512;

	FalUtil_SetRequestResults({0, 1, -1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(cfg, numBytes, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::resource_unavailable);
	CHECK_EQUAL(3, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(0U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_006_EventSelectFails
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources, numBytes
*		Expectation: result == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_006_EventSelectFails) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);
	uint32_t numBytes = 1024;

	FalUtil_SetRequestResults({0, 1, 2});
	XAie_SetEventSelectResult(1);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(cfg, numBytes, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::aie_driver_error);
	CHECK_EQUAL(3, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(1, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(2, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(3U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_007_PerfCounterControl0Fails
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 1, acquiredResources, numBytes
*		Expectation: result == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_007_PerfCounterControl0Fails) {
	shim_config cfg = make_nominal_cfg(1, 2, 1);
	uint32_t numBytes = 1024;

	FalUtil_SetRequestResults({0, 1, 2});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(1);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(cfg, numBytes, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_TRUE(rc == err_code::aie_driver_error);
	CHECK_EQUAL(3, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(1, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(2, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(3U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_008_ZeroEventValueBoundary
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, numBytes = 0, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_008_ZeroEventValueBoundary) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);
	uint32_t numBytes = 0;

	FalUtil_SetRequestResults({0, 1, 2});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(cfg, numBytes, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::ok);
	CHECK_EQUAL(3, FalUtil_GetRequestCallCount());
	CHECK_EQUAL(1, XAie_GetEventSelectCallCount());
	CHECK_EQUAL(2, XAie_GetPerfCounterControlCallCount());
	CHECK_EQUAL(3U, acquiredResources.size());
}

//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_009_NominalSuccessMaster
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 1, numBytes = 64, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_009_NominalSuccessMaster) {
	double total_clock_enable_us = 0.0;

	shim_config cfg = make_nominal_cfg(1, 2, 1);

	FalUtil_SetRequestResults({0, 1, 2});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(cfg, 64U, acquiredResources);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rc == err_code::ok);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_010_NegativeShimColumnIsolated
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = -2, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, numBytes = 64, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_010_NegativeShimColumnIsolated) {
	double total_clock_enable_us = 0.0;

	shim_config cfg{}; cfg.shimColumn = -2; cfg.streamPortId = 2; cfg.slaveOrMaster = 0;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_stream_start_to_transfer_complete_cycles(cfg, 64U, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: StartTransfer_TC_011_NegativeStreamPortIdIsolated
*		Function Name: profile_stream_start_to_transfer_complete_cycles
*		Input: cfg.shimColumn = 1, cfg.streamPortId = -1, cfg.slaveOrMaster = 0, numBytes = 64, acquiredResources
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfileStreamStartToTransferComplete, StartTransfer_TC_011_NegativeStreamPortIdIsolated) {
	double total_clock_enable_us = 0.0;

	shim_config cfg{}; cfg.shimColumn = 1; cfg.streamPortId = -1; cfg.slaveOrMaster = 0;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL((int)err_code::internal_error,
	            (int)profiling::profile_stream_start_to_transfer_complete_cycles(cfg, 64U, acquiredResources));

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
}

TEST_GROUP(ProfilingStopTests){void setup(){ResetAll();

	static fal_util::DevWrapper devWrapper;
	fal_util::s_pXAieDev = &devWrapper;

	abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
}

void teardown() {
	fal_util::s_pXAieDev = nullptr;
	abr::config_manager::s_pDevInst = nullptr;
	FalUtil_SetReleaseResult(true);
	mock().clear();
	mock_c()->clear();
}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_ValidExecution
*		Function Name: stop
*		Input: acquiredResources = empty, startTimeDifference = false
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_001_ValidExecution) {

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	bool startTimeDifference = false;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code result = profiling::stop(acquiredResources, startTimeDifference);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(static_cast<int>(err_code::ok), static_cast<int>(result));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_StopAfterRunningToIdle
*		Function Name: stop
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_002_StopAfterRunningToIdle) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rcStart = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	CHECK_TRUE(rcStart == err_code::ok);
	CHECK_EQUAL(2U, acquiredResources.size());
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_StopWithStreamPortSelectFirst
*		Function Name: stop
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_003_StopWithStreamPortSelectFirst) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rcStart = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	CHECK_TRUE(rcStart == err_code::ok);

	std::reverse(acquiredResources.begin(), acquiredResources.end());
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_StopAfterEventCount
*		Function Name: stop
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_004_StopAfterEventCount) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rcStart = profiling::profile_stream_running_event_count(cfg, acquiredResources);
	CHECK_TRUE(rcStart == err_code::ok);
	CHECK_EQUAL(2U, acquiredResources.size());
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_StopWithStreamPortSelectOnly
*		Function Name: stop
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_005_StopWithStreamPortSelectOnly) {
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;


	auto pStreamPort = std::make_shared<xaiefal::XAieStreamPortSelect>();
	StubXaieSingleTile_SetGetRscIdResult(pStreamPort, XAIE_OK, 5);
	acquiredResources.push_back(pStreamPort);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_StopWithPCEventOnly
*		Function Name: stop
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
//*********************************************************************************/
TEST(ProfilingStopTests, TC_006_StopWithPCEventOnly) {
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;


	auto pPCEvent = std::make_shared<xaiefal::XAiePCEvent>();
	StubXaieSingleTile_SetGetRscIdResult(pPCEvent, XAIE_OK, 10);
	acquiredResources.push_back(pPCEvent);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_StopAfterStartToTransferComplete
*		Function Name: stop
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources, numBytes
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_007_StopAfterStartToTransferComplete) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);
	uint32_t numBytes = 1024;

	FalUtil_SetRequestResults({0, 1, 2});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rcStart = profiling::profile_stream_start_to_transfer_complete_cycles(cfg, numBytes, acquiredResources);
	CHECK_TRUE(rcStart == err_code::ok);
	CHECK_EQUAL(3U, acquiredResources.size());

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_StopAfterRunningEventCount
*		Function Name: stop
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_008_StopAfterRunningEventCount) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);

	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rcStart = profiling::profile_stream_running_event_count(cfg, acquiredResources);
	CHECK_TRUE(rcStart == err_code::ok);
	CHECK_EQUAL(2U, acquiredResources.size());

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_StopAfterStartTimeDiffSameTile
*		Function Name: stop
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 1, cfg2.streamPortId = 3, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_009_StopAfterStartTimeDiffSameTile) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, 3, 1); // same column => no broadcast resource

	FalUtil_SetRequestResults({0, 1, 2, 3});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rcStart = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	CHECK_TRUE(rcStart == err_code::ok);
	CHECK_EQUAL(4U, acquiredResources.size());

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_010_StopAfterStartTimeDiffDifferentTiles
*		Function Name: stop
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_010_StopAfterStartTimeDiffDifferentTiles) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3, 4});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rcStart = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	CHECK_TRUE(rcStart == err_code::ok);
	CHECK_EQUAL(5U, acquiredResources.size());

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_011_StopWithBroadcastResource
*		Function Name: stop
*		Input: acquiredResources = broadcast resource, acquiredResources
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_011_StopWithBroadcastResource) {
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;


	auto pBroadcast = std::make_shared<xaiefal::XAieBroadcast>();
	std::vector<XAie_LocType> locs;
	locs.push_back(XAie_TileLoc(1, 0));
	locs.push_back(XAie_TileLoc(2, 0));
	locs.push_back(XAie_TileLoc(3, 0));
	pBroadcast->setChannel(locs, XAIE_PL_MOD, XAIE_PL_MOD);
	pBroadcast->setBc(2);
	acquiredResources.push_back(pBroadcast);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profile_stream_start_to_transfer_complete_cycles function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_011a_StopWithBroadcastResourceNoStartTimeDifference
*		Function Name: stop
*		Input: acquiredResources = broadcast resource, startTimeDifference = false
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_011a_StopWithBroadcastResourceNoStartTimeDifference) {
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	auto pBroadcast = std::make_shared<xaiefal::XAieBroadcast>();
	std::vector<XAie_LocType> locs;
	locs.push_back(XAie_TileLoc(1, 0));
	locs.push_back(XAie_TileLoc(3, 0));
	pBroadcast->setChannel(locs, XAIE_PL_MOD, XAIE_PL_MOD);
	pBroadcast->setBc(2);
	acquiredResources.push_back(pBroadcast);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_011b_StopBroadcastNonPlEndModule
*		Function Name: stop
*		Input: acquiredResources = broadcast resource with non-PL end module, startTimeDifference = true
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_011b_StopBroadcastNonPlEndModule) {
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	auto pBroadcast = std::make_shared<xaiefal::XAieBroadcast>();
	std::vector<XAie_LocType> locs;
	locs.push_back(XAie_TileLoc(1, 0));
	locs.push_back(XAie_TileLoc(3, 0));
	pBroadcast->setChannel(locs, XAIE_PL_MOD, static_cast<XAie_ModuleType>(1));
	pBroadcast->setBc(2);
	acquiredResources.push_back(pBroadcast);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_012_StopWithDriverError
*		Function Name: stop
*		Input: acquiredResources = PerfCounter resource, acquiredResources
*		Expectation: result == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_012_StopWithDriverError) {
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;


	auto pPerfCounter = std::make_shared<xaiefal::XAiePerfCounter>();
	Stub_XAiePerfCounter_SetGetRscIdResult(0, 0, 0, XAIE_PL_MOD, 5, XAIE_OK);
	acquiredResources.push_back(pPerfCounter);


	XAie_SetPerfCounterResetResult(XAIE_ERR);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::aie_driver_error);


	XAie_SetPerfCounterResetResult(XAIE_OK);
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_013_ReleaseFailure
*		Function Name: stop
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_013_ReleaseFailure) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);
	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rcStart = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	CHECK_TRUE(rcStart == err_code::ok);


	Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_OK);
	FalUtil_SetReleaseResult(false);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::aie_driver_error);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_014_StopWithUnknownResource
*		Function Name: stop
*		Input: acquiredResources, isBroadcast = false
*		Expectation: result == internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_014_StopWithUnknownResource) {
	double total_clock_enable_us = 0.0;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	acquiredResources.push_back(std::make_shared<xaiefal::XAieRsc>());
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	(void)profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_014a_StopWithNullResource
*		Function Name: stop
*		Input: acquiredResources = {nullptr}, startTimeDifference = false
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_014a_StopWithNullResource) {
	double total_clock_enable_us = 0.0;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	acquiredResources.push_back(std::shared_ptr<xaiefal::XAieRsc>{});
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::ok);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_015_StopBroadcastReversedTileLocs
*		Function Name: stop
*		Input: acquiredResources, isBroadcast = true
*		Expectation: result == ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_015_StopBroadcastReversedTileLocs) {
	double total_clock_enable_us = 0.0;

	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);
	FalUtil_SetRequestResults({0, 1, 2, 3, 4});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	CHECK_TRUE(profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources)
	           == err_code::ok);

	for (auto &rsc : acquiredResources) {
		if (auto pBcast = std::dynamic_pointer_cast<xaiefal::XAieBroadcast>(rsc)) {
			std::vector<XAie_LocType> reversedLocs = {
				XAie_TileLoc(5, 0),
				XAie_TileLoc(4, 0),
				XAie_TileLoc(3, 0),
			};
			pBcast->setChannel(reversedLocs, XAIE_PL_MOD, XAIE_PL_MOD);
			break;
		}
	}
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	CHECK_TRUE(rcStop == err_code::ok);
	acquiredResources.clear();


	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling stop function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_016_StopSingleTileGetRscIdFails
*		Function Name: stop
*		Input: acquiredResources, isBroadcast = false
*		Expectation: result == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_016_StopSingleTileGetRscIdFails) {
	double total_clock_enable_us = 0.0;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	auto pStreamPort = std::make_shared<xaiefal::XAieStreamPortSelect>();
	StubXaieSingleTile_SetGetRscIdResult(pStreamPort, XAIE_ERR, 5);
	acquiredResources.push_back(pStreamPort);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::aie_driver_error);
	acquiredResources.clear();

}

//*********************************************************************************/
/*
*		Unique test case ID: TC_017_StopPerfCounterResetControlFails
*		Function Name: stop
*		Input: acquiredResources, isBroadcast = false
*		Expectation: result == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_017_StopPerfCounterResetControlFails) {
	double total_clock_enable_us = 0.0;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	acquiredResources.push_back(std::make_shared<xaiefal::XAiePerfCounter>());
	Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_OK);
	XAie_SetPerfCounterResetResult(XAIE_OK);
	XAie_SetPerfCounterResetCtrlResult(XAIE_ERR);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::aie_driver_error);
	XAie_SetPerfCounterResetCtrlResult(XAIE_OK);
	acquiredResources.clear();

}

//*********************************************************************************/
/*
*		Unique test case ID: TC_018_StopStreamPortResetFails
*		Function Name: stop
*		Input: acquiredResources, isBroadcast = false
*		Expectation: result == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_018_StopStreamPortResetFails) {
	double total_clock_enable_us = 0.0;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	auto pStreamPort = std::make_shared<xaiefal::XAieStreamPortSelect>();
	StubXaieSingleTile_SetGetRscIdResult(pStreamPort, XAIE_OK, 5);
	acquiredResources.push_back(pStreamPort);
	XAie_SetEventSelectStrmPortResetResult(XAIE_ERR);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::aie_driver_error);
	XAie_SetEventSelectStrmPortResetResult(XAIE_OK);
	acquiredResources.clear();

}

//*********************************************************************************/
/*
*		Unique test case ID: TC_019_StopPCEventResetFails
*		Function Name: stop
*		Input: acquiredResources, isBroadcast = false
*		Expectation: result == aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingStopTests, TC_019_StopPCEventResetFails) {
	double total_clock_enable_us = 0.0;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	auto pPCEvent = std::make_shared<xaiefal::XAiePCEvent>();
	StubXaieSingleTile_SetGetRscIdResult(pPCEvent, XAIE_OK, 10);
	acquiredResources.push_back(pPCEvent);
	XAie_SetEventPCResetResult(XAIE_ERR);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err_code rcStop = profiling::stop(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling stop function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_TRUE(rcStop == err_code::aie_driver_error);
	XAie_SetEventPCResetResult(XAIE_OK);
	acquiredResources.clear();
}

TEST_GROUP(ProfilingReadTests){void setup(){ResetAll();
	static fal_util::DevWrapper devWrapper;
	fal_util::s_pXAieDev = &devWrapper;
	abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
}

void teardown() {
	fal_util::s_pXAieDev = nullptr;
	abr::config_manager::s_pDevInst = nullptr;
	FalUtil_SetReleaseResult(true);
	mock_c()->clear();
}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_ReadStartTimeDifferenceDifferentColumns
*		Function Name: read
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 3, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == 106
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_001_ReadStartTimeDifferenceDifferentColumns) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3, 4});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	CHECK_TRUE(rc == err_code::ok);

	Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_OK);
	Stub_XAiePerfCounter_SetGetRscIdResult(1, 3, 0, XAIE_PL_MOD, 3, XAIE_ERR);
	Stub_XAie_PerfCounterGet_SetValue(1, 0, XAIE_PL_MOD, 1, 100, XAIE_OK);
	Stub_XAie_PerfCounterGet_SetValue(3, 0, XAIE_PL_MOD, 3, 200, XAIE_ERR);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uint64_t result = profiling::read(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by read function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(106, result);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_InvalidResourceType
*		Function Name: read
*		Input: acquiredResources = {pSSwitchPortRsc}, acquiredResources
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 1
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_002_InvalidResourceType) {
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	auto pSSwitchPortRsc = fal_util::s_pXAieDev->tile(1, 0).sswitchPort();
	acquiredResources.push_back(pSSwitchPortRsc);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uint64_t result = profiling::read(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by read function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_InvalidResourcesForTimeDifference
*		Function Name: read
*		Input: acquiredResources = {pSSwitchPortRsc1, pSSwitchPortRsc2}, acquiredResources
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 1
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_003_InvalidResourcesForTimeDifference) {
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	auto pSSwitchPortRsc1 = fal_util::s_pXAieDev->tile(1, 0).sswitchPort();
	auto pSSwitchPortRsc2 = fal_util::s_pXAieDev->tile(2, 0).sswitchPort();
	acquiredResources.push_back(pSSwitchPortRsc1);
	acquiredResources.push_back(pSSwitchPortRsc2);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uint64_t result = profiling::read(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by read function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003a_InvalidSecondResourceForTimeDifference
*		Function Name: read
*		Input: acquiredResources = {pPerfCounterRsc, pSSwitchPortRsc}, startTimeDifference = true
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 1
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_003a_InvalidSecondResourceForTimeDifference) {
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	auto pPerfCounterRsc = fal_util::s_pXAieDev->tile(1, 0).pl().perfCounter();
	auto pSSwitchPortRsc = fal_util::s_pXAieDev->tile(2, 0).sswitchPort();
	acquiredResources.push_back(pPerfCounterRsc);
	acquiredResources.push_back(pSSwitchPortRsc);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uint64_t result = profiling::read(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling read function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003b_NullResourcesForTimeDifference
*		Function Name: read
*		Input: acquiredResources = {nullptr, nullptr}, startTimeDifference = true
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 1
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_003b_NullResourcesForTimeDifference) {
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	acquiredResources.push_back(std::shared_ptr<xaiefal::XAieRsc>{});
	acquiredResources.push_back(std::shared_ptr<xaiefal::XAieRsc>{});

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uint64_t result = profiling::read(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling read function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003c_NullResourceForStandardRead
*		Function Name: read
*		Input: acquiredResources = {nullptr}, startTimeDifference = false
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 1
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_003c_NullResourceForStandardRead) {
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	acquiredResources.push_back(std::shared_ptr<xaiefal::XAieRsc>{});

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uint64_t result = profiling::read(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling read function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_GetRscIdFailureInTimeDifference
*		Function Name: read
*		Input: cfg1.shimColumn = 1, cfg1.streamPortId = 2, cfg1.slaveOrMaster = 0, cfg2.shimColumn = 1, cfg2.streamPortId = 4, cfg2.slaveOrMaster = 1, acquiredResources
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_004_GetRscIdFailureInTimeDifference) {
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, 4, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	CHECK_TRUE(rc == err_code::ok);

	Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_ERR);
	Stub_XAiePerfCounter_SetGetRscIdResult(1, 1, 0, XAIE_PL_MOD, 3, XAIE_OK);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uint64_t result = profiling::read(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling::read function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_GetRscIdFailureInStandardRead
*		Function Name: read
*		Input: cfg.shimColumn = 1, cfg.streamPortId = 2, cfg.slaveOrMaster = 0, acquiredResources
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 1
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_005_GetRscIdFailureInStandardRead) {
	shim_config cfg = make_nominal_cfg(1, 2, 0);
	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	CHECK_TRUE(rc == err_code::ok);

	Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_ERR);

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uint64_t result = profiling::read(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by read function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}
//*********************************************************************************/
/*
*		Unique test case ID: TC_006_ReadPerfCounterSuccess
*		Function Name: read
*		Input: acquiredResources, isBroadcast = false
*		Expectation: result == 106
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_006_ReadPerfCounterSuccess) {
	double total_clock_enable_us = 0.0;

	shim_config cfg = make_nominal_cfg(1, 2, 0);
	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	CHECK_TRUE(profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources) == err_code::ok);
	Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_OK);
	Stub_XAie_PerfCounterGet_SetValue(1, 0, XAIE_PL_MOD, 1, 106U, XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uint64_t result = profiling::read(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling read function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(106, result);
	acquiredResources.clear();

}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_ReadPerfCounterGetRscIdFails
*		Function Name: read
*		Input: acquiredResources, isBroadcast = false
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_007_ReadPerfCounterGetRscIdFails) {
	double total_clock_enable_us = 0.0;

	shim_config cfg = make_nominal_cfg(1, 2, 0);
	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	CHECK_TRUE(profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources) == err_code::ok);

	Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_ERR);
	Stub_XAie_PerfCounterGet_SetValue(1, 0, XAIE_PL_MOD, 1, 0U, XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	(void)profiling::read(acquiredResources, false);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling read function: " << total_clock_enable_us << " microseconds" << std::endl;
	acquiredResources.clear();
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_ReadStartTimeDifferenceReversedColumns
*		Function Name: read
*		Input: acquiredResources, isBroadcast = false
*		Expectation: result == 106
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ProfilingReadTests, TC_008_ReadStartTimeDifferenceReversedColumns) {
	double total_clock_enable_us = 0.0;
	shim_config cfg1 = make_nominal_cfg(3, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, 4, 1);

	FalUtil_SetRequestResults({0, 1, 2, 3, 4});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	CHECK_TRUE(rc == err_code::ok);

	Stub_XAiePerfCounter_SetGetRscIdResult(0, 3, 0, XAIE_PL_MOD, 1, XAIE_OK);
	Stub_XAiePerfCounter_SetGetRscIdResult(1, 1, 0, XAIE_PL_MOD, 3, XAIE_OK);
	Stub_XAie_PerfCounterGet_SetValue(3, 0, XAIE_PL_MOD, 1, 100, XAIE_OK);
	Stub_XAie_PerfCounterGet_SetValue(1, 0, XAIE_PL_MOD, 3, 200, XAIE_OK);
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	uint64_t result = profiling::read(acquiredResources, true);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by profiling read function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(106, result);
	(void)profiling::stop(acquiredResources, true);
	acquiredResources.clear();
}

/*@section Test Execution details
| Test Case ID                                           | Test Result | CR | Impact | Test Details |
|--------------------------------------------------------|-------------|----|--------|--------------|
| TC_001_DefaultConstructor                              | PASS        | No | NA     | NA           |
| TC_002_GmioConstructorAie2gm                           | PASS        | No | NA     | NA           |
| TC_003_GmioConstructorGm2aie                           | PASS        | No | NA     | NA           |
| TC_004_PlioConstructor                                 | PASS        | No | NA     | NA           |
| TC_005_NullPointers                                    | PASS        | No | NA     | NA           |
| TC_001_InvalidTileLocation                             | PASS        | No | NA     | NA           |
| TC_002_NominalSuccess                                  | PASS        | No | NA     | NA           |
| TC_003_StreamSwitchRequestFails                        | PASS        | No | NA     | NA           |
| TC_004_PerfCounterRequestFails                         | PASS        | No | NA     | NA           |
| TC_005_EventSelectFails                                | PASS        | No | NA     | NA           |
| TC_006_PerfCounterControlFails                         | PASS        | No | NA     | NA           |
| TC_007_NominalSuccessMaster                            | PASS        | No | NA     | NA           |
| TC_008_NegativeShimColumnIsolated                      | PASS        | No | NA     | NA           |
| TC_009_NegativeStreamPortIdIsolated                    | PASS        | No | NA     | NA           |
| EventCount_TC_001_NominalSuccess                       | PASS        | No | NA     | NA           |
| EventCount_TC_002_InvalidStreamPortId                  | PASS        | No | NA     | NA           |
| EventCount_TC_003_StreamSwitchRequestFails             | PASS        | No | NA     | NA           |
| EventCount_TC_004_PerfCounterRequestFails              | PASS        | No | NA     | NA           |
| EventCount_TC_005_EventSelectFails                     | PASS        | No | NA     | NA           |
| EventCount_TC_006_PerfCounterControlFails              | PASS        | No | NA     | NA           |
| EventCount_TC_007_NominalSuccessMaster                 | PASS        | No | NA     | NA           |
| EventCount_TC_008_NegativeShimColumnIsolated           | PASS        | No | NA     | NA           |
| EventCount_TC_009_ShimColumnMinusOneTriggersColInvalid | PASS        | No | NA     | NA           |
| TC_001_InvalidShimColumn1                              | PASS        | No | NA     | NA           |
| TC_002_SameTileNominalSuccess                          | PASS        | No | NA     | NA           |
| TC_003_DifferentTileNominalSuccess                     | PASS        | No | NA     | NA           |
| TC_004_InvalidSecondConfig                             | PASS        | No | NA     | NA           |
| TC_005_StreamSwitch1RequestFails                       | PASS        | No | NA     | NA           |
| TC_006_PerfCounter1RequestFails                        | PASS        | No | NA     | NA           |
| TC_007_StreamSwitch2RequestFails                       | PASS        | No | NA     | NA           |
| TC_008_PerfCounter2RequestFails                        | PASS        | No | NA     | NA           |
| TC_009_BroadcastRequestFails                           | PASS        | No | NA     | NA           |
| TC_010_BroadcastRequestReleaseFails                    | PASS        | No | NA     | NA           |
| TC_011_EventSelectFailureSameTile                      | PASS        | No | NA     | NA           |
| TC_012_PerfCounterControlFailureDifferentTiles         | PASS        | No | NA     | NA           |
| TC_013_SameTileBothMaster                              | PASS        | No | NA     | NA           |
| TC_014_DifferentTileBothMasterReversedColumns          | PASS        | No | NA     | NA           |
| TC_015_Stream1NegativeShimColumnIsolated               | PASS        | No | NA     | NA           |
| TC_016_Stream1NegativeStreamPortIdIsolated             | PASS        | No | NA     | NA           |
| TC_017_Stream2InvalidCol                               | PASS        | No | NA     | NA           |
| TC_018_Stream2NegativeShimColumnIsolated               | PASS        | No | NA     | NA           |
| TC_019_Stream2NegativeStreamPortIdIsolated             | PASS        | No | NA     | NA           |
| TC_020_BroadcastFailThirdReleaseFails                  | PASS        | No | NA     | NA           |
| TC_021_BroadcastFailFourthReleaseFails                 | PASS        | No | NA     | NA           |
| TC_022_BroadcastRequestFailsAllCleanupReleases         | PASS        | No | NA     | NA           |
| TC_023_BroadcastFailSecondReleaseFails                 | PASS        | No | NA     | NA           |
| StartTransfer_TC_001_InvalidShimColumn                 | PASS        | No | NA     | NA           |
| StartTransfer_TC_002_InvalidStreamPortId               | PASS        | No | NA     | NA           |
| StartTransfer_TC_003_StreamSwitchRequestFails          | PASS        | No | NA     | NA           |
| StartTransfer_TC_004_FirstPerfCounterRequestFails      | PASS        | No | NA     | NA           |
| StartTransfer_TC_005_SecondPerfCounterRequestFails     | PASS        | No | NA     | NA           |
| StartTransfer_TC_006_EventSelectFails                  | PASS        | No | NA     | NA           |
| StartTransfer_TC_007_PerfCounterControl0Fails          | PASS        | No | NA     | NA           |
| StartTransfer_TC_008_ZeroEventValueBoundary            | PASS        | No | NA     | NA           |
| StartTransfer_TC_009_NominalSuccessMaster              | PASS        | No | NA     | NA           |
| StartTransfer_TC_010_NegativeShimColumnIsolated        | PASS        | No | NA     | NA           |
| StartTransfer_TC_011_NegativeStreamPortIdIsolated      | PASS        | No | NA     | NA           |
| TC_001_ValidExecution                                  | PASS        | No | NA     | NA           |
| TC_002_StopAfterRunningToIdle                          | PASS        | No | NA     | NA           |
| TC_003_StopWithStreamPortSelectFirst                   | PASS        | No | NA     | NA           |
| TC_004_StopAfterEventCount                             | PASS        | No | NA     | NA           |
| TC_005_StopWithStreamPortSelectOnly                    | PASS        | No | NA     | NA           |
| TC_006_StopWithPCEventOnly                             | PASS        | No | NA     | NA           |
| TC_007_StopAfterStartToTransferComplete                | PASS        | No | NA     | NA           |
| TC_008_StopAfterRunningEventCount                      | PASS        | No | NA     | NA           |
| TC_009_StopAfterStartTimeDiffSameTile                  | PASS        | No | NA     | NA           |
| TC_010_StopAfterStartTimeDiffDifferentTiles            | PASS        | No | NA     | NA           |
| TC_011_StopWithBroadcastResource                       | PASS        | No | NA     | NA           |
| TC_011a_StopWithBroadcastResourceNoStartTimeDifference | PASS        | No | NA     | NA           |
| TC_011b_StopBroadcastNonPlEndModule                    | PASS        | No | NA     | NA           |
| TC_012_StopWithDriverError                             | PASS        | No | NA     | NA           |
| TC_013_ReleaseFailure                                  | PASS        | No | NA     | NA           |
| TC_014_StopWithUnknownResource                         | PASS        | No | NA     | NA           |
| TC_014a_StopWithNullResource                           | PASS        | No | NA     | NA           |
| TC_015_StopBroadcastReversedTileLocs                   | PASS        | No | NA     | NA           |
| TC_016_StopSingleTileGetRscIdFails                     | PASS        | No | NA     | NA           |
| TC_017_StopPerfCounterResetControlFails                | PASS        | No | NA     | NA           |
| TC_018_StopStreamPortResetFails                        | PASS        | No | NA     | NA           |
| TC_019_StopPCEventResetFails                           | PASS        | No | NA     | NA           |
| TC_001_ReadStartTimeDifferenceDifferentColumns         | PASS        | No | NA     | NA           |
| TC_002_InvalidResourceType                             | PASS        | No | NA     | NA           |
| TC_003_InvalidResourcesForTimeDifference               | PASS        | No | NA     | NA           |
| TC_003a_InvalidSecondResourceForTimeDifference         | PASS        | No | NA     | NA           |
| TC_003b_NullResourcesForTimeDifference                 | PASS        | No | NA     | NA           |
| TC_003c_NullResourceForStandardRead                    | PASS        | No | NA     | NA           |
| TC_004_GetRscIdFailureInTimeDifference                 | PASS        | No | NA     | NA           |
| TC_005_GetRscIdFailureInStandardRead                   | PASS        | No | NA     | NA           |
| TC_006_ReadPerfCounterSuccess                          | PASS        | No | NA     | NA           |
| TC_007_ReadPerfCounterGetRscIdFails                    | PASS        | No | NA     | NA           |
| TC_008_ReadStartTimeDifferenceReversedColumns          | PASS        | No | NA     | NA           |
/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)                               | %      | Result     | CR/Status    | Impact          |
|-------------------------------------------------------------|--------|------------|--------------|-----------------|
| shim_config::shim_config()                                  | <100>  | <Pass>     | <NA>         | No Impact       |
| shim_config::shim_config(const gmio_config*)                | <100>  | <Pass>     | <NA>         | No Impact       |
| shim_config::shim_config(const plio_config*)                | <100>  | <Pass>     | <NA>         | No Impact       |
| profiling::profile_stream_running_to_idle_cycles            | <100>  | <Pass>     | <NA>         | No Impact       |
| profiling::profile_stream_running_event_count               | <100>  | <Pass>     | <NA>         | No Impact       |
| profiling::profile_start_time_difference_btw_two_streams    | <100>  | <Pass>     | <NA>         | No Impact       |
| profiling::profile_stream_start_to_transfer_complete_cycles | <100>  | <Pass>     | <NA>         | No Impact       |
| accumulateStatus                                            | <100>  | <Pass>     | <NA>         | No Impact       |
| profiling::stop                                             | <100>  | <Pass>     | <NA>         | No Impact       |
| profiling::read                                             | <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)                               | 100    | <Pass>     | <NA>         | No Impact       |

/*@section Uncovered details*/
/*

 *Function: profiling::profile_stream_running_to_idle_cycles /
 *           profiling::profile_stream_start_to_transfer_complete_cycles /
 *           profiling::profile_stream_running_event_count /
 *           profiling::profile_start_time_difference_btw_two_streams
 *  Uncovered details:
 *
		if ((tileLoc.Row == INVALID_TILE_COORD) || (tileLoc.Col == INVALID_TILE_COORD) ||
			(shimColumn < 0) || (streamPortId < 0)) {
			return errorMsg(err_code::internal_error, ...);
		}
		// profile_start_time_difference_btw_two_streams uses the same validator with two tiles
 *	Justification: For all four functions the Unit tests stubs always generate valid tile
 *	row/col and non-negative ids, so the individual (INVALID_TILE_COORD / negative ids) branches
 *	are unreachable and have no impact on source code behavior.
 *
 *Function: profiling::stop
 *  Uncovered details:
 *
		if (auto pPerfCounterRsc = dynamic_cast<xaiefal::XAiePerfCounter *>(pSingleTileRsc)) {
			...
		} else if (auto pSSwitchPortEventRsc = dynamic_cast<xaiefal::XAieStreamPortSelect *>(pSingleTileRsc)) {
			...
		} else if (auto pProgramCounterRsc = dynamic_cast<xaiefal::XAiePCEvent *>(pSingleTileRsc)) {
			...
		}
 *	Justification: UT scenarios use one xaiefal resource subtype per test, so the
 *	"cast failed, try next" branches between casts are never exercised. The cleanup body for
 *	each subtype is covered through targeted tests; the fall-through branches have no impact.
*/
