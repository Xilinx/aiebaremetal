/**
 * @file test_xaiemem.cpp
 * @brief Unit tests for the XAieMem class
 *
 * This file contains unit tests for the XAieMem class, which provides memory management
 * functionality for AI Engine devices. The tests verify memory allocation, freeing,
 * synchronization, and device address retrieval functionality.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport_c.h"
#include "xaiemem.h"

/**
 * @brief Test group for XAieMem class functionality
 *
 * This test group verifies the functionality of the XAieMem class, which provides
 * a C++ wrapper for the AIE memory management functions. The tests in this group
 * validate memory allocation, freeing, device synchronization, and device address
 * retrieval operations.
 */
TEST_GROUP(XAieMemTests)
{
	// Test fixtures
	XAie_DevInst devInst;
	XAieMem* memInstance;

	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Initializes the device instance with a pattern (0xaa) and creates a new
	 * XAieMem instance to be used in the tests.
	 */
	void setup()
	{
		// Initialize test fixtures
		devInst.NumRows = 8;
		devInst.StartCol = 0;
		devInst.NumCols = 4;

		// Create memory instance
		memInstance = new XAieMem(&devInst);
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Deletes the XAieMem instance and clears any mock expectations
	 * to prepare for the next test.
	 */
	void teardown()
	{
		delete memInstance;
		// Clear any mock expectations
		mock_c()->clear();
	}
};
/**
 * @brief TC_001: Verify successful memory allocation
 *
 * This test verifies that XAieMem::malloc correctly calls XAie_MemAllocate
 * with the proper parameters and returns a valid pointer when the allocation
 * is successful. The test uses a mock to simulate a successful memory allocation.
 */
TEST(XAieMemTests, TC_001_SuccessfulMemAllocation)
{
	// SETUP MOCK RETURN VALUE
	XAie_MemInst mockMemInst;
	int  Cache = 1 ;
	void* result = nullptr;
	size_t testSize = 1024;

	// ARRANGE: Configure mock for successful allocation
	mock_c()->expectOneCall("XAie_MemAllocate")
		->withPointerParameters("DevInst", &devInst)
		->withUnsignedLongLongIntParameters("Size", testSize)
		->withIntParameters("Cache", Cache)
		->andReturnPointerValue((void *)&mockMemInst);

	// ACT: Call the function under test
	result = memInstance->malloc(testSize);

	// ASSERT: Verify allocation succeeded
	CHECK(result != nullptr);
}

/**
 * @brief TC_002: Verify memory allocation failure with zero size
 *
 * This test verifies that XAieMem::malloc returns nullptr when the allocation
 * fails. The test configures the mock to return nullptr when called with a
 * zero size parameter, simulating an allocation failure.
 */
TEST(XAieMemTests, TC_002_MemAllocationFailure)
{
	// SETUP MOCK RETURN VALUE
	XAie_MemInst mockMemInst;
	int  Cache = 1 ;
	void* result = nullptr;
	size_t testSize = 0;

	// ARRANGE: Configure stub for zero size allocation
	mock_c()->expectOneCall("XAie_MemAllocate")
		->withPointerParameters("DevInst", &devInst)
		->withUnsignedLongLongIntParameters("Size", testSize)
		->withIntParameters("Cache", Cache)
		->andReturnPointerValue(nullptr);

	// ACT: Call the function under test
	result = memInstance->malloc(testSize);

	// ASSERT: Verify result is nullptr
	CHECK(result == nullptr);
}

/**
 * @brief TC_003: Verify memory allocation with null device instance
 *
 * This test verifies that XAieMem::malloc handles the case when the device
 * instance is null (all zeros). The test expects the malloc function to
 * return nullptr when XAie_MemAllocate fails due to an invalid device instance.
 */
TEST(XAieMemTests, TC_003_NullDevInstForMemAllocation)
{
	// SETUP MOCK RETURN VALUE
	XAie_MemInst mockMemInst;
	int  Cache = 1 ;
	void* result = nullptr;
	size_t testSize = 100;
	memset(&devInst, 0x0, sizeof(XAie_DevInst));

	// ARRANGE: Configure mock for Null device Instance allocation
	mock_c()->expectOneCall("XAie_MemAllocate")
		->withPointerParameters("DevInst", &devInst)
		->withUnsignedLongLongIntParameters("Size", testSize)
		->withIntParameters("Cache", Cache)
		->andReturnPointerValue(nullptr);

	// ACT: Call the function under test
	result = memInstance->malloc(testSize);

	// ASSERT: Verify result is nullptr
	CHECK(result == nullptr);
}

/**
 * @brief TC_004: Verify successful memory free operation
 *
 * This test verifies that XAieMem::free correctly calls XAie_MemFreeVAddr
 * with the proper parameters and returns XAIE_OK when the free operation
 * is successful. The test uses a mock to simulate a successful memory free.
 */
TEST(XAieMemTests, TC_004_SuccessfulFree)
{
	// SETUP MOCK RETURN VALUE
	void *TestAddress;
	TestAddress = malloc(1024);

	// ARRANGE: Configure mock for successful Free operation
	mock_c()->expectOneCall("XAie_MemFreeVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->andReturnIntValue(XAIE_OK);

	// ACT: Call the function under test
	int result = memInstance->free(TestAddress);

	// ASSERT: Verify free operation succeeded
	CHECK_EQUAL(XAIE_OK, result);

	// Clean up
	free(TestAddress);
}

/**
 * @brief TC_005: Verify return value passing from XAie_MemFreeVAddr to XAieMem::free
 *
 * This test verifies that XAieMem::free correctly passes through the return value
 * from XAie_MemFreeVAddr. It configures the mock to return a specific test value
 * and confirms that the free method returns exactly the same value.
 */
TEST(XAieMemTests, TC_005_ReturnValueCheck)
{
	// SETUP MOCK RETURN VALUE
	void *TestAddress;
	TestAddress = malloc(1024);
	int testReturnValue = 42; // Some unique value for testing

	// ARRANGE: Configure mock for successful Free operation
	mock_c()->expectOneCall("XAie_MemFreeVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->andReturnIntValue(testReturnValue);

	// ACT: Call the function under test
	int result = memInstance->free(TestAddress);

	// ASSERT: Verify the return value is correctly passed through
	CHECK_EQUAL(testReturnValue, result);

	// Clean up
	free(TestAddress);
}

/**
 * @brief TC_006: Verify free with null device instance
 *
 * This test verifies that XAieMem::free handles the case when the device
 * instance is null (all zeros). The test expects the free function to
 * return XAIE_ERR when XAie_MemFreeVAddr fails due to an invalid device instance.
 */
TEST(XAieMemTests, TC_006_NullDevInstForFree)
{
	// SETUP MOCK RETURN VALUE
	void *TestAddress;
	TestAddress = malloc(1024);
	memset(&devInst, 0x0, sizeof(XAie_DevInst));

	// ARRANGE: Configure mock for Null device Instance allocation
	mock_c()->expectOneCall("XAie_MemFreeVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->andReturnIntValue(XAIE_ERR);

	// ACT: Call the function under test
	int result = memInstance->free(TestAddress);

	// ASSERT: Verify the result
	CHECK_EQUAL(XAIE_ERR, result);

	// Clean up
	free(TestAddress);
}

/**
 * @brief TC_007: Verify successful sync_for_cpu operation
 *
 * This test verifies that XAieMem::sync_for_cpu correctly calls XAie_MemSyncForCPUVAddr
 * with the proper parameters and returns XAIE_OK when the sync operation is successful.
 * The test uses a mock to simulate a successful memory synchronization.
 */
TEST(XAieMemTests, TC_007_SuccessfulSync)
{
	// SETUP MOCK RETURN VALUE
	void *TestAddress;
	TestAddress = malloc(1024);
	uint64_t testSize = 1024;
	memset(&devInst, 0xaa, sizeof(XAie_DevInst));

	// ARRANGE: Configure mock for successful sync for cpu addr allocation
	mock_c()->expectOneCall("XAie_MemSyncForCPUVAddr")
		->ignoreOtherParameters()
		->withPointerParameters("VAddr", TestAddress)
		->withUnsignedLongLongIntParameters("size", testSize)
		->andReturnIntValue(XAIE_OK);

	// ACT: Call the function under test
	int result = memInstance->sync_for_cpu(TestAddress, testSize);

	// ASSERT: Verify synchronization succeeded
	CHECK_EQUAL(XAIE_OK, result);

	// Clean up
	free(TestAddress);
}

/**
 * @brief TC_008: Verify sync_for_cpu with null device instance
 *
 * This test verifies that XAieMem::sync_for_cpu handles the case when the device
 * instance is null (all zeros). The test expects the sync_for_cpu function to
 * return XAIE_ERR when XAie_MemSyncForCPUVAddr fails due to an invalid device instance.
 */
TEST(XAieMemTests, TC_008_NullDevInstForSync)
{
	// SETUP MOCK RETURN VALUE
	void *TestAddress;
	TestAddress = malloc(1024);
	uint64_t testSize = 1024;
	memset(&devInst, 0x0, sizeof(XAie_DevInst));

	// ARRANGE: Configure mock for Null device Instance allocation
	mock_c()->expectOneCall("XAie_MemSyncForCPUVAddr")
		->ignoreOtherParameters()
		->withPointerParameters("VAddr", TestAddress)
		->withUnsignedLongLongIntParameters("size", testSize)
		->andReturnIntValue(XAIE_ERR);

	// ACT: Call the function under test
	int result = memInstance->sync_for_cpu(TestAddress, testSize);

	// ASSERT: Verify the result
	CHECK_EQUAL(XAIE_ERR, result);

	// Clean up
	free(TestAddress);
}

/**
 * @brief TC_009: Verify successful sync_for_dev operation
 *
 * This test verifies that XAieMem::sync_for_dev correctly calls XAie_MemSyncForDevVAddr
 * with the proper parameters and returns XAIE_OK when the sync operation is successful.
 * The test uses a mock to simulate a successful memory synchronization for device access.
 */
TEST(XAieMemTests, TC_009_SuccessfulSyncForDev)
{
	// SETUP MOCK RETURN VALUE
	void *TestAddress;
	TestAddress = malloc(1024);
	uint64_t testSize = 1024;
	memset(&devInst, 0xaa, sizeof(XAie_DevInst));

	// ARRANGE: Configure mock for successful sync for dev addr allocation
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->withUnsignedLongLongIntParameters("size", testSize)
		->andReturnIntValue(XAIE_OK);

	// ACT: Call the function under test
	int result = memInstance->sync_for_dev(TestAddress, testSize);

	// ASSERT: Verify synchronization succeeded
	CHECK_EQUAL(XAIE_OK, result);

	// Clean up
	free(TestAddress);
}

/**
 * @brief TC_010: Verify sync_for_dev with null device instance
 *
 * This test verifies that XAieMem::sync_for_dev handles the case when the device
 * instance is null (all zeros). The test expects the sync_for_dev function to
 * return XAIE_ERR when XAie_MemSyncForDevVAddr fails due to an invalid device instance.
 */
TEST(XAieMemTests, TC_010_NullDevInstForDev)
{
	// SETUP MOCK RETURN VALUE
	void *TestAddress;
	TestAddress = malloc(1024);
	uint64_t testSize = 1024;
	memset(&devInst, 0x0, sizeof(XAie_DevInst));


	// ARRANGE: Configure mock for Null device Instance allocation
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->withUnsignedLongLongIntParameters("size", testSize)
		->andReturnIntValue(XAIE_ERR);

	// ACT: Call the function under test
	int result = memInstance->sync_for_dev(TestAddress, testSize);

	// ASSERT: Verify the result
	CHECK_EQUAL(XAIE_ERR, result);

	// Clean up
	free(TestAddress);
}

/**
 * @brief TC_011: Verify successful get_dev_addr operation
 *
 * This test verifies that XAieMem::get_dev_addr correctly calls XAie_MemGetDevAddrFromVAddr
 * with the proper parameters and returns XAIE_OK when the address retrieval is successful.
 * The test configures the mock to simulate successful device address retrieval.
 */
TEST(XAieMemTests, TC_011_SuccessfulGetDevAddr)
{
	void *TestAddress;
	uint64_t *DevAddr;

	// Allocate memory for TestAddress and DevAddr
	TestAddress = malloc(1024);
	CHECK(TestAddress != nullptr); // Verify memory allocation success
	DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	CHECK(DevAddr != nullptr);

	devInst.NumRows = 2;

	// ARRANGE: Configure mock
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->withPointerParameters("DevAddr", DevAddr)
		->andReturnIntValue(XAIE_OK);

	// ACT: Call the function under test
	int result = memInstance->get_dev_addr(TestAddress, DevAddr);

	// ASSERT: Verify response
	CHECK_EQUAL(XAIE_OK, result);

	// Free allocated memory
	free(TestAddress);
	free(DevAddr);
}

/**
 * @brief TC_012: Verify get_dev_addr with null device instance
 *
 * This test verifies that XAieMem::get_dev_addr handles the case when the device
 * instance is null (all zeros). The test expects the get_dev_addr function to
 * return XAIE_ERR when XAie_MemGetDevAddrFromVAddr fails due to an invalid device instance.
 */
TEST(XAieMemTests, TC_012_NullDevInstGetDevAddr)
{
	void *TestAddress;
	uint64_t *DevAddr;

	// Allocate memory for TestAddress and DevAddr
	TestAddress = malloc(1024);
	CHECK(TestAddress != nullptr); // Verify memory allocation success
	DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	CHECK(DevAddr != nullptr);

	// Mock device instance with null memory
	memset(&devInst, 0x0, sizeof(XAie_DevInst));

	// ARRANGE: Configure mock
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->withPointerParameters("DevInst", &devInst)
		->withPointerParameters("VAddr", TestAddress)
		->withPointerParameters("DevAddr", DevAddr) // Add OutputParam to mock
		->andReturnIntValue(XAIE_ERR);

	// ACT: Call the function under test
	int result = memInstance->get_dev_addr(TestAddress, DevAddr);

	// ASSERT: Verify error return
	CHECK_EQUAL(XAIE_ERR, result);

	// Free allocated memory
	free(TestAddress);
	free(DevAddr);
}
