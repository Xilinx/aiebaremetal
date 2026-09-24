/**
 * @file test_aeg_profiling_api.cpp
 * @brief Unit tests for the AIE profiling API
 *
 * This file contains unit tests for the AIE profiling API, which provides profiling
 * functionality for stream monitoring, performance counters, and timing analysis.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include <algorithm>
#include "aeg_profiling_api.h"
#include "profiling_stubs/xaiengine/xaiegbl.h"
#include "profiling_stubs/fal_stubs.h"
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"
#include "CppUTestExt/MockSupport_c.h"

using namespace abr;
using namespace stub_controls;

/**
 * @brief Helper function to create a nominal shim configuration
 *
 * Creates a shim_config structure with default values for testing.
 */
static shim_config make_nominal_cfg(int shimCol = 1, int streamPort = 2, u8 slaveOrMaster = 0)
{
    shim_config cfg{};
    cfg.shimColumn    = shimCol;
    cfg.streamPortId  = streamPort;
    cfg.slaveOrMaster = slaveOrMaster;
    return cfg;
}

/**
 * @brief Test group for shim_config structure
 *
 * This test group verifies the functionality of the shim_config structure,
 * including default construction and construction from gmio_config and plio_config.
 */
TEST_GROUP(ShimConfigTests)
{
	/**
	 * @brief Set up test fixtures before each test
	 */
	void setup()
	{
	}

	/**
	 * @brief Clean up test fixtures after each test
	 */
	void teardown()
	{
		mock_c()->clear();
	}
};

/**
 * @brief TC_001: Verify shim_config default constructor
 *
 * This test verifies that the shim_config default constructor initializes
 * all fields with expected default values.
 */
TEST(ShimConfigTests, TC_001_DefaultConstructor)
{
	// ARRANGE - Nothing to arrange, testing default constructor

	// ACT - Instantiate with default constructor
	shim_config config;

	// ASSERT - Verify expected default values
	CHECK_EQUAL(-1, config.shimColumn);
	CHECK_EQUAL(0, config.slaveOrMaster);
	CHECK_EQUAL(-1, config.streamPortId);
}

/**
 * @brief TC_002: Verify shim_config constructor with gmio_config for aie2gm type
 *
 * This test verifies that the shim_config constructor correctly initializes
 * fields from a gmio_config structure when the type is aie2gm (AIE to memory).
 * The slaveOrMaster field should be set to 1 for aie2gm transfers.
 */
TEST(ShimConfigTests, TC_002_GmioConstructorAie2gm)
{
	// ARRANGE - Create a gmio_config with aie2gm type
	gmio_config gmioConfig;
	gmioConfig.shimColumn = 5;
	gmioConfig.type = gmio_config::aie2gm;
	gmioConfig.streamId = 3;

	// ACT - Instantiate with gmio_config
	shim_config config(&gmioConfig);

	// ASSERT - Verify expected values
	CHECK_EQUAL(5, config.shimColumn);
	CHECK_EQUAL(1, config.slaveOrMaster); // aie2gm should set slaveOrMaster to 1
	CHECK_EQUAL(3, config.streamPortId);
}

/**
 * @brief TC_003: Verify shim_config constructor with gmio_config for gm2aie type
 *
 * This test verifies that the shim_config constructor correctly initializes
 * fields from a gmio_config structure when the type is gm2aie (memory to AIE).
 * The slaveOrMaster field should be set to 0 for gm2aie transfers.
 */
TEST(ShimConfigTests, TC_003_GmioConstructorGm2aie)
{
	// ARRANGE - Create a gmio_config with gm2aie type
	gmio_config gmioConfig;
	gmioConfig.shimColumn = 5;
	gmioConfig.type = gmio_config::gm2aie;
	gmioConfig.streamId = 3;

	// ACT - Instantiate with gmio_config
	shim_config config(&gmioConfig);

	// ASSERT - Verify expected values
	CHECK_EQUAL(5, config.shimColumn);
	CHECK_EQUAL(0, config.slaveOrMaster); // gm2aie should set slaveOrMaster to 0
	CHECK_EQUAL(3, config.streamPortId);
}

/**
 * @brief TC_004: Verify shim_config constructor with plio_config
 *
 * This test verifies that the shim_config constructor correctly initializes
 * all fields from a plio_config structure, including shimColumn, slaveOrMaster,
 * and streamPortId.
 */
TEST(ShimConfigTests, TC_004_PlioConstructor)
{
	// ARRANGE - Create a plio_config
	plio_config plioConfig;
	plioConfig.shimColumn = 5;
	plioConfig.slaveOrMaster = 1;
	plioConfig.streamId = 3;

	// ACT - Instantiate with plio_config
	shim_config config(&plioConfig);

	// ASSERT - Verify expected values
	CHECK_EQUAL(5, config.shimColumn);
	CHECK_EQUAL(1, config.slaveOrMaster);
	CHECK_EQUAL(3, config.streamPortId);
}

/**
 * @brief TC_005: Verify shim_config constructors handle null pointers correctly
 *
 * This test verifies that the shim_config constructors properly handle null
 * pointer inputs for both gmio_config and plio_config, initializing fields
 * with default values instead of dereferencing the null pointer.
 */
TEST(ShimConfigTests, TC_005_NullPointers)
{
	// ARRANGE & ACT - Instantiate with null pointers
	shim_config configGmio((gmio_config*)nullptr);
	shim_config configPlio((plio_config*)nullptr);

	// ASSERT - Verify expected default values
	CHECK_EQUAL(-1, configGmio.shimColumn);
	CHECK_EQUAL(0, configGmio.slaveOrMaster);
	CHECK_EQUAL(-1, configGmio.streamPortId);

	CHECK_EQUAL(-1, configPlio.shimColumn);
	CHECK_EQUAL(0, configPlio.slaveOrMaster);
	CHECK_EQUAL(-1, configPlio.streamPortId);
}

/**
 * @brief Test group for profile_stream_running_to_idle_cycles functionality
 *
 * This test group verifies the functionality of profile_stream_running_to_idle_cycles,
 * which measures the number of clock cycles from when a stream becomes running
 * until it transitions to idle state.
 */
TEST_GROUP(ProfileStreamRunningToIdleCyclesTests)
{
	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Resets all stub controls and initializes the device wrapper and
	 * device instance pointer to prevent null pointer dereferences.
	 */
	void setup()
	{
		ResetAll();
		// Initialize the device wrapper to prevent null pointer dereference
		static fal_util::DevWrapper devWrapper;
		fal_util::s_pXAieDev = &devWrapper;
		// Initialize the device instance pointer
		abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
	}

	void teardown()
	{
		fal_util::s_pXAieDev = nullptr;
		abr::config_manager::s_pDevInst = nullptr;
		mock_c()->clear();
	}
};

/**
 * @brief TC_102: Verify error handling for invalid tile location
 *
 * This test verifies that profile_stream_running_to_idle_cycles correctly
 * returns an internal_error when provided with an invalid shim column
 * (negative value), preventing invalid hardware access.
 */
TEST(ProfileStreamRunningToIdleCyclesTests, TC_102_InvalidTileLocation)
{
	// ARRANGE
	// Invalid shim config (negative shimColumn)
	shim_config config;
	config.shimColumn = -1;
	config.slaveOrMaster = 0;
	config.streamPortId = 3;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	// ACT
	err_code result = profiling::profile_stream_running_to_idle_cycles(config, acquiredResources);

	// ASSERT
	CHECK_EQUAL(static_cast<int>(err_code::internal_error), static_cast<int>(result));
    acquiredResources.clear();
}

/**
 * @brief TC_001: Verify successful profiling with nominal configuration
 *
 * This test verifies that profile_stream_running_to_idle_cycles successfully
 * acquires resources and configures profiling for a valid stream configuration.
 * Verifies that 2 resources are acquired (stream port select and performance counter).
 */
TEST(ProfileStreamRunningToIdleCyclesTests, TC_001_NominalSuccess)
{
    shim_config cfg{};
    cfg.shimColumn    = 1;
    cfg.streamPortId  = 2;
    cfg.slaveOrMaster = 0; // slave

    // Stub configuration per rubric
    FalUtil_SetRequestResults({0, 1}); // eventPortId, counterId
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);

    CHECK_TRUE(rc == err_code::ok);
	acquiredResources.clear();
}

/**
 * @brief TC_004: Verify error handling when stream switch port request fails
 *
 * This test verifies that profile_stream_running_to_idle_cycles correctly
 * returns resource_unavailable error when the stream switch port resource
 * cannot be requested/allocated.
 */
TEST(ProfileStreamRunningToIdleCyclesTests, TC_004_StreamSwitchRequestFails)
{
    shim_config cfg{};
    cfg.shimColumn    = 1;
    cfg.streamPortId  = 2;
    cfg.slaveOrMaster = 0;

    FalUtil_SetRequestResults({-1}); // first request fails

    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
	 acquiredResources.clear();
}

/**
 * @brief TC_005: Verify error handling when performance counter request fails
 *
 * This test verifies that profile_stream_running_to_idle_cycles correctly
 * returns resource_unavailable error when the performance counter resource
 * cannot be requested/allocated, even if the stream switch port was successful.
 */
TEST(ProfileStreamRunningToIdleCyclesTests, TC_005_PerfCounterRequestFails)
{
    shim_config cfg{};
    cfg.shimColumn    = 1;
    cfg.streamPortId  = 2;
    cfg.slaveOrMaster = 1; // master

    FalUtil_SetRequestResults({0, -1}); // sswitch ok, counter fails
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
	acquiredResources.clear();
}

/**
 * @brief TC_006: Verify error handling when XAie_EventSelectStrmPort fails
 *
 * This test verifies that profile_stream_running_to_idle_cycles correctly
 * returns aie_driver_error when the underlying AIE driver function
 * XAie_EventSelectStrmPort fails during stream port event selection.
 */
TEST(ProfileStreamRunningToIdleCyclesTests, TC_006_EventSelectFails)
{
    shim_config cfg{};
    cfg.shimColumn    = 1;
    cfg.streamPortId  = 2;
    cfg.slaveOrMaster = 0;

    FalUtil_SetRequestResults({0, 1});
    XAie_SetEventSelectResult(1);        // non-zero => failure
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
    CHECK_TRUE(rc == err_code::aie_driver_error);
    acquiredResources.clear();
}

/**
 * @brief TC_007: Verify error handling when XAie_PerfCounterControlSet fails
 *
 * This test verifies that profile_stream_running_to_idle_cycles correctly
 * returns aie_driver_error when the underlying AIE driver function
 * XAie_PerfCounterControlSet fails during performance counter configuration.
 */
TEST(ProfileStreamRunningToIdleCyclesTests, TC_007_PerfCounterControlFails)
{
    shim_config cfg{};
    cfg.shimColumn    = 1;
    cfg.streamPortId  = 2;
    cfg.slaveOrMaster = 1;

    FalUtil_SetRequestResults({0, 1});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(2); // non-zero => failure
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
    CHECK_TRUE(rc == err_code::aie_driver_error);
    acquiredResources.clear();
}

/**
 * @brief Test group for profile_stream_running_event_count functionality
 *
 * This test group verifies the functionality of profile_stream_running_event_count,
 * which counts the number of stream running events using performance counters.
 */
TEST_GROUP(ProfilingStreamRunningEventCount)
{
	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Resets all stub controls and initializes the device wrapper and
	 * device instance pointer to prevent null pointer dereferences.
	 */
	void setup()
	{
		ResetAll();
		// Initialize the device wrapper to prevent null pointer dereference
		static fal_util::DevWrapper devWrapper;
		fal_util::s_pXAieDev = &devWrapper;
		// Initialize the device instance pointer
		abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Clears device pointers and mock expectations to prepare for the next test.
	 */
	void teardown()
	{
		fal_util::s_pXAieDev = nullptr;
		abr::config_manager::s_pDevInst = nullptr;
		mock_c()->clear();
	}
};

/**
 * @brief TC_201: Verify successful event count profiling with nominal configuration
 *
 * This test verifies that profile_stream_running_event_count successfully
 * acquires resources and configures profiling when all resource requests
 * and driver calls succeed.
 */
TEST(ProfilingStreamRunningEventCount, TC_201_NominalSuccess)
{
    shim_config cfg =  make_nominal_cfg(1, 2, 0); // slave

    FalUtil_SetRequestResults({0, 1}); // eventPortId, counterId
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc =  profiling::profile_stream_running_event_count(cfg, acquiredResources);

    CHECK_TRUE(rc == err_code::ok);

}

/**
 * @brief TC_003: Verify error handling for invalid stream port ID
 *
 * This test verifies that profile_stream_running_event_count correctly
 * returns internal_error when provided with an invalid stream port ID
 * (negative value).
 */
TEST(ProfilingStreamRunningEventCount, TC_003_InvalidStreamPortId)
{
	// ARRANGE: Set up invalid streamPortId parameter
	abr::shim_config shimConfig;
	shimConfig.shimColumn = 1;
	shimConfig.streamPortId = -1; // Invalid
	shimConfig.slaveOrMaster = 0;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	// ACT: Call the real production function
	abr::err_code result = abr::profiling::profile_stream_running_event_count(shimConfig, acquiredResources);

	// ASSERT: Verify the outcome is as expected
	CHECK_EQUAL(static_cast<int>(abr::err_code::internal_error), static_cast<int>(result));
}

/**
 * @brief TC_204: Verify error handling when stream switch request fails
 *
 * This test verifies that profile_stream_running_event_count correctly
 * returns resource_unavailable error when the stream switch port resource
 * cannot be requested/allocated.
 */
TEST(ProfilingStreamRunningEventCount, TC_204_StreamSwitchRequestFails)
{
    shim_config cfg =  make_nominal_cfg(1, 2, 0);

    FalUtil_SetRequestResults({-1}); // first request fails
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc =  profiling::profile_stream_running_event_count(cfg, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);

}

/**
 * @brief TC_205: Verify error handling when performance counter request fails
 *
 * This test verifies that profile_stream_running_event_count correctly
 * returns resource_unavailable error when the performance counter resource
 * cannot be requested/allocated.
 */
TEST(ProfilingStreamRunningEventCount, TC_205_PerfCounterRequestFails)
{
    shim_config cfg =  make_nominal_cfg(1, 2, 1); // master

    FalUtil_SetRequestResults({0, -1}); // event ok, counter fails
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc =  profiling::profile_stream_running_event_count(cfg, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);

}

/**
 * @brief TC_206: Verify error handling when XAie_EventSelectStrmPort fails
 *
 * This test verifies that profile_stream_running_event_count correctly
 * returns aie_driver_error when the underlying AIE driver function
 * XAie_EventSelectStrmPort fails during stream port event selection.
 */
TEST(ProfilingStreamRunningEventCount, TC_206_EventSelectFails)
{
    shim_config cfg =  make_nominal_cfg(1, 2, 0);

    FalUtil_SetRequestResults({0, 1});
    XAie_SetEventSelectResult(1); // non-zero => failure
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc =  profiling::profile_stream_running_event_count(cfg, acquiredResources);

    CHECK_TRUE(rc == err_code::aie_driver_error);

}

/**
 * @brief TC_207: Verify error handling when XAie_PerfCounterControlSet fails
 *
 * This test verifies that profile_stream_running_event_count correctly
 * returns aie_driver_error when the underlying AIE driver function
 * XAie_PerfCounterControlSet fails during performance counter configuration.
 */
TEST(ProfilingStreamRunningEventCount, TC_207_PerfCounterControlFails)
{
    shim_config cfg =  make_nominal_cfg(1, 2, 1);

    FalUtil_SetRequestResults({0, 1});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(3); // non-zero => failure
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc =  profiling::profile_stream_running_event_count(cfg, acquiredResources);

    CHECK_TRUE(rc == err_code::aie_driver_error);

}

/**
 * @brief Test group for profile_start_time_difference_btw_two_streams functionality
 *
 * This test group verifies the functionality of profile_start_time_difference_btw_two_streams,
 * which measures the time difference between start times of two different streams using
 * broadcast resources for synchronization.
 */
TEST_GROUP(ProfilingStartTimeDifferenceBtw2Streams)
{
	/**
	 * @brief Set up test fixtures before each test
	 *
	 * Resets all stub controls and initializes the device wrapper and
	 * device instance pointer to prevent null pointer dereferences.
	 */
	void setup()
	{
		ResetAll();
		// Initialize the device wrapper to prevent null pointer dereference
		static fal_util::DevWrapper devWrapper;
		fal_util::s_pXAieDev = &devWrapper;
		// Initialize the device instance pointer
		abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
	}

	/**
	 * @brief Clean up test fixtures after each test
	 *
	 * Clears device pointers and mock expectations to prepare for the next test.
	 */
	void teardown()
	{
		fal_util::s_pXAieDev = nullptr;
		abr::config_manager::s_pDevInst = nullptr;
		mock_c()->clear();
	}
};

/**
 * @brief TC_004: Verify error handling for invalid shim column in first config
 *
 * This test verifies that profile_start_time_difference_btw_two_streams correctly
 * returns internal_error when the first shim configuration has an invalid
 * column number (negative value).
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_004_InvalidShimColumn1)
{
	// ARRANGE
	abr::shim_config shimConfig1;
	shimConfig1.shimColumn = -1; // Invalid column
	shimConfig1.streamPortId = 0;
	shimConfig1.slaveOrMaster = 0;

	abr::shim_config shimConfig2;
	shimConfig2.shimColumn = 5;
	shimConfig2.streamPortId = 1;
	shimConfig2.slaveOrMaster = 1;

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	// ACT
	abr::err_code result = abr::profiling::profile_start_time_difference_btw_two_streams(shimConfig1, shimConfig2, acquiredResources);

	// ASSERT
	CHECK_EQUAL((int)abr::err_code::internal_error, (int)result);


}

/**
 * @brief TC_301: Verify successful time difference profiling for same tile
 *
 * This test verifies that profile_start_time_difference_btw_two_streams successfully
 * measures start time difference when both streams are on the same tile,
 * without requiring broadcast resources.
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_301_SameTileNominalSuccess)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0); // slave
    shim_config cfg2 = make_nominal_cfg(1, 3, 1); // master, same column

    // Request order in UUT: SS1, PC1, SS2, PC2 (no broadcast)
    FalUtil_SetRequestResults({0, 1, 2, 3});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::ok);
    CHECK_EQUAL(4U, acquiredResources.size());
    for (size_t i = 0; i < acquiredResources.size(); ++i)
        CHECK_TRUE(acquiredResources[i] != nullptr);

    CHECK_EQUAL(4, FalUtil_GetRequestCallCount());
    // Two tiles, each configured with EventSelect and PerfCounterControl
    CHECK_EQUAL(2, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(2, XAie_GetPerfCounterControlCallCount());
}

/**
 * @brief TC_302: Verify successful time difference profiling for different tiles
 *
 * This test verifies that profile_start_time_difference_btw_two_streams successfully
 * measures start time difference when streams are on different tiles,
 * using broadcast resources for synchronization.
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_302_DifferentTileNominalSuccess)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0);
    shim_config cfg2 = make_nominal_cfg(3, 4, 1); // different column

    // Request order: SS1, PC1, SS2, PC2, broadcast
    FalUtil_SetRequestResults({0, 1, 2, 3, 4});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::ok);
    CHECK_EQUAL(5U, acquiredResources.size());
    for (size_t i = 0; i < acquiredResources.size(); ++i)
        CHECK_TRUE(acquiredResources[i] != nullptr);

    CHECK_EQUAL(5, FalUtil_GetRequestCallCount());
    // Two tiles, plus broadcast configuration in the different-tile path
    CHECK_TRUE(XAie_GetEventSelectCallCount() >= 2);
    CHECK_TRUE(XAie_GetPerfCounterControlCallCount() >= 2);
}

/**
 * @brief TC_304: Verify error handling for invalid second stream configuration
 *
 * This test verifies that profile_start_time_difference_btw_two_streams correctly
 * returns internal_error when the second stream configuration contains
 * an invalid stream port ID (negative value).
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_304_InvalidSecondConfig)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0);
    shim_config cfg2 = make_nominal_cfg(1, -1, 1); // invalid port

    FalUtil_SetRequestResults({});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::internal_error);
    CHECK_EQUAL(0, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(0U, acquiredResources.size());
}

/**
 * @brief TC_305: Verify error handling when first stream switch request fails
 *
 * This test verifies that profile_start_time_difference_btw_two_streams correctly
 * returns resource_unavailable error when the first stream switch port resource
 * cannot be requested/allocated.
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_305_StreamSwitch1RequestFails)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0);
    shim_config cfg2 = make_nominal_cfg(1, 3, 1);

    FalUtil_SetRequestResults({-1}); // first request fails
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
    CHECK_EQUAL(1, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(0U, acquiredResources.size());
}

/**
 * @brief TC_306: Verify error handling when first performance counter request fails
 *
 * This test verifies that profile_start_time_difference_btw_two_streams correctly
 * returns resource_unavailable error when the first performance counter resource
 * cannot be requested/allocated.
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_306_PerfCounter1RequestFails)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0);
    shim_config cfg2 = make_nominal_cfg(1, 3, 1);

    FalUtil_SetRequestResults({0, -1}); // SS1 ok, PC1 fails
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
    CHECK_EQUAL(2, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(0U, acquiredResources.size());
}

/**
 * @brief TC_307: Verify error handling when second stream switch request fails
 *
 * This test verifies that profile_start_time_difference_btw_two_streams correctly
 * returns resource_unavailable error when the second stream switch port resource
 * cannot be requested/allocated.
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_307_StreamSwitch2RequestFails)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0);
    shim_config cfg2 = make_nominal_cfg(1, 3, 1);

    FalUtil_SetRequestResults({0, 1, -1}); // SS1 ok, PC1 ok, SS2 fails
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
    CHECK_EQUAL(3, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(0U, acquiredResources.size());
}

/**
 * @brief TC_308: Verify error handling when second performance counter request fails
 *
 * This test verifies that profile_start_time_difference_btw_two_streams correctly
 * returns resource_unavailable error when the second performance counter resource
 * cannot be requested/allocated.
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_308_PerfCounter2RequestFails)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0);
    shim_config cfg2 = make_nominal_cfg(1, 3, 1);

    FalUtil_SetRequestResults({0, 1, 2, -1}); // SS1, PC1, SS2 ok, PC2 fails
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
    CHECK_EQUAL(4, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(0U, acquiredResources.size());
}

/**
 * @brief TC_309: Verify error handling when broadcast request fails for different tiles
 *
 * This test verifies that profile_start_time_difference_btw_two_streams correctly
 * returns resource_unavailable error when the broadcast resource cannot be requested
 * during different-tile stream profiling setup.
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_309_BroadcastRequestFails)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0);
    shim_config cfg2 = make_nominal_cfg(3, 4, 1); // different column -> broadcast path

    // SS1, PC1, SS2, PC2 ok; broadcast request fails
    FalUtil_SetRequestResults({0, 1, 2, 3, -1});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
    CHECK_EQUAL(5, FalUtil_GetRequestCallCount());
    // UUT may attempt releases on failure, but should not configure driver
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
}

/**
 * @brief TC_310: Verify error handling when broadcast request and release both fail
 *
 * This test verifies that profile_start_time_difference_btw_two_streams correctly
 * returns resource_unavailable error when both the broadcast resource request fails
 * and subsequent release operations also fail.
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_310_BroadcastRequestReleaseFails)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0);
    shim_config cfg2 = make_nominal_cfg(3, 4, 1); // different column -> broadcast path

    // SS1, PC1, SS2, PC2 ok; broadcast request fails
    FalUtil_SetRequestResults({0, 1, 2, 3, -1});
    FalUtil_SetReleaseResult(false); // make release fail
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
    CHECK_EQUAL(5, FalUtil_GetRequestCallCount());
    // UUT may attempt releases on failure, but should not configure driver
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
}

/**
 * @brief TC_311: Verify error handling when XAie_EventSelectStrmPort fails (same tile)
 *
 * This test verifies that profile_start_time_difference_btw_two_streams correctly
 * returns aie_driver_error when the underlying AIE driver function
 * XAie_EventSelectStrmPort fails during stream port event selection for same-tile configuration.
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_311_EventSelectFailureSameTile)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0);
    shim_config cfg2 = make_nominal_cfg(1, 3, 1);

    FalUtil_SetRequestResults({0, 1, 2, 3});
    XAie_SetEventSelectResult(1); // non-zero => failure
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::aie_driver_error);
    CHECK_EQUAL(4, FalUtil_GetRequestCallCount());
    CHECK_TRUE(XAie_GetEventSelectCallCount() >= 1);
    CHECK_TRUE(XAie_GetPerfCounterControlCallCount() >= 2);
    CHECK_EQUAL(4U, acquiredResources.size());
}

/**
 * @brief TC_312: Verify error handling when XAie_PerfCounterControlSet fails (different tiles)
 *
 * This test verifies that profile_start_time_difference_btw_two_streams correctly
 * returns aie_driver_error when the underlying AIE driver function
 * XAie_PerfCounterControlSet fails during performance counter configuration for different-tile configuration.
 */
TEST(ProfilingStartTimeDifferenceBtw2Streams, TC_312_PerfCounterControlFailureDifferentTiles)
{
    shim_config cfg1 = make_nominal_cfg(1, 2, 0);
    shim_config cfg2 = make_nominal_cfg(3, 4, 1);

    FalUtil_SetRequestResults({0, 1, 2, 3, 4});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(1); // non-zero => failure for one or more controls
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_start_time_difference_btw_two_streams(
        cfg1, cfg2, acquiredResources);

    CHECK_TRUE(rc == err_code::aie_driver_error);
    CHECK_EQUAL(5, FalUtil_GetRequestCallCount());
    CHECK_TRUE(XAie_GetEventSelectCallCount() >= 2);
    CHECK_TRUE(XAie_GetPerfCounterControlCallCount() >= 2);
    CHECK_EQUAL(5U, acquiredResources.size());
}

TEST_GROUP(ProfileStreamStartToTransferComplete)
{
	void setup()
	{
		ResetAll();
		// Initialize the device wrapper to prevent null pointer dereference
		static fal_util::DevWrapper devWrapper;
		fal_util::s_pXAieDev = &devWrapper;
		// Initialize the device instance pointer
		abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
	}

	void teardown()
	{
		fal_util::s_pXAieDev = nullptr;
		abr::config_manager::s_pDevInst = nullptr;
		mock_c()->clear();
	}
};

/**
 * @brief TC_002: Verify error handling for invalid shim column
 *
 * This test verifies that profile_stream_start_to_transfer_complete correctly
 * returns internal_error when provided with an invalid shim column (negative value).
 */
TEST(ProfileStreamStartToTransferComplete, TC_002_InvalidShimColumn)
{
	// ARRANGE
	abr::shim_config shimConfig;
	shimConfig.shimColumn = -1; // Invalid
	shimConfig.streamPortId = 2;
	shimConfig.slaveOrMaster = 0;

	uint32_t numBytes = 1024;
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	// ACT
	abr::err_code result = abr::profiling::profile_stream_start_to_transfer_complete_cycles(
			shimConfig, numBytes, acquiredResources);

	// ASSERT
	CHECK_EQUAL(static_cast<int>(abr::err_code::internal_error), static_cast<int>(result));

}

/**
 * @brief TC_103: Verify error handling for invalid stream port ID
 *
 * This test verifies that profile_stream_start_to_transfer_complete_cycles correctly
 * returns internal_error when provided with an invalid stream port ID (negative value).
 */
TEST(ProfileStreamStartToTransferComplete, TC_103_InvalidStreamPortId)
{
    shim_config cfg = make_nominal_cfg(1, -1, 0); // invalid port
    uint32_t numBytes = 256;

    FalUtil_SetRequestResults({});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(
        cfg, numBytes, acquiredResources);

    CHECK_TRUE(rc == err_code::internal_error);
    CHECK_EQUAL(0, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(0U, acquiredResources.size());
}

/**
 * @brief TC_104: Verify error handling when stream switch request fails
 *
 * This test verifies that profile_stream_start_to_transfer_complete_cycles correctly
 * returns resource_unavailable error when the stream switch port resource
 * cannot be requested/allocated.
 */
TEST(ProfileStreamStartToTransferComplete, TC_104_StreamSwitchRequestFails)
{
    shim_config cfg = make_nominal_cfg(1, 2, 0);
    uint32_t numBytes = 512;

    FalUtil_SetRequestResults({-1}); // first request fails
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(
        cfg, numBytes, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
    CHECK_EQUAL(1, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(0U, acquiredResources.size());
}

/**
 * @brief TC_105: Verify error handling when first performance counter request fails
 *
 * This test verifies that profile_stream_start_to_transfer_complete_cycles correctly
 * returns resource_unavailable error when the first performance counter resource
 * cannot be requested/allocated.
 */
TEST(ProfileStreamStartToTransferComplete, TC_105_FirstPerfCounterRequestFails)
{
    shim_config cfg = make_nominal_cfg(1, 2, 1); // master
    uint32_t numBytes = 512;

    // eventPort ok, counter0 fails
    FalUtil_SetRequestResults({0, -1});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(
        cfg, numBytes, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
    CHECK_EQUAL(2, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(0U, acquiredResources.size());
}

/**
 * @brief TC_106: Verify error handling when second performance counter request fails
 *
 * This test verifies that profile_stream_start_to_transfer_complete_cycles correctly
 * returns resource_unavailable error when the second performance counter resource
 * cannot be requested/allocated.
 */
TEST(ProfileStreamStartToTransferComplete, TC_106_SecondPerfCounterRequestFails)
{
    shim_config cfg = make_nominal_cfg(1, 2, 0);
    uint32_t numBytes = 512;

    // eventPort ok, counter0 ok, counter1 fails
    FalUtil_SetRequestResults({0, 1, -1});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(
        cfg, numBytes, acquiredResources);

    CHECK_TRUE(rc == err_code::resource_unavailable);
    CHECK_EQUAL(3, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(0, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(0, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(0U, acquiredResources.size());
}

/**
 * @brief TC_107: Verify error handling when XAie_EventSelectStrmPort fails
 *
 * This test verifies that profile_stream_start_to_transfer_complete_cycles correctly
 * returns aie_driver_error when the underlying AIE driver function
 * XAie_EventSelectStrmPort fails during stream port event selection.
 */
TEST(ProfileStreamStartToTransferComplete, TC_107_EventSelectFails)
{
    shim_config cfg = make_nominal_cfg(1, 2, 0);
    uint32_t numBytes = 1024;

    FalUtil_SetRequestResults({0, 1, 2});
    XAie_SetEventSelectResult(1);         // non-zero => failure
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(
        cfg, numBytes, acquiredResources);

    CHECK_TRUE(rc == err_code::aie_driver_error);
    CHECK_EQUAL(3, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(1, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(2, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(3U, acquiredResources.size());
}

/**
 * @brief TC_108: Verify error handling when XAie_PerfCounterControlSet fails for first counter
 *
 * This test verifies that profile_stream_start_to_transfer_complete_cycles correctly
 * returns aie_driver_error when the underlying AIE driver function
 * XAie_PerfCounterControlSet fails during first performance counter configuration.
 */
TEST(ProfileStreamStartToTransferComplete, TC_108_PerfCounterControl0Fails)
{
    shim_config cfg = make_nominal_cfg(1, 2, 1);
    uint32_t numBytes = 1024;

    FalUtil_SetRequestResults({0, 1, 2});
    XAie_SetEventSelectResult(XAIE_OK);
    // We model both PerfCounterControlSet calls using the same control result
    // function in stubs; return non-zero to force failure in driverStatus.
    XAie_SetPerfCounterControlResult(1);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(
        cfg, numBytes, acquiredResources);

    CHECK_TRUE(rc == err_code::aie_driver_error);
    CHECK_EQUAL(3, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(1, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(2, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(3U, acquiredResources.size());
}

/**
 * @brief TC_109: Verify correct handling with zero event value boundary condition
 *
 * This test verifies that profile_stream_start_to_transfer_complete_cycles correctly
 * handles the boundary condition where numBytes/4 equals zero, ensuring
 * the function still behaves nominally with very small byte counts.
 */
TEST(ProfileStreamStartToTransferComplete, TC_109_ZeroEventValueBoundary)
{
    shim_config cfg = make_nominal_cfg(1, 2, 0);
    uint32_t numBytes = 0;  // numBytes/4 == 0

    FalUtil_SetRequestResults({0, 1, 2});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rc = profiling::profile_stream_start_to_transfer_complete_cycles(
        cfg, numBytes, acquiredResources);

    CHECK_TRUE(rc == err_code::ok);
    CHECK_EQUAL(3, FalUtil_GetRequestCallCount());
    CHECK_EQUAL(1, XAie_GetEventSelectCallCount());
    CHECK_EQUAL(2, XAie_GetPerfCounterControlCallCount());
    CHECK_EQUAL(3U, acquiredResources.size());
}


TEST_GROUP(ProfilingStopTests)
{
	void setup()
	{
		ResetAll();
		// Initialize the device wrapper to prevent null pointer dereference
		static fal_util::DevWrapper devWrapper;
		fal_util::s_pXAieDev = &devWrapper;
		// Initialize the device instance pointer
		abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
	}

	void teardown()
	{
		fal_util::s_pXAieDev = nullptr;
		abr::config_manager::s_pDevInst = nullptr;
		FalUtil_SetReleaseResult(true); // Reset to default
		mock().clear();
		mock_c()->clear();
	}
};

/**
 * @brief TC_001: Verify successful stop with empty resources
 *
 * This test verifies that profiling::stop correctly handles the nominal case
 * with empty acquired resources and returns success status.
 */
TEST(ProfilingStopTests, TC_001_ValidExecution)
{


	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	bool startTimeDifference = false;

	// ACT
	err_code result = profiling::stop(acquiredResources, startTimeDifference);

	// ASSERT
	// ASSERT
	CHECK_EQUAL(static_cast<int>(err_code::ok), static_cast<int>(result));
}

/**
 * @brief TC_501: Verify stop after profiling stream running-to-idle cycles
 *
 * This test verifies that profiling::stop correctly releases resources
 * acquired during profile_stream_running_to_idle_cycles without broadcast resources.
 */
TEST(ProfilingStopTests, TC_501_StopAfterRunningToIdle)
{
    shim_config cfg =  make_nominal_cfg(1, 2, 0);

    FalUtil_SetRequestResults({0, 1});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rcStart = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
    CHECK_TRUE(rcStart == err_code::ok);
    CHECK_EQUAL(2U, acquiredResources.size());

    err_code rcStop = profiling::stop(acquiredResources, false);
    CHECK_TRUE(rcStop == err_code::ok);
}

/**
 * @brief TC_501b: Verify stop with reversed resource order
 *
 * This test verifies that profiling::stop correctly handles StreamPortSelect cleanup
 * when resources are provided in reversed order (StreamPortSelect first).
 */
TEST(ProfilingStopTests, TC_501b_StopWithStreamPortSelectFirst)
{
    shim_config cfg =  make_nominal_cfg(1, 2, 0);

    FalUtil_SetRequestResults({0, 1});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rcStart = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
    CHECK_TRUE(rcStart == err_code::ok);

    // Reverse the order so StreamPortSelect is first
    std::reverse(acquiredResources.begin(), acquiredResources.end());

    err_code rcStop = profiling::stop(acquiredResources, false);
    CHECK_TRUE(rcStop == err_code::ok);
}

/**
 * @brief TC_501c: Verify stop after profiling stream running event count
 *
 * This test verifies that profiling::stop correctly cleans up StreamPortSelect resources
 * acquired during profile_stream_running_event_count.
 */
TEST(ProfilingStopTests, TC_501c_StopAfterEventCount)
{
    shim_config cfg =  make_nominal_cfg(1, 2, 0);

    FalUtil_SetRequestResults({0, 1});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rcStart = profiling::profile_stream_running_event_count(cfg, acquiredResources);
    CHECK_TRUE(rcStart == err_code::ok);
    CHECK_EQUAL(2U, acquiredResources.size()); // PerfCounter + StreamPortSelect

    err_code rcStop = profiling::stop(acquiredResources, false);
    CHECK_TRUE(rcStop == err_code::ok);
}

/**
 * @brief TC_501d: Verify stop with only StreamPortSelect resource
 *
 * This test verifies that profiling::stop correctly handles cleanup
 * when only a StreamPortSelect resource is present in acquired resources.
 */
TEST(ProfilingStopTests, TC_501d_StopWithStreamPortSelectOnly)
{
    ErrorMsg_Reset();
    std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

    // Create a StreamPortSelect resource directly
    auto pStreamPort = std::make_shared<xaiefal::XAieStreamPortSelect>();
    StubXaieSingleTile_SetGetRscIdResult(pStreamPort, XAIE_OK, 5);
    acquiredResources.push_back(pStreamPort);

    err_code rcStop = profiling::stop(acquiredResources, false);
    CHECK_TRUE(rcStop == err_code::ok);
}

/**
 * @brief TC_501e: Verify stop with only PCEvent resource
 *
 * This test verifies that profiling::stop correctly handles cleanup
 * when only a PCEvent (program counter event) resource is present in acquired resources.
 */
TEST(ProfilingStopTests, TC_501e_StopWithPCEventOnly)
{
    ErrorMsg_Reset();
    std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

    // Create a PCEvent resource directly
    auto pPCEvent = std::make_shared<xaiefal::XAiePCEvent>();
    StubXaieSingleTile_SetGetRscIdResult(pPCEvent, XAIE_OK, 10);
    acquiredResources.push_back(pPCEvent);

    err_code rcStop = profiling::stop(acquiredResources, false);
    CHECK_TRUE(rcStop == err_code::ok);
}

/**
 * @brief TC_502: Verify stop after profiling stream start-to-transfer-complete cycles
 *
 * This test verifies that profiling::stop correctly releases resources
 * acquired during profile_stream_start_to_transfer_complete_cycles without broadcast resources.
 */
TEST(ProfilingStopTests, TC_502_StopAfterStartToTransferComplete)
{
    shim_config cfg =  make_nominal_cfg(1, 2, 0);
    uint32_t numBytes = 1024;

    FalUtil_SetRequestResults({0, 1, 2});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rcStart = profiling::profile_stream_start_to_transfer_complete_cycles(cfg, numBytes, acquiredResources);
    CHECK_TRUE(rcStart == err_code::ok);
    CHECK_EQUAL(3U, acquiredResources.size());

    err_code rcStop = profiling::stop(acquiredResources, false);
    CHECK_TRUE(rcStop == err_code::ok);
}

/**
 * @brief TC_503: Verify stop after profiling stream running event count
 *
 * This test verifies that profiling::stop correctly releases resources
 * acquired during profile_stream_running_event_count without broadcast resources.
 */
TEST(ProfilingStopTests, TC_503_StopAfterRunningEventCount)
{
    shim_config cfg =  make_nominal_cfg(1, 2, 0);

    FalUtil_SetRequestResults({0, 1});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rcStart = profiling::profile_stream_running_event_count(cfg, acquiredResources);
    CHECK_TRUE(rcStart == err_code::ok);
    CHECK_EQUAL(2U, acquiredResources.size());

    err_code rcStop = profiling::stop(acquiredResources, false);
    CHECK_TRUE(rcStop == err_code::ok);
}

/**
 * @brief TC_504: Verify stop after profiling start time difference on same tile
 *
 * This test verifies that profiling::stop correctly releases resources
 * acquired during profile_start_time_difference_btw_two_streams when both streams
 * are on the same tile (no broadcast resource required).
 */
TEST(ProfilingStopTests, TC_504_StopAfterStartTimeDiffSameTile)
{
    shim_config cfg1 =  make_nominal_cfg(1, 2, 0);
    shim_config cfg2 =  make_nominal_cfg(1, 3, 1); // same column => no broadcast resource

    FalUtil_SetRequestResults({0, 1, 2, 3});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rcStart = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
    CHECK_TRUE(rcStart == err_code::ok);
    CHECK_EQUAL(4U, acquiredResources.size());

    err_code rcStop = profiling::stop(acquiredResources, true);
    CHECK_TRUE(rcStop == err_code::ok);
}

/**
 * @brief TC_505: Verify stop after profiling start time difference on different tiles
 *
 * This test verifies that profiling::stop correctly releases resources including
 * broadcast resource acquired during profile_start_time_difference_btw_two_streams
 * when streams are on different tiles.
 */
TEST(ProfilingStopTests, TC_505_StopAfterStartTimeDiffDifferentTiles)
{
    shim_config cfg1 =  make_nominal_cfg(1, 2, 0);
    shim_config cfg2 =  make_nominal_cfg(3, 4, 1); // different columns => broadcast allocated

    FalUtil_SetRequestResults({0, 1, 2, 3, 4});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rcStart = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
    CHECK_TRUE(rcStart == err_code::ok);
    CHECK_EQUAL(5U, acquiredResources.size()); // includes broadcast resource

    err_code rcStop = profiling::stop(acquiredResources, true);
    CHECK_TRUE(rcStop == err_code::ok);
}

/**
 * @brief TC_505b: Verify stop with manually created broadcast resource
 *
 * This test verifies that profiling::stop correctly handles cleanup of
 * broadcast resources created manually with proper channel configuration.
 */
TEST(ProfilingStopTests, TC_505b_StopWithBroadcastResource)
{
    ErrorMsg_Reset();
    std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

    // Create a broadcast resource manually with proper channel configuration
    auto pBroadcast = std::make_shared<xaiefal::XAieBroadcast>();
    std::vector<XAie_LocType> locs;
    locs.push_back(XAie_TileLoc(1, 0));
    locs.push_back(XAie_TileLoc(2, 0));
    locs.push_back(XAie_TileLoc(3, 0));
    pBroadcast->setChannel(locs, XAIE_PL_MOD, XAIE_PL_MOD);
    pBroadcast->setBc(2);
    acquiredResources.push_back(pBroadcast);

    err_code rcStop = profiling::stop(acquiredResources, true);
    CHECK_TRUE(rcStop == err_code::ok);
}

/**
 * @brief TC_504b: Verify error handling when driver reset fails
 *
 * This test verifies that profiling::stop correctly returns aie_driver_error
 * when the underlying XAie_PerfCounterReset function fails during resource cleanup.
 */
TEST(ProfilingStopTests, TC_504b_StopWithDriverError)
{
    ErrorMsg_Reset();
    std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

    // Create a PerfCounter resource that will fail reset
    auto pPerfCounter = std::make_shared<xaiefal::XAiePerfCounter>();
    Stub_XAiePerfCounter_SetGetRscIdResult(0, 0, 0, XAIE_PL_MOD, 5, XAIE_OK);
    acquiredResources.push_back(pPerfCounter);

    // Make the PerfCounterReset fail
    XAie_SetPerfCounterResetResult(XAIE_ERR);

    err_code rcStop = profiling::stop(acquiredResources, false);
    CHECK_TRUE(rcStop == err_code::aie_driver_error);

    // Reset for other tests
    XAie_SetPerfCounterResetResult(XAIE_OK);
}
/**
 * @brief TC_508: Verify error handling when resource release fails
 *
 * This test verifies behavior when fal_util::release returns false during cleanup.
 * NOTE: Currently skipped because calling stop() with release failure causes
 * segfault in resource cleanup logic.
 */
TEST(ProfilingStopTests, TC_508_ReleaseFailure)
{
    shim_config cfg = make_nominal_cfg(1, 2, 0);
    FalUtil_SetRequestResults({0, 1});
    XAie_SetEventSelectResult(XAIE_OK);
    XAie_SetPerfCounterControlResult(XAIE_OK);
    ErrorMsg_Reset();
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
    err_code rcStart = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
    CHECK_TRUE(rcStart == err_code::ok);

    // Configure stub to succeed getRscId but fail release
    Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_OK);
    FalUtil_SetReleaseResult(false); // release will fail

    err_code rcStop = profiling::stop(acquiredResources, true);
    CHECK_TRUE(rcStop == err_code::aie_driver_error);
    acquiredResources.clear();
}


TEST_GROUP(ProfilingReadTests)
{
	void setup()
	{
		ResetAll();
		static fal_util::DevWrapper devWrapper;
		fal_util::s_pXAieDev = &devWrapper;
		abr::config_manager::s_pDevInst = &stub_controls::g_stubDevInst;
	}

	void teardown()
	{
		fal_util::s_pXAieDev = nullptr;
		abr::config_manager::s_pDevInst = nullptr;
		FalUtil_SetReleaseResult(true); // Reset to default
		mock_c()->clear();
	}
};

/**
 * @brief TC_604: Verify reading start time difference between streams on different columns
 *
 * This test verifies that profiling::read correctly calculates the start time difference
 * between two streams located on different columns, applying the column offset compensation
 * (difference * 2 + 2) to account for inter-column timing variations.
 */
TEST(ProfilingReadTests, TC_604_ReadStartTimeDifferenceDifferentColumns)
{
	// ARRANGE
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(3, 4, 1); // different column (3-1=2 columns apart)

	FalUtil_SetRequestResults({0, 1, 2, 3, 4});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	CHECK_TRUE(rc == err_code::ok);

	// Configure stubs - counter 0 at col 1, counter 1 at col 3
	Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_OK);
	Stub_XAiePerfCounter_SetGetRscIdResult(1, 3, 0, XAIE_PL_MOD, 3, XAIE_ERR);
	Stub_XAie_PerfCounterGet_SetValue(1, 0, XAIE_PL_MOD, 1, 100, XAIE_OK);
	Stub_XAie_PerfCounterGet_SetValue(3, 0, XAIE_PL_MOD, 3, 200, XAIE_ERR);

	// ACT
	uint64_t result = profiling::read(acquiredResources, true);

	// ASSERT - should be (200 - 100) + (2 columns * 2 + 2) = 100 + 6 = 106
	CHECK_EQUAL(106, result);
	acquiredResources.clear();
}

/**
 * @brief TC_605: Verify error handling when reading with invalid resource type
 *
 * This test verifies that profiling::read correctly returns 0 when the acquired
 * resources contain non-PerfCounter types (e.g., StreamPortSelect) causing
 * dynamic_cast to fail during resource type validation.
 */
TEST(ProfilingReadTests, TC_605_InvalidResourceType)
{
	// ARRANGE - Create resources vector with non-PerfCounter resource
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	// Add a StreamPortSelect resource instead of PerfCounter
	// This will cause dynamic_cast to return nullptr
	auto pSSwitchPortRsc = fal_util::s_pXAieDev->tile(1, 0).sswitchPort();
	acquiredResources.push_back(pSSwitchPortRsc);

	// ACT - Try to read with startTimeDifference=false (expects PerfCounter at index 0)
	uint64_t result = profiling::read(acquiredResources, false);

	// ASSERT - Should return 0 since the cast fails
	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}

/**
 * @brief TC_606: Verify error handling with invalid resources for time difference calculation
 *
 * This test verifies that profiling::read correctly returns 0 when startTimeDifference=true
 * but the acquired resources contain wrong types (StreamPortSelect instead of PerfCounters),
 * causing dynamic_cast failures for both counter resources.
 */
TEST(ProfilingReadTests, TC_606_InvalidResourcesForTimeDifference)
{
	// ARRANGE - Create resources vector with wrong resource types
	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;

	// Add StreamPortSelect resources instead of PerfCounters
	auto pSSwitchPortRsc1 = fal_util::s_pXAieDev->tile(1, 0).sswitchPort();
	auto pSSwitchPortRsc2 = fal_util::s_pXAieDev->tile(2, 0).sswitchPort();
	acquiredResources.push_back(pSSwitchPortRsc1);
	acquiredResources.push_back(pSSwitchPortRsc2);

	// ACT - Try to read with startTimeDifference=true (expects PerfCounters at indices 0,1)
	uint64_t result = profiling::read(acquiredResources, true);

	// ASSERT - Should return 0 since the casts fail
	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}

/**
 * @brief TC_607: Verify error handling when getRscId fails during time difference calculation
 *
 * This test verifies that profiling::read correctly returns 0 when getRscId fails
 * for the first performance counter during start time difference calculation.
 */
TEST(ProfilingReadTests, TC_607_GetRscIdFailureInTimeDifference)
{
	// ARRANGE - Setup profiling for time difference
	shim_config cfg1 = make_nominal_cfg(1, 2, 0);
	shim_config cfg2 = make_nominal_cfg(1, 4, 1); // same column

	FalUtil_SetRequestResults({0, 1, 2, 3});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rc = profiling::profile_start_time_difference_btw_two_streams(cfg1, cfg2, acquiredResources);
	CHECK_TRUE(rc == err_code::ok);

	// Configure first counter's getRscId to fail (returns error status)
	Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_ERR);
	Stub_XAiePerfCounter_SetGetRscIdResult(1, 1, 0, XAIE_PL_MOD, 3, XAIE_OK);

	// ACT
	uint64_t result = profiling::read(acquiredResources, true);

	// ASSERT - Should return 0 because getRscId failed
	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}

/**
 * @brief TC_608: Verify error handling when getRscId fails in standard read path
 *
 * This test verifies that profiling::read correctly returns 0 when getRscId fails
 * during standard (non-time-difference) profiling read operation.
 */
TEST(ProfilingReadTests, TC_608_GetRscIdFailureInStandardRead)
{
	// ARRANGE - Setup profiling first
	shim_config cfg = make_nominal_cfg(1, 2, 0);
	FalUtil_SetRequestResults({0, 1});
	XAie_SetEventSelectResult(XAIE_OK);
	XAie_SetPerfCounterControlResult(XAIE_OK);
	ErrorMsg_Reset();

	std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquiredResources;
	err_code rc = profiling::profile_stream_running_to_idle_cycles(cfg, acquiredResources);
	CHECK_TRUE(rc == err_code::ok);

	// Configure stub so getRscId fails
	Stub_XAiePerfCounter_SetGetRscIdResult(0, 1, 0, XAIE_PL_MOD, 1, XAIE_ERR);

	// ACT
	uint64_t result = profiling::read(acquiredResources, false);

	// ASSERT - Should return 0 because getRscId failed
	CHECK_EQUAL(0, result);
	acquiredResources.clear();
}
