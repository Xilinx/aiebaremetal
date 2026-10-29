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
 * under the License.-->
 */

#ifndef AEG_API_MESSAGE_H
#define AEG_API_MESSAGE_H

#include <cerrno>
#include <cstdint>
#include <string_view>
#include "aeg_log.h"

namespace abr {
enum class err_code : std::int32_t {
    ok = 0,
    user_error = EINVAL,
    internal_error = ENOTSUP,
    aie_driver_error = EIO,
    resource_unavailable = EAGAIN
};
}
namespace aeg_api_message {
abr::err_code errorMsg(abr::err_code code, std::string_view msg);

void debugMsg(std::string_view msg);

void infoMsg(std::string_view msg);

template <typename... Args>
inline void discardArgs(const Args &...) noexcept {}

template <typename First, typename... Rest>
inline void debugMsg(const char *fmt, First first, Rest... rest) {
    AEG_LOG(fmt, first, rest...);
    discardArgs(fmt, first, rest...);
}

template <typename First, typename... Rest>
inline void infoMsg(const char *fmt, First first, Rest... rest) {
    AEG_LOG(fmt, first, rest...);
    discardArgs(fmt, first, rest...);
}

template <typename First, typename... Rest>
inline abr::err_code errorMsg(abr::err_code code, const char *fmt, First first, Rest... rest) {
    discardArgs(fmt, first, rest...);
    return code;
}
} // namespace aeg_api_message
#endif /* AEG_API_MESSAGE_H */
