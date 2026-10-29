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
* @file test_barrier.cpp
*  This file contains test cases for WRITE_ONCE and READ_ONCE barrier macros
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	 Who	 Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera	 05/03/2026	 Initial creation
* 1.1	 Veera   10/05/2026  updated comment section for WRITE_ONCE and READ_ONCE barrier macros
* 1.2	 Veera   26/05/2026  updated timing analysis for WRITE_ONCE and READ_ONCE barrier macros
*</pre>
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
#include "barrier.h"
#include <chrono>
#include <iostream>
TEST_GROUP(WriteOnceTests){
	void setup(){

	}

	void teardown(){
		mock_c()->clear();
	}
}
;

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_WriteOnceIntegerValue
*		Function Name: WRITE_ONCE
*		Input: test_variable = 0, new_value = 42
*		Expectation: test_variable == new_value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/

TEST(WriteOnceTests, TC_001_WriteOnceIntegerValue) {
	int test_variable = 0;
	int new_value = 42;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	WRITE_ONCE(test_variable, new_value);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by WRITE_ONCE function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(new_value, test_variable);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_WriteOnceIntegerPtr
*		Function Name: WRITE_ONCE
*		Input: test_ptr = &test_variable, new_value = 42
*		Expectation: *test_ptr == new_value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(WriteOnceTests, TC_002_WriteOnceIntegerPtr) {
	int test_variable = 0;
	int *test_ptr = &test_variable;
	int new_value = 42;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	WRITE_ONCE(test_ptr, new_value);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by WRITE_ONCE function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(new_value, *test_ptr);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_WriteOnceFloatValue
*		Function Name: WRITE_ONCE
*		Input: test_variable = 0.0f, new_value = 3.14159f
*		Expectation: test_variable == new_value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(WriteOnceTests, TC_003_WriteOnceFloatValue) {
	float test_variable = 0.0f;
	float new_value = 3.14159f;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	WRITE_ONCE(test_variable, new_value);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by WRITE_ONCE function: " << total_clock_enable_us << " microseconds" << std::endl;
	DOUBLES_EQUAL(new_value, test_variable, 0.0001);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_WriteOnceFloatPtr
*		Function Name: WRITE_ONCE
*		Input: test_ptr = &test_variable, new_value = 3.14159f
*		Expectation: *test_ptr == new_value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(WriteOnceTests, TC_004_WriteOnceFloatPtr) {
	float test_variable = 0.0f;
	float *test_ptr = &test_variable;
	float new_value = 3.14159f;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	WRITE_ONCE(test_ptr, new_value);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by WRITE_ONCE function: " << total_clock_enable_us << " microseconds" << std::endl;
	DOUBLES_EQUAL(new_value, *test_ptr, 0.0001);
}

TEST_GROUP(ReadOnceTests){
	void setup(){
	}
	void teardown(){
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_ReadOnceIntegerValue
*		Function Name: READ_ONCE
*		Input: test_value = 42
*		Expectation: result == test_value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ReadOnceTests, TC_005_ReadOnceIntegerValue) {
	int test_value = 42;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = READ_ONCE(test_value);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by READ_ONCE function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(test_value, result);
}


//*********************************************************************************/
/*
*		Unique test case ID: TC_006_ReadOnceIntegerPtr
*		Function Name: READ_ONCE
*		Input: test_ptr = &test_value
*		Expectation: result == *test_ptr
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ReadOnceTests, TC_006_ReadOnceIntegerPtr) {
	int test_value = 42;
	int *test_ptr = &test_value;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = READ_ONCE<int>(test_ptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by READ_ONCE function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(test_value, result);
}


//*********************************************************************************/
/*
*		Unique test case ID: TC_007_ReadOnceFloatValue
*		Function Name: READ_ONCE
*		Input: test_value = 3.14159f
*		Expectation: result == test_value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ReadOnceTests, TC_007_ReadOnceFloatValue) {
	float test_value = 3.14159f;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	float result = READ_ONCE(test_value);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by READ_ONCE function: " << total_clock_enable_us << " microseconds" << std::endl;
	DOUBLES_EQUAL(test_value, result, 0.0001);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_ReadOnceFloatPtr
*		Function Name: READ_ONCE
*		Input: test_ptr = &test_value
*		Expectation: result == test_value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ReadOnceTests, TC_008_ReadOnceFloatPtr) {
	float test_value = 3.14159f;
	float *test_ptr = &test_value;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	float result = READ_ONCE<float>(test_ptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by READ_ONCE function: " << total_clock_enable_us << " microseconds" << std::endl;
	DOUBLES_EQUAL(test_value, result, 0.0001);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_ReadOnceUnsignedIntPtr
*		Function Name: READ_ONCE
*		Input: test_ptr = &test_value
*		Expectation: result == test_value
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(ReadOnceTests, TC_009_ReadOnceUnsignedIntPtr) {
	unsigned int test_value = 0xDEADBEEF;
	unsigned int *test_ptr = &test_value;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	unsigned int result = READ_ONCE<unsigned int>(test_ptr);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by READ_ONCE function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(test_value, result);
}

/*@section Test Execution details
| Test Case ID                         | Test Result | CR  | Impact | Test Details |
|--------------------------------------|-------------|-----|--------|--------------|
| TC_001_WriteOnceIntegerValue         | PASS        | No  | NA     | NA           |
| TC_002_WriteOnceIntegerPtr           | PASS        | No  | NA     | NA           |
| TC_003_WriteOnceFloatValue           | PASS        | No  | NA     | NA           |
| TC_004_WriteOnceFloatPtr             | PASS        | No  | NA     | NA           |
| TC_005_ReadOnceIntegerValue          | PASS        | No  | NA     | NA           |
| TC_006_ReadOnceIntegerPtr            | PASS        | No  | NA     | NA           |
| TC_007_ReadOnceFloatValue            | PASS        | No  | NA     | NA           |
| TC_008_ReadOnceFloatPtr              | PASS        | No  | NA     | NA           |
| TC_009_ReadOnceUnsignedIntPtr        | PASS        | No  | NA     | NA           |

/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)                  | Stmt % | Branch % | MC/DC % | Result | CR/Status | Impact     |
|------------------------------------------------|--------|----------|---------|--------|-----------|------------|
| READ_ONCE<float>(const float&)                 | <100>  | <100>    | <100>   | <Pass> | <NA>      | No Impact  |
| READ_ONCE<int>(const int&)                     | <100>  | <100>    | <100>   | <Pass> | <NA>      | No Impact  |
| READ_ONCE<unsigned int*>(const unsigned int*&) | <100>  | <100>    | <100>   | <Pass> | <NA>      | No Impact  |
| READ_ONCE<float>(const float*)                 | <100>  | <100>    | <100>   | <Pass> | <NA>      | No Impact  |
| READ_ONCE<int>(const int*)                     | <100>  | <100>    | <100>   | <Pass> | <NA>      | No Impact  |
| READ_ONCE<unsigned int>(const unsigned int*)   | <100>  | <100>    | <100>   | <Pass> | <NA>      | No Impact  |
| WRITE_ONCE<float>(float&, float)               | <100>  | <100>    | <100>   | <Pass> | <NA>      | No Impact  |
| WRITE_ONCE<int>(int&, int)                     | <100>  | <100>    | <100>   | <Pass> | <NA>      | No Impact  |
| WRITE_ONCE<float>(float*, float)               | <100>  | <100>    | <100>   | <Pass> | <NA>      | No Impact  |
| WRITE_ONCE<int>(int*, int)                     | <100>  | <100>    | <100>   | <Pass> | <NA>      | No Impact  |
| Total file coverage (overall)                  | 100    | 100      | 100     | <Pass> | <NA>      | No Impact  |

/*@section Uncovered details*/
/* No uncovered details */
