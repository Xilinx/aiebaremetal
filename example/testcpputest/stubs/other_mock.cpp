// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "aeg_profiling_api.h"
#include "stubs.h"
#include <iostream>
#include <memory>

#ifdef FAL_UTIL_MOCK_CLASS_BASED

namespace {
bool g_fal_util_initialize_should_fail = false;
}

extern "C" void set_fal_util_initialize_should_fail(bool fail) {
    g_fal_util_initialize_should_fail = fail;
}

bool fal_util::initialize(XAie_DevInst*) {
    return !g_fal_util_initialize_should_fail;
}
int32_t fal_util::request(std::shared_ptr<xaiefal::XAieRsc>) { return 0; }
bool fal_util::release(std::shared_ptr<xaiefal::XAieRsc>) { return true; }
std::shared_ptr<xaiefal::XAieDev> fal_util::s_pXAieDev;
#else

namespace xaiefal {
class XAieRsc;
}

namespace fal_util {
void* s_pXAieDev = nullptr;
void request(std::shared_ptr<xaiefal::XAieRsc>) {}
void release(std::shared_ptr<xaiefal::XAieRsc>) {}
void initialize(XAie_DevInst*) {}
}
#endif

extern "C" {
unsigned char cert_elf[1] = {0};
unsigned int cert_elf_len = 1;
}

namespace abr
{

static bool g_enable_profiling_success = true;
static uint64_t g_profiling_counter = 1000;

shim_config::shim_config()
{
    shimColumn = -1;
    streamPortId = -1;
    slaveOrMaster = 0;
}

shim_config::shim_config(const gmio_config* pConfig)
{
    if (pConfig) {
        shimColumn = pConfig->shimColumn;
        streamPortId = pConfig->streamId;
        slaveOrMaster = 0;
    } else {
        shimColumn = -1;
        streamPortId = -1;
        slaveOrMaster = 0;
    }
}

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

err_code profiling::profile_stream_running_to_idle_cycles(
    shim_config shimConfig,
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources)
{

    if (shimConfig.shimColumn == -1 && !g_enable_profiling_success) {
        return err_code::internal_error;
    }

    if (g_enable_profiling_success) {

        return err_code::ok;
    }
    return err_code::internal_error;
}

err_code profiling::profile_stream_start_to_transfer_complete_cycles(
    shim_config shimConfig,
    uint32_t numBytes,
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources)
{

    if (shimConfig.shimColumn == -1 && !g_enable_profiling_success) {
        return err_code::internal_error;
    }

    if (g_enable_profiling_success) {
        return err_code::ok;
    }
    return err_code::internal_error;
}

err_code profiling::profile_start_time_difference_btw_two_streams(
    shim_config shimConfig1,
    shim_config shimConfig2,
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources)
{

    if ((shimConfig1.shimColumn == -1 || shimConfig2.shimColumn == -1) && !g_enable_profiling_success) {
        return err_code::internal_error;
    }

    if (g_enable_profiling_success) {
        return err_code::ok;
    }
    return err_code::internal_error;
}

err_code profiling::profile_stream_running_event_count(
    shim_config shimConfig,
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources)
{

    if (shimConfig.shimColumn == -1 && !g_enable_profiling_success) {
        return err_code::internal_error;
    }

    if (g_enable_profiling_success) {
        return err_code::ok;
    }
    return err_code::internal_error;
}

uint64_t profiling::read(
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources,
    bool startTimeDifference)
{

    return g_profiling_counter++;
}

err_code profiling::stop(
    std::vector<std::shared_ptr<xaiefal::XAieRsc>>& acquiredResources,
    bool startTimeDifference)
{

    acquiredResources.clear();
    return err_code::ok;
}

}

extern "C" void set_profiling_stub_mode(bool enable_success)
{
    abr::g_enable_profiling_success = enable_success;
}

extern "C" void reset_profiling_counter()
{
    abr::g_profiling_counter = 1000;
}
