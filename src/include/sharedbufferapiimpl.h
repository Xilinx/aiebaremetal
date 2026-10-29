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

#ifndef SHAREDBUFFERAPIIMPL_H_
#define SHAREDBUFFERAPIIMPL_H_

#include "common_layer/aeg_api_config.h"
#include "common_layer/aeg_runtime_api.h"
#include <memory>

/**
 * @brief SharedBuffer API implementation class that wraps graph_api for shared buffer operations
 *
 * This class holds the configuration and graph_api reference for direct shared buffer operations,
 * eliminating the need for string-based lookups on each method call.
 */
namespace sharedbufferapiimpl {
class SharedBufferApiImpl {
  public:
    SharedBufferApiImpl(const abr::shared_buffer_config &config, std::shared_ptr<abr::graph_api> graphApi,
                        const std::string_view bufferName)
        : bufferConfig(config), graphApi(std::move(graphApi)), bufferName(bufferName) {}

    /**
     * @brief Updates shared buffer with new data
     * @param data Pointer to data to write
     * @param len Size of data in bytes
     * @return Error code
     */
    abr::err_code update(void *data, size_t len) const {
        if (!graphApi) {
            return abr::err_code::internal_error;
        }
        return graphApi->update(&bufferConfig, data, len);
    }

    const std::string_view getBufferName() const { return bufferName; }

  private:
    abr::shared_buffer_config bufferConfig;
    std::shared_ptr<abr::graph_api> graphApi;
    std::string_view bufferName;
};
} // namespace sharedbufferapiimpl
#endif // SHAREDBUFFERAPIIMPL_H_
