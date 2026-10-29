/**<!--
 * Copyright (C) 2021-2022 Xilinx, Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 * -->
 */
// Original code by AMD Xilinx
// This copy is maintained by the SSW AIE Team
// Modifications by SSW AIE Team on 2024-01-18
// Description: Correctly handle the aie driver version check

/*
 * Changes made:
 * -SSW AIE Team, 2024-01-18
 *   - Fix the issue of AIE driver gen checking.
 */
/*
 * Changes made:
 * -SSW AIE Team, 2026-04-17
 *   - Fix the misra cpp 2023 violations.
 */
 /*
 * Changes made:
 * -SSW AIE Team, 2026-08-18
 *   - Fix the namespace violation issue.
 */
#include "aeg_runtime_api.h"
#include "aeg_api_message.h"
#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <map>
#include <unordered_set>

#include "xaiengine.h"


namespace abr {
using aeg_api_message::debugMsg;
using aeg_api_message::errorMsg;
using aeg_api_message::infoMsg;

namespace {
    std::unordered_set<int32_t> readOnlySharedBufferInitialized = {};
}

/********************************* Statics & Constants *********************************/

static constexpr int32_t ACQ_WRITE = 0;
static constexpr int32_t ACQ_READ = 1;
static constexpr int32_t REL_READ = 1;
static constexpr int32_t REL_WRITE = 0;
static constexpr int32_t AIE_ML_REL_WRITE = -1;
static constexpr int32_t AIE_ML_ASYNC_ACQ = -1; // negative lock value -> acquire_greater_equal
static constexpr int32_t AIE_ML_ASYNC_ACQ_FIRST_TIME = 0;
static constexpr uint32_t LOCK_TIMEOUT = 0x7FFFFFFFU;

/********************************* Helper Functions *********************************/

/**
 * @brief Normalize device generation value.
 * Maps XAIE_DEV_GEN_S100 and XAIE_DEV_GEN_S200 to XAIE_DEV_GEN_AIE
 * since they are variants of AIE architecture.
 *
 * @param devGen The raw device generation value from DevProp.DevGen
 * @return Normalized device generation value
 */
namespace {
inline uint8_t getNormalizedDevGen(uint8_t devGen) {
    // S100 (6) and S200 (7) are AIE variants, map them to AIE (1)
    if ((devGen == XAIE_DEV_GEN_S100) || (devGen == XAIE_DEV_GEN_S200)) { // XAIE_DEV_GEN_S100 || XAIE_DEV_GEN_S200
        return XAIE_DEV_GEN_AIE;
    }
    return devGen;
}
}

/********************************* config_manager *************************************/

XAie_DevInst *config_manager::s_pDevInst = nullptr;
bool config_manager::s_bInitialized = false;
size_t config_manager::s_num_reserved_rows = 0;
bool config_manager::s_broadcast_enable_core = false;

err_code config_manager::initialize(XAie_DevInst *devInst, size_t num_reserved_rows, bool broadcast_enable_core) {
    if (!s_bInitialized) {
        if (!devInst) {
            return errorMsg(
                err_code::internal_error,
                "ERROR: config_manager::initialize: Cannot initialize device instance.");
        }

        s_pDevInst = devInst;
        s_num_reserved_rows = num_reserved_rows;
        s_broadcast_enable_core = broadcast_enable_core;
        s_bInitialized = true;
    }
    return err_code::ok;
}

/************************************ graph_api ************************************/

graph_api::graph_api(const graph_config *pConfig)
    : pGraphConfig(pConfig), isConfigured(false), isRunning(false), startTime(0) {}

err_code graph_api::configure() {
    if (!pGraphConfig) {
        return errorMsg(err_code::internal_error,"ERROR: abr::graph_api::configure: Invalid graph configuration.");
    }

    size_t numCores = pGraphConfig->coreColumns.size();
    if ((pGraphConfig->coreRows.size() != numCores) || (pGraphConfig->iterMemAddrs.size() != numCores) || (pGraphConfig->triggered.size() != numCores) ||
        (pGraphConfig->iterMemColumns.size() != numCores) || (pGraphConfig->iterMemRows.size() != numCores)) {
        return errorMsg(err_code::internal_error,"ERROR: abr::graph_api::configure: inconsistent number of cores.");
    }

    coreTiles.resize(numCores);
    iterMemTiles.resize(numCores);
    for (size_t i = 0; i < numCores; i++) {
        const int32_t reserved = static_cast<int32_t>(config_manager::s_num_reserved_rows);
        const int32_t coreRowAbs = static_cast<int32_t>(pGraphConfig->coreRows[i]) + reserved + 1;
        const int32_t iterRowAbs = static_cast<int32_t>(pGraphConfig->iterMemRows[i]) + reserved + 1;
        coreTiles[i] = XAie_TileLoc(static_cast<uint8_t>(pGraphConfig->coreColumns[i]), static_cast<uint8_t>(coreRowAbs));
        iterMemTiles[i] =
            XAie_TileLoc(static_cast<uint8_t>(pGraphConfig->iterMemColumns[i]), static_cast<uint8_t>(iterRowAbs));
    }

    isConfigured = true;
    return err_code::ok;
}

err_code graph_api::run() {
    if (!isConfigured || (pGraphConfig == nullptr)) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::graph::run: graph is not configured.");
    }
    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0
    size_t numCores = coreTiles.size();

    // Record a snapshot of the graph cores startup/enable time
    if (numCores != 0U) {
        driverStatus |=
            static_cast<uint32_t>(XAie_ReadTimer(config_manager::s_pDevInst, coreTiles[0], XAIE_CORE_MOD, static_cast<uint64_t *>(&startTime)));
    }

    if (config_manager::s_broadcast_enable_core) {
        static_cast<void>(XAie_StartTransaction(config_manager::s_pDevInst, XAIE_TRANSACTION_ENABLE_AUTO_FLUSH));
        for (size_t i = 0; i < numCores; i++) {
            // Clear disable event occurred bit of Enable_Event
            static_cast<void>(XAie_ClearCoreDisableEventOccurred(config_manager::s_pDevInst, coreTiles[i]));
            // Set Enable_Event to XAIE_EVENT_BROADCAST_7_CORE. The resources have been acquired by aiecompiler.
            static_cast<void>(XAie_CoreConfigureEnableEvent(config_manager::s_pDevInst, coreTiles[i], XAIE_EVENT_BROADCAST_7_CORE));
        }
        static_cast<void>(XAie_SubmitTransaction(config_manager::s_pDevInst, nullptr));

        // Trigger event XAIE_EVENT_BROADCAST_A_8_PL in shim_tile at column 0 by writing to Event_Generate. The
        // resources have been acquired by aiecompiler.
        //  In case of multi-partition flow still XAie_TileLoc(0, 0) will be used to generate event trigger, but here
        //  (0,0) indicates the relative tile location i.e. absolute bottom left tile post translation by partition
        //  start column is always 0,0.
        static_cast<void>(XAie_EventGenerate(config_manager::s_pDevInst,XAie_TileLoc(static_cast<uint8_t>(pGraphConfig->broadcast_column), static_cast<uint8_t>(0U)),
                        XAIE_PL_MOD, XAIE_EVENT_BROADCAST_A_8_PL));

        // Waiting for 150 cycles to reset the core enable event
        uint64_t StartTime = 0U;
        uint64_t CurrentTime = 0U;
        driverStatus |= static_cast<uint32_t>(XAie_ReadTimer(config_manager::s_pDevInst,XAie_TileLoc(static_cast<uint8_t>(pGraphConfig->broadcast_column),static_cast<uint8_t>(0U)),
                                    XAIE_PL_MOD, static_cast<uint64_t *>(&StartTime)));
        do {
            driverStatus |= static_cast<uint32_t>(XAie_ReadTimer(config_manager::s_pDevInst,XAie_TileLoc(static_cast<uint8_t>(pGraphConfig->broadcast_column), static_cast<uint8_t>(0U)),
                                        XAIE_PL_MOD, static_cast<uint64_t *>(&CurrentTime)));
        } while ((CurrentTime - StartTime) <= 150);

        static_cast<void>(XAie_StartTransaction(config_manager::s_pDevInst, XAIE_TRANSACTION_ENABLE_AUTO_FLUSH));
        for (size_t i = 0; i < numCores; i++) {
            // Set Enable_Event to 0
            static_cast<void>(XAie_CoreConfigureEnableEvent(config_manager::s_pDevInst, coreTiles[i], XAIE_EVENT_NONE_CORE));
        }
        static_cast<void>(XAie_SubmitTransaction(config_manager::s_pDevInst, nullptr));
    } else {
        static_cast<void>(XAie_StartTransaction(config_manager::s_pDevInst, XAIE_TRANSACTION_ENABLE_AUTO_FLUSH));
        for (size_t i = 0; i < numCores; i++) {
            driverStatus |= static_cast<uint32_t>(XAie_CoreEnable(config_manager::s_pDevInst, coreTiles[i]));
        }
        static_cast<void>(XAie_SubmitTransaction(config_manager::s_pDevInst, nullptr));
    }

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::graph::run: AIE driver error.");
    }

    isRunning = true; // Set graph enable after enabling all cores
    return err_code::ok;
}

err_code graph_api::run(int32_t testIter) {
    if (!isConfigured || (pGraphConfig == nullptr)) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::graph::run: graph is not configured.");
    }

    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0

    // Set iterations for the core(s) of graph
    size_t numCores = coreTiles.size();
    static_cast<void>(XAie_StartTransaction(config_manager::s_pDevInst, XAIE_TRANSACTION_ENABLE_AUTO_FLUSH));
    for (size_t i = 0; i < numCores; i++) {
        driverStatus |=
            static_cast<uint32_t>(XAie_DataMemWrWord(config_manager::s_pDevInst, iterMemTiles[i], static_cast<uint32_t>(pGraphConfig->iterMemAddrs[i]), static_cast<uint32_t>(testIter)));
    }
    static_cast<void>(XAie_SubmitTransaction(config_manager::s_pDevInst, nullptr));

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::graph::run: AIE driver error.");
    }

    return run();
}

err_code graph_api::wait() {
    if (!isConfigured || (pGraphConfig == nullptr)) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::graph::wait: graph is not configured.");
    }

    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0

    size_t numCores = coreTiles.size();
    for (size_t i = 0; i < numCores; i++) {
        if (pGraphConfig->triggered[i] == 0U) {
            // Default timeout is 500us. The timeout is counted on AIE clock.
            // So even for a simple test-case this API call returns with error code XAIE_CORE_STATUS_TIMEOUT.
            while (XAie_CoreWaitForDone(config_manager::s_pDevInst, coreTiles[i], static_cast<std::uint32_t>(0U)) ==
                   XAIE_CORE_STATUS_TIMEOUT) {
            }

            driverStatus |= static_cast<std::uint32_t>(XAie_CoreDisable(config_manager::s_pDevInst, coreTiles[i]));
        }
    }

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::graph::wait: AIE driver error.");
    }

    isRunning = false;
    return err_code::ok;
}

err_code graph_api::wait(uint64_t cycleTimeout) {
    if (!isConfigured) {
        return errorMsg(err_code::aie_driver_error,"ERROR: abr::graph::wait: graph is not configured.");
    }

    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0

    if (cycleTimeout > 0xFFFFFFFFFFFFU) {
        return errorMsg(err_code::user_error,
            "ERROR: abr::graph::wait: Max cycle timeout value can be 0xFFFFFFFFFFFF.");
    }

    size_t numCores = coreTiles.size();
    if (numCores != 0U) {
        // Adjust the cycle-timeout value
        uint64_t elapsedTime = 0U;
        driverStatus |= static_cast<uint32_t>(XAie_ReadTimer(config_manager::s_pDevInst, coreTiles[0], XAIE_CORE_MOD, static_cast<uint64_t *>(&elapsedTime)));
        elapsedTime -= startTime;
        if (cycleTimeout > elapsedTime) {
            driverStatus |=
                static_cast<uint32_t>(XAie_WaitCycles(config_manager::s_pDevInst, coreTiles[0], XAIE_CORE_MOD, static_cast<uint64_t>(cycleTimeout - elapsedTime)));
        }
    }

    for (size_t i = 0; i < numCores; i++) {
        driverStatus |= static_cast<uint32_t>(XAie_CoreDisable(config_manager::s_pDevInst, coreTiles[i]));
    }

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::graph::wait: AIE driver error.");
    }

    isRunning = false;
    return err_code::ok;
}

err_code graph_api::resume() {
    if (!isConfigured) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::graph::resume: graph is not configured.");
    }

    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0

    size_t numCores = coreTiles.size();
    if (numCores != 0U) { // Reset the graph timer
        driverStatus |= static_cast<uint32_t>(XAie_ReadTimer(config_manager::s_pDevInst, coreTiles[0], XAIE_CORE_MOD, static_cast<uint64_t *>(&startTime)));
    }

    for (size_t i = 0; i < numCores; i++) {
        uint8_t isDoneByte = 0U;
        driverStatus |= static_cast<uint32_t>(XAie_CoreReadDoneBit(config_manager::s_pDevInst, coreTiles[i],static_cast<uint8_t *>(&isDoneByte)));
        if (isDoneByte == 0U) {
            driverStatus |= static_cast<uint32_t>(XAie_CoreEnable(config_manager::s_pDevInst,
                                                                    coreTiles[i])); // Core Enable will clear Core_Done status bit
        }
    }

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::graph::resume: AIE driver error.");
    }

    return err_code::ok;
}

err_code graph_api::end() {
    if (!isConfigured || (pGraphConfig == nullptr)) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::graph::end: graph is not configured.");
    }

    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0

    bool isRunningBefore = isRunning;
    err_code ret = wait(); // wait core done. //wait() sets isRunning to false
    if (ret != err_code::ok) {
        return ret;
    }

    size_t numCores = coreTiles.size();
    for (size_t i = 0; i < numCores; i++) {
        // if the end sequence is done before, do not do it again. this is to allow multiple g.end(), g.end() ...
        if (isRunningBefore && (pGraphConfig->triggered[i] == 0U)) {
            driverStatus |= static_cast<uint32_t>(XAie_DataMemWrWord(config_manager::s_pDevInst, iterMemTiles[i],
                                            static_cast<uint32_t>(pGraphConfig->iterMemAddrs[i] - static_cast<size_t>(4U)),static_cast<uint32_t>(1U)));
            driverStatus |= static_cast<uint32_t>(XAie_CoreEnable(config_manager::s_pDevInst, coreTiles[i]));

            // Default timeout is 500us. The timeout is counted on AIE clock.
            // So even for a simple test-case this API call returns with error code XAIE_CORE_STATUS_TIMEOUT.
            while (XAie_CoreWaitForDone(config_manager::s_pDevInst, coreTiles[i], static_cast<uint32_t>(0U)) ==
                   XAIE_CORE_STATUS_TIMEOUT) {
            }
            driverStatus |= static_cast<uint32_t>(XAie_CoreDisable(config_manager::s_pDevInst, coreTiles[i]));
        }
    }

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error,"ERROR: abr::graph::end: AIE driver error.");
    }

    return err_code::ok;
}

err_code graph_api::end(uint64_t cycleTimeout) {
    if (!isConfigured || (pGraphConfig == nullptr)) {
        return errorMsg(err_code::aie_driver_error,"ERROR: abr::graph::end: graph is not configured.");
    }

    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0

    err_code ret = wait(cycleTimeout);
    if (ret != err_code::ok) {
        return ret;
    }

    size_t numCores = iterMemTiles.size();
    // set the end signal in sync_buffer[0] (which is 4 byte before iteration address)
    for (size_t i = 0; i < numCores; i++) {
        driverStatus |=
            static_cast<uint32_t>(XAie_DataMemWrWord(config_manager::s_pDevInst, iterMemTiles[i],static_cast<uint32_t>(pGraphConfig->iterMemAddrs[i] - static_cast<size_t>(4U)),static_cast<uint32_t>(1U)));
    }

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error,"ERROR: abr::graph::end: AIE driver error.");
    }

    return err_code::ok;
}

err_code checkRTPConfigForUpdate(const rtp_config *pRTPConfig, const graph_config *pGraphConfig, size_t numBytes,
                                bool isRunning) {
    if ((pRTPConfig == nullptr) || (pGraphConfig == nullptr)) {
        return errorMsg(err_code::internal_error,"ERROR: abr::graph::update: invalid RTP configuration.");
    }

    // error checking: does this port belong to the graph
    if (pRTPConfig->graphId != pGraphConfig->id) {
        return errorMsg(err_code::user_error,
                        "ERROR: abr::graph::update: RTP port %.*s does not belong to graph %.*s.",
                        static_cast<std::int32_t>(pRTPConfig->portName.size()), pRTPConfig->portName.data(),
                        static_cast<std::int32_t>(pGraphConfig->name.size()), pGraphConfig->name.data());
    }

    // error checking: direction
    if (!pRTPConfig->isInput) {
        return errorMsg(err_code::user_error, "ERROR: abr::graph::update only supports input RTP port.");
    }

    // error checking: size
    if (numBytes != pRTPConfig->numBytes) {
        return errorMsg(err_code::user_error,
                        "ERROR: abr::graph::update parameter size %zu is inconsistent with RTP port %.*s size %zu.",
                        static_cast<size_t>(numBytes),
                        static_cast<std::int32_t>(pRTPConfig->portName.size()), pRTPConfig->portName.data(),
                        static_cast<size_t>(pRTPConfig->numBytes));
    }

    // error checking: connected RTP port
    if (pRTPConfig->isConnect) {
        if (pRTPConfig->isPL) {
            return errorMsg(err_code::user_error,
                            "ERROR: abr::graph::update to connected RL input RTP is not supported.");
        } else { // AIE RTP
            // For connected async input RTP, only allow graph update before graph run
            if (pRTPConfig->isAsync) {
                if (isRunning) {
                    return errorMsg(
                        err_code::user_error,
                        "ERROR: abr::graph::update to connected asynchronous input "
                        "RTP is not allowed during graph run.");
                }
            } else {
                // Does not support connected sync input RTP
                return errorMsg(
                    err_code::user_error,
                    "ERROR: abr::graph::update to connected synchronous input RTP is not supported.");
            }
        }
    }

    return err_code::ok;
}

err_code checkSharedBufferConfigForUpdate(const shared_buffer_config *pSharedBufferConfig,
                                        const graph_config *pGraphConfig, size_t numBytes) {
    if ((pSharedBufferConfig == nullptr) || (pGraphConfig == nullptr)) {
        return errorMsg(err_code::internal_error, "ERROR: adf::graph::update: Invalid Shared Buffer configuration.");
    }

    // error checking: does this shared buffer belong to the graph
    if (pSharedBufferConfig->graphId != pGraphConfig->id) {
        return errorMsg(err_code::user_error,
                        "ERROR: adf::graph::update: Shared Buffer %.*s does not belong to graph %.*s.",
                        static_cast<std::int32_t>(pSharedBufferConfig->name.size()), pSharedBufferConfig->name.data(),
                        static_cast<std::int32_t>(pGraphConfig->name.size()), pGraphConfig->name.data());
    }
    // error checking: has only output ports
    if ((pSharedBufferConfig->numInputs != static_cast<int16_t>(0)) ||(pSharedBufferConfig->numOutputs == static_cast<int16_t>(0))) {
        return errorMsg(err_code::user_error,
                        "ERROR: adf::graph::update: API cannot be used for Shared Buffer %.*s. adf::graph::update can be "
                        "used when shared buffer has zero input ports.",
                        static_cast<std::int32_t>(pSharedBufferConfig->name.size()), pSharedBufferConfig->name.data());
    }

    // error checking: size
    if (numBytes != pSharedBufferConfig->numBytes) {
        return errorMsg(err_code::user_error,
                        "ERROR: adf::graph::update parameter size %zu bytes is inconsistent with Shared Buffer %.*s size "
                        "%zu bytes.",
                        static_cast<size_t>(numBytes),
                        static_cast<std::int32_t>(pSharedBufferConfig->name.size()), pSharedBufferConfig->name.data(),
                        static_cast<size_t>(pSharedBufferConfig->numBytes));
    }
    return err_code::ok;
}

err_code graph_api::update(const shared_buffer_config *pSharedBufferConfig, const void *pValue, size_t numBytes) const {

    if (pGraphConfig == nullptr) {
        return errorMsg(err_code::internal_error, "ERROR: abr::graph::update: Invalid graph configuration.");
    }

    err_code ret = checkSharedBufferConfigForUpdate(pSharedBufferConfig, pGraphConfig, numBytes);
    if (ret != err_code::ok) {
        return ret;
    }

     ///////////////////////////// Configuration //////////////////////////////


    // XAie_TileLoc(col, row) stores { .Row = row, .Col = col } (see xaiengine/xaiegbl.h).
    const XAie_LocType tile = XAie_TileLoc(static_cast<u8>(pSharedBufferConfig->column),
                                           static_cast<u8>(static_cast<std::int32_t>(pSharedBufferConfig->row) + 1));

    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK));

    int32_t prd_lock_acq_val = -1;
    int32_t cns_lock_acq_val = -pSharedBufferConfig->numOutputs;

    if (readOnlySharedBufferInitialized.find(pSharedBufferConfig->id) == readOnlySharedBufferInitialized.end()) {
        if (!pSharedBufferConfig->initialized) {
            prd_lock_acq_val = 0;
            cns_lock_acq_val = 0;
        }
        static_cast<void>(readOnlySharedBufferInitialized.insert(pSharedBufferConfig->id));
    }

    // Acquire update lock to prevent further reads from shared buffer.
    driverStatus |=
        static_cast<uint32_t>(XAie_LockAcquire(config_manager::s_pDevInst, tile,
                        XAie_LockInit(static_cast<u16>(pSharedBufferConfig->producerLocks[0]),static_cast<s8>(prd_lock_acq_val)),LOCK_TIMEOUT));

    // Acquire buffer lock after all the readers are done reading
    driverStatus |=
        static_cast<uint32_t>(XAie_LockAcquire(config_manager::s_pDevInst, tile,
                        XAie_LockInit(static_cast<u16>(pSharedBufferConfig->consumerLocks[0]),static_cast<s8>(cns_lock_acq_val)),LOCK_TIMEOUT));

    // Update data in the shared buffer
    driverStatus |=
        static_cast<uint32_t>(XAie_DataMemBlockWrite(
        config_manager::s_pDevInst, tile,
        static_cast<u32>(pSharedBufferConfig->addr[0]), pValue,
        static_cast<u32>(numBytes)));

    // Release the locks
    driverStatus |= static_cast<uint32_t>(XAie_LockRelease(config_manager::s_pDevInst, tile,
                                    XAie_LockInit(static_cast<u16>(pSharedBufferConfig->producerLocks[0]),static_cast<s8>(1)),LOCK_TIMEOUT));
    driverStatus |= static_cast<uint32_t>(XAie_LockRelease(
        config_manager::s_pDevInst, tile,
        XAie_LockInit(static_cast<u16>(pSharedBufferConfig->consumerLocks[0]),static_cast<s8>(pSharedBufferConfig->numOutputs)),LOCK_TIMEOUT));

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error,
                    "ERROR: adf::graph::update: XAieTile_LockAcquire timeout or AIE driver error.");
    }

    return err_code::ok;
}

err_code graph_api::update(const rtp_config *pRTPConfig, const void *pValue, size_t numBytes) {
    ///////////////////////////// Error Checking //////////////////////////////

    if (pGraphConfig == nullptr) {
        return errorMsg(err_code::internal_error, "ERROR: abr::graph::update: Invalid graph configuration.");
    }

    err_code ret = checkRTPConfigForUpdate(pRTPConfig, pGraphConfig, numBytes, isRunning);
    if (ret != err_code::ok) {
        return ret;
    }

    ///////////////////////////// Configuration //////////////////////////////

    size_t numReservedRows = config_manager::s_num_reserved_rows;
    const int32_t reservedRowsI32 = static_cast<int32_t>(numReservedRows);
    const int32_t selectorRowAbs =static_cast<int32_t>(pRTPConfig->selectorRow) + reservedRowsI32 + 1;
    const int32_t pingRowAbs = static_cast<int32_t>(pRTPConfig->pingRow) + reservedRowsI32 + 1;
    const int32_t pongRowAbs = static_cast<int32_t>(pRTPConfig->pongRow) + reservedRowsI32 + 1;
    const XAie_LocType selectorTile =
    XAie_TileLoc(static_cast<u8>(pRTPConfig->selectorColumn), static_cast<u8>(selectorRowAbs));
    const XAie_LocType pingTile =XAie_TileLoc(static_cast<u8>(pRTPConfig->pingColumn), static_cast<u8>(pingRowAbs));
    const XAie_LocType pongTile = XAie_TileLoc(static_cast<u8>(pRTPConfig->pongColumn), static_cast<u8>(pongRowAbs));

    // Do NOT lock async RTP when graph is suspended; otherwise, it may deadlock. We don't support synchronous RTP in
    // suspended mode
    bool bAcquireLock = !(pRTPConfig->isAsync && !isRunning);

    int8_t acquireVal =pRTPConfig->isAsync ? static_cast<int8_t>(XAIE_LOCK_WITH_NO_VALUE) : static_cast<int8_t>(ACQ_WRITE); // Versal
    int8_t releaseVal = static_cast<int8_t>(REL_READ); // Versal

    if (getNormalizedDevGen(config_manager::s_pDevInst->DevProp.DevGen) >=
        XAIE_DEV_GEN_AIEML) // modification to accommodate AIEML semaphore
    {
        if (pRTPConfig->isAsync) {
            auto it = std::find(asyncNotFirstTimePorts.begin(), asyncNotFirstTimePorts.end(), pRTPConfig->portId);
            if (it != asyncNotFirstTimePorts.end()) {
                acquireVal = static_cast<int8_t>(static_cast<int32_t>(AIE_ML_ASYNC_ACQ));
            } else {
                acquireVal = static_cast<int8_t>(static_cast<int32_t>(AIE_ML_ASYNC_ACQ_FIRST_TIME));
                asyncNotFirstTimePorts.push_back(pRTPConfig->portId);
            }
        }
    }

    ///////////////////////////// RTP update operation //////////////////////////////

    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0

    // sync ports acquire selector lock for WRITE, async ports acquire selector lock unconditionally
    if (pRTPConfig->hasLock && bAcquireLock) {
        driverStatus |= static_cast<uint32_t>(XAie_LockAcquire(config_manager::s_pDevInst, selectorTile,
                                        XAie_LockInit(static_cast<u16>(pRTPConfig->selectorLockId),static_cast<s8>(acquireVal)),LOCK_TIMEOUT));
    }

    // Read the selector value
    uint32_t selector = 0U;
    driverStatus |=
        static_cast<uint32_t>(XAie_DataMemRdWord(config_manager::s_pDevInst, selectorTile,static_cast<u32>(pRTPConfig->selectorAddr), static_cast<u32 *>(&selector)));
    selector = 1 - selector;

    if (selector == 1U) { // pong
        // sync ports acquire buffer lock for WRITE, async ports acquire buffer lock unconditionally
        if (pRTPConfig->hasLock && bAcquireLock) {
            driverStatus |= static_cast<uint32_t>(XAie_LockAcquire(
                config_manager::s_pDevInst, pongTile,
                XAie_LockInit(static_cast<u16>(pRTPConfig->pongLockId), static_cast<s8>(acquireVal)), LOCK_TIMEOUT));
        }

        driverStatus |= static_cast<uint32_t>(XAie_DataMemBlockWrite(
            config_manager::s_pDevInst, pongTile, static_cast<u32>(pRTPConfig->pongAddr), pValue, static_cast<u32>(numBytes)));
    } else { // ping
        // sync ports acquire buffer lock for WRITE, async ports acquire buffer lock unconditionally
        if (pRTPConfig->hasLock && bAcquireLock) {
            driverStatus |= static_cast<uint32_t>(XAie_LockAcquire(
                config_manager::s_pDevInst, pingTile,
                XAie_LockInit(static_cast<u16>(pRTPConfig->pingLockId), static_cast<s8>(acquireVal)), LOCK_TIMEOUT));
        }

        driverStatus |= static_cast<uint32_t>(XAie_DataMemBlockWrite(
            config_manager::s_pDevInst, pingTile, static_cast<u32>(pRTPConfig->pingAddr), pValue, static_cast<u32>(numBytes)));
    }

    // write the new selector value
    driverStatus |= static_cast<uint32_t>(XAie_DataMemWrWord(
        config_manager::s_pDevInst, selectorTile,
        static_cast<u32>(pRTPConfig->selectorAddr),
        static_cast<u32>(selector)));

    if (pRTPConfig->hasLock) {
        // release selector and buffer locks for ME
        // still need to release async RTP selector lock FOR_READ even when the graph is suspended;
        // otherwise, the ME side may deadlock in acquiring selector lock FOR_READ
        driverStatus |= static_cast<uint32_t>(XAie_LockRelease(config_manager::s_pDevInst, selectorTile,
                                        XAie_LockInit(static_cast<u16>(pRTPConfig->selectorLockId),static_cast<s8>(releaseVal)),LOCK_TIMEOUT));

        // still need to release async RTP buffer lock FOR_READ even when the graph is suspended;
        // otherwise, the AIE side may deadlock in acquiring buffer lock FOR_READ
        // (note that there is one selector lock but two buffer locks)
        if (selector == 1U) { // pong
            driverStatus |= static_cast<uint32_t>(XAie_LockRelease(
                config_manager::s_pDevInst, pongTile,
                XAie_LockInit(static_cast<u16>(pRTPConfig->pongLockId), static_cast<s8>(releaseVal)), LOCK_TIMEOUT));
        } else { // ping
            driverStatus |= static_cast<uint32_t>(XAie_LockRelease(
                config_manager::s_pDevInst, pingTile,
                XAie_LockInit(static_cast<u16>(pRTPConfig->pingLockId), static_cast<s8>(releaseVal)), LOCK_TIMEOUT));
        }
    }

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error,
                        "ERROR: abr::graph::update: XAieTile_LockAcquire timeout or AIE driver error.");
    }

    return err_code::ok;
}

err_code checkRTPConfigForRead(const rtp_config *pRTPConfig, const graph_config *pGraphConfig, size_t numBytes) {
    if ((pRTPConfig == nullptr) || (pGraphConfig == nullptr)) {
        return errorMsg(err_code::internal_error, "ERROR: abr::graph::read: Invalid RTP configuration.");
    }

    // error checking: does this port belong to the graph
    if (pRTPConfig->graphId != pGraphConfig->id) {
        return errorMsg(err_code::user_error,
                        "ERROR: abr::graph::read: RTP port %.*s does not belong to graph %.*s.",
                        static_cast<std::int32_t>(pRTPConfig->portName.size()), pRTPConfig->portName.data(),
                        static_cast<std::int32_t>(pGraphConfig->name.size()), pGraphConfig->name.data());
    }
    // error checking: direction
    if (pRTPConfig->isInput) {
        return errorMsg(err_code::user_error, "ERROR: abr::graph::read does not support input RTP port.");
    }
    // error checking: size
    if (numBytes != pRTPConfig->numBytes) {
        return errorMsg(err_code::user_error,
                        "ERROR: abr::graph::read parameter size %zu is inconsistent with RTP port %.*s size %zu.",
                        static_cast<size_t>(numBytes),
                        static_cast<std::int32_t>(pRTPConfig->portName.size()), pRTPConfig->portName.data(),
                        static_cast<size_t>(pRTPConfig->numBytes));
    }
    // error checking: connected RTP port
    if (pRTPConfig->isConnect) {
        return errorMsg(err_code::user_error, "ERROR: abr::graph::read from connected RTP port is not supported.");
    }
    return err_code::ok;
}

err_code graph_api::read(const rtp_config *pRTPConfig, void *pValue, size_t numBytes) const {
    ///////////////////////////// Error Checking //////////////////////////////

    if (pGraphConfig == nullptr) {
        return errorMsg(err_code::internal_error, "ERROR: abr::graph::read: Invalid graph configuration.");
    }

    err_code ret = checkRTPConfigForRead(pRTPConfig, pGraphConfig, numBytes);
    if (ret != err_code::ok) {
        return ret;
    }
    ///////////////////////////// Configuration //////////////////////////////

    // Do NOT lock async RTP when graph is suspended; otherwise, it may deadlock. We don't support synchronous RTP in
    // suspended mode
    bool bHasAndAcquireLock = !(pRTPConfig->isAsync && !isRunning) && pRTPConfig->hasLock;

    int8_t acquireVal = static_cast<int8_t>(ACQ_READ); // Versal
    int8_t releaseVal = pRTPConfig->isAsync ? static_cast<int8_t>(REL_READ) : static_cast<int8_t>(REL_WRITE); // Versal

    size_t numReservedRows = config_manager::s_num_reserved_rows;
    const int32_t reservedRowsI32 = static_cast<int32_t>(numReservedRows);
    const int32_t selectorRowAbs =static_cast<int32_t>(pRTPConfig->selectorRow) + reservedRowsI32 + 1;
    const int32_t pingRowAbs = static_cast<int32_t>(pRTPConfig->pingRow) + reservedRowsI32 + 1;
    const int32_t pongRowAbs = static_cast<int32_t>(pRTPConfig->pongRow) + reservedRowsI32 + 1;
    const XAie_LocType selectorTile =XAie_TileLoc(static_cast<u8>(pRTPConfig->selectorColumn), static_cast<u8>(selectorRowAbs));
    const XAie_LocType pingTile =XAie_TileLoc(static_cast<u8>(pRTPConfig->pingColumn), static_cast<u8>(pingRowAbs));
    const XAie_LocType pongTile =XAie_TileLoc(static_cast<u8>(pRTPConfig->pongColumn), static_cast<u8>(pongRowAbs));

    if (getNormalizedDevGen(config_manager::s_pDevInst->DevProp.DevGen) >=
        XAIE_DEV_GEN_AIEML) { // modification to accommodate AIEML semaphore
        if (pRTPConfig->isAsync) {
            const int32_t acqMl = static_cast<int32_t>(AIE_ML_ASYNC_ACQ);
            acquireVal = static_cast<int8_t>(acqMl);
        } else {
            const int32_t relWrite = static_cast<int32_t>(AIE_ML_REL_WRITE);
            releaseVal = static_cast<int8_t>(relWrite);
        }
    }

    ///////////////////////////// RTP read operation //////////////////////////////

    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0

    if (bHasAndAcquireLock) {
        // synchronous RTP acquires lock for READ, async RTP requiring first-time sync acquires lock for READ
        driverStatus |= static_cast<uint32_t>(XAie_LockAcquire(config_manager::s_pDevInst, selectorTile,
                                        XAie_LockInit(static_cast<u16>(pRTPConfig->selectorLockId),static_cast<s8>(acquireVal)),LOCK_TIMEOUT));
    }

    // Read the selector value
    uint32_t selector = 0U;
    driverStatus |=
        static_cast<uint32_t>(XAie_DataMemRdWord(config_manager::s_pDevInst, selectorTile,static_cast<u32>(pRTPConfig->selectorAddr), static_cast<u32 *>(&selector)));

    if (bHasAndAcquireLock) {
        // synchronous RTP acquires buffer for READ, async RTP requiring first-time sync acquires lock for READ
        if (selector == 1U) { // pong
            driverStatus |= static_cast<uint32_t>(XAie_LockAcquire(
                config_manager::s_pDevInst, pongTile,
                XAie_LockInit(static_cast<u16>(pRTPConfig->pongLockId), static_cast<s8>(acquireVal)), LOCK_TIMEOUT));
        } else { // ping
            driverStatus |= static_cast<uint32_t>(XAie_LockAcquire(
                config_manager::s_pDevInst, pingTile,
                XAie_LockInit(static_cast<u16>(pRTPConfig->pingLockId), static_cast<s8>(acquireVal)), LOCK_TIMEOUT));
        }
    }

    // if lock was aquired, release the selector lock
    if (bHasAndAcquireLock) {
        // synchronous RTP releases lock for WRITE, async RTP requiring first-time sync release lock for READ
        driverStatus |= static_cast<uint32_t>(XAie_LockRelease(config_manager::s_pDevInst, selectorTile,
                                        XAie_LockInit(static_cast<u16>(pRTPConfig->selectorLockId),static_cast<s8>(releaseVal)),LOCK_TIMEOUT));
    }

    if (selector == 1U) { // pong
        driverStatus |=
            static_cast<uint32_t>(XAie_DataMemBlockRead(
                config_manager::s_pDevInst, pongTile, static_cast<u32>(pRTPConfig->pongAddr), pValue, static_cast<u32>(numBytes)));
    } else { // ping
        driverStatus |=
            static_cast<uint32_t>(XAie_DataMemBlockRead(
                config_manager::s_pDevInst, pingTile, static_cast<u32>(pRTPConfig->pingAddr), pValue, static_cast<u32>(numBytes)));
    }

    // release buffer lock
    if (bHasAndAcquireLock) {
        if (selector == 1U) { // pong
            driverStatus |= static_cast<uint32_t>(XAie_LockRelease(
                config_manager::s_pDevInst, pongTile,
                XAie_LockInit(static_cast<u16>(pRTPConfig->pongLockId), static_cast<s8>(releaseVal)), LOCK_TIMEOUT));
        } else { // ping
            driverStatus |= static_cast<uint32_t>(XAie_LockRelease(
                config_manager::s_pDevInst, pingTile,
                XAie_LockInit(static_cast<u16>(pRTPConfig->pingLockId), static_cast<s8>(releaseVal)), LOCK_TIMEOUT));
        }
    }

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error,
                    "ERROR: abr::graph::read: XAieTile_LockAcquire timeout or AIE driver error.");
    }

    return err_code::ok;
}

/************************************ gmio_api ************************************/

/// GMIO API helper functions
namespace {
inline uint8_t convertLogicalToPhysicalDMAChNum(int16_t logicalChNum) {
    const int16_t physical =(logicalChNum > static_cast<int16_t>(1))? static_cast<int16_t>(logicalChNum - static_cast<int16_t>(2))
            : logicalChNum;
    return static_cast<uint8_t>(physical);
}
}

namespace {
size_t frontAndPop(std::queue<size_t> &bdQueue) {
    size_t bd = bdQueue.front();
    bdQueue.pop();
    return bd;
}
}

gmio_api::gmio_api(const gmio_config *pConfig) : pGMIOConfig(pConfig), isConfigured(false), dmaStartQMaxSize(4) {}

err_code gmio_api::configure() {
    if (!pGMIOConfig) {
        return errorMsg(err_code::internal_error, "ERROR: gmio_api::configure: Invalid GMIO configuration.");
    }

    if ((pGMIOConfig->type == gmio_config::gm2aie) || (pGMIOConfig->type == gmio_config::aie2gm)) {
        uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0
        gmioTileLoc = XAie_TileLoc(static_cast<u8>(pGMIOConfig->shimColumn), static_cast<u8>(0U));
        driverStatus |= static_cast<uint32_t>(XAie_DmaDescInit(config_manager::s_pDevInst, &shimDmaInst, gmioTileLoc));
        // enable shim DMA channel, need to start first so the status is correct
        driverStatus |= static_cast<uint32_t>(XAie_DmaChannelEnable(
            config_manager::s_pDevInst, gmioTileLoc, static_cast<u8>(convertLogicalToPhysicalDMAChNum(pGMIOConfig->channelNum)),
            ((pGMIOConfig->type == gmio_config::gm2aie) ? DMA_MM2S : DMA_S2MM)));
        driverStatus |=
            static_cast<uint32_t>(XAie_DmaGetMaxQueueSize(config_manager::s_pDevInst, gmioTileLoc, static_cast<u8 *>(&dmaStartQMaxSize)));

        // decide 4 BD numbers to use for this GMIO based on channel number (0-S2MM0,1-S2MM1,2-MM2S0,3-MM2S1)
        const int32_t dmaQueueSize = static_cast<int32_t>(static_cast<uint32_t>(dmaStartQMaxSize));
        const int32_t channelNumI32 = static_cast<int32_t>(pGMIOConfig->channelNum);
        for (int32_t j = 0; j < dmaQueueSize; j++) {
            const int32_t bdNum = (channelNumI32 * dmaQueueSize) + j;
            availableBDs.push(bdNum);

            // set AXI burst length, this won't change during runtime
            driverStatus |=
                static_cast<uint32_t>(XAie_DmaSetAxi(&shimDmaInst, static_cast<u8>(0U) /*Smid*/, static_cast<u8>(pGMIOConfig->burstLength) /*BurstLen*/, static_cast<u8>(0U) /*Qos*/, static_cast<u8>(0U) /*Cache*/, static_cast<u8>(0U) /*Secure*/));
            debugMsg("GMIO id %u assigned BD num %d", static_cast<std::uint32_t>(pGMIOConfig->id), static_cast<std::int32_t>(bdNum));
        }

        if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
            return errorMsg(err_code::aie_driver_error, "ERROR: abr::gmio_api::configure: AIE driver error.");
        }
    } else {
        return errorMsg(
            err_code::aie_driver_error,
            "ERROR: abr::gmio_api::configure: GM - PL connection is not supported in GMIO AIE API.");
    }

    isConfigured = true;
    return err_code::ok;
}

#if !defined(__AIESIM__) && defined(__NOT_BARE_METAL__)
    err_code gmio_api::enqueueBD(XAie_MemInst *memInst, uint64_t offset, size_t size)
#else
err_code gmio_api::enqueueBD(uint64_t address, size_t size)
#endif
{
    if (!isConfigured || (pGMIOConfig == nullptr)) {
        return errorMsg(err_code::internal_error, "ERROR: abr::gmio_api::enqueueBD: GMIO is not configured.");
    }

    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0

    // wait for available BD
    while (availableBDs.empty()) {
        uint8_t numPendingBDs = 0U;
        driverStatus |= static_cast<uint32_t>(XAie_DmaGetPendingBdCount(
            config_manager::s_pDevInst, gmioTileLoc,static_cast<u8>(convertLogicalToPhysicalDMAChNum(pGMIOConfig->channelNum)),
            ((pGMIOConfig->type == gmio_config::gm2aie) ? DMA_MM2S : DMA_S2MM), static_cast<u8 *>(&numPendingBDs)));

        const uint32_t maxQueueU32 = static_cast<uint32_t>(static_cast<uint32_t>(dmaStartQMaxSize));
        const uint32_t pendingU32 = static_cast<uint32_t>(static_cast<uint32_t>(numPendingBDs));
        const int32_t numBDCompleted = static_cast<int32_t>(maxQueueU32 - pendingU32);
        // move completed BDs from enqueuedBDs to availableBDs
        for (int32_t i = 0; i < numBDCompleted; i++) {
            size_t bdNumber = static_cast<size_t>(frontAndPop(enqueuedBDs));
            availableBDs.push(bdNumber);
        }
    }

    // get an available BD
    size_t bdNumber = static_cast<size_t>(frontAndPop(availableBDs));

    // set up BD
#if !defined(__AIESIM__) && defined(__NOT_BARE_METAL__)
    driverStatus |= static_cast<uint32_t>(XAie_DmaSetAddrOffsetLen(&shimDmaInst, memInst, static_cast<u64>(offset), static_cast<u32>(size)));
#else
    driverStatus |= static_cast<uint32_t>(XAie_DmaSetAddrLen(&shimDmaInst, static_cast<u64>(address), static_cast<u32>(size)));
#endif

    if (getNormalizedDevGen(config_manager::s_pDevInst->DevProp.DevGen) >=
        XAIE_DEV_GEN_AIEML) { // AIEML (note AIE1 XAIE_LOCK_WITH_NO_VALUE is -1, which does not work for AIEML)
        driverStatus |= static_cast<uint32_t>(XAie_DmaSetLock(&shimDmaInst,XAie_LockInit(static_cast<u16>(bdNumber), static_cast<s8>(0)),XAie_LockInit(static_cast<u16>(bdNumber), static_cast<s8>(0))));
    } else {
        driverStatus |= static_cast<uint32_t>(XAie_DmaSetLock(&shimDmaInst,XAie_LockInit(static_cast<u16>(bdNumber),static_cast<s8>(XAIE_LOCK_WITH_NO_VALUE)),
        XAie_LockInit(static_cast<u16>(bdNumber),static_cast<s8>(XAIE_LOCK_WITH_NO_VALUE))));
    }

    driverStatus |= static_cast<uint32_t>(XAie_DmaEnableBd(&shimDmaInst));

    // write BD
    driverStatus |= static_cast<uint32_t>(XAie_DmaWriteBd(config_manager::s_pDevInst, &shimDmaInst, gmioTileLoc,static_cast<u16>(bdNumber)));

    // enqueue BD
    driverStatus |= static_cast<uint32_t>(XAie_DmaChannelPushBdToQueue(
        config_manager::s_pDevInst, gmioTileLoc,static_cast<u8>(convertLogicalToPhysicalDMAChNum(pGMIOConfig->channelNum)),
        ((pGMIOConfig->type == gmio_config::gm2aie) ? DMA_MM2S : DMA_S2MM),static_cast<u16>(bdNumber)));
    enqueuedBDs.push(bdNumber);

#if !defined(__AIESIM__) && defined(__NOT_BARE_METAL__)
    debugMsg("gmio_api::enqueueBD: (id %u) enqueue BD num %zu to shim DMA channel %d, DDR offset %" PRIx64 ", transaction size %zu",
             static_cast<std::uint32_t>(pGMIOConfig->id), static_cast<std::size_t>(bdNumber), static_cast<std::int32_t>(pGMIOConfig->channelNum),
             static_cast<std::uint64_t>(offset), static_cast<std::size_t>(size));
#else
    debugMsg("gmio_api::enqueueBD: (id %u) enqueue BD num %zu to shim DMA channel %d, DDR address %" PRIx64 ", transaction size %zu",
             static_cast<std::uint32_t>(pGMIOConfig->id), static_cast<std::size_t>(bdNumber), static_cast<std::int32_t>(pGMIOConfig->channelNum),
             static_cast<std::uint64_t>(address), static_cast<std::size_t>(size));
#endif

    // Update status after using AIE driver
    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::gmio_api::enqueueBD: AIE driver error.");
    }

    return err_code::ok;
}

err_code gmio_api::wait(bool syncmode) {
    auto ret = err_code::ok;
    if (!isConfigured || (pGMIOConfig == nullptr)) {
        return errorMsg(err_code::internal_error, "ERROR: abr::gmio_api::enqueueBD: GMIO is not configured.");
    }

    if ((pGMIOConfig->type == gmio_config::gm2pl) || (pGMIOConfig->type == gmio_config::pl2gm)) {
        return errorMsg(err_code::user_error,
                    "ERROR: GMIO::wait can only be used by GMIO objects connecting to AIE, not PL.");
    }

    debugMsg("gmio_api::wait::XAie_DmaWaitForDone ...");

    if (syncmode) {
        while (XAie_DmaWaitForDone(config_manager::s_pDevInst, gmioTileLoc,
                static_cast<u8>(convertLogicalToPhysicalDMAChNum(pGMIOConfig->channelNum)),
                ((pGMIOConfig->type == gmio_config::gm2aie) ? DMA_MM2S : DMA_S2MM), static_cast<u32>(0U)) != XAIE_OK) {
        }
    } else {
        if (XAie_DmaWaitForDone(config_manager::s_pDevInst, gmioTileLoc,
                static_cast<u8>(convertLogicalToPhysicalDMAChNum(pGMIOConfig->channelNum)),
                ((pGMIOConfig->type == gmio_config::gm2aie) ? DMA_MM2S : DMA_S2MM), static_cast<u32>(0U)) != XAIE_OK) {
            ret = err_code::resource_unavailable;
        }
    }
    while (!enqueuedBDs.empty()) {
        size_t bdNumber = static_cast<size_t>(frontAndPop(enqueuedBDs));
        availableBDs.push(bdNumber);
    }

    return ret;
}

/************************************ dma_api ************************************/

namespace {
uint8_t relativeToAbsoluteRow(int32_t tileType, uint8_t row) {
    uint8_t absoluteRow = row;
    if (tileType == 0) { // aie tile
        const uint32_t sum = static_cast<uint32_t>(static_cast<uint32_t>(absoluteRow) + static_cast<uint32_t>(1U) +
                                                   static_cast<std::uint32_t>(config_manager::s_num_reserved_rows));
        absoluteRow = static_cast<uint8_t>(sum);
    } else if (tileType == 2) { // memory_tile
        absoluteRow = static_cast<uint8_t>(static_cast<uint32_t>(absoluteRow) + 1U);
    } else {
        static_cast<void>(tileType);
    }
    return absoluteRow;
}
}

err_code dma_api::configureBdWaitQueueEnqueueTask(int32_t tileType, uint8_t column, uint8_t row, int32_t dir, uint8_t channel,
                                                uint32_t repeatCount, bool enableTaskCompleteToken,
                                                std::vector<uint8_t> bdIds,
                                                std::vector<dma_api::buffer_descriptor> bdParams) {
    err_code status = err_code::ok;

    if (getNormalizedDevGen(config_manager::s_pDevInst->DevProp.DevGen) == XAIE_DEV_GEN_AIE) {
        return errorMsg(err_code::internal_error,
                        "ERROR: abr::dma_api::enqueueTask: Does not support AIE architecture.");
    }

    if (bdParams.empty()) {
        return errorMsg(err_code::internal_error, "ERROR: abr::dma_api::enqueueTask: Empty buffer descriptors.");
    }

    if (bdIds.size() != bdParams.size()) {
        return errorMsg(err_code::internal_error,
                    "ERROR: abr::dma_api::enqueueTask: The number of BD IDs and the number of BDs are different.");
    }

    // configure BD
    const size_t numBdParams = bdParams.size();
    for (size_t i = 0; i < numBdParams; i++) {
        if (status == err_code::ok) {
            status = configureBD(tileType, column, row, bdIds[i], bdParams[i]);
        }
    }

    // wait task queue
    if (status == err_code::ok) {
        status = waitDMAChannelTaskQueue(tileType, column, row, dir, channel);
    }

    // start queue
    if (status == err_code::ok) {
        status = enqueueTask(tileType, column, row, dir, channel, repeatCount, enableTaskCompleteToken, bdIds[0]);
    }

    return status;
}

err_code dma_api::configureBD(int32_t tileType, uint8_t column, uint8_t row, uint8_t bdId,
                            const dma_api::buffer_descriptor &bdParam) {
    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0
    debugMsg("dma_api::configureBD");
    const XAie_LocType tileLoc =
        XAie_TileLoc(static_cast<u8>(column), relativeToAbsoluteRow(tileType, row));
    XAie_DmaDesc dmaInst;
    driverStatus |= static_cast<uint32_t>(XAie_DmaDescInit(config_manager::s_pDevInst, &dmaInst, tileLoc));

    // address, length, stepsize, wrap,
    std::vector<XAie_DmaDimDesc> dimDescs;
    const size_t numStepDims = bdParam.stepsize.size();
    for (size_t j = 0; j < numStepDims; j++) {
        XAie_DmaDimDesc dimDesc;
        dimDesc.AieMlDimDesc = {static_cast<u32>(bdParam.stepsize[j]),
                                static_cast<u16>((j < bdParam.wrap.size()) ? bdParam.wrap[j] : 0U)};
        dimDescs.push_back(dimDesc);
        debugMsg("D%zu stepsize %u, wrap %u", static_cast<size_t>(j), static_cast<std::uint32_t>(bdParam.stepsize[j]),
                 static_cast<std::uint32_t>((j < bdParam.wrap.size()) ? bdParam.wrap[j] : 0U));
    }
    XAie_DmaTensor dims = {static_cast<u8>(dimDescs.size()), dimDescs.data()};
    driverStatus |= static_cast<uint32_t>(XAie_DmaSetMultiDimAddr(&dmaInst, &dims, static_cast<u64>(bdParam.address), static_cast<u32>(bdParam.length)));
    debugMsg("address %" PRIx64 ", length %u", static_cast<std::uint64_t>(bdParam.address),
             static_cast<std::uint32_t>(bdParam.length));

    // zero padding
    const size_t numPaddingDims = bdParam.padding.size();
    for (std::size_t j = 0; j < numPaddingDims; j++) {
        driverStatus |= static_cast<uint32_t>(XAie_DmaSetZeroPadding(&dmaInst, static_cast<u8>(j), DMA_ZERO_PADDING_BEFORE, static_cast<u8>(bdParam.padding[j].first)));
        driverStatus |= static_cast<uint32_t>(XAie_DmaSetZeroPadding(&dmaInst, static_cast<u8>(j), DMA_ZERO_PADDING_AFTER, static_cast<u8>(bdParam.padding[j].second)));
        debugMsg("D%zu zero before %u, zero after %u", static_cast<size_t>(j),
                 static_cast<std::uint32_t>(bdParam.padding[j].first), static_cast<std::uint32_t>(bdParam.padding[j].second));
    }

    // packet id
    if (bdParam.enable_packet) {
        driverStatus |= static_cast<uint32_t>(XAie_DmaSetPkt(&dmaInst, XAie_PacketInit(static_cast<u8>(bdParam.packet_id), static_cast<u8>(0U))));
        driverStatus |= static_cast<uint32_t>(XAie_DmaSetOutofOrderBdId(&dmaInst, static_cast<u8>(bdParam.out_of_order_bd_id)));
    }
    debugMsg("enable packet %s, packet id %u, out_of_order_bd_id %u", (bdParam.enable_packet ? "1" : "0"),
             static_cast<std::uint32_t>(bdParam.packet_id), static_cast<std::uint32_t>(bdParam.out_of_order_bd_id));

    // tlast suppress
    if (bdParam.tlast_suppress) {
        driverStatus |= static_cast<uint32_t>(XAie_DmaTlastDisable(&dmaInst));
    }
    debugMsg("tlast suppress %s", (bdParam.tlast_suppress ? "1" : "0"));

    // iteration
    if ((bdParam.iteration_stepsize > 0U) || (static_cast<uint32_t>(bdParam.iteration_wrap) > 0U) ||
        (static_cast<uint32_t>(bdParam.iteration_current) > 0U)) {
        driverStatus |= static_cast<uint32_t>(XAie_DmaSetBdIteration(&dmaInst, bdParam.iteration_stepsize,bdParam.iteration_wrap,
                                            bdParam.iteration_current));
    }
    debugMsg("iteration stepsize %u, iteration wrap %u, iteration current %u",
             static_cast<std::uint32_t>(bdParam.iteration_stepsize), static_cast<std::uint32_t>(bdParam.iteration_wrap),
             static_cast<std::uint32_t>(bdParam.iteration_current));

    // enable compression
    if (bdParam.enable_compression) {
        driverStatus |= static_cast<uint32_t>(XAie_DmaEnableCompression(&dmaInst));
    }
    debugMsg("enable compression %s", (bdParam.enable_compression ? "1" : "0"));

    // lock
    if (bdParam.lock_acq_enable) {
        driverStatus |= static_cast<uint32_t>(XAie_DmaSetLock(&dmaInst,XAie_LockInit(static_cast<u16>(bdParam.lock_acq_id), static_cast<s8>(bdParam.lock_acq_value)),
                                        XAie_LockInit(static_cast<u16>(bdParam.lock_rel_id), static_cast<s8>(bdParam.lock_rel_value))));
    }
    debugMsg("lock_acq_enable %s, lock_acq_id %u, lock_acq_value %d, lock_rel_id %u, lock_rel_value %d",
             (bdParam.lock_acq_enable ? "1" : "0"), static_cast<std::uint32_t>(bdParam.lock_acq_id),
             static_cast<std::int32_t>(bdParam.lock_acq_value), static_cast<std::uint32_t>(bdParam.lock_rel_id),
             static_cast<std::int32_t>(bdParam.lock_rel_value));

    // burst length
    if (static_cast<uint32_t>(tileLoc.Row) == 0U) // shim tile
    {
        driverStatus |= static_cast<uint32_t>(XAie_DmaSetAxi(&dmaInst, static_cast<u8>(0U) /*Smid*/,static_cast<u8>(bdParam.burst_length) /*BurstLen*/, static_cast<u8>(0U) /*Qos*/,
                                        static_cast<u8>(0U) /*Cache*/, static_cast<u8>(0U) /*Secure*/));
        debugMsg("burst length %u", static_cast<std::uint32_t>(bdParam.burst_length));
    }

    // next bd
    if (bdParam.use_next_bd) {
        driverStatus |= static_cast<uint32_t>(XAie_DmaSetNextBd(&dmaInst, static_cast<u16>(bdParam.next_bd), static_cast<u8>(XAIE_ENABLE)));
        debugMsg("next bd %u", static_cast<std::uint32_t>(bdParam.next_bd));
    }

    // valid bd
    driverStatus |= static_cast<uint32_t>(XAie_DmaEnableBd(&dmaInst));

    // write bd
    driverStatus |= static_cast<uint32_t>(XAie_DmaWriteBd(config_manager::s_pDevInst, &dmaInst, tileLoc, static_cast<u16>(bdId)));
    debugMsg("XAie_DmaWriteBd %u", static_cast<std::uint32_t>(bdId));

    // Update status after using AIE driver
    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::dma_api::configureBD: AIE driver error.");
    }

    return err_code::ok;
}

err_code dma_api::enqueueTask(int32_t tileType, uint8_t column, uint8_t row, int32_t dir, uint8_t channel, uint32_t repeatCount,
                            bool enableTaskCompleteToken, uint8_t startBdId) {
    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0
    XAie_LocType tileLoc{};
    tileLoc = XAie_TileLoc(static_cast<u8>(column),static_cast<u8>(relativeToAbsoluteRow(tileType, row)));

    // start queue
    driverStatus |= static_cast<uint32_t>(XAie_DmaChannelSetStartQueue(config_manager::s_pDevInst, tileLoc, static_cast<u8>(channel), static_cast<XAie_DmaDirection>(dir),static_cast<u16>(startBdId), static_cast<u32>(repeatCount),
                                                static_cast<u8>(enableTaskCompleteToken ? 1U : 0U)));
    debugMsg("XAie_DmaChannelSetStartQueue col %u, row %u, channel %u, dir %d, startBD %u, repeat count %u, enable task complete token %s",
             static_cast<std::uint32_t>(tileLoc.Col), static_cast<std::uint32_t>(tileLoc.Row), static_cast<std::uint32_t>(channel),
             static_cast<std::int32_t>(dir), static_cast<std::uint32_t>(startBdId), static_cast<std::uint32_t>(repeatCount),
             (enableTaskCompleteToken ? "1" : "0"));

    // Update status after using AIE driver
    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::dma_api::enqueueTask: AIE driver error.");
    }

    return err_code::ok;
}

err_code dma_api::waitDMAChannelTaskQueue(int32_t tileType, uint8_t column, uint8_t row, int32_t dir, uint8_t channel) {
    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0
    XAie_LocType tileLoc{};
    tileLoc = XAie_TileLoc(static_cast<u8>(column),static_cast<u8>(relativeToAbsoluteRow(tileType, row)));

    debugMsg("To call XAie_DmaGetPendingBdCount col %u, row %u, channel %u, dir %d", static_cast<std::uint32_t>(tileLoc.Col),
             static_cast<std::uint32_t>(tileLoc.Row), static_cast<std::uint32_t>(channel), static_cast<std::int32_t>(dir));

    uint8_t numPendingBDs = 4U;
    while (static_cast<uint32_t>(numPendingBDs) > 3U) {
        // FIXME this driver API plus one if there is a BD running, what's needed us just the queue size register
        driverStatus |= static_cast<uint32_t>(XAie_DmaGetPendingBdCount(config_manager::s_pDevInst, tileLoc, static_cast<u8>(channel),static_cast<XAie_DmaDirection>(dir),
                                                static_cast<u8 *>(&numPendingBDs)));
    }

    // Update status after using AIE driver
    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::dma_api::waitDMAChannelTaskQueue: AIE driver error.");
    }

    return err_code::ok;
}

err_code dma_api::waitDMAChannelDone(int32_t tileType, uint8_t column, uint8_t row, int32_t dir, uint8_t channel) {
    //uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0
    XAie_LocType tileLoc{};
    tileLoc = XAie_TileLoc(static_cast<u8>(column),static_cast<u8>(relativeToAbsoluteRow(tileType, row)));

    debugMsg("To call XAie_DmaWaitForDone col %u, row %u, channel %u, dir %d", static_cast<std::uint32_t>(tileLoc.Col),
             static_cast<std::uint32_t>(tileLoc.Row), static_cast<std::uint32_t>(channel), static_cast<std::int32_t>(dir));

    while (static_cast<uint32_t>(XAie_DmaWaitForDone(config_manager::s_pDevInst, tileLoc, static_cast<u8>(channel),static_cast<XAie_DmaDirection>(dir), static_cast<u32>(0U))) != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
    }

    // Update status after using AIE driver
    // if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
    //     return errorMsg(err_code::aie_driver_error, "ERROR: abr::dma_api::waitDMAChannelDone: AIE driver error.");
    // }
    return err_code::ok;
}

err_code dma_api::updateBDAddress(int32_t tileType, uint8_t column, uint8_t row, uint8_t bdId, uint64_t address) {
    uint32_t driverStatus = static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK)); // 0
    XAie_LocType tileLoc{};
    tileLoc = XAie_TileLoc(static_cast<u8>(column),static_cast<u8>(relativeToAbsoluteRow(tileType, row)));

    driverStatus |= static_cast<uint32_t>(XAie_DmaUpdateBdAddr(config_manager::s_pDevInst, tileLoc, static_cast<u64>(address),static_cast<u16>(bdId)));
    debugMsg("XAie_DmaUpdateBdAddr col %u, row %u, address %" PRIx64 ", bdId %u", static_cast<std::uint32_t>(tileLoc.Col),
             static_cast<std::uint32_t>(tileLoc.Row), static_cast<std::uint64_t>(address), static_cast<std::uint32_t>(bdId));

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: adf::dma_api::updateBDAddress: AIE driver error.");
    }

    return err_code::ok;
}
/************************************ lock_api ************************************/

err_code lock_api::initializeLock(int32_t tileType, uint8_t column, uint8_t row, uint16_t lockId, int8_t initVal) {
    XAie_LocType tileLoc{};
    tileLoc = XAie_TileLoc(static_cast<u8>(column),static_cast<u8>(relativeToAbsoluteRow(tileType, row)));
    uint32_t driverStatus = static_cast<uint32_t>(XAie_LockSetValue(config_manager::s_pDevInst, tileLoc,XAie_LockInit(static_cast<u16>(lockId), static_cast<s8>(initVal))));
    debugMsg("XAie_LockSetValue col %u, row %u, lock id %u, value %u", static_cast<std::uint32_t>(tileLoc.Col),
             static_cast<std::uint32_t>(tileLoc.Row), static_cast<std::uint32_t>(lockId),
             static_cast<std::uint32_t>(static_cast<std::uint16_t>(initVal)));

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::lock_api::initializeLock: AIE driver error.");
    }
    return err_code::ok;
}

err_code lock_api::acquireLock(int32_t tileType, uint8_t column, uint8_t row, uint16_t lockId, int8_t acqVal) {
    XAie_LocType tileLoc{};
    tileLoc = XAie_TileLoc(static_cast<u8>(column),static_cast<u8>(relativeToAbsoluteRow(tileType, row)));
    debugMsg("To call XAie_LockAcquire col %u, row %u, lock id %u, acquire value %u", static_cast<std::uint32_t>(tileLoc.Col),
             static_cast<std::uint32_t>(tileLoc.Row), static_cast<std::uint32_t>(lockId),
             static_cast<std::uint32_t>(static_cast<std::uint16_t>(acqVal)));
    uint32_t driverStatus = static_cast<uint32_t>(XAie_LockAcquire(
        config_manager::s_pDevInst, tileLoc, XAie_LockInit(static_cast<u16>(lockId), static_cast<s8>(acqVal)), LOCK_TIMEOUT));
    debugMsg("XAie_LockAcquire col %u, row %u, lock id %u, acquire value %u", static_cast<std::uint32_t>(tileLoc.Col),
             static_cast<std::uint32_t>(tileLoc.Row), static_cast<std::uint32_t>(lockId),
             static_cast<std::uint32_t>(static_cast<std::uint16_t>(acqVal)));

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error,
                        "ERROR: abr::lock_api::acquireLock: XAieTile_LockAcquire timeout or AIE driver error.");
    }
    return err_code::ok;
}

err_code lock_api::releaseLock(int32_t tileType, uint8_t column, uint8_t row, uint16_t lockId, int8_t relVal) {
    XAie_LocType tileLoc{};
    tileLoc = XAie_TileLoc(static_cast<u8>(column),static_cast<u8>(relativeToAbsoluteRow(tileType, row)));
    uint32_t driverStatus = static_cast<uint32_t>(XAie_LockRelease(
        config_manager::s_pDevInst, tileLoc, XAie_LockInit(static_cast<u16>(lockId), static_cast<s8>(relVal)), LOCK_TIMEOUT));
    debugMsg("XAie_LockRelease col %u, row %u, lock id %u, release value %u", static_cast<std::uint32_t>(tileLoc.Col),
             static_cast<std::uint32_t>(tileLoc.Row), static_cast<std::uint32_t>(lockId),
             static_cast<std::uint32_t>(static_cast<std::uint16_t>(relVal)));

    if (driverStatus != static_cast<uint32_t>(static_cast<int32_t>(XAIE_OK))) {
        return errorMsg(err_code::aie_driver_error, "ERROR: abr::lock_api::releaseLock: AIE driver error.");
    }
    return err_code::ok;
}

} // namespace abr
