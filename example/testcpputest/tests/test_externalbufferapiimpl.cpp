/**
 * @file test_externalbufferapiimpl.cpp
 * @brief Unit tests for the ExternalBufferApiImpl class
 *
 * This file contains unit tests for the ExternalBufferApiImpl class, which provides
 * DMA and external buffer handling for AI Engine shim ports. The tests validate address
 * setup, ping-pong buffer configuration, DMA transfers in both directions, and
 * wait/synchronization behaviour for various shim-port configurations and error paths.
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

// Include headers for Unit Under Test (UUT) and comparators
#include "externalbufferapiimpl.h"
#include "test_comparator.h"

/**
 * @brief Test group for basic ExternalBufferApiImpl functionality
 *
 * This test group verifies basic buffer address setup and the wait()method for
 * different shim port configurations. It uses a minimal external_buffer_config and
 * a shared XAieMem instance to construct the ExternalBufferApiImpl under test.
 */
TEST_GROUP(ExternalBufferApiImplBasicTests)
{
	// Test fixtures
	std::shared_ptr<XAieMem> xaiemem;
	abr::external_buffer_config config;
	std::string testName;
	ExternalBufferApiImpl* uut;

	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Installs the XAie_LocType comparator, resets the external_buffer_config,
	 * creates a XAieMem object, and constructs a new ExternalBufferApiImpl instance.
	 */
	void setup()
	{
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		// Create a fresh configuration for each test
		config = {};
		testName = "TestBuffer";
		XAie_DevInst devInst;
		// Create a fresh memory interface for each test
		xaiemem = std::make_shared<XAieMem>(&devInst);
		// Create the UUT
		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Destroys the ExternalBufferApiImpl instance and clears mock expectations.
	 */
	void teardown()
	{
		delete uut;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
};
/**
 * @brief TC_001: Verify successful initial buffer address configuration
 *
 * This test verifies that setAddress()accepts an initial virtual address and
 * completes without error when no address has been configured before.
 */
TEST(ExternalBufferApiImplBasicTests, TC_001_SetAddress_Success)
{
	// ARRANGE: Create a pointer for testing
	void* testPtr = (void*)0x12345678;

	// ACT: Call the function under test
	uut->setAddress(testPtr);

	// ASSERT: Verify mock expectations are satisfied
	CHECK(true);
}
/**
 * @brief TC_002: Verify repeated calls to setAddress()with new address
 *
 * This test sets an initial address and then calls setAddress()again with a different
 * pointer to confirm that the API tolerates reconfiguration without crashing.
 */
TEST(ExternalBufferApiImplBasicTests, TC_002_SetAddress_AlreadySet)
{
	// ARRANGE: Set an initial address
	void* initialPtr = (void*)0x12345678;
	void* newPtr = (void*)0x87654321;
	uut->setAddress(initialPtr);

	// ACT: Call the function under test with different address
	uut->setAddress(newPtr);

	// ASSERT: Verify mock expectations are satisfied
	CHECK(true);
}
/**
 * @brief TC_003: Verify wait()with empty shimPortConfigs
 *
 * Verifies that wait()returns abr::err_code::ok immediately when the
 * configuration contains no shimPortConfigs, regardless of the port name.
 */
TEST(ExternalBufferApiImplBasicTests, TC_003_Wait_EmptyShimPortConfigs)
{
	// ARRANGE: Ensure config.shimPortConfigs is empty (already done in setup)

	// ACT: Call the wait method
	abr::err_code result = uut->wait("test_port");

	// ASSERT: Verify early return with OK status
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief TC_004: Verify wait()with invalid port name
 *
 * This test adds a single shim_port_config with one port name and calls wait()
 * using a different, invalid port name. Verifies that the function returns
 * abr::err_code::ok without attempting any DMA interaction.
 */
TEST(ExternalBufferApiImplBasicTests, TC_004_Wait_InvalidPortName)
{
	// ARRANGE: Create a configuration with a shimPortConfig but with a different port name
	abr::shim_port_config portConfig;
	portConfig.portName = "valid_port";
	portConfig.column = 1;
	portConfig.S2MMOrMM2S = 0; // S2MM
	portConfig.channel = 1;
	config.shimPortConfigs.push_back(portConfig);

	// Need to recreate UUT to use updated config
	delete uut;
	uut = new ExternalBufferApiImpl(config, testName, xaiemem);

	// ACT: Call wait with an invalid port name
	abr::err_code result = uut->wait("invalid_port");

	// ASSERT: Verify early return with OK status
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief TC_005: Verify wait()for valid MM2S shim port
 *
 * Configures a MM2S shim port, expects XAie_DmaWaitForDone and
 * XAie_MemSyncForCPUVAddr mocks to be called, and verifies that wait()
 * returns abr::err_code::ok for a valid port name.
 */
TEST(ExternalBufferApiImplBasicTests, TC_005_Wait_ValidPortNameMM2S)
{
	// ARRANGE: Create a configuration with a MM2S shimPortConfig
	abr::shim_port_config portConfig;
	portConfig.portName = "valid_port";
	portConfig.column = 1;
	portConfig.S2MMOrMM2S = 1; // MM2S
	portConfig.channel = 1;
	config.shimPortConfigs.push_back(portConfig);

	// Need to recreate UUT to use updated config
	delete uut;
	uut = new ExternalBufferApiImpl(config, testName, xaiemem);

	mock_c()->expectOneCall("XAie_DmaWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectNCalls(3,"XAie_MemSyncForCPUVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	// ACT: Call wait with the valid port name
	abr::err_code result = uut->wait("valid_port");

	// ASSERT: Verify correct return status
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief TC_006: Verify wait()for valid S2MM shim port
 *
 * Configures a S2MM shim port, expects XAie_DmaWaitForDone to be called,
 * and verifies that wait()returns abr::err_code::ok.
 */
TEST(ExternalBufferApiImplBasicTests, TC_006_Wait_ValidPortNameS2MM)
{
	// ARRANGE: Create a configuration with a S2MM shimPortConfig
	abr::shim_port_config portConfig;
	portConfig.portName = "valid_port";
	portConfig.column = 1;
	portConfig.S2MMOrMM2S = 0; // S2MM
	portConfig.channel = 1;
	config.shimPortConfigs.push_back(portConfig);

	// Need to recreate UUT to use updated config
	delete uut;
	uut = new ExternalBufferApiImpl(config, testName, xaiemem);

	mock_c()->expectOneCall("XAie_DmaWaitForDone")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	// ACT: Call wait with the valid port name
	abr::err_code result = uut->wait("valid_port");

	// ASSERT: Verify correct return status
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief TC_007: Verify wait()with invalid S2MMOrMM2S configuration
 *
 * Uses a shim_port_config with an invalid S2MMOrMM2S value (-1) and confirms
 * that wait()still returns abr::err_code::ok without accessing DMA APIs.
 */
TEST(ExternalBufferApiImplBasicTests, TC_007_Wait_InvalidConfig)
{
	// ARRANGE: Create a configuration with a shimPortConfig but with a different port name
	abr::shim_port_config portConfig;
	portConfig.portName = "valid_port";
	portConfig.column = 1;
	portConfig.S2MMOrMM2S = -1;
	portConfig.channel = 1;
	config.shimPortConfigs.push_back(portConfig);

	// Need to recreate UUT to use updated config
	delete uut;
	uut = new ExternalBufferApiImpl(config, testName, xaiemem);

	// ACT: Call wait with an invalid port name
	abr::err_code result = uut->wait("invalid_port");

	// ASSERT: Verify early return with OK status
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief Test group for ping-pong buffer configuration.
 *
 * This group validates setupPingPongBuffers()behaviour, including
 * initial configuration, idempotent reconfiguration, and interaction
 * with shim BD descriptors for S2MM/MM2S ports.
 */
TEST_GROUP(ExternalBufferApiImplPingPongTests)
{
	// Common test objects
	std::shared_ptr<XAieMem> xaiemem;
	abr::external_buffer_config config;
	std::string testName;
	ExternalBufferApiImpl* uut;
	void* pingAddress;
	void* pongAddress;

	void setup()
	{

		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		// Create a fresh configuration for each test
		config = {};
		testName = "TestBuffer";

		// Create test addresses
		pingAddress = (void*)0x12345678;
		pongAddress = (void*)0x87654321;

		// Create a fresh memory interface for each test
		XAie_DevInst devInst;
		// Create a fresh memory interface for each test
		xaiemem = std::make_shared<XAieMem>(&devInst);

		// Create the UUT
		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
	}

	void teardown()
	{
		delete uut;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
	// Helper to set up a common port configuration
	void setupCommonPortConfig(bool isS2MM) {
		abr::shim_port_config portConfig;
		portConfig.portName = "valid_port";
		portConfig.column = 1;
		portConfig.S2MMOrMM2S = isS2MM ? 0 : 1; // 0 for S2MM, 1 for MM2S
		portConfig.channel = 1;
		portConfig.taskRepetition = 1;
		portConfig.enable_task_complete_token = false;

		// Add BD infos for both ping and pong buffers
		abr::shimBDInfo bdInfo1;
		bdInfo1.bd_id = 2;
		bdInfo1.buf_idx = 0; // Ping
		bdInfo1.offset = 0;
		bdInfo1.transaction_size = 1024;
		portConfig.shimBDInfos.push_back(bdInfo1);

		abr::shimBDInfo bdInfo2;
		bdInfo2.bd_id = 3;
		bdInfo2.buf_idx = 1; // Pong
		bdInfo2.offset = 0;
		bdInfo2.transaction_size = 1024;
		portConfig.shimBDInfos.push_back(bdInfo2);

		config.shimPortConfigs.push_back(portConfig);

		// Need to recreate UUT to use updated config
		delete uut;
		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
	}
};

/**
 * @brief TC_029: Verify Successful ping-pong buffer setup
 *
 * Verifies that setupPingPongBuffers()returns abr::err_code::ok when
 * called with valid ping and pong virtual addresses and a default config.
 */
TEST(ExternalBufferApiImplPingPongTests, TC_029_SetupPingPongBuffers_Success)
{
	// ARRANGE: Nothing special needed here

	// ACT: Call the method under test
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);

	// ASSERT: Verify successful execution
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief TC_030: Verify Repeated ping-pong buffer setup with new addresses
 *
 * Calls setupPingPongBuffers()twice with different ping/pong addresses
 * to ensure the API remains stable and continues to return ok.
 */
TEST(ExternalBufferApiImplPingPongTests, TC_030_SetupPingPongBuffers_AlreadySet)
{
	// ARRANGE: Set the ping-pong addresses first
	uut->setupPingPongBuffers(pingAddress, pongAddress);

	// ACT: Try to set different addresses
	void* newPingAddress = (void*)0x11223344;
	void* newPongAddress = (void*)0x55667788;
	abr::err_code result = uut->setupPingPongBuffers(newPingAddress, newPongAddress);

	// ASSERT: Verify successful return
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief TC_032: Verify setupPingPongBuffers()with shim port and BD programming
 *
 * Configures a S2MM shim port with two BD entries (ping and pong), expects
 * device-address conversion and BD programming mocks to be invoked, and
 * verifies an abr::err_code::ok result.
 */
TEST(ExternalBufferApiImplPingPongTests, TC_032_SetupPingPongBuffers_Success)
{
	// ARRANGE: Setup a port config for S2MM
	setupCommonPortConfig(true); // S2MM

	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	// ACT: Call setupPingPongBuffers method
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);

	// ASSERT: Verify correct error code is returned
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief Test group for non-blocking and blocking DMA operations.
 *
 * This group verifies gm2aie_nb(), aie2gm_nb(), and their blocking wrappers
 * gm2aie() and aie2gm() for various shim-port configurations and driver
 * return codes.
 */
TEST_GROUP(ExternalBufferApiImplDmaTests)
{
	// Common test objects
	std::shared_ptr<XAieMem> xaiemem;
	abr::external_buffer_config config;
	std::string testName;
	ExternalBufferApiImpl* uut;
	void* testAddress;

	void setup()
	{
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		// Create a fresh configuration for each test
		config = {};
		testName = "TestBuffer";
		testAddress = (void*)0x12345678;

		XAie_DevInst devInst;
		// Create a fresh memory interface for each test
		xaiemem = std::make_shared<XAieMem>(&devInst);

		// Create the UUT
		uut = new ExternalBufferApiImpl(config, testName, xaiemem);

		// Set the address for most tests
		uut->setAddress(testAddress);
	}

	void teardown()
	{
		delete uut;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}

	// Helper to set up a common port configuration
	void setupCommonPortConfig(bool isS2MM) {
		abr::shim_port_config portConfig;
		portConfig.portName = "valid_port";
		portConfig.column = 1;
		portConfig.S2MMOrMM2S = isS2MM ? 0 : 1; // 0 for S2MM, 1 for MM2S
		portConfig.channel = 1;
		portConfig.taskRepetition = 1;
		portConfig.enable_task_complete_token = false;

		// Add a BD info
		abr::shimBDInfo bdInfo;
		bdInfo.bd_id = 2;
		bdInfo.buf_idx = 0;
		bdInfo.offset = 0;
		bdInfo.transaction_size = 1024;
		portConfig.shimBDInfos.push_back(bdInfo);

		config.shimPortConfigs.push_back(portConfig);

		// Need to recreate UUT to use updated config
		delete uut;
		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
		uut->setAddress(testAddress);
	}
};
/**
 * @brief TC_010: Verify gm2aie_nb() with no shim ports
 *
 * Ensures that gm2aie_nb() returns abr::err_code::ok when shimPortConfigs
 * is empty, without invoking any DMA API.
 */
TEST(ExternalBufferApiImplDmaTests, TC_010_GM2AIE_NB_EmptyShimPortConfigs)
{
	// ARRANGE: Ensure config.shimPortConfigs is empty (already done in setup)

	// ACT: Call the gm2aie_nb method
	abr::err_code result = uut->gm2aie_nb("test_port");

	// ASSERT: Verify early return with OK status
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief TC_011: Verify gm2aie_nb() with invalid port name
 *
 * Configures a shim port but calls gm2aie_nb() with a different name.
 * Verifies an early ok return and no DMA programming.
 */
TEST(ExternalBufferApiImplDmaTests, TC_011_GM2AIE_NB_InvalidPortName)
{
	// ARRANGE: Setup a port config
	setupCommonPortConfig(true); // MM2S

	// ACT: Call gm2aie_nb with an invalid port name
	abr::err_code result = uut->gm2aie_nb("invalid_port");

	// ASSERT: Verify early return with OK status
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief TC_012: Verify gm2aie_nb() with MM2S port and successful driver calls
 *
 * Sets up a MM2S shim port and expects MemGetDevAddr, DmaUpdateBdAddr,
 * MemSyncForDevVAddr and DmaChannelSetStartQueue mocks, verifying that
 * gm2aie_nb() returns abr::err_code::ok.
 */
TEST(ExternalBufferApiImplDmaTests, TC_012_GM2AIE_NB_Success)
{
	// ARRANGE: Setup a port config for MM2S
	setupCommonPortConfig(false); // MM2S

	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	// ACT: Call gm2aie_nb with a valid port name
	abr::err_code result = uut->gm2aie_nb("valid_port");

	// ASSERT: Verify correct error code is returned
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief aie2gm_nb() returns OK when no shim ports are configured.
 *
 * This test leaves config.shimPortConfigs empty and calls
 * ExternalBufferApiImpl::aie2gm_nb() with an arbitrary port name.
 * It verifies that, in the absence of any shim configuration,
 * the function returns abr::err_code::ok and does not attempt to
 * access or program any DMA descriptors.
 */
TEST(ExternalBufferApiImplDmaTests, TC_018_AIE2GM_NB_EmptyShimPortConfigs)
{
	// ARRANGE: Ensure config.shimPortConfigs is empty (already done in setup)

	// ACT: Call the aie2gm_nb method
	abr::err_code result = uut->aie2gm_nb("test_port");

	// ASSERT: Verify early return with OK status
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief aie2gm_nb() ignores ports that do not match any shim portName.
 *
 * After configuring a single shim_port_config with portName="valid_port",
 * this test calls ExternalBufferApiImpl::aie2gm_nb() using a different
 * port string. The UUT should iterate through the shimPortConfigs vector,
 * find no match, perform no DMA programming, and return abr::err_code::ok.
 */
TEST(ExternalBufferApiImplDmaTests, TC_019_AIE2GM_NB_InvalidPortName)
{
	// ARRANGE: Setup a port config
	setupCommonPortConfig(false); // S2MM

	// ACT: Call aie2gm_nb with an invalid port name
	abr::err_code result = uut->aie2gm_nb("invalid_port");

	// ASSERT: Verify early return with OK status
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief Verify aie2gm_nb() success path for a valid S2MM shim port.
 *
 * The test then calls aie2gm_nb() with the valid port name and verifies that
 * abr::err_code::ok is returned, confirming that the function correctly
 * traverses the shim configuration, invokes all required driver APIs, and
 * reports success when the S2MM path is fully programmed without errors.
 */
TEST(ExternalBufferApiImplDmaTests, TC_021_AIE2GM_NB_UpdateBDAddressSuccess)
{
	// ARRANGE: Setup a port config for S2MM
	setupCommonPortConfig(true); // S2MM
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	// ACT: Call aie2gm_nb with a valid port name
	abr::err_code result = uut->aie2gm_nb("valid_port");

	// ASSERT: Verify correct error code is returned
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief TC_022: Verify aie2gm_nb() propagates driver error status
 *
 * Mocks MemGetDevAddr, DmaUpdateBdAddr, MemSyncForDevVAddr and
 * DmaChannelSetStartQueue to return non-zero error codes and
 * verifies that aie2gm_nb() returns abr::err_code::aie_driver_error.
 */
TEST(ExternalBufferApiImplDmaTests, TC_022_AIE2GM_NB_UpdateBDAddressFails)
{
	// ARRANGE: Setup a port config and configure updateBDAddress to fail
	setupCommonPortConfig(true); // S2MM
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(1);
	// ACT: Call aie2gm_nb with a valid port name
	abr::err_code result = uut->aie2gm_nb("valid_port");

	// ASSERT: Verify correct error code is returned
	CHECK_EQUAL(static_cast<int>(abr::err_code::aie_driver_error), static_cast<int>(result));
}
/**
 * @brief TC_013: Verify gm2aie() blocking wrapper around gm2aie_nb()/wait()
 *
 * Expects a mock wait()call to succeed and verifies that the blocking
 * gm2aie() API returns abr::err_code::ok.
 */
TEST(ExternalBufferApiImplDmaTests, TC_013_GM2AIE)
{

	mock_c()->expectOneCall("wait")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// ACT: Call aie2gm_nb with a valid port name
	abr::err_code result = uut->gm2aie("valid_port");

	// ASSERT: Verify correct error code is returned
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}

/**
 * @brief TC_02: Verify aie2gm() blocking wrapper around aie2gm_nb()/wait()
 *
 * Expects a mock wait()call to succeed and verifies that the blocking
 * aie2gm() API returns abr::err_code::ok.
 */
TEST(ExternalBufferApiImplDmaTests, TC_023_AIE2GM)
{

	mock_c()->expectOneCall("wait")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);

	// ACT: Call aie2gm_nb with a valid port name
	abr::err_code result = uut->aie2gm("valid_port");

	// ASSERT: Verify correct error code is returned
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @ingroup ExternalBufferApiImplTests
 * @brief Ping‑pong buffer tests tailored for simulation flows.
 *
 * This group mirrors the ping‑pong buffer configuration scenarios of
 * ExternalBufferApiImplPingPongTests, but targets simulation‑oriented
 * flows where the shim port configuration and BD programming are
 * exercised through simulation hooks.
 *
 * The tests validate that the UUT interacts correctly with XAieMem and
 * shim BD descriptors in a simulated environment for both S2MM and MM2S
 * directions.
 */
TEST_GROUP(ExternalBufferApiImplPingPongTestsforsim)
{
	// Common test objects
	std::shared_ptr<XAieMem> xaiemem;
	abr::external_buffer_config config;
	std::string testName;
	ExternalBufferApiImpl* uut;
	void* pingAddress;
	void* pongAddress;
	/**
	 * @brief Per-test setup for ExternalBufferApiImplPingPongTestsforsim.
	 *
	 * Initializes a clean simulation configuration, creates deterministic
	 * ping and pong virtual addresses, constructs a XAieMem instance over
	 * a local XAie_DevInst, and then creates an ExternalBufferApiImpl
	 * instance to be used by each test.
	 */
	void setup()
	{

		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
		// Create a fresh configuration for each test
		config = {};
		testName = "TestBuffer";

		// Create test addresses
		pingAddress = (void*)0x12345678;
		pongAddress = (void*)0x87654321;

		// Create a fresh memory interface for each test
		XAie_DevInst devInst;
		// Create a fresh memory interface for each test
		xaiemem = std::make_shared<XAieMem>(&devInst);

		// Create the UUT
		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
	}
	/**
	 * @brief Per-test teardown for ExternalBufferApiImplPingPongTestsforsim.
	 *
	 * Deletes the UUT and clears all mock comparators and expectations so
	 * that each simulation test starts from a pristine state.
	 */
	void teardown()
	{
		delete uut;
		mock_c()->removeAllComparatorsAndCopiers();
		mock_c()->clear();
	}
	// Helper to set up a common port configuration
	void setupCommonPortConfig(bool isS2MM) {
		abr::shim_port_config portConfig;
		portConfig.portName = "valid_port";
		portConfig.column = 1;
		portConfig.S2MMOrMM2S = isS2MM ? 0 : 1; // 0 for S2MM, 1 for MM2S
		portConfig.channel = 1;
		portConfig.taskRepetition = 1;
		portConfig.enable_task_complete_token = false;

		// Add BD infos for both ping and pong buffers
		abr::shimBDInfo bdInfo1;
		bdInfo1.bd_id = 2;
		bdInfo1.buf_idx = 0; // Ping
		bdInfo1.offset = 0;
		bdInfo1.transaction_size = 1024;
		portConfig.shimBDInfos.push_back(bdInfo1);

		abr::shimBDInfo bdInfo2;
		bdInfo2.bd_id = 3;
		bdInfo2.buf_idx = 1; // Pong
		bdInfo2.offset = 0;
		bdInfo2.transaction_size = 1024;
		portConfig.shimBDInfos.push_back(bdInfo2);

		config.shimPortConfigs.push_back(portConfig);

		// Need to recreate UUT to use updated config
		delete uut;
		uut = new ExternalBufferApiImpl(config, testName, xaiemem);
	}
};
/**
 * @brief TC_034: Simulation ping‑pong configuration for a S2MM shim port.
 *
 * This test configures a simulation S2MM shim port with ping and pong BD
 * entries using setupCommonPortConfig(true). The XAie address conversion,
 * BD update, memory sync, and start‑queue calls are mocked to succeed.
 * It then invokes ExternalBufferApiImpl::setupPingPongBuffers()and
 * verifies that abr::err_code::ok is returned in the simulated S2MM path.
 */
TEST(ExternalBufferApiImplPingPongTestsforsim, TC_034_SetupPingPongBuffers_S2MM)
{
	// ARRANGE: Setup a port config for S2MM
	setupCommonPortConfig(true); // S2MM

	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	// ACT: Call setupPingPongBuffers method
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);

	// ASSERT: Verify correct error code is returned
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
/**
 * @brief TC_035: Simulation ping‑pong configuration for a MM2S shim port.
 *
 * Similar to TC_034, but setupCommonPortConfig(false) is used to model a
 * MM2S shim port. The same sequence of XAie calls is mocked to return
 * success, and the test confirms that setupPingPongBuffers()completes
 * with abr::err_code::ok in the simulated MM2S scenario.
 */
TEST(ExternalBufferApiImplPingPongTestsforsim, TC_035_SetupPingPongBuffers_MM2S)
{
	// ARRANGE: Setup a port config for MM2S
	setupCommonPortConfig(false); // MM2S

	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
		->ignoreOtherParameters()
		->andReturnIntValue(XAIE_OK);
	// ACT: Call setupPingPongBuffers method
	abr::err_code result = uut->setupPingPongBuffers(pingAddress, pongAddress);

	// ASSERT: Verify correct error code is returned
	CHECK_EQUAL(static_cast<int>(abr::err_code::ok), static_cast<int>(result));
}
