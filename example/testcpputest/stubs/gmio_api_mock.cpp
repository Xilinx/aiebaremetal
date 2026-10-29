// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "common_layer/aeg_runtime_api.h"
#include "stubs.h"

namespace abr {

gmio_api::gmio_api(const gmio_config *pConfig) noexcept
    : pGMIOConfig(pConfig), isConfigured(false), dmaStartQMaxSize(4) {}

err_code gmio_api::configure() noexcept {
    if (g_ut_gmio_configure_force_code != 0) {
        return static_cast<err_code>(g_ut_gmio_configure_force_code);
    }
    if (g_ut_gmio_configure_fail != 0) {
        return err_code::aie_driver_error;
    }
    isConfigured = true;
    return err_code::ok;
}

err_code gmio_api::enqueueBD(uint64_t , size_t ) noexcept {
    if (!isConfigured) {
        return err_code::internal_error;
    }
    if (g_ut_externalbuffer_force_dma_enqueue_fail != 0) {
        return err_code::aie_driver_error;
    }
    return err_code::ok;
}

err_code gmio_api::wait(bool syncmode) noexcept {
    if (g_ut_gmio_wait_nb_fail != 0 && !syncmode) {
        return err_code::resource_unavailable;
    }
    return err_code::ok;
}

err_code dma_api::configureBdWaitQueueEnqueueTask(int32_t , uint8_t , uint8_t ,
                                                   int32_t , uint8_t , uint32_t ,
                                                   bool , std::vector<uint8_t> ,
                                                   std::vector<dma_api::buffer_descriptor> ) noexcept {
    return err_code::ok;
}

err_code dma_api::configureBD(int32_t , uint8_t , uint8_t , uint8_t ,
                              const dma_api::buffer_descriptor &) noexcept {
    return err_code::ok;
}

err_code dma_api::enqueueTask(int32_t , uint8_t , uint8_t , int32_t ,
                               uint8_t , uint32_t , bool ,
                               uint8_t ) noexcept {
    if (g_ut_externalbuffer_force_dma_enqueue_fail != 0) {
        return err_code::aie_driver_error;
    }
    return err_code::ok;
}

err_code dma_api::waitDMAChannelTaskQueue(int32_t , uint8_t , uint8_t , int32_t ,
                                           uint8_t ) noexcept {
    return err_code::ok;
}

err_code dma_api::waitDMAChannelDone(int32_t , uint8_t , uint8_t , int32_t ,
                                      uint8_t ) noexcept {
    return err_code::ok;
}

err_code dma_api::updateBDAddress(int32_t , uint8_t , uint8_t , uint8_t ,
                                   uint64_t ) noexcept {
    if (g_ut_externalbuffer_force_dma_update_bd_fail != 0) {
        return err_code::aie_driver_error;
    }
    return err_code::ok;
}

}
