/**
 * @file test_fal_util.cpp
 * @brief Unit tests for the FAL utility functions
 *
 * This file contains unit tests for the FAL (Functional Abstraction Layer) utility functions,
 * which provide helper functions for initializing, requesting, and releasing AIE resources.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include "fal_util.h"
#include "test_comparator.h"
#include "stubs.h"
#include <xaiegbl.h>
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport_c.h"
// Use mock XAieFAL classes for unit testing
#include "profiling_stubs/xaiefal_mock.hpp"
using namespace xaiefal;

#include <memory>

/**
 * @brief Test group for FAL utility initialization and resource management
 *
 * This test group verifies the functionality of fal_util helper functions,
 * including initialization, resource request, and resource release operations.
 */
TEST_GROUP(FalUtilInitializeTests)
{
	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Clears any existing mock expectations to ensure a clean state.
	 */
	void setup()
	{
		mock_c()->clear();
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Clears mock expectations to prepare for the next test.
	 */
	void teardown()
	{
		mock_c()->clear();
	}
};


/**
 * @brief TC_001: Verify initialization failure with NULL device instance pointer
 *
 * This test verifies that fal_util::initialize correctly handles a NULL
 * XAie_DevInst pointer and returns false to indicate failure.
 */
TEST(FalUtilInitializeTests, TC_001_InitializeWithNullPointer)
{
	// ARRANGE: Set up NULL device instance pointer
	XAie_DevInst* pDevInst = nullptr;

	// ACT: Call initialize with null pointer
	bool result = fal_util::initialize(pDevInst);

	// ASSERT: Verify initialization fails
	CHECK_FALSE(result); // Should return false for null pointer
}

/**
 * @brief TC_002: Verify successful initialization with valid device instance pointer
 *
 * This test verifies that fal_util::initialize correctly initializes the shared
 * device pointer when provided with a valid XAie_DevInst pointer.
 */
TEST(FalUtilInitializeTests, TC_002_InitializeWithValidPointer)
{
	// ARRANGE: Set up valid device instance
	XAie_DevInst dev{};

	// ACT: Call initialize with valid pointer
	bool result = fal_util::initialize(&dev);

	// ASSERT: Verify initialization succeeds and shared pointer is set
	CHECK_TRUE(result);
	CHECK(fal_util::s_pXAieDev != nullptr);
}

/**
 * @brief TC_003: Verify release failure with null resource pointer
 *
 * This test verifies that fal_util::release correctly handles a null
 * resource pointer and returns false to indicate failure.
 */
TEST(FalUtilInitializeTests, TC_003_ReleaseNullResourceFailure)
{
	// ARRANGE: Set up null resource pointer
	std::shared_ptr<xaiefal::XAieRsc> resource = nullptr;

	// ACT: Call release with null resource
	bool result = fal_util::release(resource);

	// ASSERT: Verify release fails
	CHECK_FALSE(result); // Should return false for null resource
}

/**
 * @brief TC_004: Verify successful resource release
 *
 * This test verifies that fal_util::release correctly releases a resource
 * when the underlying driver returns success.
 */
TEST(FalUtilInitializeTests, TC_004_ReleaseSuccess)
{
	// ARRANGE: Create a generic resource and configure mock for success
	auto r = StubXaie_CreateGenericRsc();
	StubXaieRsc_SetReleaseResult(r, XAIE_OK);

	// ACT: Call release
	bool result = fal_util::release(r);

	// ASSERT: Verify release succeeds and was called once
	CHECK_TRUE(result);
	LONGS_EQUAL(1, StubXaieRsc_GetReleaseCallCount(r));
}

/**
 * @brief TC_005: Verify resource release failure
 *
 * This test verifies that fal_util::release correctly handles a release
 * failure from the underlying driver and returns false.
 */
TEST(FalUtilInitializeTests, TC_005_ReleaseFailure)
{
	// ARRANGE: Create a generic resource and configure mock for failure
	auto r = StubXaie_CreateGenericRsc();
	StubXaieRsc_SetReleaseResult(r, StubXaieRc_ErrorCode());

	// ACT: Call release
	bool result = fal_util::release(r);

	// ASSERT: Verify release fails
	CHECK_FALSE(result);
}

/**
 * @brief TC_006: Verify request failure with null resource pointer
 *
 * This test verifies that fal_util::request correctly handles a null
 * resource pointer and returns -1 to indicate failure.
 */
TEST(FalUtilInitializeTests, TC_006_RequestNullResourceFailure)
{
	// ARRANGE: Set up null resource pointer
	std::shared_ptr<xaiefal::XAieRsc> resource = nullptr;

	// ACT: Call request with null resource
	int result = fal_util::request(resource);

	// ASSERT: Verify request fails
	CHECK(result == -1); // Should return -1 for null resource
}

/**
 * @brief TC_007: Verify successful broadcast resource request
 *
 * This test verifies that fal_util::request correctly requests a broadcast
 * resource and returns the broadcast ID when reservation succeeds.
 */
TEST(FalUtilInitializeTests, TC_007_RequestBroadcastSuccess)
{
	// ARRANGE: Create broadcast resource and configure mock for success
	auto bc = StubXaie_CreateBroadcast();
	StubXaieBroadcast_SetReserveResult(bc, XAIE_OK);
	StubXaieBroadcast_SetBcValue(bc, 5);

	// ACT: Call request
	int id = fal_util::request(bc);

	// ASSERT: Verify correct broadcast ID is returned
	LONGS_EQUAL(5, id);
}

/**
 * @brief TC_008: Verify broadcast resource request failure
 *
 * This test verifies that fal_util::request correctly handles a broadcast
 * reservation failure and returns -1.
 */
TEST(FalUtilInitializeTests, TC_008_RequestBroadcastFailure)
{
	// ARRANGE: Create broadcast resource and configure mock for failure
	auto bc = StubXaie_CreateBroadcast();
	StubXaieBroadcast_SetReserveResult(bc, StubXaieRc_ErrorCode());
	StubXaieBroadcast_SetBcValue(bc, 7);

	// ACT: Call request
	int id = fal_util::request(bc);

	// ASSERT: Verify request fails
	LONGS_EQUAL(-1, id);
}

/**
 * @brief TC_009: Verify successful single tile resource request
 *
 * This test verifies that fal_util::request correctly requests a single tile
 * resource and returns the resource ID when both reservation and ID retrieval succeed.
 */
TEST(FalUtilInitializeTests, TC_009_RequestSingleTileSuccess)
{
	// ARRANGE: Create single tile resource and configure mock for success
	auto r = StubXaie_CreateSingleTile();
	StubXaieSingleTile_SetReserveResult(r, XAIE_OK);
	StubXaieSingleTile_SetGetRscIdResult(r, XAIE_OK, 42);

	// ACT: Call request
	int id = fal_util::request(r);

	// ASSERT: Verify correct resource ID is returned and functions were called
	LONGS_EQUAL(42, id);
	LONGS_EQUAL(1, StubXaieSingleTile_GetReserveCallCount(r));
	LONGS_EQUAL(1, StubXaieSingleTile_GetGetRscIdCallCount(r));
}

/**
 * @brief TC_010: Verify single tile resource request failure during reservation
 *
 * This test verifies that fal_util::request correctly handles a single tile
 * reservation failure and returns -1 without attempting to get the resource ID.
 */
TEST(FalUtilInitializeTests, TC_010_RequestSingleTileReserveFailure)
{
	// ARRANGE: Create single tile resource and configure mock for reserve failure
	auto r = StubXaie_CreateSingleTile();
	StubXaieSingleTile_SetReserveResult(r, StubXaieRc_ErrorCode());

	// ACT: Call request
	int id = fal_util::request(r);

	// ASSERT: Verify request fails and GetRscId was not called
	LONGS_EQUAL(-1, id);
	LONGS_EQUAL(0, StubXaieSingleTile_GetGetRscIdCallCount(r));
}

/**
 * @brief TC_011: Verify single tile resource request failure during ID retrieval
 *
 * This test verifies that fal_util::request correctly handles a failure when
 * retrieving the resource ID and returns -1.
 */
TEST(FalUtilInitializeTests, TC_011_RequestSingleTileGetRscIdFailure)
{
	// ARRANGE: Create single tile resource and configure mock for GetRscId failure
	auto r = StubXaie_CreateSingleTile();
	StubXaieSingleTile_SetReserveResult(r, XAIE_OK);
	StubXaieSingleTile_SetGetRscIdResult(r, StubXaieRc_ErrorCode(), 13);

	// ACT: Call request
	int id = fal_util::request(r);

	// ASSERT: Verify request fails
	LONGS_EQUAL(-1, id);
}

/**
 * @brief TC_012: Verify request failure with unsupported resource type
 *
 * This test verifies that fal_util::request correctly handles an unsupported
 * resource type (generic XAieRsc) and returns -1.
 */
TEST(FalUtilInitializeTests, TC_012_RequestUnsupportedResourceType)
{
	// ARRANGE: Create a generic (unsupported) resource
	auto r = StubXaie_CreateGenericRsc();

	// ACT: Call request
	int id = fal_util::request(r);

	// ASSERT: Verify request fails
	LONGS_EQUAL(-1, id);
}
