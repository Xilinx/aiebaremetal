// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef AEG_AIE_CONTROL_API_H
#define AEG_AIE_CONTROL_API_H

#include "aeg_api_message.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace abr {
class dma_api {
  public:
    struct buffer_descriptor {
        uint64_t address = 0;
        uint32_t length = 0;
        std::vector<uint32_t> stepsize;
        std::vector<uint32_t> wrap;
        std::vector<std::pair<uint32_t, uint32_t>> padding;
        bool enable_packet = false;
        uint8_t packet_id = 0;
        uint8_t out_of_order_bd_id = 0;
        bool tlast_suppress = false;
        uint32_t iteration_stepsize = 0;
        uint16_t iteration_wrap = 0;
        uint8_t iteration_current = 0;
        bool enable_compression = false;
        bool lock_acq_enable = false;
        int8_t lock_acq_value = 0;
        uint8_t lock_acq_id = 0;
        int8_t lock_rel_value = 0;
        uint8_t lock_rel_id = 0;
        bool use_next_bd = false;
        uint8_t next_bd = 0;
        uint8_t burst_length = 4;
    };

    static err_code configureBdWaitQueueEnqueueTask(int32_t tileType, uint8_t column, uint8_t row, int32_t dir,
                                                    uint8_t channel, uint32_t repeatCount,
                                                    bool enableTaskCompleteToken, std::vector<uint8_t> bdIds,
                                                    std::vector<dma_api::buffer_descriptor> bdParams) noexcept;

    static err_code configureBD(int32_t tileType, uint8_t column, uint8_t row, uint8_t bdId,
                                const dma_api::buffer_descriptor &bdParam) noexcept;

    static err_code enqueueTask(int32_t tileType, uint8_t column, uint8_t row, int32_t dir, uint8_t channel,
                                uint32_t repeatCount, bool enableTaskCompleteToken, uint8_t startBdId) noexcept;

    static err_code waitDMAChannelTaskQueue(int32_t tileType, uint8_t column, uint8_t row, int32_t dir,
                                            uint8_t channel) noexcept;

    static err_code waitDMAChannelDone(int32_t tileType, uint8_t column, uint8_t row, int32_t dir,
                                       uint8_t channel) noexcept;

    static err_code updateBDAddress(int32_t tileType, uint8_t column, uint8_t row, uint8_t bdId,
                                    uint64_t address) noexcept;
};

class lock_api {
  public:
    static err_code initializeLock(int32_t tileType, uint8_t column, uint8_t row, uint16_t lockId,
                                   int8_t initVal) noexcept;

    static err_code acquireLock(int32_t tileType, uint8_t column, uint8_t row, uint16_t lockId, int8_t acqVal) noexcept;

    static err_code releaseLock(int32_t tileType, uint8_t column, uint8_t row, uint16_t lockId, int8_t relVal) noexcept;
};
}

#endif
