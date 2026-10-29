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

#ifndef AEG_PROFILING_API_H
#define AEG_PROFILING_API_H

#include "aeg_api_config.h"
#include "aeg_api_message.h"

#include <cstdint>
#include <memory>

namespace xaiefal {
class XAieRsc;
}

namespace abr {

struct shim_config {
    shim_config();
    explicit shim_config(const gmio_config *pConfig);
    explicit shim_config(const plio_config *pConfig);

    int32_t shimColumn;
    int32_t streamPortId;
    uint8_t slaveOrMaster; // 0:slave, 1:master
};

class profiling {
  public:
    static err_code
    profile_stream_running_to_idle_cycles(shim_config shimConfig,
                                          std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources);
    static err_code
    profile_stream_start_to_transfer_complete_cycles(shim_config shimConfig, uint32_t numBytes,
                                                     std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources);
    static err_code
    profile_start_time_difference_btw_two_streams(shim_config shimConfig1, shim_config shimConfig2,
                                                  std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources);
    static err_code
    profile_stream_running_event_count(shim_config shimConfig,
                                       std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources);

    static uint64_t read(std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources,
                         bool startTimeDifference = false);
    static err_code stop(std::vector<std::shared_ptr<xaiefal::XAieRsc>> &acquiredResources,
                         bool startTimeDifference = false);
};

} // namespace abr
#endif // AEG_PROFILING_API_H
