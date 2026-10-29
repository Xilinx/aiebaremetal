// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "common_layer/aeg_api_message.h"
#include "common_layer/aeg_runtime_api.h"
#include "stubs.h"

namespace abr {

XAie_DevInst *config_manager::s_pDevInst = nullptr;
bool config_manager::s_bInitialized = false;
size_t config_manager::s_num_reserved_rows = 0;
bool config_manager::s_broadcast_enable_core = false;

err_code config_manager::initialize(XAie_DevInst *devInst, size_t num_reserved_rows,
                                    bool broadcast_enable_core) noexcept {
    s_pDevInst = devInst;
    s_num_reserved_rows = num_reserved_rows;
    s_broadcast_enable_core = broadcast_enable_core;
    s_bInitialized = true;
    return err_code::ok;
}

graph_api::graph_api(const graph_config *pConfig) noexcept
    : pGraphConfig(pConfig), isConfigured(false), isRunning(false), startTime(0) {}

err_code graph_api::configure() noexcept {
    if (!pGraphConfig) {
        return errorMsg(err_code::internal_error, "ERROR: abr::graph_api::configure: Invalid graph configuration.");
    }
    isConfigured = true;
    return err_code::ok;
}

err_code graph_api::run() noexcept {
    return err_code::ok;
}

err_code graph_api::run(int32_t ) noexcept {
    if (::g_ut_graph_api_run_int_fail != 0) {
        return err_code::aie_driver_error;
    }
    return err_code::ok;
}

err_code graph_api::wait() noexcept {
    return err_code::ok;
}

err_code graph_api::wait(uint64_t ) noexcept {
    return err_code::ok;
}

err_code graph_api::resume() noexcept {
    return err_code::ok;
}

err_code graph_api::end() noexcept {
    return err_code::ok;
}

err_code graph_api::end(uint64_t ) noexcept {
    return err_code::ok;
}

err_code graph_api::update(const shared_buffer_config * , const void * ,
                           size_t ) const noexcept {
    return err_code::ok;
}

err_code graph_api::update(const rtp_config * , const void * , size_t ) noexcept {
    if (::g_ut_graph_api_update_force_code != 0) {
        return static_cast<err_code>(::g_ut_graph_api_update_force_code);
    }
    if (::g_ut_graph_api_update_rtp_user_error != 0) {
        return err_code::user_error;
    }
    return err_code::ok;
}

err_code graph_api::read(const rtp_config * , void * , size_t ) const noexcept {
    return err_code::ok;
}

}
