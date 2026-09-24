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

#ifndef __RTPAPIIMPL_H__
#define __RTPAPIIMPL_H__

#include "common_layer/aeg_api_config.h"
#include "common_layer/aeg_runtime_api.h"
#include <memory>

/**
 * @brief RTP API implementation class that wraps graph_api for RTP operations
 *
 * This class holds the configuration and graph_api reference for direct RTP operations,
 * eliminating the need for string-based lookups on each method call.
 */
class RtpApiImpl {
  public:
    RtpApiImpl(const abr::rtp_config &config, std::shared_ptr<abr::graph_api> graphApi, const std::string &portName)
        : rtpConfig(config), graphApi(graphApi), portName(portName) {}

    /**
     * @brief Updates RTP port with new data
     * @param data Pointer to data to write
     * @param len Size of data in bytes
     * @return Error code
     */
    abr::err_code update(void *data, size_t len) {
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
    abr::err_code read(char *data, int len) {
        if (!graphApi) {
            return abr::err_code::internal_error;
        }
        return graphApi->read(&rtpConfig, data, len);
    }

    const std::string &getPortName() const { return portName; }

  private:
    abr::rtp_config rtpConfig;
    std::shared_ptr<abr::graph_api> graphApi;
    std::string portName;
};

#endif // __RTPAPIIMPL_H__
