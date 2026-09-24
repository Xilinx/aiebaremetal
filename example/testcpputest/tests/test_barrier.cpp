/**
 * @file test_barrier.cpp
 * @brief Unit tests for WRITE_ONCE and READ_ONCE barrier macros
 *
 * This file contains unit tests for the WRITE_ONCE and READ_ONCE barrier macros.
 * The tests verify that these macros correctly store and read values for different
 * data types (int and float) using both value and pointer parameters.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport_c.h"
#include "barrier.h"

/**
 * @brief Test group for WRITE_ONCE macro functionality
 *
 * This test group verifies the functionality of the WRITE_ONCE macro, which provides
 * memory barrier semantics for write operations. The tests exercise WRITE_ONCE with
 * integer and float types using both direct variables and pointers.
 */
TEST_GROUP(WriteOnceTests)
{
	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Initializes test fixtures before each test case execution.
	 */
	void setup()
	{
		// Per-test initialization
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Cleans up test fixtures after each test case execution.
	 */
	void teardown()
	{
		// Per-test cleanup
		mock_c()->clear();
	}
};
/**
 * @brief TC_001: Verify WRITE_ONCE with value parameter for integer type
 *
 * This test verifies that WRITE_ONCE correctly assigns a new integer value when
 * the destination is provided as a plain integer variable.
 */
TEST(WriteOnceTests, TC_001_WriteOnceIntegerValue)
{
	// ARRANGE: Set up inputs
	int test_variable = 0;
	int new_value = 42;

	// ACT: Call the function under test
	WRITE_ONCE(test_variable, new_value);

	// ASSERT: Verify the value was written correctly
	CHECK_EQUAL(new_value, test_variable);
}

/**
 * @brief TC_002: Verify WRITE_ONCE with pointer parameter for integer type
 *
 * This test verifies that WRITE_ONCE correctly assigns a new integer value when
 * the destination is provided as a pointer to an integer.
 */
TEST(WriteOnceTests, TC_002_WriteOnceIntegerPtr)
{
	// ARRANGE: Set up inputs
	int test_variable = 0;
	int* test_ptr = &test_variable;
	int new_value = 42;

	// ACT: Call the function under test
	WRITE_ONCE(test_ptr, new_value);

	// ASSERT: Verify the value was written correctly
	CHECK_EQUAL(new_value, *test_ptr);
}

/**
 * @brief TC_003: Verify WRITE_ONCE with value parameter for float type
 *
 * This test verifies that WRITE_ONCE correctly assigns a new float value when
 * the destination is provided as a plain float variable.
 */
TEST(WriteOnceTests, TC_003_WriteOnceFloatValue)
{
	// ARRANGE: Set up inputs
	float test_variable = 0.0f;
	float new_value = 3.14159f;

	// ACT: Call the function under test
	WRITE_ONCE(test_variable, new_value);

	// ASSERT: Verify the value was written correctly
	DOUBLES_EQUAL(new_value, test_variable, 0.0001);
}

/**
 * @brief TC_004: Verify WRITE_ONCE with pointer parameter for float type
 *
 * This test verifies that WRITE_ONCE correctly assigns a new float value when
 * the destination is provided as a pointer to a float.
 */
TEST(WriteOnceTests, TC_004_WriteOnceFloatPtr)
{
	// ARRANGE: Set up inputs
	float test_variable = 0.0f;
	float* test_ptr = &test_variable;
	float new_value = 3.14159f;

	// ACT: Call the function under test
	WRITE_ONCE(test_ptr, new_value);

	// ASSERT: Verify the value was written correctly
	DOUBLES_EQUAL(new_value, *test_ptr, 0.0001);
}

/**
 * @brief Test group for READ_ONCE macro functionality
 *
 * This test group verifies the functionality of the READ_ONCE macro, which provides
 * memory barrier semantics for read operations. The tests exercise READ_ONCE with
 * integer and float types using both direct variables and pointers.
 */
TEST_GROUP(ReadOnceTests)
{
	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Initializes test fixtures before each test case execution.
	 */
	void setup()
	{
		// Per-test initialization
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Cleans up test fixtures after each test case execution.
	 */
	void teardown()
	{
		// Per-test cleanup
		mock_c()->clear();
	}
};
/**
 * @brief TC_001: Verify READ_ONCE with value parameter for integer type
 *
 * This test verifies that READ_ONCE returns the same integer value when reading
 * from a plain integer variable.
 */
TEST(ReadOnceTests, TC_001_ReadOnceIntegerValue)
{
	// ARRANGE: Set up inputs
	int test_value = 42;

	// ACT: Call the function under test
	int result = READ_ONCE(test_value);

	// ASSERT: Verify the outcome is as expected
	CHECK_EQUAL(test_value, result);
}

/**
 * @brief TC_002: Verify READ_ONCE with pointer parameter for integer type
 *
 * This test verifies that READ_ONCE returns the same integer value when reading
 * from a pointer to an integer.
 */
TEST(ReadOnceTests, TC_002_ReadOnceIntegerPtr)
{
	// ARRANGE: Set up inputs
	int test_value = 42;
	int* test_ptr = &test_value;

	// ACT: Call the function under test
	int result = READ_ONCE(test_ptr);

	// ASSERT: Verify the outcome is as expected
	CHECK_EQUAL(test_value, result);
}

/**
 * @brief TC_003: Verify READ_ONCE with value parameter for float type
 *
 * This test verifies that READ_ONCE returns the same float value when reading
 * from a plain float variable, within a small tolerance.
 */
TEST(ReadOnceTests, TC_003_ReadOnceFloatValue)
{
	// ARRANGE: Set up inputs
	float test_value = 3.14159f;

	// ACT: Call the function under test
	float result = READ_ONCE(test_value);

	// ASSERT: Verify the outcome is as expected
	DOUBLES_EQUAL(test_value, result, 0.0001);
}

/**
 * @brief TC_004: Verify READ_ONCE with pointer parameter for float type
 *
 * This test verifies that READ_ONCE returns the same float value when reading
 * from a pointer to a float, within a small tolerance.
 */
TEST(ReadOnceTests, TC_004_ReadOnceFloatPtr)
{
	// ARRANGE: Set up inputs
	float test_value = 3.14159f;
	float* test_ptr = &test_value;

	// ACT: Call the function under test
	float result = READ_ONCE(test_ptr);

	// ASSERT: Verify the outcome is as expected
	DOUBLES_EQUAL(test_value, result, 0.0001);
}

/**
 * @brief TC_005: Verify READ_ONCE with pointer parameter for unsigned int type
 *
 * This test verifies that READ_ONCE returns the same unsigned int value when reading
 * from a pointer to an unsigned int.
 */
TEST(ReadOnceTests, TC_005_ReadOnceUnsignedIntPtr)
{
	// ARRANGE: Set up inputs
	unsigned int test_value = 0xDEADBEEF;
	unsigned int* test_ptr = &test_value;

	// ACT: Call the function under test
	unsigned int result = READ_ONCE(test_ptr);

	// ASSERT: Verify the outcome is as expected
	CHECK_EQUAL(test_value, result);
}
