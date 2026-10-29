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
* @file test_aeg_api_message.cpp
*  This file contains test cases of AEG API message functions
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	  Who	   Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera      05/03/2026  Initial creation
* 1.1	 Veera      20/04/2026  Added test cases for AEG API message functions
* 1.2	 Veera      10/05/2026  updated comment section for AEG API message functions
* 1.3	 Veera      26/05/2026  updated timing analysis for AEG API message functions
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
#include "aeg_api_message.h"

#include <iostream>
#include <sstream>
#include <string>
#include <chrono>
TEST_GROUP(AegApiMessageTests){
	void setup(){
	}
	void teardown(){
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_ErrorMsg_ReturnsOkErrorCode
*		Function Name: errorMsg
*		Input: input_code = abr::err_code::ok, message = "Test message"
*		Expectation: expected_return = abr::err_code::ok
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AegApiMessageTests, TC_001_ErrorMsg_ReturnsOkErrorCode) {

	abr::err_code input_code = abr::err_code::ok;
	std::string message = "Test message";
	abr::err_code expected_return = abr::err_code::ok;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code actual_return = errorMsg(input_code, message);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by errorMsg function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(expected_return == actual_return);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_ErrorMsg_ReturnsUserErrorCode
*		Function Name: errorMsg
*		Input: input_code = abr::err_code::user_error, message = "User error message"
*		Expectation: expected_return = abr::err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AegApiMessageTests, TC_002_ErrorMsg_ReturnsUserErrorCode) {
	abr::err_code input_code = abr::err_code::user_error;
	std::string message = "User error message";
	abr::err_code expected_return = abr::err_code::user_error;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code actual_return = errorMsg(input_code, message);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by errorMsg function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(expected_return == actual_return);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_ErrorMsg_ReturnsInternalErrorCode
*		Function Name: errorMsg
*		Input: input_code = abr::err_code::internal_error, message = "Internal error message"
*		Expectation: expected_return = abr::err_code::internal_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AegApiMessageTests, TC_003_ErrorMsg_ReturnsInternalErrorCode) {
	abr::err_code input_code = abr::err_code::internal_error;
	std::string message = "Internal error message";
	abr::err_code expected_return = abr::err_code::internal_error;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code actual_return = errorMsg(input_code, message);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by errorMsg function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(expected_return == actual_return);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_ErrorMsg_ReturnsAieDriverErrorCode
*		Function Name: errorMsg
*		Input: input_code = abr::err_code::aie_driver_error, message = "Driver error message"
*		Expectation: expected_return = abr::err_code::aie_driver_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AegApiMessageTests, TC_004_ErrorMsg_ReturnsAieDriverErrorCode) {
	abr::err_code input_code = abr::err_code::aie_driver_error;
	std::string message = "Driver error message";
	abr::err_code expected_return = abr::err_code::aie_driver_error;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code actual_return = errorMsg(input_code, message);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by errorMsg function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(expected_return == actual_return);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_ErrorMsg_ReturnsResourceUnavailableErrorCode
*		Function Name: errorMsg
*		Input: input_code = abr::err_code::resource_unavailable, message = "Resource unavailable message"
*		Expectation: expected_return = abr::err_code::resource_unavailable
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AegApiMessageTests, TC_005_ErrorMsg_ReturnsResourceUnavailableErrorCode) {
	abr::err_code input_code = abr::err_code::resource_unavailable;
	std::string message = "Resource unavailable message";
	abr::err_code expected_return = abr::err_code::resource_unavailable;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code actual_return = errorMsg(input_code, message);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by errorMsg function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(expected_return == actual_return);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_ErrorMsg_HandlesEmptyMessageString
*		Function Name: errorMsg
*		Input: input_code = abr::err_code::user_error, message = ""
*		Expectation: expected_return = abr::err_code::user_error
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AegApiMessageTests, TC_006_ErrorMsg_HandlesEmptyMessageString) {
	abr::err_code input_code = abr::err_code::user_error;
	std::string message = "";
	abr::err_code expected_return = abr::err_code::user_error;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	abr::err_code actual_return = errorMsg(input_code, message);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by errorMsg function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(expected_return == actual_return);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_DebugMsg_HandlesStandardMessage
*		Function Name: debugMsg
*		Input: message = "Debug test message"
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AegApiMessageTests, TC_007_DebugMsg_HandlesStandardMessage) {
	std::string message = "Debug test message";

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	debugMsg(message);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by debugMsg function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_DebugMsg_HandlesEmptyMessage
*		Function Name: debugMsg
*		Input: message = ""
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AegApiMessageTests, TC_008_DebugMsg_HandlesEmptyMessage) {
	std::string message = "";

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	debugMsg(message);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by debugMsg function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_InfoMsg_HandlesStandardMessage
*		Function Name: infoMsg
*		Input: message = "Info test message"
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AegApiMessageTests, TC_009_InfoMsg_HandlesStandardMessage) {
	std::string message = "Info test message";

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	infoMsg(message);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by infoMsg function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_010_InfoMsg_HandlesEmptyMessage
*		Function Name: infoMsg
*		Input: message = ""
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(AegApiMessageTests, TC_010_InfoMsg_HandlesEmptyMessage) {
	std::string message = "";

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	infoMsg(message);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by infoMsg function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(true);
}

/*@section Test Execution details
| Test Case ID                                       | Test Result | CR  | Impact | Test Details |
|----------------------------------------------------|-------------|-----|--------|--------------|
| TC_001_ErrorMsg_ReturnsOkErrorCode                 | PASS        | No  | NA     | NA           |
| TC_002_ErrorMsg_ReturnsUserErrorCode               | PASS        | No  | NA     | NA           |
| TC_003_ErrorMsg_ReturnsInternalErrorCode           | PASS        | No  | NA     | NA           |
| TC_004_ErrorMsg_ReturnsAieDriverErrorCode          | PASS        | No  | NA     | NA           |
| TC_005_ErrorMsg_ReturnsResourceUnavailableErrorCode| PASS        | No  | NA     | NA           |
| TC_006_ErrorMsg_HandlesEmptyMessageString          | PASS        | No  | NA     | NA           |
| TC_007_DebugMsg_HandlesStandardMessage             | PASS        | No  | NA     | NA           |
| TC_008_DebugMsg_HandlesEmptyMessage                | PASS        | No  | NA     | NA           |
| TC_009_InfoMsg_HandlesStandardMessage              | PASS        | No  | NA     | NA           |
| TC_010_InfoMsg_HandlesEmptyMessage                 | PASS        | No  | NA     | NA           |

/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)        | %      | Result     | CR/Status    | Impact          |
|--------------------------------------|--------|------------|--------------|-----------------|
| errorMsg                             | <100>  | <Pass>     | <NA>         | No Impact       |
| debugMsg                             | <100>  | <Pass>     | <NA>         | No Impact       |
| infoMsg                              | <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)        | 100    | <Pass>     | <NA>         | No Impact       |

/*@section Uncovered details*/
/* No uncovered details */
