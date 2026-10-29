/**<!--
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
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

#ifndef RTPAPIIMPL_H_
#define RTPAPIIMPL_H_

#include "common_layer/aeg_api_config.h"
#include "common_layer/aeg_runtime_api.h"
#include <memory>
#include <string_view>
/**
 * @brief RTP API implementation class that wraps graph_api for RTP operations
 *
 * This class holds the configuration and graph_api reference for direct RTP operations,
 * eliminating the need for string-based lookups on each method call.
 */
namespace rtpapiimpl {
class RtpApiImpl {
  public:
    RtpApiImpl(const abr::rtp_config &config, std::shared_ptr<abr::graph_api> graphApi, const std::string_view portName)
        : rtpConfig(config), graphApi(std::move(graphApi)), portName(portName) {}

    /**
     * @brief Updates RTP port with new data
     * @param data Pointer to data to write
     * @param len Size of data in bytes
     * @return Error code
     */
    abr::err_code update(void *data, size_t len) const {
        if (!graphApi) {
            return abr::err_code::internal_error;
        }
        return graphApi->update(&rtpConfig, data, len);
    }

    /**
     * @brief Reads data from RTP port
     * @param data Pointer to buffer for read data
     * @param len Size of data to read in bytes
     * @return Error code
     */
    abr::err_code read(char *data, int32_t len) const {
        if (!graphApi) {
            return abr::err_code::internal_error;
        }
        return graphApi->read(&rtpConfig, data, static_cast<std::size_t>(len));
    }

    const std::string_view getPortName() const { return portName; }

  private:
    abr::rtp_config rtpConfig;
    std::shared_ptr<abr::graph_api> graphApi;
    std::string_view portName;
};
} // namespace rtpapiimpl

#endif // RTPAPIIMPL_H_
