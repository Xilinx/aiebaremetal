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
#ifndef AEG_LOG_H
#define AEG_LOG_H

#ifndef AEG_LOG
#if defined(AEG_DEBUG)
#ifdef __cplusplus
#include <cstdio>
#define AEG_LOG(...)                                                                                                   \
    do {                                                                                                               \
        std::printf(__VA_ARGS__);                                                                                      \
    } while (0)
#else
#include <stdio.h>
#define AEG_LOG(...)                                                                                                   \
    do {                                                                                                               \
        printf(__VA_ARGS__);                                                                                           \
    } while (0)
#endif
#else
#define AEG_LOG(...)                                                                                                   \
    do {                                                                                                               \
    } while (0)
#endif
#endif /* !AEG_LOG */

#endif /* AEG_LOG_H */
