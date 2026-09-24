#include "aeg_profiling_api.h"
#include "stubs.h"
#include <iostream>
#include <memory>

namespace xaiefal {
class XAieRsc;
}

namespace fal_util {
void* s_pXAieDev = nullptr;
void request(std::shared_ptr<xaiefal::XAieRsc>) {}
void release(std::shared_ptr<xaiefal::XAieRsc>) {}
void initialize(XAie_DevInst*) {}
}

extern "C" {
unsigned char cert_elf[1] = {0};
unsigned int cert_elf_len = 1;
}

namespace abr
{

// Global control variables for test simulation
static bool g_enable_profiling_success = true;
static uint64_t g_profiling_counter = 1000;

/**
 * @brief Default stub constructor for shim_config
 * Creates an invalid/empty configuration
 */
shim_config::shim_config()
{
    shimColumn = -1;
    streamPortId = -1;
    slaveOrMaster = 0;
}

/**
 * @brief Stub constructor for shim_config from gmio_config
 */
shim_config::shim_config(const gmio_config* pConfig)
{
    if (pConfig) {
        shimColumn = pConfig->shimColumn;
        streamPortId = pConfig->streamId;
        slaveOrMaster = 0; // GMIO doesn't have slaveOrMaster field
    } else {
        shimColumn = -1;
        streamPortId = -1;
        slaveOrMaster = 0;
    }
}

/**
 * @brief Stub constructor for shim_config from plio_config
 */
shim_config::shim_config(const plio_config* pConfig)
{
    if (pConfig) {
        shimColumn = pConfig->shimColumn;
        streamPortId = pConfig->streamId;
        slaveOrMaster = pConfig->slaveOrMaster;
    } else {
        shimColumn = -1;
        streamPortId = -1;
        slaveOrMaster = 0;
    }
}

/**
 * @brief Stub for profile_stream_running_to_idle_cycles
 * Returns error if shimConfig is invalid, ok if g_enable_profiling_success is true, otherwise error
 */
err_code profiling::profile_stream_running_to_idle_cycles(
    shim_config shimConfig,
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources)
{
    // Check for invalid config (ambiguous or not found port) - only in strict mode
    if (shimConfig.shimColumn == -1 && !g_enable_profiling_success) {
        return err_code::internal_error;
    }

    if (g_enable_profiling_success) {
        // Simulate successful resource acquisition by adding a dummy resource
        // In real implementation, this would allocate performance counters
        return err_code::ok;
    }
    return err_code::internal_error;
}

/**
 * @brief Stub for profile_stream_start_to_transfer_complete_cycles
 * Returns error if shimConfig is invalid, ok if g_enable_profiling_success is true
 */
err_code profiling::profile_stream_start_to_transfer_complete_cycles(
    shim_config shimConfig,
    uint32_t numBytes,
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources)
{
    // Check for invalid config - only in strict mode
    if (shimConfig.shimColumn == -1 && !g_enable_profiling_success) {
        return err_code::internal_error;
    }

    if (g_enable_profiling_success) {
        return err_code::ok;
    }
    return err_code::internal_error;
}

/**
 * @brief Stub for profile_start_time_difference_btw_two_streams
 * Returns error if either shimConfig is invalid, ok if g_enable_profiling_success is true
 */
err_code profiling::profile_start_time_difference_btw_two_streams(
    shim_config shimConfig1,
    shim_config shimConfig2,
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources)
{
    // Check for invalid configs - only in strict mode
    if ((shimConfig1.shimColumn == -1 || shimConfig2.shimColumn == -1) && !g_enable_profiling_success) {
        return err_code::internal_error;
    }

    if (g_enable_profiling_success) {
        return err_code::ok;
    }
    return err_code::internal_error;
}

/**
 * @brief Stub for profile_stream_running_event_count
 * Returns error if shimConfig is invalid, ok if g_enable_profiling_success is true
 */
err_code profiling::profile_stream_running_event_count(
    shim_config shimConfig,
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources)
{
    // Check for invalid config - only in strict mode
    if (shimConfig.shimColumn == -1 && !g_enable_profiling_success) {
        return err_code::internal_error;
    }

    if (g_enable_profiling_success) {
        return err_code::ok;
    }
    return err_code::internal_error;
}

/**
 * @brief Stub for read - returns simulated profiling data
 * Returns incrementing counter value to simulate profiling measurements
 */
uint64_t profiling::read(
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources,
    bool startTimeDifference)
{
    // Return simulated profiling data
    return g_profiling_counter++;
}

/**
 * @brief Stub for stop - cleans up profiling resources
 * Always returns ok in stub implementation
 */
err_code profiling::stop(
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources,
    bool startTimeDifference)
{
    // Clear the acquired resources vector to simulate cleanup
    acquiredResources.clear();
    return err_code::ok;
}

} // namespace abr

/**
 * @brief Helper function to enable/disable profiling success for testing
 * Call this from test cases to control stub behavior
 */
extern "C" void set_profiling_stub_mode(bool enable_success)
{
    abr::g_enable_profiling_success = enable_success;
}

/**
 * @brief Helper function to reset profiling counter
 */
extern "C" void reset_profiling_counter()
{
    abr::g_profiling_counter = 1000;
}
