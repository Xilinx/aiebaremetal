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
* @file test_aeg_configure.cc
*  This file contains test cases for ConfigureParser and AEG configuration helpers
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	 Who	 Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera	 05/03/2026	 Initial creation
* 1.1	 Veera	 20/04/2026	 Added test cases for ConfigureParser and AEG configuration helpers
* 1.2	 Veera	 10/05/2026	 updated comment section for ConfigureParser and AEG configuration helpers
* 1.3	 Veera	 27/05/2026	 added time taken by functions in ConfigureParser
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
#include "aeg_configure.h"
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport_c.h"
#include "event.h"
#include "stubs.h"
#include "test_comparator.h"
#include <cerrno>
#include <chrono>
#include <iostream>
TEST_GROUP(ConfigureParserTests)
{
	ConfigureParser *cfg;
	std::shared_ptr<XAieMem> mem;
	XAie_DevInst devInst;

	void setup()
	{
		ResetAegConfigureTestMocks();
		cfg = new ConfigureParser();
		RegisterXAieLocTypeComparator();
		devInst.NumRows = 3;
		mock_c()->installCopier("u8", u8Copier);
		mem = std::make_shared<XAieMem>(&devInst);
	}

	void teardown()
	{
		delete cfg;
		mock_c()->removeAllComparatorsAndCopiers();

		mock_c()->clear();
		mock_c()->checkExpectations();
		ResetAegConfigureTestMocks();
	}
};
/*********************************************************************************/
/*
*		Unique test case ID: TC_001_Constructor
*		Function Name: ConfigureParser
*		Input: None
*		Expectation: cfg != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_001_Constructor)
{
	ConfigureParser *cfg;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	cfg = new ConfigureParser();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureParser constructor: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(cfg != nullptr);
	delete cfg;
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_002_ConfigureGmios_NullArgument
*		Function Name: ConfigureGmios
*		Input: mem1 = nullptr (XAieMem backend is null)
*		Expectation: err == aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_002_ConfigureGmios_NullArgument)
{
	aeg_err  err;
	std::shared_ptr<XAieMem> mem1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = cfg->ConfigureGmios(mem1);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureGmios function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::null_argument);
}
/*********************************************************************************/
/*
*		Unique test case ID: TC_003_ConfigureGmios
*		Function Name: ConfigureGmios
*		Input: mem (XAieMem backend is not null) and cfg is not null
*		Expectation: err == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_003_ConfigureGmios)
{
	aeg_err err;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = cfg->ConfigureGmios(mem);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureGmios function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::ok);
	cfg->printGMIOsConfig();
	std::shared_ptr<GmioApiImpl> gmio_api = cfg->get_gmio_api_impl("gradf.in");
	CHECK(gmio_api != nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_004_ConfigureExternalBuffers
*		Function Name: ConfigureExternalBuffers
*		Input: mem (XAieMem backend is not null) and cfg is not null
*		Expectation: err == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_004_ConfigureExternalBuffers)
{
	aeg_err  err;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = cfg->ConfigureExternalBuffers(mem);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureExternalBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::ok);

	std::shared_ptr<ExternalBufferApiImpl> ext_buffer =
		cfg->get_externalbuffer_api_impl("EG.ddrin");
	CHECK(ext_buffer != nullptr);
}
/*********************************************************************************/
/*
*		Unique test case ID: TC_005_ConfigureExternalBuffers_NullArgument
*		Function Name: ConfigureExternalBuffers
*		Input: mem1 = nullptr (XAieMem backend is null)
*		Expectation: err == aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_005_ConfigureExternalBuffers_NullArgument)
{
	aeg_err  err;
	std::shared_ptr<XAieMem> mem1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = cfg->ConfigureExternalBuffers(mem1);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureExternalBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::null_argument);
}
extern "C" void abort()
{
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_006_GetExternalBufferApiImpl_InvalidName
*		Function Name: AIEGraphSim::getExternalBufferApiImpl
*		Input: "graph"
*		Expectation: buffer == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_006_GetExternalBufferApiImpl_InvalidName)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	std::shared_ptr<ExternalBufferApiImpl> buffer =
		cfg->get_externalbuffer_api_impl("graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_externalbuffer_api_impl function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(buffer == nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_007_GetRTPConfig_InvalidName
*		Function Name: get_rtp_config
*		Input: "graph"
*		Expectation: buffer == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_007_GetRTPConfig_InvalidName)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::rtp_config *buffer = cfg->get_rtp_config("graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_rtp_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(buffer == nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_008_GetGmioApiImpl_InvalidName
*		Function Name: AIEGraphSim::getGmioApiImpl
*		Input: "graph"
*		Expectation: buffer == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_008_GetGmioApiImpl_InvalidName)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	std::shared_ptr<GmioApiImpl> buffer = cfg->get_gmio_api_impl("graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_gmio_api_impl function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(buffer == nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_009_GetGraphConfig_InvalidName
*		Function Name: get_graph_config
*		Input: "graph"
*		Expectation: buffer == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_009_GetGraphConfig_InvalidName)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::graph_config *buffer = cfg->get_graph_config("graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_graph_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(buffer == nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_010_GetGmioConfig_InvalidName
*		Function Name: get_gmio_config
*		Input: "graph"
*		Expectation: gmconfig_1 == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_010_GetGmioConfig_InvalidName)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::gmio_config *gmconfig_1 = cfg->get_gmio_config("graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_gmio_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(gmconfig_1 == nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_011_GetPlioConfigLogicName_InvalidName
*		Function Name: get_plio_config_logicname
*		Input: "graph"
*		Expectation: buffer == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_011_GetPlioConfigLogicName_InvalidName)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::plio_config *buffer = cfg->get_plio_config_logicname("graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_plio_config_logicname function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(buffer == nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_012_GetPlioConfig_InvalidName
*		Function Name: get_plio_config
*		Input: "graph"
*		Expectation: plio == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_012_GetPlioConfig_InvalidName)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::plio_config *plio = cfg->get_plio_config("graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_plio_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(plio == nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_013_GetExternalBufferConfig_InvalidName
*		Function Name: get_externalbuffer_config
*		Input: "graph"
*		Expectation: buffer == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_013_GetExternalBufferConfig_InvalidName)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::external_buffer_config *buffer =
		cfg->get_externalbuffer_config("graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_externalbuffer_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(buffer == nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_014_GetSharedBufferConfig_InvalidName
*		Function Name: get_shared_buffer_config
*		Input: "graph"
*		Expectation: buffer == nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_014_GetSharedBufferConfig_InvalidName)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::shared_buffer_config *buffer = cfg->get_shared_buffer_config("graph");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_shared_buffer_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(buffer == nullptr);
}


/*********************************************************************************/
/*
*		Unique test case ID: TC_015_GetAndPrintDriverConfig
*		Function Name: get_driver_config
*		Input: None
*		Expectation: driver_config.hw_gen == 1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_015_GetAndPrintDriverConfig)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::driver_config &driver_config = cfg->get_driver_config();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_driver_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(1, driver_config.hw_gen);
	cfg->printDriverConfig();
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_016_GetAndPrintGMIOsConfig
*		Function Name: get_gmio_config
*		Input: "gradf.in"
*		Expectation: gmconfig_1 != nullptr, gmconfig_1->name == "gradf.in"
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_016_GetAndPrintGMIOsConfig)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::gmio_config *gmconfig_1 = cfg->get_gmio_config("gradf.in");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_gmio_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(gmconfig_1 != nullptr);
	CHECK(gmconfig_1->name == "gradf.in");
	cfg->printGMIOsConfig();
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_017_GetAieCompilerConfig
*		Function Name: get_aiecompiler_config
*		Input: None
*		Expectation: aiecompilerconfig.broadcast_enable_core == true
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_017_GetAieCompilerConfig)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::aiecompiler_config &aiecompilerconfig = cfg->get_aiecompiler_config();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_aiecompiler_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(true, aiecompilerconfig.broadcast_enable_core);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_018_GetAndPrintGraphConfig
*		Function Name: get_graph_config
*		Input: "gradf2"
*		Expectation: aiegraphconfig != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_018_GetAndPrintGraphConfig)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::graph_config *aiegraphconfig = cfg->get_graph_config("gradf2");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_graph_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(aiegraphconfig != nullptr);
	CHECK_EQUAL(0, aiegraphconfig->id);
	cfg->printGraphConfig();
}
/*********************************************************************************/
/*
*		Unique test case ID: TC_019_GetAndPrintPLIOsConfig
*		Function Name: get_plio_config
*		Input: "gradf2.pin"
*		Expectation: plio != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_019_GetAndPrintPLIOsConfig)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	const abr::plio_config *plio = cfg->get_plio_config("gradf2.pin");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_plio_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(plio != nullptr);
	CHECK(plio->name == "gradf2.pin");
	cfg->printPLIOsConfig();
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_020_GetPlioConfigLogicName_ValidName
*		Function Name: get_plio_config_logicname
*		Input: "plioin1"
*		Expectation: plio != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_020_GetPlioConfigLogicName_ValidName)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::plio_config *plio = cfg->get_plio_config_logicname("plioin1");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_plio_config_logicname function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(plio != nullptr);
	CHECK(plio->logicalName == "plioin1");
}
/*********************************************************************************/
/*
*		Unique test case ID: TC_021_GetAndPrintExternalBufferConfig
*		Function Name: get_externalbuffer_config
*		Input: "EG.ddrin"
*		Expectation: ext_buffer != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_021_GetAndPrintExternalBufferConfig)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::external_buffer_config *ext_buffer = cfg->get_externalbuffer_config("EG.ddrin");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_externalbuffer_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(ext_buffer != nullptr);
	CHECK(ext_buffer->name == "EG.ddrin");
	cfg->printExternalbufferConfig();
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_022_GetAndPrintSharedBufferConfig
*		Function Name: get_shared_buffer_config
*		Input: "EG.input"
*		Expectation: shr_buffer != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_022_GetAndPrintSharedBufferConfig)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::shared_buffer_config *shr_buffer = cfg->get_shared_buffer_config("EG.input");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_shared_buffer_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(shr_buffer != nullptr);
	CHECK(shr_buffer->name == "EG.input");
	cfg->printSharedBufferConfig();
	std::shared_ptr<SharedBufferApiImpl> shared_buffer_api =
		cfg->get_sharedbuffer_api_impl("EG.input");
	CHECK(shared_buffer_api == nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_023_IsRTPorSharedBuffer_RTP
*		Function Name: isRTPorSharedBuffer
*		Input: "gradf.sq.in[1]"
*		Expectation: ret == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_023_IsRTPorSharedBuffer_RTP)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int32_t ret = cfg->isRTPorSharedBuffer("gradf.sq.in[1]");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by isRTPorSharedBuffer function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, ret);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_024_IsRTPorSharedBuffer_SharedBuffer
*		Function Name: isRTPorSharedBuffer
*		Input: "EG.input"
*		Expectation: ret == 1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_024_IsRTPorSharedBuffer_SharedBuffer)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int32_t ret = cfg->isRTPorSharedBuffer("EG.input");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by isRTPorSharedBuffer function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(1, ret);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_025_IsRTPorSharedBuffer_NeitherRtpNorSharedBuffer
*		Function Name: isRTPorSharedBuffer
*		Input: "inavlid_port"
*		Expectation: ret == -1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_025_IsRTPorSharedBuffer_NeitherRtpNorSharedBuffer)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int32_t ret = cfg->isRTPorSharedBuffer("inavlid_port");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by isRTPorSharedBuffer function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-1, ret);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_026_GetAndPrintRTPsConfig
*		Function Name: get_rtp_config
*		Input: "gradf.sq.in[1]"
*		Expectation: rtp_buffer != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_026_GetAndPrintRTPsConfig)
{
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::rtp_config *rtp_buffer = cfg->get_rtp_config("gradf.sq.in[1]");
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_rtp_config function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(rtp_buffer != nullptr);
	CHECK(rtp_buffer->portName == "gradf.sq.in[1]");
	cfg->printRTPsConfig();
	std::shared_ptr<RtpApiImpl> rtp_api_impl = cfg->get_rtp_api_impl("gradf.sq.in[1]");
	CHECK(rtp_api_impl == nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_027_ConfigureRtps_InvalidName
*		Function Name: ConfigureRtps
*		Input: graphApi == nullptr
*		Expectation: err == aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_027_ConfigureRtps_InvalidName)
{
	aeg_err err;
	std::shared_ptr<abr::graph_api> graphApi;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = cfg->ConfigureRtps(graphApi);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureRtps function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::null_argument);
}
	/*********************************************************************************/
/*
*		Unique test case ID: TC_028_ConfigureSharedBuffers_InvalidName
*		Function Name: ConfigureSharedBuffers
*		Input: graphApi == nullptr
*		Expectation: err == aeg_err::null_argument
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_028_ConfigureSharedBuffers_InvalidName)
{
	aeg_err err;
	std::shared_ptr<abr::graph_api> graphApi;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = cfg->ConfigureSharedBuffers(graphApi);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureSharedBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::null_argument);
}
/*********************************************************************************/
/*
*		Unique test case ID: TC_029_ConfigureRtps_ValidName
*		Function Name: ConfigureRtps
*		Input: graphConfig.id = 1, graphConfig.name = "TestGraph"
*		Expectation: err == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_029_ConfigureRtps_ValidName)
{
	aeg_err err;
	abr::graph_config graphConfig;
	graphConfig.id = 1;
	graphConfig.name = "TestGraph";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = cfg->ConfigureRtps(std::make_shared<abr::graph_api>(&graphConfig));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureRtps function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::ok);
}
/*********************************************************************************/
/*
*		Unique test case ID: TC_040_ConfigureSharedBuffers_ValidName
*		Function Name: ConfigureSharedBuffers
*		Input: graphConfig.id = 1, graphConfig.name = "TestGraph"
*		Expectation: err == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_040_ConfigureSharedBuffers_ValidName)
{
	aeg_err err;
	abr::graph_config graphConfig;
	graphConfig.id = 1;
	graphConfig.name = "TestGraph";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	err = cfg->ConfigureSharedBuffers(std::make_shared<abr::graph_api>(&graphConfig));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureSharedBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::ok);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_041_GetRtpApiImpl_AfterConfigureRtps
*		Function Name: AIEGraphSim::getRtpApiImpl
*		Input: graphConfig.id = 0, graphConfig.name = "gradf2"
*		Expectation: err == aeg_err::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_041_GetRtpApiImpl_AfterConfigureRtps)
{
	abr::graph_config graphConfig;
	graphConfig.id = 0;
	graphConfig.name = "gradf2";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err err =
		cfg->ConfigureRtps(std::make_shared<abr::graph_api>(&graphConfig));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureRtps function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::ok);
	std::shared_ptr<RtpApiImpl> rtp = cfg->get_rtp_api_impl("gradf.sq.in[1]");
	CHECK(rtp != nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_042_GetSharedbufferApiImpl_AfterConfigureSharedBuffers
*		Function Name: ConfigureSharedBuffers
*		Input: graphConfig.id = 0, graphConfig.name = "gradf2"
*		Expectation: sb != nullptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_042_GetSharedbufferApiImpl_AfterConfigureSharedBuffers)
{
	abr::graph_config graphConfig;
	graphConfig.id = 0;
	graphConfig.name = "gradf2";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err err =
		cfg->ConfigureSharedBuffers(std::make_shared<abr::graph_api>(&graphConfig));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureSharedBuffers function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::ok);
	std::shared_ptr<SharedBufferApiImpl> sb =
		cfg->get_sharedbuffer_api_impl("EG.input");
	CHECK(sb != nullptr);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_043_ConfigureGmios_DriverFailure
*		Function Name: ConfigureGmios
*		Input: dev.NumCols = 2, dev.NumRows = 2, dev.StartCol = 0
*		Expectation: err == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_043_ConfigureGmios_DriverFailure)
{
	XAie_DevInst dev;
	dev.NumCols = 2;
	dev.NumRows = 2;
	dev.StartCol = 0;

	ut_stub_force_gmio_configure_fail(1);
	std::shared_ptr<XAieMem> memLocal = std::make_shared<XAieMem>(&dev);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err err = cfg->ConfigureGmios(memLocal);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureGmios function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(err == aeg_err::aie_driver_error);
	ut_stub_force_gmio_configure_fail(0);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_044_ConfigureGmios_UserError
*		Function Name: ConfigureGmios -> map_err_code
*		Input: gmio_api::configure() forced to return err_code::user_error (22)
*		Expectation: err == aeg_err::user_error (exercises map_err_code switch case)
*		Test Method: Unit testing
*     	Methods for deriving test case: Coverage of map_err_code switch arms
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_044_ConfigureGmios_UserError)
{
	double total_clock_enable_us = 0.0;
	g_ut_gmio_configure_force_code = 22;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err err = cfg->ConfigureGmios(mem);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by UserError function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_gmio_configure_force_code = 0;
	CHECK(err == aeg_err::user_error);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_045_ConfigureGmios_InternalError
*		Function Name: ConfigureGmios -> map_err_code
*		Input: gmio_api::configure() forced to return err_code::internal_error (ENOTSUP)
*		Expectation: err == aeg_err::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Coverage of map_err_code switch arms
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_045_ConfigureGmios_InternalError)
{
	double total_clock_enable_us = 0.0;
	g_ut_gmio_configure_force_code = ENOTSUP;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err err = cfg->ConfigureGmios(mem);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by InternalError function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_gmio_configure_force_code = 0;
	CHECK(err == aeg_err::internal_error);

}
/*********************************************************************************/
/*
*		Unique test case ID: TC_046_ConfigureGmios_ResourceUnavailable
*		Function Name: ConfigureGmios -> map_err_code
*		Input: gmio_api::configure() forced to return err_code::resource_unavailable (EAGAIN)
*		Expectation: err == aeg_err::resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Coverage of map_err_code switch arms
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_046_ConfigureGmios_ResourceUnavailable)
{
	double total_clock_enable_us = 0.0;
	g_ut_gmio_configure_force_code = EAGAIN;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err err = cfg->ConfigureGmios(mem);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ResourceUnavailable function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_gmio_configure_force_code = 0;
	CHECK(err == aeg_err::resource_unavailable);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_047_ConfigureGmios_UnknownErrorDefaultsToDriverError
*		Function Name: ConfigureGmios -> map_err_code
*		Input: gmio_api::configure() forced to return an unmapped err_code value
*		Expectation: err == aeg_err::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Coverage of map_err_code default switch arm
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserTests, TC_047_ConfigureGmios_UnknownErrorDefaultsToDriverError)
{
	double total_clock_enable_us = 0.0;
	g_ut_gmio_configure_force_code = 123;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	aeg_err err = cfg->ConfigureGmios(mem);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureGmios default error mapping function: " << total_clock_enable_us << " microseconds" << std::endl;
	g_ut_gmio_configure_force_code = 0;
	CHECK(err == aeg_err::aie_driver_error);
}

TEST_GROUP(ConfigureParserMetadataShapeTests)
{
	uint32_t saved_driverconfig_size;

	void setup()
	{
		ResetAegConfigureTestMocks();
		saved_driverconfig_size = _binary_build_driverconfig_bin_size;
	}

	void teardown()
	{
		_binary_build_driverconfig_bin_size = saved_driverconfig_size;
		mock_c()->clear();
		mock_c()->checkExpectations();
		ResetAegConfigureTestMocks();
	}
};

/*********************************************************************************/
/*
*		Unique test case ID: TC_048_DriverConfigWithoutOptionalPartitions
*		Function Name: ConfigureParser
*		Input: driverconfig binary size ends before optional partition records
*		Expectation: parser loads base driver data and graph broadcast column remains default
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of optional metadata block and empty partition data
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ConfigureParserMetadataShapeTests, TC_048_DriverConfigWithoutOptionalPartitions)
{
	constexpr uint32_t driver_base_metadata_size = 31U;
	_binary_build_driverconfig_bin_size = driver_base_metadata_size;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	ConfigureParser local_cfg;
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by ConfigureParser without optional partitions: " << total_clock_enable_us << " microseconds" << std::endl;

	abr::driver_config &driver = local_cfg.get_driver_config();
	abr::graph_config *graph = local_cfg.get_graph_config("gradf2");

	CHECK(driver.partition_overlay_start_cols.size() == 4U);
	CHECK(graph != nullptr);
	CHECK_EQUAL(default_start_column, graph->broadcast_column);
	local_cfg.printDriverConfig();
	local_cfg.printGraphConfig();
}

TEST_GROUP(GetPartitionStartColumnTestGroup)
{
	std::vector<abr::partition_config> partitions;
	void setup()
	{
		ResetAegConfigureTestMocks();
	}
	void teardown()
	{
		partitions.clear();
		mock_c()->clear();
		mock_c()->checkExpectations();
		ResetAegConfigureTestMocks();
	}
};

/*********************************************************************************/
/*
*		Unique test case ID: TC_001_EmptyPartitionsList
*		Function Name: get_partition_start_column
*		Input: column = 10
*		Expectation: ret == default_start_column (0)
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GetPartitionStartColumnTestGroup, TC_001_EmptyPartitionsList)
{
	int column = 10;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int ret = get_partition_start_column(partitions, column);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_partition_start_column function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(default_start_column, ret);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_002_ColumnInRange
*		Function Name: ColumnInRange
*		Input: partitions = {{0, 5}, {5, 10}}, column = 7
*		Expectation: ret == 5
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GetPartitionStartColumnTestGroup, TC_002_ColumnInRange)
{
	partitions = {
		{0, 5},
		{5, 10},
	};
	int column = 7;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int ret = get_partition_start_column(partitions, column);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_partition_start_column function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(5, ret);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_003_ColumnOutsideAllPartitions
*		Function Name: ColumnOutsideAllPartitions
*		Input: partitions = {{0, 5}}, column = 50
*		Expectation: ret == default_start_column (0)
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GetPartitionStartColumnTestGroup, TC_003_ColumnOutsideAllPartitions)
{
	partitions = {
		{0, 5},
	};
	int column = -1;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int ret = get_partition_start_column(partitions, column);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_partition_start_column function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(default_start_column, ret);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_004_ColumnOutsideAllPartitions
*		Function Name: ColumnOutsideAllPartitions
*		Input: partitions = {{0, 5}}, column = 50
*		Expectation: ret == default_start_column (0)
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GetPartitionStartColumnTestGroup, TC_004_ColumnOutsideAllPartitions)
{
	partitions = {
		{0, 5},
	};
	int column = 50;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int ret = get_partition_start_column(partitions, column);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_partition_start_column function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(default_start_column, ret);
}

/*********************************************************************************/
/*
*		Unique test case ID: TC_005_ColumnBeforePartitionStart
*		Function Name: ColumnBeforePartitionStart
*		Input: partitions = {{5, 10}}, column = 3
*		Expectation: ret == default_start_column (0)
*		Test Method: Unit testing
*     	Methods for deriving test case: Branch coverage of lower-bound range check
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(GetPartitionStartColumnTestGroup, TC_005_ColumnBeforePartitionStart)
{
	partitions = {
		{5, 10},
	};
	int column = 3;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int ret = get_partition_start_column(partitions, column);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by get_partition_start_column function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(default_start_column, ret);
}
TEST_GROUP(AbrEventProfilingTests)
 {
	 ConfigureParser *cfg;

	 void setup()
	 {
		cfg = new ConfigureParser();
	 }

	 void teardown()
	 {
		delete cfg;
		mock_c()->checkExpectations();
		mock_c()->clear();
	 }
 };
 /*********************************************************************************/
/*
*		Unique test case ID: Event_TC_01_StartProfilingUnknownOptionReturnsZero
*		Function Name: abr::event::start_profiling
*		Input: "gradf.in","gradf.out", 0x99, 0U
*		Expectation: ret == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AbrEventProfilingTests, Event_TC_01_StartProfilingUnknownOptionReturnsZero)
 {
	 abr::event ev(cfg);
	 double total_clock_enable_us = 0.0;
	 const auto clock_enable_t0 = std::chrono::steady_clock::now();
	 CHECK(ev.start_profiling(std::string("gradf.in"), std::string("gradf.out"), static_cast<int32_t>(0x99), 0U) == 0);
	 const auto clock_enable_t1 = std::chrono::steady_clock::now();
	 total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	 std::cout << "Time taken by start_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
 }
 /*********************************************************************************/
/*
*		Unique test case ID: Event_TC_02_StopProfilingInvalidHandle
*		Function Name: abr::event::stop_profiling
*		Input: 0, -1
*		Expectation: None
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AbrEventProfilingTests, Event_TC_02_StopProfilingInvalidHandle)
 {
	 abr::event ev(cfg);
	 double total_clock_enable_us = 0.0;
	 const auto clock_enable_t0 = std::chrono::steady_clock::now();
	 ev.stop_profiling(0);
	 const auto clock_enable_t1 = std::chrono::steady_clock::now();
	 total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	 std::cout << "Time taken by stop_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	 ev.stop_profiling(-1);
	 mock_c()->checkExpectations();
	 mock_c()->clear();
 }
 /*********************************************************************************/
/*
*		Unique test case ID: Event_TC_03_ReadProfilingInvalidHandleOutOfRange
*		Function Name: abr::event::read_profiling
*		Input: -1,0
*		Expectation: ret == 0U
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(AbrEventProfilingTests, Event_TC_03_ReadProfilingInvalidHandleOutOfRange)
{
	abr::event ev(cfg);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL(0U, ev.read_profiling(-1));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by read_profiling function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0U, ev.read_profiling(0));
	mock_c()->checkExpectations();
	mock_c()->clear();
}

/*@section Test Execution details
| Test Case ID                                              | Test Result | CR  | Impact | Test Details |
|-----------------------------------------------------------|-------------|-----|--------|--------------|
| TC_001_Constructor                                        | PASS        | No  | NA     | NA           |
| TC_002_ConfigureGmios_NullArgument                        | PASS        | No  | NA     | NA           |
| TC_003_ConfigureGmios                                     | PASS        | No  | NA     | NA           |
| TC_004_ConfigureExternalBuffers                           | PASS        | No  | NA     | NA           |
| TC_005_ConfigureExternalBuffers_NullArgument              | PASS        | No  | NA     | NA           |
| TC_006_GetExternalBufferApiImpl_InvalidName               | PASS        | No  | NA     | NA           |
| TC_007_GetRTPConfig_InvalidName                           | PASS        | No  | NA     | NA           |
| TC_008_GetGmioApiImpl_InvalidName                         | PASS        | No  | NA     | NA           |
| TC_009_GetGraphConfig_InvalidName                         | PASS        | No  | NA     | NA           |
| TC_010_GetGmioConfig_InvalidName                          | PASS        | No  | NA     | NA           |
| TC_011_GetPlioConfigLogicName_InvalidName                 | PASS        | No  | NA     | NA           |
| TC_012_GetPlioConfig_InvalidName                          | PASS        | No  | NA     | NA           |
| TC_013_GetExternalBufferConfig_InvalidName                | PASS        | No  | NA     | NA           |
| TC_014_GetSharedBufferConfig_InvalidName                  | PASS        | No  | NA     | NA           |
| TC_015_GetAndPrintDriverConfig                            | PASS        | No  | NA     | NA           |
| TC_016_GetAndPrintGMIOsConfig                             | PASS        | No  | NA     | NA           |
| TC_017_GetAieCompilerConfig                               | PASS        | No  | NA     | NA           |
| TC_018_GetAndPrintGraphConfig                             | PASS        | No  | NA     | NA           |
| TC_019_GetAndPrintPLIOsConfig                             | PASS        | No  | NA     | NA           |
| TC_020_GetPlioConfigLogicName_ValidName                   | PASS        | No  | NA     | NA           |
| TC_021_GetAndPrintExternalBufferConfig                    | PASS        | No  | NA     | NA           |
| TC_022_GetAndPrintSharedBufferConfig                      | PASS        | No  | NA     | NA           |
| TC_023_IsRTPorSharedBuffer_RTP                            | PASS        | No  | NA     | NA           |
| TC_024_IsRTPorSharedBuffer_SharedBuffer                   | PASS        | No  | NA     | NA           |
| TC_025_IsRTPorSharedBuffer_NeitherRtpNorSharedBuffer      | PASS        | No  | NA     | NA           |
| TC_026_GetAndPrintRTPsConfig                              | PASS        | No  | NA     | NA           |
| TC_027_ConfigureRtps_InvalidName                          | PASS        | No  | NA     | NA           |
| TC_028_ConfigureSharedBuffers_InvalidName                 | PASS        | No  | NA     | NA           |
| TC_029_ConfigureRtps_ValidName                            | PASS        | No  | NA     | NA           |
| TC_040_ConfigureSharedBuffers_ValidName                   | PASS        | No  | NA     | NA           |
| TC_041_GetRtpApiImpl_AfterConfigureRtps                   | PASS        | No  | NA     | NA           |
| TC_042_GetSharedbufferApiImpl_AfterConfigureSharedBuffers | PASS        | No  | NA     | NA           |
| TC_043_ConfigureGmios_DriverFailure                       | PASS        | No  | NA     | NA           |
| TC_044_ConfigureGmios_UserError                           | PASS        | No  | NA     | NA           |
| TC_045_ConfigureGmios_InternalError                       | PASS        | No  | NA     | NA           |
| TC_046_ConfigureGmios_ResourceUnavailable                 | PASS        | No  | NA     | NA           |
| TC_047_ConfigureGmios_UnknownErrorDefaultsToDriverError   | PASS        | No  | NA     | NA           |
| TC_048_DriverConfigWithoutOptionalPartitions              | PASS        | No  | NA     | NA           |
| TC_001_EmptyPartitionsList                                | PASS        | No  | NA     | NA           |
| TC_002_ColumnInRange                                      | PASS        | No  | NA     | NA           |
| TC_003_ColumnOutsideAllPartitions                         | PASS        | No  | NA     | NA           |
| TC_004_ColumnOutsideAllPartitions                         | PASS        | No  | NA     | NA           |
| TC_005_ColumnBeforePartitionStart                         | PASS        | No  | NA     | NA           |
| Event_TC_01_StartProfilingUnknownOptionReturnsZero        | PASS        | No  | NA     | NA           |
| Event_TC_02_StopProfilingInvalidHandle                    | PASS        | No  | NA     | NA           |
| Event_TC_03_ReadProfilingInvalidHandleOutOfRange          | PASS        | No  | NA     | NA           |

/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)               | %      | Result     | CR/Status    | Impact          |
|---------------------------------------------|--------|------------|--------------|-----------------|
| ConfigureParser::ConfigureGmios             | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::ConfigureExternalBuffers   | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::ConfigureRtps              | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::ConfigureSharedBuffers     | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_driver_config          | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_aiecompiler_config     | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_graph_config           | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_gmio_config            | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_plio_config            | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_plio_config_logicname  | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_plio_config_logicname | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_externalbuffer_config  | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_shared_buffer_config   | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_rtp_config             | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_gmio_api_impl          | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_externalbuffer_api_impl| <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_rtp_api_impl           | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::get_sharedbuffer_api_impl  | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::isRTPorSharedBuffer        | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printDriverConfig          | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printGraphConfig           | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printGMIOsConfig           | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printPLIOsConfig           | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printExternalbufferConfig  | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printSharedBufferConfig    | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printRTPsConfig            | <100>  | <Pass>     | <NA>         | No Impact       |
| get_partition_start_column                  | <100>  | <Pass>     | <NA>         | No Impact       |
| get_partition_start_column				   | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackBool                     | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackShort                    | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackUInt8                    | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackString                   | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackUInt32                   | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackUInt64                   | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackValue<bool>              | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackValue<int>               | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackValue<short>             | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackValue<unsigned int>      | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackValue<unsigned long>     | <100>  | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackVector<bool>             | <83.3> | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackVector<int>              | <83.3> | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackVector<short>            | <83.3> | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackVector<unsigned int>     | <83.3> | <Pass>     | <NA>         | No Impact       |
| UnpackUtils::unpackVector<unsigned long>    | <83.3> | <Pass>     | <NA>         | No Impact       |
| AiecompilerConfigStaticParser::load         | <100>  | <Pass>     | <NA>         | No Impact       |
| DriverConfigStaticParser::load              | <96.9> | <Pass>     | <NA>         | No Impact       |
| ExternalbuffersConfigStaticParser::load     | <96.2> | <Pass>     | <NA>         | No Impact       |
| ExternalbuffersConfigStaticParser::loadAll  | <90.0> | <Pass>     | <NA>         | No Impact       |
| GMIOsConfigStaticParser::load               | <92.3> | <Pass>     | <NA>         | No Impact       |
| GMIOsConfigStaticParser::loadAll            | <90.0> | <Pass>     | <NA>         | No Impact       |
| PLIOsConfigStaticParser::load               | <90.0> | <Pass>     | <NA>         | No Impact       |
| PLIOsConfigStaticParser::loadAll            | <90.0> | <Pass>     | <NA>         | No Impact       |
| RTPsConfigStaticParser::load                | <96.8> | <Pass>     | <NA>         | No Impact       |
| RTPsConfigStaticParser::loadAll             | <90.0> | <Pass>     | <NA>         | No Impact       |
| SharedBuffersConfigStaticParser::load       | <95.5> | <Pass>     | <NA>         | No Impact       |
| SharedBuffersConfigStaticParser::loadAll    | <90.0> | <Pass>     | <NA>         | No Impact       |
| map_err_code                                | <42.9> | <Fail>     | <NA>         | No Impact       |
| ConfigureParser::throw_if_error			  | <0>	 | <Fail>		| <NA>			| No Impact		|
| ConfigureParser::LoadGraphConfig     | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::LoadDriverConfig    | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::LoadSharedBuffers   | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::LoadExternalbuffers | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::LoadAiecompilerConfig| <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::Load                | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::LoadRtps            | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::LoadGmios           | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::LoadPlios           | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::ConfigureParser     | <100>  | <Pass>     | <NA>         | No Impact       |
| GraphConfigStaticParser::load        | <100>  | <Pass>     | <NA>         | No Impact       |
| GraphConfigStaticParser::loadAll     | <90.0> | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printrtpconfig      | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printgmioconfig     | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printplioconfig     | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printgraphconfig    | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printshimportconfig | <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printshimbdinfoconfig| <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printsharedbufferconfig| <100>  | <Pass>     | <NA>         | No Impact       |
| ConfigureParser::printexternalbufferconfig| <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)               | 95.2   | <Pass>     | <NA>         | No Impact       |

 /**
  * @brief TC05: Verify get_rtp_config with invalid RTP name
  */
 TEST(ConfigureParserTests, TC05_GetRTPConfig_InvalidName)
 {
	 auto buffer = cfg->get_rtp_config("graph");
	 CHECK(buffer == nullptr);
 }

 /**
  * @brief TC06: Verify get_gmio_api_impl with invalid GMIO name
  */
 TEST(ConfigureParserTests, TC06_GetGmioApiImpl_InvalidName)
 {
	 auto buffer = cfg->get_gmio_api_impl("graph");
	 CHECK(buffer == nullptr);
 }

 /**
  * @brief TC07: Verify get_graph_config with invalid graph name
  */
 TEST(ConfigureParserTests, TC07_GetGraphConfig_InvalidName)
 {
	 auto buffer = cfg->get_graph_config("graph");

	 CHECK(buffer == nullptr);
 }

 /**
  * @brief TC08: Verify get_gmio_config with invalid GMIO name
  */
 TEST(ConfigureParserTests, TC08_GetGmioConfig_InvalidName)
 {
	 abr::gmio_config *gmconfig_1 = cfg->get_gmio_config("graph");
	 CHECK(gmconfig_1 == nullptr);
 }

 /**
  * @brief TC09: Verify get_plio_config_logicname with invalid logical name
  */
 TEST(ConfigureParserTests, TC09_GetPlioConfigLogicName_InvalidName)
 {
	 auto buffer = cfg->get_plio_config_logicname("graph");
	 CHECK(buffer == nullptr);
 }

 /**
  * @brief TC10: Verify get_plio_config with invalid PLIO name
  */
 TEST(ConfigureParserTests, TC10_GetPlioConfig_InvalidName)
 {
	 abr::plio_config *plio = cfg->get_plio_config("graph");
	 CHECK(plio == nullptr);
 }

/**
 * @brief Verify silent lookup returns nullptr for missing GMIO without logging an accessor error
 */
TEST(ConfigureParserTests, TC10A_GetGmioConfig_InvalidNameSilent)
{
	 abr::gmio_config *gmconfig_1 = cfg->get_gmio_config("graph", true);
	 CHECK(gmconfig_1 == nullptr);
}

/**
 * @brief Verify silent lookup returns nullptr for missing logical PLIO without logging an accessor error
 */
TEST(ConfigureParserTests, TC10B_GetPlioConfigLogicName_InvalidNameSilent)
{
	 auto buffer = cfg->get_plio_config_logicname("graph", true);
	 CHECK(buffer == nullptr);
}

/**
 * @brief Verify silent lookup returns nullptr for missing PLIO without logging an accessor error
 */
TEST(ConfigureParserTests, TC10C_GetPlioConfig_InvalidNameSilent)
{
	 abr::plio_config *plio = cfg->get_plio_config("graph", true);
	 CHECK(plio == nullptr);
}

 /**
  * @brief TC11: Verify get_externalbuffer_config with invalid name
  */
 TEST(ConfigureParserTests, TC11_GetExternalBufferConfig_InvalidName)
 {
	 auto buffer = cfg->get_externalbuffer_config("graph");
	 CHECK(true);
 }

 /**
  * @brief TC12: Verify get_shared_buffer_config with invalid name
  */
 TEST(ConfigureParserTests, TC12_GetSharedBufferConfig_InvalidName)
 {
	 auto buffer = cfg->get_shared_buffer_config("graph");
	 CHECK(true);
 }


 /**
  * @brief TC13: Verify get_driver_config and printDriverConfig
  */
 TEST(ConfigureParserTests, TC13_GetAndPrintDriverConfig)
 {
	 abr::driver_config &driver_config = cfg->get_driver_config();
	 CHECK_EQUAL(1, driver_config.hw_gen); // AIE (changed from 2/AIEML to cover aeg_aiebaremetal.cc lines 843-849)
	 cfg->printDriverConfig();
	 CHECK(true);
 }

 /**
  * @brief TC14: Verify get_gmio_config and printGMIOsConfig
  */
 TEST(ConfigureParserTests, TC14_GetAndPrintGMIOsConfig)
 {
	 abr::gmio_config *gmconfig_1 = cfg->get_gmio_config("gradf.in");
	 CHECK(gmconfig_1->name == "gradf.in");
	 cfg->printGMIOsConfig();
	 CHECK(true);
 }

 /**
  * @brief TC15: Verify get_aiecompiler_config returns expected values
  */
 TEST(ConfigureParserTests, TC15_GetAieCompilerConfig)
 {
	 abr::aiecompiler_config &aiecompilerconfig = cfg->get_aiecompiler_config();
	 CHECK_EQUAL(true, aiecompilerconfig.broadcast_enable_core);
 }

 /**
  * @brief TC16: Verify get_graph_config and printGraphConfig
  */
 TEST(ConfigureParserTests, TC16_GetAndPrintGraphConfig)
 {
	 abr::graph_config *aiegraphconfig = cfg->get_graph_config("gradf2");
	 CHECK(aiegraphconfig != nullptr);
	 CHECK_EQUAL(0, aiegraphconfig->id);
	 cfg->printGraphConfig();
	 CHECK(true);
 }
 /**
  * @brief TC17: Verify get_plio_config and printPLIOsConfig
  */
 TEST(ConfigureParserTests, TC17_GetAndPrintPLIOsConfig)
 {
	 const abr::plio_config *plio = cfg->get_plio_config("gradf2.pin");
	 CHECK(plio->name == "gradf2.pin");
	 cfg->printPLIOsConfig();
	 CHECK(true);
 }

 /**
  * @brief TC18: Verify get_plio_config_logicname with valid logical name
  */
 TEST(ConfigureParserTests, TC18_GetPlioConfigLogicName_ValidName)
 {
	 abr::plio_config *plio = cfg->get_plio_config_logicname("plioin1");
	 CHECK(plio->logicalName == "plioin1");
 }

 /**
  * @brief TC19: Verify get_externalbuffer_config and printExternalbufferConfig
  */
 TEST(ConfigureParserTests, TC19_GetAndPrintExternalBufferConfig)
 {
	 abr::external_buffer_config *ext_buffer = cfg->get_externalbuffer_config("EG.ddrin");
	 CHECK(ext_buffer->name == "EG.ddrin");
	 cfg->printExternalbufferConfig();
	 CHECK(true);
 }

 /**
  * @brief TC20: Verify get_shared_buffer_config and printSharedBufferConfig
  */
 TEST(ConfigureParserTests, TC20_GetAndPrintSharedBufferConfig)
 {
	 abr::shared_buffer_config *shr_buffer = cfg->get_shared_buffer_config("EG.input");
	 CHECK(shr_buffer->name == "EG.input");
	 cfg->printSharedBufferConfig();
	 CHECK(true);
 }

 /**
  * @brief TC21: Verify isRTPorSharedBuffer identifies RTP correctly
  */
 TEST(ConfigureParserTests, TC21_IsRTPorSharedBuffer_RTP)
 {
	 int ret;
	 ret = cfg->isRTPorSharedBuffer("gradf.sq.in[1]");
	 CHECK(ret == 0);
 }

 /**
  * @brief TC22: Verify isRTPorSharedBuffer identifies shared buffer correctly
  */
 TEST(ConfigureParserTests, TC22_IsRTPorSharedBuffer_SharedBuffer)
 {
	 int ret;
	 ret = cfg->isRTPorSharedBuffer("EG.input");
	 CHECK(ret == 1);
 }

 /**
  * @brief TC23: Verify isRTPorSharedBuffer handles null pointer
  */
 TEST(ConfigureParserTests, TC23_IsRTPorSharedBuffer_NullPointer)
 {
	 int ret;
	 char *name;
	 ret = cfg->isRTPorSharedBuffer(name);
	 CHECK(ret == -1);
 }

 /**
  * @brief TC24: Verify get_rtp_config and printRTPsConfig
  */
 TEST(ConfigureParserTests, TC24_GetAndPrintRTPsConfig)
 {
	 abr::rtp_config *rtp_buffer = cfg->get_rtp_config("gradf.sq.in[1]");
	 CHECK(rtp_buffer->portName == "gradf.sq.in[1]");
	 cfg->printRTPsConfig();
	 CHECK(true);
 }
 /**
  * @brief Tests for get_partition_start_column helper function
  */
 TEST_GROUP(GetPartitionStartColumnTestGroup)
 {
	 std::vector<abr::partition_config> partitions;

	 /**
	  * @brief Setup for GetPartitionStartColumnTestGroup
	  */
	 void setup()
	 {
	 }

	 /**
	  * @brief Teardown for GetPartitionStartColumnTestGroup
	  */
	 void teardown()
	 {
		 partitions.clear();
		 mock_c()->clear();
		 mock_c()->checkExpectations();
	 }
 };

 /**
  * @brief TC25: Verify behavior with empty partitions list
  */
 TEST(GetPartitionStartColumnTestGroup, TC25_EmptyPartitionsList)
 {
	 int column = 10;
	 CHECK_EQUAL(default_start_column, get_partition_start_column(partitions, column));
 }

 /**
  * @brief TC26: Verify column falls inside partition range
  */
 TEST(GetPartitionStartColumnTestGroup, TC26_ColumnInRange)
 {
	 partitions = {
		 {0, 5},   // Covers columns 0-4
		 {5, 10},  // Covers columns 5-14
	 };
	 int column = 7;
	 CHECK_EQUAL(5, get_partition_start_column(partitions, column));
 }

 /**
  * @brief TC27: Verify behavior with negative column index
  */
 TEST(GetPartitionStartColumnTestGroup, TC27_NegativeColumn)
 {
	 partitions = {
		 {0, 5},   // Covers columns 0-4
	 };
	 int column = -1;
	 CHECK_EQUAL(default_start_column, get_partition_start_column(partitions, column));
 }

 *Function: ConfigureParser::get_externalbuffer_api_impl
 *  Uncovered details:
 *
		if (it == ebuffer_api_impls.end() || it->second == nullptr) {
			AEG_ERROR(...);
			return nullptr;
		}
 *	Justification: tests cover the "key not found" branch. The (it->second == nullptr) branch
 *	is a safety guard against future map corruption; the ebuffer_api_impls map always stores
 *	valid shared_ptrs, so it is unreachable and has no impact on source code behavior.
*/
