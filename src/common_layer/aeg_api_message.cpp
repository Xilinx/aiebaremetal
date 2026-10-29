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
 * under the License. -->
 */
// Original code by AMD Xilinx
// This copy is maintained by the SSW AIE Team
// Modifications by SSW AIE Team on 2023-12-30
// Description: Remove the error.h header file

/*
 * Changes made:
 * -SSW AIE Team, 2023-12-30
 *   - Remove the error.h header file.
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
#include "aeg_api_message.h"
#include <cstdint>
#include "aeg_log.h"
namespace aeg_api_message {
abr::err_code errorMsg(abr::err_code code, std::string_view msg) {
    (void)msg;
    return code;
}

void debugMsg(std::string_view msg) {
    AEG_LOG("%.*s\n", static_cast<int32_t>(msg.size()), msg.data());
    (void)msg;
}

void infoMsg(std::string_view msg) {
    AEG_LOG("%.*s\n", static_cast<int32_t>(msg.size()), msg.data());
    (void)msg;
}
} // namespace aeg_api_message