/**
 * @file test_gmioapiimpl.cpp
 * @brief Unit tests for the GmioApiImpl class
 *
 * This file contains unit tests for the GmioApiImpl class, which provides GMIO
 * (Global Memory I/O) functionality for AI Engine devices. The tests verify
 * memory transfers between global memory and AI Engine, including blocking and
 * non-blocking operations.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport_c.h"
#ifdef new
#undef new
#endif
#ifdef node
#undef node
#endif
#ifdef __node
#undef __node
#endif
//#define __AIESIM__
#include "gmioapiimpl.h"
#include "test_comparator.h"
using namespace abr;

/**
 * @brief Test group for GmioApiImpl class functionality
 *
 * This test group verifies the functionality of the GmioApiImpl class, which provides
 * GMIO operations for AI Engine devices. The tests validate constructor initialization,
 * wait operations, and data transfer functions in both GM2AIE and AIE2GM directions.
 */
TEST_GROUP(GmioApiImplTests)
{
	// Test fixtures
	std::shared_ptr<abr::gmio_api> api;
	std::shared_ptr<XAieMem> mem;
	abr::gmio_config gmioconfig;
	GmioApiImpl* gmio;
	XAie_LocType testLoc;
	XAie_DevInst devInst;

	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Initializes the GMIO configuration, device instance, and creates the GmioApiImpl
	 * instance. Also installs custom comparators and copiers for mocking.
	 */
	void setup()
	{
		// Setup gmio_config
		gmioconfig.name = "gradf.in";
		gmioconfig.type = abr::gmio_config::gm2aie;
		gmioconfig.id = 1;
		gmioconfig.shimColumn = 0;
		gmioconfig.channelNum = 0;
		gmioconfig.burstLength = 4;
		devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML - 1;
		abr::config_manager::s_pDevInst = &devInst;
		abr::config_manager::s_bInitialized = true;

		// Create mock objects
		testLoc.Col = 0;
		testLoc.Row = 0;
		api = std::make_shared<abr::gmio_api>(&gmioconfig);
		mem = std::make_shared<XAieMem>(&devInst);

		// Create gmio_api
		gmio = new GmioApiImpl(api, gmioconfig, mem, "test_gmio");
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installCopier("u8", u8Copier);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Deletes the GmioApiImpl instance, resets configuration manager, removes
	 * comparators and copiers, and clears mock expectations.
	 */
	void teardown()
	{
		delete gmio;
		abr::config_manager::s_pDevInst = nullptr;
		abr::config_manager::s_bInitialized = false;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};
/**
 * @brief TC_001: Verify constructor initializes members correctly
 *
 * This test verifies that the GmioApiImpl constructor properly initializes all
 * member variables. Since private members cannot be directly checked, verification
 * is done implicitly through behavior in subsequent tests.
 */
TEST(GmioApiImplTests, TC_001_ConstructorInitialization)
{
	// ASSERT: Implicit verification through setup
	CHECK(gmio != nullptr);
}

/**
 * @brief TC_002: Verify wait() calls gmio_api wait with blocking enabled
 *
 * This test verifies that the wait() function properly calls the underlying GMIO API
 * wait function when blocking is enabled (true parameter).
 */
TEST(GmioApiImplTests, TC_002_WaitCallsGmioApiWait)
{
	// ARRANGE: Configure DMA mocks
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// ACT: Configure API
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	int dir = 0x0;
	uint8_t channel = 0x1;
	mock_c()->expectOneCall("XAie_DmaWaitForDone")
		->withPointerParameters("DevInst", &devInst)
		->withParameterOfType("XAie_LocType", "Loc", &testLoc)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// ACT: Call wait function
	int result = gmio->wait(true);

	// ASSERT: Verify result
	CHECK_EQUAL(0, result);
}

/**
 * @brief TC_003: Verify gm2aie_nb() works correctly with valid parameters
 *
 * This test verifies that the non-blocking GM2AIE transfer function successfully
 * transfers data from global memory to AI Engine with valid parameters.
 */
TEST(GmioApiImplTests, TC_003_Gm2aie_NB_Success)
{
	// ARRANGE: Setup buffer and mock expectations
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::gm2aie;
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// ACT: Call the function under test
	abr::err_code result = gmio->gm2aie_nb(buffer, 100);

	// ASSERT: Verify result
	CHECK(result == err_code::ok);
}
/**
 * @brief TC_004: Verify gm2aie_nb() handles failure correctly
 *
 * This test verifies that the non-blocking GM2AIE transfer function properly handles
 * failures when XAie_MemGetDevAddrFromVAddr returns an error.
 */
TEST(GmioApiImplTests, TC_004_Gm2aie_NB_GetAddressFailure)
{
	// ARRANGE: Setup buffer and mock expectations for failure
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::gm2aie;
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// ACT: Call the function under test
	abr::err_code result = gmio->gm2aie_nb(buffer, 100);

	// ASSERT: Verify error is returned
	CHECK(result == err_code::aie_driver_error);
}

/**
 * @brief TC_005: Verify aie2gm_nb() works correctly with valid parameters
 *
 * This test verifies that the non-blocking AIE2GM transfer function successfully
 * transfers data from AI Engine to global memory with valid parameters.
 */
TEST(GmioApiImplTests, TC_005_Aie2gm_NB_Success)
{
	// ARRANGE: Setup buffer and configure API first
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::aie2gm;

	// Setup DMA configuration mocks
	u8 FirstCallOutput = 1;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// Configure API
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	// Setup mocks for aie2gm_nb call
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// Setup BD programming mocks for gmio_sync_nb
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

	// ACT: Call the function under test
	abr::err_code result = gmio->aie2gm_nb(buffer, 100);

	// ASSERT: Verify result
	CHECK(result == err_code::ok);
}
/**
 * @brief TC_006: Verify aie2gm_nb() handles memory address retrieval failure
 *
 * This test verifies that the non-blocking AIE2GM transfer function properly handles
 * failures when XAie_MemGetDevAddrFromVAddr returns an error.
 */
TEST(GmioApiImplTests, TC_006_Aie2gm_NB_GetAddressFailure)
{
	// ARRANGE: Setup buffer and mock expectations for failure
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::aie2gm;
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// ACT: Call the function under test
	abr::err_code result = gmio->aie2gm_nb(buffer, 100);

	// ASSERT: Verify error is returned
	CHECK(result == err_code::aie_driver_error);
}
/**
 * @brief TC_007: Verify aie2gm_nb() handles memory sync failure
 *
 * This test verifies that the non-blocking AIE2GM transfer function properly handles
 * failures when XAie_MemSyncForDevVAddr returns an error.
 */
TEST(GmioApiImplTests, TC_007_Aie2gm_NB_SyncFailure)
{
	// ARRANGE: Setup buffer and mock expectations for sync failure
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::aie2gm;
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(1);

	// ACT: Call the function under test
	abr::err_code result = gmio->aie2gm_nb(buffer, 100);

	// ASSERT: Verify error is returned
	CHECK(result == err_code::internal_error);
}

/**
 * @brief TC_008: Verify gm2aie() works correctly with successful operations
 *
 * This test verifies that the blocking GM2AIE transfer function successfully transfers
 * data from global memory to AI Engine when both gm2aie_nb() and wait() succeed.
 */
TEST(GmioApiImplTests, TC_008_Gm2aie_Success)
{
	// ARRANGE: Setup buffer and mock expectations
	char buffer[100] = {0};
	gmioconfig.type = abr::gmio_config::gm2aie;
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// ACT: Call the function under test
	abr::err_code result = gmio->gm2aie(buffer, 100);

	// ASSERT: Verify result
	CHECK(result == err_code::ok);
}

/**
 * @brief Test group for GmioApiImpl synchronization tests
 *
 * This test group verifies the synchronization functionality of the GmioApiImpl class,
 * including gmio_sync_nb() operations in different directions and aie2gm() blocking transfers.
 */
TEST_GROUP(GmioApiImplsyncTests)
{
	// Test fixtures
	std::shared_ptr<abr::gmio_api> api;
	std::shared_ptr<XAieMem> mem;
	abr::gmio_config gmioconfig;
	XAie_DevInst devInst;

	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Initializes the device instance, GMIO configuration, and sets up custom
	 * comparators and copiers for mocking.
	 */
	void setup()
	{
		// Setup device and config
		devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
		abr::config_manager::s_pDevInst = &devInst;
		abr::config_manager::s_bInitialized = true;
		gmioconfig.name = "gradf.in";
		gmioconfig.id = 1;
		gmioconfig.name = "test_gmio";
		gmioconfig.shimColumn = 0;
		gmioconfig.channelNum = 0;
		gmioconfig.burstLength = 4;
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		mock_c()->installCopier("u8", u8Copier);
		mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
		mem = std::make_shared<XAieMem>(&devInst);
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Removes comparators and copiers, resets memory, clears mock expectations,
	 * and validates that all expectations were met.
	 */
	void teardown()
	{
		mock_c()->removeAllComparatorsAndCopiers();
		mem.reset();
		mock_c()->clear();
		mock_c()->checkExpectations();
	}
};
/**
 * @brief TC_009: Verify gmio_sync_nb() works correctly for AIE2GM direction
 *
 * This test verifies that the non-blocking synchronization function properly handles
 * transfers from AI Engine to global memory (AIE2GM direction).
 */
TEST(GmioApiImplsyncTests, TC_009_GmioSyncNb_AIE2GM_Success)
{
	// ARRANGE: Setup GMIO config and test location
	gmioconfig.type = abr::gmio_config::gm2aie;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;

	// Setup DMA mocks
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// Configure API
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	// Setup BD mocks
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

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	// ACT: Call the function under test
	abr::err_code result = gmio.gmio_sync_nb(AIE2GM, 0x1000, 100);

	// ASSERT: Verify result
	CHECK(result == err_code::ok);
	abr::config_manager::s_pDevInst = nullptr;
	abr::config_manager::s_bInitialized = false;
	mock_c()->checkExpectations();
}
/**
 * @brief TC_010: Verify gmio_sync_nb() works correctly for GM2AIE direction
 *
 * This test verifies that the non-blocking synchronization function properly handles
 * transfers from global memory to AI Engine (GM2AIE direction).
 */
TEST(GmioApiImplsyncTests, TC_010_GmioSyncNb_GM2AIE_Success)
{
	// ARRANGE: Setup GMIO config and test location
	gmioconfig.type = abr::gmio_config::aie2gm;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;

	// Setup DMA mocks
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// Configure API
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	// Setup BD mocks
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

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	// ACT: Call the function under test
	abr::err_code result = gmio.gmio_sync_nb(GM2AIE, 0x1000, 100);

	// ASSERT: Verify result
	CHECK(result == err_code::ok);
	abr::config_manager::s_pDevInst = nullptr;
	abr::config_manager::s_bInitialized = false;
	mock_c()->checkExpectations();
}
/**
 * @brief TC_011: Verify gmio_sync_nb() handles invalid direction
 *
 * This test verifies that the non-blocking synchronization function properly handles
 * an invalid direction parameter.
 */
TEST(GmioApiImplsyncTests, TC_011_GmioSyncNb_InvalidDirection)
{
	// ARRANGE: Setup GMIO config and test location
	gmioconfig.type = abr::gmio_config::gm2aie;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;

	// Setup DMA mocks
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// Configure API
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	// Setup BD mocks
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

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	// ACT: Call the function under test with invalid direction
	abr::err_code result = gmio.gmio_sync_nb(static_cast<AIE_GMIO_DIR>(99), 0x1000, 100);

	// ASSERT: Verify result
	CHECK(result == err_code::ok);
	abr::config_manager::s_pDevInst = nullptr;
	abr::config_manager::s_bInitialized = false;
	mock_c()->checkExpectations();
}
/**
 * @brief TC_012: Verify aie2gm() handles successful transfer
 *
 * This test verifies that the blocking AIE2GM transfer function successfully transfers
 * data from AI Engine to global memory.
 */
TEST(GmioApiImplsyncTests, TC_012_Aie2gm_Success)
{
	// ARRANGE: Setup buffer and memory mocks
	char buffer[100] = {0};
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
#ifndef __AIESIM__
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
#endif

	// Setup GMIO config and API
	gmioconfig.type = abr::gmio_config::gm2aie;
	api = std::make_shared<abr::gmio_api>(&gmioconfig);
	XAie_LocType testLoc;
	testLoc.Col = 0;
	testLoc.Row = 0;

	// Setup DMA mocks
	u8 FirstCallOutput = 1;
	u8 secondCallOutput = 2;
	mock_c()->expectOneCall("XAie_DmaDescInit")
		->withPointerParameters("DevInst", &devInst)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaChannelEnable")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	mock_c()->expectOneCall("XAie_DmaSetAxi")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// Configure API
	abr::err_code res = api->configure();
	CHECK_EQUAL((int)abr::err_code::ok, (int)res);

	// Setup BD mocks
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

	GmioApiImpl gmio(api, gmioconfig, mem, "test_gmio");

	// Setup wait mocks
	mock_c()->expectOneCall("XAie_DmaWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
#ifdef __AIESIM__
	mock_c()->expectOneCall("XAie_MemSyncForCPUVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
#endif

	// ACT: Call the function under test
	abr::err_code result = gmio.aie2gm(buffer, 100);

	// ASSERT: Verify result
	CHECK(result == err_code::ok);
	abr::config_manager::s_pDevInst = nullptr;
	abr::config_manager::s_bInitialized = false;
	mock_c()->checkExpectations();
}
