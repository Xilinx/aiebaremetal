/**<!--
 * Copyright (C) 2021 Xilinx, Inc
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

#include "aeg_profiling_api.h"
#include "aeg_runtime_api.h"
#include "fal_util.h"
#include "xaiengine.h"
#include <cstdint>
#include <map>

namespace abr {
using aeg_api_message::debugMsg;
using aeg_api_message::errorMsg;
using aeg_api_message::infoMsg;
using fal::fal_util;
static constexpr std::uint32_t INVALID_TILE_COORD = 0xFFU;

namespace {
/* Helper: accumulate AieRC status via bitwise OR */
inline AieRC accumulateStatus(AieRC existing, AieRC newVal) {
    return static_cast<AieRC>(static_cast<uint32_t>(existing) | static_cast<uint32_t>(newVal));
}
} // namespace (anonymous)

constexpr std::array<XAie_Events, 8U> COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING = {XAIE_EVENT_PORT_RUNNING_0_PL, XAIE_EVENT_PORT_RUNNING_1_PL,
                                                        XAIE_EVENT_PORT_RUNNING_2_PL, XAIE_EVENT_PORT_RUNNING_3_PL,
                                                        XAIE_EVENT_PORT_RUNNING_4_PL, XAIE_EVENT_PORT_RUNNING_5_PL,
                                                        XAIE_EVENT_PORT_RUNNING_6_PL, XAIE_EVENT_PORT_RUNNING_7_PL};


constexpr std::array<XAie_Events, 8U> COMMON_XAIETILE_EVENT_SHIM_PORT_IDLE = {
   XAIE_EVENT_PORT_IDLE_0_PL, XAIE_EVENT_PORT_IDLE_1_PL, XAIE_EVENT_PORT_IDLE_2_PL, XAIE_EVENT_PORT_IDLE_3_PL,
   XAIE_EVENT_PORT_IDLE_4_PL, XAIE_EVENT_PORT_IDLE_5_PL, XAIE_EVENT_PORT_IDLE_6_PL, XAIE_EVENT_PORT_IDLE_7_PL};

constexpr std::array<XAie_Events, 16U> COMMON_XAIETILE_EVENT_SHIM_BROADCAST_A = {
   XAIE_EVENT_BROADCAST_A_0_PL,  XAIE_EVENT_BROADCAST_A_1_PL,  XAIE_EVENT_BROADCAST_A_2_PL,
   XAIE_EVENT_BROADCAST_A_3_PL,  XAIE_EVENT_BROADCAST_A_4_PL,  XAIE_EVENT_BROADCAST_A_5_PL,
   XAIE_EVENT_BROADCAST_A_6_PL,  XAIE_EVENT_BROADCAST_A_7_PL,  XAIE_EVENT_BROADCAST_A_8_PL,
   XAIE_EVENT_BROADCAST_A_9_PL,  XAIE_EVENT_BROADCAST_A_10_PL, XAIE_EVENT_BROADCAST_A_11_PL,
   XAIE_EVENT_BROADCAST_A_12_PL, XAIE_EVENT_BROADCAST_A_13_PL, XAIE_EVENT_BROADCAST_A_14_PL,
   XAIE_EVENT_BROADCAST_A_15_PL};

//////////////////////// shim_config ////////////////////////

shim_config::shim_config()
   : shimColumn{-1}
   , streamPortId{-1}
   , slaveOrMaster{0}
{}

shim_config::shim_config(const gmio_config *pConfig)
   : shimColumn{pConfig ? static_cast<int32_t>(pConfig->shimColumn) : static_cast<int32_t>(-1)}
   , streamPortId{pConfig ? static_cast<int32_t>(pConfig->streamId) : static_cast<int32_t>(-1)}
   , slaveOrMaster{pConfig ? static_cast<uint8_t>((pConfig->type == gmio_config::aie2gm) ? 1U : 0U) : static_cast<uint8_t>(0U)}
{}

shim_config::shim_config(const plio_config *pConfig)
   : shimColumn{pConfig ? static_cast<int32_t>(pConfig->shimColumn) : static_cast<int32_t>(-1)}
   , streamPortId{pConfig ? static_cast<int32_t>(pConfig->streamId) : static_cast<int32_t>(-1)}
   , slaveOrMaster{pConfig ? static_cast<uint8_t>(pConfig->slaveOrMaster) : static_cast<uint8_t>(0U)}
{}

err_code
profiling::profile_stream_running_to_idle_cycles(shim_config shimConfig,
                                                 std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources) {
    int32_t shimColumn = shimConfig.shimColumn;
    int32_t streamPortId = shimConfig.streamPortId;
    uint8_t slaveOrMaster = shimConfig.slaveOrMaster; // 0:slave, 1:master
    XAie_LocType tileLoc{};
    tileLoc = XAie_TileLoc(static_cast<uint8_t>(shimColumn), static_cast<uint8_t>(0U));
     AieRC driverStatus = AieRC::XAIE_OK;

    if ((static_cast<std::uint32_t>(tileLoc.Row) == INVALID_TILE_COORD) || (static_cast<std::uint32_t>(tileLoc.Col) == INVALID_TILE_COORD) || (shimColumn < 0) || (streamPortId < 0)) {
        return errorMsg(
            err_code::internal_error,
            "ERROR: event::start_profiling: Failed to access configuration information from IoAttr object.");
    }

    auto pSSwitchPortRsc = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn), static_cast<uint32_t>(0U)).sswitchPort();
    int32_t eventPortId = fal_util::request(pSSwitchPortRsc);
    if (eventPortId < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request stream switch event port resources.");
    }

    auto pPerfCounterRsc = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn), static_cast<uint32_t>(0U)).pl().perfCounter();
    std::int32_t counterId = fal_util::request(pPerfCounterRsc);
    if (counterId < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request performance counter resources.");
    }

    debugMsg("event::io_total_stream_running_to_idle_cycles");

    driverStatus = accumulateStatus(driverStatus, XAie_EventSelectStrmPort(config_manager::s_pDevInst, tileLoc, static_cast<uint8_t>(eventPortId),
                                             ((static_cast<std::uint32_t>(slaveOrMaster) == 0U) ? XAIE_STRMSW_SLAVE : XAIE_STRMSW_MASTER), SOUTH,
                                            static_cast<uint8_t>(streamPortId)));
    debugMsg("XAie_EventSelectStrmPort: col %d row %d , event port id %d, slave or master %d, port interface SOUTH, stream switch port id %d",
             static_cast<int32_t>(tileLoc.Col), static_cast<int32_t>(tileLoc.Row), static_cast<int32_t>(eventPortId),
             static_cast<int32_t>(slaveOrMaster), static_cast<int32_t>(streamPortId));

    driverStatus = accumulateStatus(driverStatus, XAie_PerfCounterControlSet(config_manager::s_pDevInst, tileLoc, XAIE_PL_MOD, static_cast<uint8_t>(counterId),
                                               COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[static_cast<size_t>(eventPortId)],
                                              COMMON_XAIETILE_EVENT_SHIM_PORT_IDLE[static_cast<size_t>(eventPortId)]));
    debugMsg("XAie_PerfCounterControlSet: col %d row %d, module XAIE_PL_MOD, counter id %d, start event COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[%d], stop event COMMON_XAIETILE_EVENT_SHIM_PORT_IDLE[%d]",
             static_cast<int32_t>(tileLoc.Col), static_cast<int32_t>(tileLoc.Row), static_cast<int32_t>(counterId),
             static_cast<int32_t>(eventPortId), static_cast<int32_t>(eventPortId));

    acquiredResources = {pPerfCounterRsc, pSSwitchPortRsc}; // order of insertion matters, see profiling::read

    if (driverStatus != AieRC::XAIE_OK) {
        return errorMsg(err_code::aie_driver_error, "ERROR: event::start_profiling: AIE driver error.");
    }

    return err_code::ok;
}

err_code profiling::profile_stream_start_to_transfer_complete_cycles(
    shim_config shimConfig, uint32_t numBytes, std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources) {
    int32_t shimColumn = shimConfig.shimColumn;
    int32_t streamPortId = shimConfig.streamPortId;
    uint8_t slaveOrMaster = shimConfig.slaveOrMaster; // 0:slave, 1:master
    XAie_LocType tileLoc{};
    tileLoc = XAie_TileLoc(static_cast<uint8_t>(shimColumn), static_cast<uint8_t>(0U));
     AieRC driverStatus = AieRC::XAIE_OK;

    if ((static_cast<std::uint32_t>(tileLoc.Row) == INVALID_TILE_COORD) || (static_cast<std::uint32_t>(tileLoc.Col) == INVALID_TILE_COORD) || (shimColumn < 0) || (streamPortId < 0)) {
        return errorMsg(
            err_code::internal_error,
            "ERROR: event::start_profiling: Failed to access configuration information from IoAttr object.");
    }

    auto pSSwitchPortRsc = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn), static_cast<uint32_t>(0U)).sswitchPort();
    int32_t eventPortId = fal_util::request(pSSwitchPortRsc);
    if (eventPortId < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request stream switch event port resources.");
    }

    auto pPerfCounterRsc0 = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn), static_cast<uint32_t>(0U)).pl().perfCounter();
    int32_t counterId0 = fal_util::request(pPerfCounterRsc0);
    if (counterId0 < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request performance counter resources.");
    }

    auto pPerfCounterRsc1 = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn), static_cast<uint32_t>(0U)).pl().perfCounter();
    int32_t counterId1 = fal_util::request(pPerfCounterRsc1);
    if (counterId1 < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request performance counter resources.");
    }

    debugMsg("event::io_stream_start_to_bytes_transferred_cycles");

    driverStatus = accumulateStatus(driverStatus, XAie_EventSelectStrmPort(config_manager::s_pDevInst, tileLoc, static_cast<uint8_t>(eventPortId),
                                           ((static_cast<std::uint32_t>(slaveOrMaster) == 0U) ? XAIE_STRMSW_SLAVE : XAIE_STRMSW_MASTER), SOUTH,
                                            static_cast<uint8_t>(streamPortId)));
    debugMsg("XAie_EventSelectStrmPort: col %d row %d , event port id %d, slave or master %d, port interface SOUTH, stream switch port id %d",
             static_cast<int32_t>(tileLoc.Col), static_cast<int32_t>(tileLoc.Row), static_cast<int32_t>(eventPortId),
             static_cast<int32_t>(slaveOrMaster), static_cast<int32_t>(streamPortId));

    driverStatus = accumulateStatus(driverStatus, XAie_PerfCounterEventValueSet(config_manager::s_pDevInst, tileLoc, XAIE_PL_MOD, static_cast<uint8_t>(counterId1),
                                                  static_cast<uint32_t>(numBytes / 4U)));
    debugMsg("XAie_PerfCounterEventValueSet: col %d row %d, module XAIE_PL_MOD, counter id %d, perf counter event value %u",
             static_cast<int32_t>(tileLoc.Col), static_cast<int32_t>(tileLoc.Row), static_cast<int32_t>(counterId1),
             static_cast<std::uint32_t>(numBytes / 4U));

    driverStatus = accumulateStatus(driverStatus,
        XAie_PerfCounterControlSet(config_manager::s_pDevInst, tileLoc, XAIE_PL_MOD, static_cast<uint8_t>(counterId0),
                                   COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[static_cast<size_t>(eventPortId)], XAIE_EVENT_PERF_CNT_1_PL));
    debugMsg("XAie_PerfCounterControlSet: col %d row %d, module XAIE_PL_MOD, counter id %d, start event COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[%d], stop event XAIE_EVENT_PERF_CNT_1_PL ",
             static_cast<int32_t>(tileLoc.Col), static_cast<int32_t>(tileLoc.Row), static_cast<int32_t>(counterId0),
             static_cast<int32_t>(eventPortId));

    driverStatus = accumulateStatus(driverStatus, XAie_PerfCounterControlSet(config_manager::s_pDevInst, tileLoc, XAIE_PL_MOD, static_cast<uint8_t>(counterId1),
                                              COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[static_cast<size_t>(eventPortId)],
                                              COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[static_cast<size_t>(eventPortId)]));
    debugMsg("XAie_PerfCounterControlSet: col %d row %d, module XAIE_PL_MOD, counter id %d, start event COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[%d], stop event COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[%d]",
             static_cast<int32_t>(tileLoc.Col), static_cast<int32_t>(tileLoc.Row), static_cast<int32_t>(counterId1),
             static_cast<int32_t>(eventPortId), static_cast<int32_t>(eventPortId));

    acquiredResources = {pPerfCounterRsc0, pPerfCounterRsc1,
                         pSSwitchPortRsc}; // order of insertion matters, see profiling::read

    if (driverStatus != AieRC::XAIE_OK) {
        return errorMsg(err_code::aie_driver_error, "ERROR: event::start_profiling: AIE driver error.");
    }

    return err_code::ok;
}

err_code
profiling::profile_stream_running_event_count(shim_config shimConfig,
                                              std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources) {
    int32_t shimColumn = shimConfig.shimColumn;
    int32_t streamPortId = shimConfig.streamPortId;
    uint8_t slaveOrMaster = shimConfig.slaveOrMaster; // 0:slave, 1:master
    XAie_LocType tileLoc{};
    tileLoc = XAie_TileLoc(static_cast<uint8_t>(shimColumn), static_cast<uint8_t>(0U));
     AieRC driverStatus = AieRC::XAIE_OK;

    if ((static_cast<std::uint32_t>(tileLoc.Row) == INVALID_TILE_COORD) || (static_cast<std::uint32_t>(tileLoc.Col) == INVALID_TILE_COORD) || (shimColumn < 0) || (streamPortId < 0)) {
        return errorMsg(
            err_code::internal_error,
            "ERROR: event::start_profiling: Failed to access configuration information from IoAttr object.");
    }

    auto pSSwitchPortRsc = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn), static_cast<uint32_t>(0U)).sswitchPort();
    int32_t eventPortId = fal_util::request(pSSwitchPortRsc);
    if (eventPortId < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request stream switch event port resources.");
    }

    auto pPerfCounterRsc = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn), static_cast<uint32_t>(0U)).pl().perfCounter();
    int32_t counterId = fal_util::request(pPerfCounterRsc);
    if (counterId < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request performance counter resources.");
    }

    debugMsg("event::io_stream_running_event_count");

    driverStatus = accumulateStatus(driverStatus, XAie_EventSelectStrmPort(config_manager::s_pDevInst, tileLoc, static_cast<uint8_t>(eventPortId),
                                              ((static_cast<uint32_t>(slaveOrMaster) == 0U) ? XAIE_STRMSW_SLAVE : XAIE_STRMSW_MASTER), SOUTH,
                                            static_cast<uint8_t>(streamPortId)));
    debugMsg("XAie_EventSelectStrmPort: col %d row %d, event port id %d, slave or master %d, port interface SOUTH, stream switch port id %d",
             static_cast<int32_t>(tileLoc.Col), static_cast<int32_t>(tileLoc.Row), static_cast<int32_t>(eventPortId),
             static_cast<int32_t>(slaveOrMaster), static_cast<int32_t>(streamPortId));

    driverStatus = accumulateStatus(driverStatus, XAie_PerfCounterControlSet(config_manager::s_pDevInst, tileLoc, XAIE_PL_MOD,static_cast<uint8_t>(counterId),
                                               COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[static_cast<size_t>(eventPortId)],
                                               COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[static_cast<size_t>(eventPortId)]));
    debugMsg("XAie_PerfCounterControlSet: col %d row %d, module XAIE_PL_MOD, counter id %d, start event COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[%d], stop event COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[%d]",
             static_cast<int32_t>(tileLoc.Col), static_cast<int32_t>(tileLoc.Row), static_cast<int32_t>(counterId),
             static_cast<int32_t>(eventPortId), static_cast<int32_t>(eventPortId));

    acquiredResources = {pPerfCounterRsc, pSSwitchPortRsc}; // order of insertion matters, see read_profiling

    if (driverStatus != AieRC::XAIE_OK) {
        return errorMsg(err_code::aie_driver_error, "ERROR: event::start_profiling: AIE driver error.");
    }

    return err_code::ok;
}

err_code profiling::profile_start_time_difference_btw_two_streams(
    shim_config shimConfig1, shim_config shimConfig2,
    std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources) {
    int32_t shimColumn1 = shimConfig1.shimColumn;
    int32_t streamPortId1 = shimConfig1.streamPortId;
    uint8_t slaveOrMaster1 = shimConfig1.slaveOrMaster; // 0:slave, 1:master
    XAie_LocType tileLoc1{};
    tileLoc1 = XAie_TileLoc(static_cast<uint8_t>(shimColumn1), static_cast<uint8_t>(0U));

    int32_t shimColumn2 = shimConfig2.shimColumn;
    int32_t streamPortId2 = shimConfig2.streamPortId;
    uint8_t slaveOrMaster2 = shimConfig2.slaveOrMaster; // 0:slave, 1:master
    XAie_LocType tileLoc2{};
    tileLoc2 = XAie_TileLoc(static_cast<uint8_t>(shimColumn2), static_cast<uint8_t>(0U));

    AieRC driverStatus = AieRC::XAIE_OK;

    if ((static_cast<std::uint32_t>(tileLoc1.Row) == INVALID_TILE_COORD) || (static_cast<std::uint32_t>(tileLoc1.Col) == INVALID_TILE_COORD) || (shimColumn1 < 0) ||
        (streamPortId1 < 0) || (static_cast<std::uint32_t>(tileLoc2.Row) == INVALID_TILE_COORD) || (static_cast<std::uint32_t>(tileLoc2.Col) == INVALID_TILE_COORD) ||
        (shimColumn2 < 0) || (streamPortId2 < 0)) {
        return errorMsg(
            err_code::internal_error,
            "ERROR: event::start_profiling: Failed to access configuration information from IoAttr object.");
    }

    auto pSSwitchPortRsc1 = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn1), static_cast<uint32_t>(0U)).sswitchPort();
    int32_t eventPortId1 = fal_util::request(pSSwitchPortRsc1);
    if (eventPortId1 < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request stream switch event port resources.");
    }

    auto pPerfCounterRsc1 = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn1), static_cast<uint32_t>(0U)).pl().perfCounter();
    int32_t counterId1 = fal_util::request(pPerfCounterRsc1);
    if (counterId1 < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request performance counter resources.");
    }

    auto pSSwitchPortRsc2 = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn2), static_cast<uint32_t>(0U)).sswitchPort();
    int32_t eventPortId2 = fal_util::request(pSSwitchPortRsc2);
    if (eventPortId2 < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request stream switch event port resources.");
    }

    auto pPerfCounterRsc2 = fal_util::s_pXAieDev->tile(static_cast<uint8_t>(shimColumn2), static_cast<uint32_t>(0U)).pl().perfCounter();
    int32_t counterId2 = fal_util::request(pPerfCounterRsc2);
    if (counterId2 < 0) {
        return errorMsg(err_code::resource_unavailable,
                        "ERROR: event::start_profiling: Failed to request performance counter resources.");
    }

    debugMsg("event::io_stream_start_difference_cycles");

    if (shimColumn1 == shimColumn2) {
        driverStatus = accumulateStatus(driverStatus, XAie_EventSelectStrmPort(config_manager::s_pDevInst, tileLoc1, static_cast<uint8_t>(eventPortId1),
                                                ((static_cast<uint32_t>(slaveOrMaster1) == 0U) ? XAIE_STRMSW_SLAVE : XAIE_STRMSW_MASTER), SOUTH,
                                                static_cast<uint8_t>(streamPortId1)));
        debugMsg("XAie_EventSelectStrmPort: col %d row %d , event port id %d, slave or master %d, port interface SOUTH, stream switch port id %d",
                 static_cast<int32_t>(tileLoc1.Col), static_cast<int32_t>(tileLoc1.Row), static_cast<int32_t>(eventPortId1),
                 static_cast<int32_t>(slaveOrMaster1), static_cast<int32_t>(streamPortId1));

        driverStatus = accumulateStatus(driverStatus, XAie_PerfCounterControlSet(
            config_manager::s_pDevInst, tileLoc1, XAIE_PL_MOD, static_cast<uint8_t>(counterId1), XAIE_EVENT_USER_EVENT_0_PL,
           COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[static_cast<size_t>(eventPortId1)])); // see Table 6-17
        debugMsg("XAie_PerfCounterControlSet: col %d row %d, module XAIE_PL_MOD, counter id %d, start event XAIE_EVENT_USER_EVENT_0_PL, stop event COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[%d]",
                 static_cast<int32_t>(tileLoc1.Col), static_cast<int32_t>(tileLoc1.Row), static_cast<int32_t>(counterId1),
                 static_cast<int32_t>(eventPortId1));

        driverStatus = accumulateStatus(driverStatus, XAie_EventSelectStrmPort(config_manager::s_pDevInst, tileLoc2, static_cast<uint8_t>(eventPortId2),
                                                ((static_cast<uint32_t>(slaveOrMaster2) == 0U) ? XAIE_STRMSW_SLAVE : XAIE_STRMSW_MASTER), SOUTH,
                                                static_cast<uint8_t>(streamPortId2)));
        debugMsg("XAie_EventSelectStrmPort: col %d row %d , event port id %d, slave or master %d, port interface SOUTH, stream switch port id %d",
                 static_cast<int32_t>(tileLoc2.Col), static_cast<int32_t>(tileLoc2.Row), static_cast<int32_t>(eventPortId2),
                 static_cast<int32_t>(slaveOrMaster2), static_cast<int32_t>(streamPortId2));

        driverStatus = accumulateStatus(driverStatus, XAie_PerfCounterControlSet(
            config_manager::s_pDevInst, tileLoc2, XAIE_PL_MOD, static_cast<uint8_t>(counterId2), XAIE_EVENT_USER_EVENT_0_PL,
           COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[static_cast<size_t>(eventPortId2)])); // see Table 6-17
        debugMsg("XAie_PerfCounterControlSet: col %d row %d, module XAIE_PL_MOD, counter id %d, start event XAIE_EVENT_USER_EVENT_0_PL, stop event COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[%d]",
                 static_cast<int32_t>(tileLoc2.Col), static_cast<int32_t>(tileLoc2.Row), static_cast<int32_t>(counterId2),
                 static_cast<int32_t>(eventPortId2));

        driverStatus = accumulateStatus(driverStatus,
            XAie_EventGenerate(config_manager::s_pDevInst, tileLoc1, XAIE_PL_MOD, XAIE_EVENT_USER_EVENT_0_PL));
        debugMsg("XAie_EventGenerate: col %d row %d, module XAIE_PL_MOD, event XAIE_EVENT_USER_EVENT_0_PL",
                 static_cast<int32_t>(tileLoc1.Col), static_cast<int32_t>(tileLoc1.Row));

        acquiredResources = {pPerfCounterRsc1, pPerfCounterRsc2, pSSwitchPortRsc1, pSSwitchPortRsc2};
    } else {
        // if IO objects are located on different shim tiles, broadcast user event from shimColumn1 to shimColumn2,
        // reserve the broadcast resources for all shim tiles from shimColumn1 to shimColumn2
        // note that each shim tile pl module has broadcast switch A (handle internal events and the core or mem module
        // on top of the shim) and broadcast switch B (for relay purpose and handle the core or mem module on top of the
        // shim)
        int32_t westShimColumn = (shimColumn1 < shimColumn2) ? shimColumn1 : shimColumn2;
        int32_t eastShimColumn = (shimColumn1 < shimColumn2) ? shimColumn2 : shimColumn1;
        int32_t numBcastShimColumns = eastShimColumn - westShimColumn + 1;

        std::vector<XAie_LocType> vLocs(numBcastShimColumns);
        for (int32_t i = 0; i < numBcastShimColumns; i++) {
            vLocs[static_cast<size_t>(i)] =
                XAie_TileLoc(static_cast<uint8_t>(shimColumn1 + i), static_cast<uint8_t>(0U)); // follow shimColumn1 -> shimColumn2
        }

        // reserve broadcast channel along the shim tiles
        auto pBroadcastRsc = fal_util::s_pXAieDev->broadcast(vLocs, XAIE_PL_MOD, XAIE_PL_MOD);
        int32_t broadcastId = fal_util::request(pBroadcastRsc);
        if (broadcastId < 0) {
            const bool rel1 = fal_util::release(pPerfCounterRsc1);
            const bool rel2 = fal_util::release(pSSwitchPortRsc1);
            const bool rel3 = fal_util::release(pPerfCounterRsc2);
            const bool rel4 = fal_util::release(pSSwitchPortRsc2);
            if ((!rel1) || (!rel2) || (!rel3) || (!rel4)) {
                static_cast<void>(errorMsg(
                    err_code::aie_driver_error,
                    "ERROR: event::start_profiling: Failed to release performance counter or stream switch event port resources."));
            }

            return errorMsg(
                err_code::resource_unavailable,
                "ERROR: event::start_profiling: Failed to request event broadcast resources across shim tiles.");
        }

        // configure event stream port, performance counter, and event broadcast
        driverStatus = accumulateStatus(driverStatus, XAie_EventSelectStrmPort(config_manager::s_pDevInst, tileLoc1, static_cast<uint8_t>(eventPortId1),
                                                 ((static_cast<std::uint32_t>(slaveOrMaster1) == 0U) ? XAIE_STRMSW_SLAVE : XAIE_STRMSW_MASTER), SOUTH,
                                                static_cast<uint8_t>(streamPortId1)));
        debugMsg("XAie_EventSelectStrmPort: col %d row %d , event port id %d, slave or master %d, port interface SOUTH, stream switch port id %d",
                 static_cast<int32_t>(tileLoc1.Col), static_cast<int32_t>(tileLoc1.Row), static_cast<int32_t>(eventPortId1),
                 static_cast<int32_t>(slaveOrMaster1), static_cast<int32_t>(streamPortId1));

        driverStatus = accumulateStatus(driverStatus, XAie_PerfCounterControlSet(
            config_manager::s_pDevInst, tileLoc1, XAIE_PL_MOD, static_cast<uint8_t>(counterId1), XAIE_EVENT_USER_EVENT_0_PL,
           COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[static_cast<size_t>(eventPortId1)])); // see Table 6-17
        debugMsg("XAie_PerfCounterControlSet: col %d row %d, module XAIE_PL_MOD, counter id %d, start event XAIE_EVENT_USER_EVENT_0_PL, stop event COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[%d]",
                 static_cast<int32_t>(tileLoc1.Col), static_cast<int32_t>(tileLoc1.Row), static_cast<int32_t>(counterId1),
                 static_cast<int32_t>(eventPortId1));

        driverStatus = accumulateStatus(driverStatus, XAie_EventSelectStrmPort(config_manager::s_pDevInst, tileLoc2, static_cast<uint8_t>(eventPortId2),
                                                ((static_cast<std::uint32_t>(slaveOrMaster2) == 0U) ? XAIE_STRMSW_SLAVE : XAIE_STRMSW_MASTER), SOUTH,
                                                static_cast<uint8_t>(streamPortId2)));
        debugMsg("XAie_EventSelectStrmPort: col %d row %d , event port id %d, slave or master %d, port interface SOUTH, stream switch port id %d",
                 static_cast<int32_t>(tileLoc2.Col), static_cast<int32_t>(tileLoc2.Row), static_cast<int32_t>(eventPortId2),
                 static_cast<int32_t>(slaveOrMaster2), static_cast<int32_t>(streamPortId2));

        driverStatus = accumulateStatus(driverStatus,
            XAie_PerfCounterControlSet(config_manager::s_pDevInst, tileLoc2, XAIE_PL_MOD, static_cast<std::uint8_t>(counterId2),
                                        COMMON_XAIETILE_EVENT_SHIM_BROADCAST_A[static_cast<size_t>(broadcastId)],
                                      COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[static_cast<size_t>(eventPortId2)])); // see Table 6-17 // see Table 6-17
        debugMsg("XAie_PerfCounterControlSet: col %d row %d, module XAIE_PL_MOD, counter id %d, start event COMMON_XAIETILE_EVENT_SHIM_BROADCAST_A[%d], stop event COMMON_XAIETILE_EVENT_SHIM_PORT_RUNNING[%d]",
                 static_cast<int32_t>(tileLoc2.Col), static_cast<int32_t>(tileLoc2.Row), static_cast<int32_t>(counterId2),
                 static_cast<int32_t>(broadcastId), static_cast<int32_t>(eventPortId2));

        // block event broadcast for unintended direction, see Figure 6-27 in Spec 1.5. In shim tile, only broadcast
        // switch A connects to shim tile event generation, broadcast switch B is just a pure relay
        {
            const uint8_t bcastShift = static_cast<uint8_t>(broadcastId);
            const uint32_t shiftAmount = static_cast<uint32_t>(bcastShift);
            const uint32_t maskU32 = static_cast<uint32_t>(1U) << shiftAmount;
            const uint16_t bcastMask = static_cast<uint16_t>(maskU32);
            // west shim tile switch A
            XAie_LocType westTileLoc{};
            westTileLoc = XAie_TileLoc(static_cast<uint8_t>(westShimColumn), static_cast<uint8_t>(0U));
            driverStatus = accumulateStatus(driverStatus, XAie_EventBroadcastBlockMapDir(
                config_manager::s_pDevInst, westTileLoc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_A, static_cast<uint32_t>(bcastMask),
                static_cast<uint8_t>(static_cast<uint32_t>(XAIE_EVENT_BROADCAST_WEST) | static_cast<uint32_t>(XAIE_EVENT_BROADCAST_NORTH) |
                static_cast<uint32_t>(XAIE_EVENT_BROADCAST_SOUTH)))); // block west, north & south
            driverStatus = accumulateStatus(
                driverStatus, XAie_EventBroadcastBlockMapDir(config_manager::s_pDevInst, westTileLoc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_B,
                static_cast<uint32_t>(bcastMask), static_cast<uint8_t>(static_cast<uint32_t>(XAIE_EVENT_BROADCAST_NORTH) |
                static_cast<uint32_t>(XAIE_EVENT_BROADCAST_SOUTH)))); // block north & south

            const int32_t loopBound = numBcastShimColumns - 1;
            for (int32_t i = 1; i < loopBound; i++) {
                XAie_LocType intermediateTileLoc{};
                intermediateTileLoc = XAie_TileLoc(static_cast<uint8_t>(westShimColumn + i), static_cast<uint8_t>(0U));
                // intermediate shim tile switch A
                driverStatus = accumulateStatus(driverStatus, XAie_EventBroadcastBlockMapDir(
                    config_manager::s_pDevInst, intermediateTileLoc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_A, static_cast<uint32_t>(bcastMask),
                    static_cast<uint8_t>(static_cast<uint32_t>(XAIE_EVENT_BROADCAST_NORTH) | static_cast<uint32_t>(XAIE_EVENT_BROADCAST_SOUTH)))); // block north & south
                driverStatus = accumulateStatus(driverStatus, XAie_EventBroadcastBlockMapDir(
                    config_manager::s_pDevInst, intermediateTileLoc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_B, static_cast<uint32_t>(bcastMask),
                    static_cast<uint8_t>(static_cast<uint32_t>(XAIE_EVENT_BROADCAST_NORTH) | static_cast<uint32_t>(XAIE_EVENT_BROADCAST_SOUTH)))); // block north & south
            }

            // east shim tile switch A
            XAie_LocType eastTileLoc{};
            eastTileLoc = XAie_TileLoc(static_cast<uint8_t>(eastShimColumn), static_cast<uint8_t>(0U));
            driverStatus = accumulateStatus(driverStatus, XAie_EventBroadcastBlockMapDir(config_manager::s_pDevInst, eastTileLoc, XAIE_PL_MOD,
                                                        XAIE_EVENT_SWITCH_A, static_cast<uint32_t>(bcastMask),
                                                        static_cast<uint8_t>(static_cast<uint32_t>(XAIE_EVENT_BROADCAST_EAST) | static_cast<uint32_t>(XAIE_EVENT_BROADCAST_NORTH) |
                                                        static_cast<uint32_t>(XAIE_EVENT_BROADCAST_SOUTH)))); // block east, north & south
        }

        const AieRC eventBroadcastStatus = XAie_EventBroadcast(config_manager::s_pDevInst, tileLoc1, XAIE_PL_MOD,
                                            static_cast<uint8_t>(broadcastId), XAIE_EVENT_USER_EVENT_0_PL);
        if(eventBroadcastStatus != AieRC::XAIE_OK) {
            static_cast<void>(errorMsg(err_code::aie_driver_error,
                "ERROR: event::start_profiling: Failed to generate event broadcast."));
        }
        driverStatus = accumulateStatus(driverStatus, eventBroadcastStatus);
        driverStatus = accumulateStatus(driverStatus,
            XAie_EventGenerate(config_manager::s_pDevInst, tileLoc1, XAIE_PL_MOD, XAIE_EVENT_USER_EVENT_0_PL));

        acquiredResources = {pPerfCounterRsc1, pPerfCounterRsc2, pSSwitchPortRsc1, pSSwitchPortRsc2, pBroadcastRsc};
    }

    if (driverStatus != AieRC::XAIE_OK) {
        return errorMsg(err_code::aie_driver_error, "ERROR: event::start_profiling: AIE driver error.");
    }

    return err_code::ok;
}

uint64_t profiling::read(std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources, bool startTimeDifference) {
    uint64_t value = 0;
    AieRC driverStatus = AieRC::XAIE_OK;

    if (startTimeDifference) {
        auto pPerfCounterRsc1 = dynamic_cast<xaiefal::XAiePerfCounter *>(acquiredResources[0].get());
        auto pPerfCounterRsc2 = dynamic_cast<xaiefal::XAiePerfCounter *>(acquiredResources[1].get());

        if ((pPerfCounterRsc1 != nullptr) && (pPerfCounterRsc2 != nullptr)) {
            uint32_t value1 = 0U;
            uint32_t value2 = 0U;
            uint32_t id1 = 0U;
            uint32_t id2 = 0U;
            XAie_LocType loc1{};
            XAie_LocType loc2{};
            XAie_ModuleType XAieModuleType1{};
            XAie_ModuleType XAieModuleType2{};

            driverStatus = accumulateStatus(driverStatus, static_cast<AieRC>(pPerfCounterRsc1->getRscId(loc1, XAieModuleType1, id1)));
            driverStatus = accumulateStatus(driverStatus, static_cast<AieRC>(pPerfCounterRsc2->getRscId(loc2, XAieModuleType2, id2)));
            if (driverStatus != AieRC::XAIE_OK) {
                static_cast<void>(errorMsg(err_code::aie_driver_error,
                         "ERROR: event::read_profiling: Failed to get performance counter resource id"));
            }
            driverStatus = accumulateStatus(driverStatus, XAie_PerfCounterGet(config_manager::s_pDevInst, loc1, XAieModuleType1, static_cast<uint8_t>(id1), &value1));
            debugMsg("XAie_PerfCounterGet: col %d row %d, module %d, counterId %u", static_cast<int32_t>(loc1.Col),
                     static_cast<int32_t>(loc1.Row), static_cast<std::int32_t>(XAieModuleType1), static_cast<std::uint32_t>(id1));

            driverStatus = accumulateStatus(driverStatus, XAie_PerfCounterGet(config_manager::s_pDevInst, loc2, XAieModuleType2, static_cast<uint8_t>(id2), &value2));
            debugMsg("XAie_PerfCounterGet: col %d row %d, module %d, counterId %u", static_cast<int32_t>(loc2.Col),
                     static_cast<int32_t>(loc2.Row), static_cast<std::int32_t>(XAieModuleType2), static_cast<std::uint32_t>(id2));

            value = static_cast<uint64_t>(static_cast<int64_t>(value2) - static_cast<int64_t>(value1));

            // based on vcd file analysis
            // adjust 2 cycles per shim tile (broadcast-a, broadcast-b) broadcast propagation delay,
            // plus 1 cycle for the user event to trigger the broadcast signal in the source shim tile
            // plus 1 cycle for the ariving broadcast signal to trigger the broadcast event in the destination shim tile
            int32_t shimColumn1 = static_cast<int32_t>(loc1.Col); // shim column that initiates the broadcast
            int32_t shimColumn2 = static_cast<int32_t>(loc2.Col); // shim column that receives the broadcast
            if (shimColumn1 != shimColumn2) {
                int32_t numBcastColumns =
                    ((shimColumn2 > shimColumn1) ? (shimColumn2 - shimColumn1) : (shimColumn1 - shimColumn2));
                value += static_cast<uint64_t>((numBcastColumns * 2) + 2);
            }
        } else {
            static_cast<void>(errorMsg(err_code::internal_error,
                "ERROR: event::read_profiling: The acquired resources order does not match the profiling option"));
        }
    } else {
        auto *const pPerfCounterRsc = dynamic_cast<xaiefal::XAiePerfCounter *>(acquiredResources[0].get());
        if (pPerfCounterRsc != nullptr) {
            uint32_t id = 0U;
            XAie_LocType loc{};
            XAie_ModuleType XAieModuleType{};

            driverStatus = accumulateStatus(driverStatus, static_cast<AieRC>(pPerfCounterRsc->getRscId(loc, XAieModuleType, id)));
            if (driverStatus != AieRC::XAIE_OK) {
                static_cast<void>(errorMsg(err_code::aie_driver_error,
                         "ERROR: event::read_profiling: Failed to get performance counter resource id"));
            }

            uint32_t perfCounterVal = 0U;
            static_cast<void>(accumulateStatus(
                driverStatus, XAie_PerfCounterGet(config_manager::s_pDevInst, loc, XAieModuleType,
                                                  static_cast<uint8_t>(id), &perfCounterVal)));
            value = static_cast<uint64_t>(perfCounterVal);
            debugMsg("XAie_PerfCounterGet: col %d row %d, module %d, counterId %u", static_cast<int32_t>(loc.Col),
                     static_cast<int32_t>(loc.Row), static_cast<std::int32_t>(XAieModuleType), static_cast<std::uint32_t>(id));
        } else {
            static_cast<void>(errorMsg(err_code::internal_error,
                "ERROR: event::read_profiling: The acquired resources order does not match the profiling option"));
        }
    }

    return value;
}

err_code profiling::stop(std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources, bool startTimeDifference) {
    AieRC driverStatus = AieRC::XAIE_OK;
    debugMsg("event::stop_profiling");

    const size_t numResources = acquiredResources.size();
    for (size_t i = 0U; i < numResources; i++) {
        auto acquiredResource = acquiredResources[i];
        auto *const pSingleTileRsc = dynamic_cast<xaiefal::XAieSingleTileRsc *>(acquiredResource.get());
        auto *const pBroadcastRsc = (pSingleTileRsc == nullptr)
                                        ? dynamic_cast<xaiefal::XAieBroadcast *>(acquiredResource.get())
                                        : nullptr;
        if (pSingleTileRsc != nullptr) {
            uint32_t id = 0U;
            XAie_LocType loc{};
            XAie_ModuleType XAieModuleType{};

            driverStatus = accumulateStatus(driverStatus, static_cast<AieRC>(pSingleTileRsc->getRscId(loc, XAieModuleType, id)));
            if (driverStatus != AieRC::XAIE_OK) {
                return errorMsg(err_code::aie_driver_error, "ERROR: event::read_profiling: Failed to get resource id");
            }

            auto *const pPerfCounterRsc = dynamic_cast<xaiefal::XAiePerfCounter *>(pSingleTileRsc);
            if (pPerfCounterRsc != nullptr) {
                driverStatus = accumulateStatus(driverStatus, XAie_PerfCounterReset(config_manager::s_pDevInst, loc, XAieModuleType, static_cast<uint8_t>(id)));
                debugMsg("XAie_PerfCounterReset: col %d row %d, module %d, counterId %u", static_cast<int32_t>(loc.Col),
                         static_cast<int32_t>(loc.Row), static_cast<std::int32_t>(XAieModuleType), static_cast<std::uint32_t>(id));

                driverStatus = accumulateStatus(driverStatus,
                    XAie_PerfCounterResetControlReset(config_manager::s_pDevInst, loc, XAieModuleType, static_cast<uint8_t>(id)));
                debugMsg("XAie_PerfCounterResetControlReset: col %d row %d, module %d, counterId %u",
                         static_cast<int32_t>(loc.Col), static_cast<int32_t>(loc.Row), static_cast<std::int32_t>(XAieModuleType),
                         static_cast<std::uint32_t>(id));
            } else if (dynamic_cast<xaiefal::XAieStreamPortSelect *>(pSingleTileRsc) != nullptr) {
                driverStatus = accumulateStatus(driverStatus, XAie_EventSelectStrmPortReset(config_manager::s_pDevInst, loc, static_cast<uint8_t>(id)));
                debugMsg("XAie_EventSelectStrmPortReset: col %d row %d, event port id %u", static_cast<int32_t>(loc.Col),
                         static_cast<int32_t>(loc.Row), static_cast<std::uint32_t>(id));
            } else if (dynamic_cast<xaiefal::XAiePCEvent *>(pSingleTileRsc) != nullptr) {
                driverStatus = accumulateStatus(
                    driverStatus, XAie_EventPCReset(config_manager::s_pDevInst, loc, static_cast<uint8_t>(id)));
                debugMsg("XAie_EventPCReset: col %d row %d, PCEventIndex %u", static_cast<int32_t>(loc.Col),
                         static_cast<int32_t>(loc.Row), static_cast<std::uint32_t>(id));
            } else {
                /* not a perf counter, stream port, or PC event resource */
            }
        } else if (pBroadcastRsc != nullptr) {
            std::vector<XAie_LocType> tileLocs;
            XAie_ModuleType startModule{};
            XAie_ModuleType endModule{};
            uint8_t broadcastId = static_cast<uint8_t>(pBroadcastRsc->getBc());
            static_cast<void>(pBroadcastRsc->getChannel(tileLocs, startModule, endModule));

            const XAie_ModuleType plModule = static_cast<XAie_ModuleType>(2);
            if (startTimeDifference && (startModule == plModule) && (endModule == plModule)) {
                // release shim pl module broadcast resources from source tile to destination tile

                // Step1 find the west shim id and east shim id across the broadcast.
                int32_t shimColumn1 = static_cast<int32_t>(tileLocs.front().Col); // shim column that initiates the broadcast
                int32_t shimColumn2 = static_cast<int32_t>(tileLocs.back().Col);  // shim column that receives the broadcast
                int32_t westShimColumn = (shimColumn1 < shimColumn2) ? shimColumn1 : shimColumn2;
                int32_t eastShimColumn = (shimColumn1 < shimColumn2) ? shimColumn2 : shimColumn1;
                int32_t numBcastShimColumns = eastShimColumn - westShimColumn + 1;

                driverStatus = accumulateStatus(driverStatus, XAie_EventBroadcastReset(config_manager::s_pDevInst, XAie_TileLoc(static_cast<uint8_t>(shimColumn1), static_cast<uint8_t>(0U)),
                                                         XAIE_PL_MOD, broadcastId));

                XAie_LocType westTileLoc{};

                westTileLoc = XAie_TileLoc(static_cast<uint8_t>(westShimColumn), static_cast<uint8_t>(0U));
                // west shim tile switch A
                driverStatus = accumulateStatus(driverStatus, XAie_EventBroadcastUnblockDir(
                    config_manager::s_pDevInst, westTileLoc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_A, static_cast<uint8_t>(broadcastId),
                    static_cast<uint8_t>(static_cast<uint32_t>(XAIE_EVENT_BROADCAST_WEST) | static_cast<uint32_t>(XAIE_EVENT_BROADCAST_NORTH) |
                    static_cast<uint32_t>(XAIE_EVENT_BROADCAST_SOUTH)))); // unblock west, north & south

                // west shim tile switch B
                driverStatus = accumulateStatus(driverStatus, XAie_EventBroadcastUnblockDir(
                    config_manager::s_pDevInst, westTileLoc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_B, static_cast<uint8_t>(broadcastId),
                    static_cast<uint8_t>(static_cast<uint32_t>(XAIE_EVENT_BROADCAST_NORTH) | static_cast<uint32_t>(XAIE_EVENT_BROADCAST_SOUTH))));

                const int32_t loopBound = numBcastShimColumns - 1;
                for (int32_t j = 1; j < loopBound; j++) {
                    XAie_LocType intermediateTileLoc{};
                    intermediateTileLoc = XAie_TileLoc(static_cast<uint8_t>(westShimColumn + j), static_cast<uint8_t>(0U));

                    // intermediate shim tile switch A
                    driverStatus = accumulateStatus(driverStatus, XAie_EventBroadcastUnblockDir(
                        config_manager::s_pDevInst, intermediateTileLoc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_A, static_cast<uint8_t>(broadcastId),
                        static_cast<uint8_t>(static_cast<uint32_t>(XAIE_EVENT_BROADCAST_NORTH) | static_cast<uint32_t>(XAIE_EVENT_BROADCAST_SOUTH))));

                    // intermediate shim tile switch B
                    driverStatus = accumulateStatus(driverStatus, XAie_EventBroadcastUnblockDir(
                        config_manager::s_pDevInst, intermediateTileLoc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_B, static_cast<uint8_t>(broadcastId),
                        static_cast<uint8_t>(static_cast<uint32_t>(XAIE_EVENT_BROADCAST_NORTH) | static_cast<uint32_t>(XAIE_EVENT_BROADCAST_SOUTH))));
                }

                XAie_LocType eastTileLoc{};
                eastTileLoc = XAie_TileLoc(static_cast<uint8_t>(eastShimColumn), static_cast<uint8_t>(0U));
                // east shim tile switch A
                driverStatus = accumulateStatus(driverStatus, XAie_EventBroadcastUnblockDir(
                    config_manager::s_pDevInst, eastTileLoc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_A, static_cast<uint8_t>(broadcastId),
                    static_cast<uint8_t>(static_cast<uint32_t>(XAIE_EVENT_BROADCAST_EAST) | static_cast<uint32_t>(XAIE_EVENT_BROADCAST_NORTH) |
                    static_cast<uint32_t>(XAIE_EVENT_BROADCAST_SOUTH)))); // unblock east, north & south
            }
        } else {
            /* acquired resource is neither single-tile nor broadcast */
        }

        if (!fal_util::release(acquiredResource)) {
            static_cast<void>(errorMsg(
                err_code::aie_driver_error, "ERROR: event::stop_profiling: Failed to release acquired resources."));
            driverStatus = static_cast<AieRC>(XAIE_ERR);
        }
    }

    if (driverStatus != AieRC::XAIE_OK) {
        return errorMsg(err_code::aie_driver_error, "ERROR: event::stop_profiling: AIE driver error.");
    }

    return err_code::ok;
}

} // namespace abr