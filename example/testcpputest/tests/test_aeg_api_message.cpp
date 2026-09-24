/**
 * @file test_aeg_api_message.cpp
 * @brief Unit tests for the AEG API message functions
 *
 * This file contains unit tests for the AEG API message functions. The tests verify
 * the functionality of error message, debug message, and information message functions
 * provided by the AEG API message interface. These functions are used throughout the
 * AIE Baremetal library to report errors, debug information, and status messages.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport_c.h"
#include "aeg_api_message.h"

#include <iostream>
#include <sstream>
#include <string>

/**
 * @brief Test group for AEG API message functions functionality
 *
 * This test group verifies the functionality of errorMsg(), debugMsg(), and infoMsg()
 * functions from the AEG API message interface. These functions are used for error
 * reporting and information display throughout the AIE Baremetal library.
 */
TEST_GROUP(AegApiMessageTests)
{
	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Initializes test fixtures before each test case execution.
	 */
	void setup()
	{
		// No setup needed for these tests
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Cleans up test fixtures after each test case execution.
	 */
	void teardown()
	{
		// No teardown needed for these tests
		mock_c()->clear();
	}
};
/**
 * @brief TC_001: Verify errorMsg() returns the 'ok' error code
 *
 * This test verifies that the errorMsg() function passes through the 'ok' error
 * code correctly when provided along with a message string. The function should
 * return the same error code that was passed to it.
 */
TEST(AegApiMessageTests, TC_001_ErrorMsg_ReturnsOkErrorCode)
{
	// ARRANGE: Set up inputs for this test case
	abr::err_code input_code = abr::err_code::ok;
	std::string message = "Test message";
	abr::err_code expected_return = abr::err_code::ok;

	// ACT: Call the function under test
	abr::err_code actual_return = errorMsg(input_code, message);

	// ASSERT: Verify the outcome is as expected
	CHECK(expected_return == actual_return);
}

/**
 * @brief TC_002: Verify errorMsg() returns the 'user_error' code
 *
 * This test verifies that the errorMsg() function passes through the 'user_error'
 * error code correctly when provided with a message string. The function should
 * return the same error code that was passed to it.
 */
TEST(AegApiMessageTests, TC_002_ErrorMsg_ReturnsUserErrorCode)
{
	// ARRANGE: Set up inputs for this test case
	abr::err_code input_code = abr::err_code::user_error;
	std::string message = "User error message";
	abr::err_code expected_return = abr::err_code::user_error;

	// ACT: Call the function under test
	abr::err_code actual_return = errorMsg(input_code, message);

	// ASSERT: Verify the outcome is as expected
	CHECK(expected_return == actual_return);
}

/**
 * @brief TC_003: Verify errorMsg() returns the 'internal_error' code
 *
 * This test verifies that the errorMsg() function passes through the 'internal_error'
 * error code correctly when provided with a message string. The function should
 * return the same error code that was passed to it.
 */
TEST(AegApiMessageTests, TC_003_ErrorMsg_ReturnsInternalErrorCode)
{
	// ARRANGE: Set up inputs for this test case
	abr::err_code input_code = abr::err_code::internal_error;
	std::string message = "Internal error message";
	abr::err_code expected_return = abr::err_code::internal_error;

	// ACT: Call the function under test
	abr::err_code actual_return = errorMsg(input_code, message);

	// ASSERT: Verify the outcome is as expected
	CHECK(expected_return == actual_return);
}

/**
 * @brief TC_004: Verify errorMsg() returns the 'aie_driver_error' code
 *
 * This test verifies that the errorMsg() function passes through the 'aie_driver_error'
 * error code correctly when provided with a message string. The function should
 * return the same error code that was passed to it.
 */
TEST(AegApiMessageTests, TC_004_ErrorMsg_ReturnsAieDriverErrorCode)
{
	// ARRANGE: Set up inputs for this test case
	abr::err_code input_code = abr::err_code::aie_driver_error;
	std::string message = "Driver error message";
	abr::err_code expected_return = abr::err_code::aie_driver_error;

	// ACT: Call the function under test
	abr::err_code actual_return = errorMsg(input_code, message);

	// ASSERT: Verify the outcome is as expected
	CHECK(expected_return == actual_return);
}

/**
 * @brief TC_005: Verify errorMsg() returns the 'resource_unavailable' code
 *
 * This test verifies that the errorMsg() function passes through the 'resource_unavailable'
 * error code correctly when provided with a message string. The function should
 * return the same error code that was passed to it.
 */
TEST(AegApiMessageTests, TC_005_ErrorMsg_ReturnsResourceUnavailableErrorCode)
{
	// ARRANGE: Set up inputs for this test case
	abr::err_code input_code = abr::err_code::resource_unavailable;
	std::string message = "Resource unavailable message";
	abr::err_code expected_return = abr::err_code::resource_unavailable;

	// ACT: Call the function under test
	abr::err_code actual_return = errorMsg(input_code, message);

	// ASSERT: Verify the outcome is as expected
	CHECK(expected_return == actual_return);
}

/**
 * @brief TC_006: Verify errorMsg() handles empty message strings
 *
 * This test verifies that the errorMsg() function properly handles an empty
 * message string while still returning the correct error code.
 */
TEST(AegApiMessageTests, TC_006_ErrorMsg_HandlesEmptyMessageString)
{
	// ARRANGE: Set up inputs for this test case
	abr::err_code input_code = abr::err_code::user_error;
	std::string message = "";
	abr::err_code expected_return = abr::err_code::user_error;

	// ACT: Call the function under test
	abr::err_code actual_return = errorMsg(input_code, message);

	// ASSERT: Verify the outcome is as expected
	CHECK(expected_return == actual_return);
}
/**
 * @brief TC_007: Verify debugMsg() handles standard messages
 *
 * This test verifies that the debugMsg() function can process a standard debug
 * message string without crashing or causing any errors.
 */
TEST(AegApiMessageTests, TC_007_DebugMsg_HandlesStandardMessage)
{
	// ARRANGE: Set up inputs for this test case
	std::string message = "Debug test message";

	// ACT: Call the function under test
	debugMsg(message);

	// ASSERT: Verify test completes without error (no crash)
}
/**
 * @brief TC_008: Verify debugMsg() handles empty messages
 *
 * This test verifies that the debugMsg() function can properly handle an empty
 * message string without crashing or causing errors.
 */
TEST(AegApiMessageTests, TC_008_DebugMsg_HandlesEmptyMessage)
{
	// ARRANGE: Set up inputs for this test case
	std::string message = "";

	// ACT: Call the function under test
	debugMsg(message);

	// ASSERT: Verify test completes without error (no crash)
}
