// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef _K_LOG_COMMON_
#define _K_LOG_COMMON_

#include <stdint.h>

#define LOG_MAX_LEN (4 * 1024)
#define MAICMARK 0XDEADFEEB
#define AIEML_CORE_LOG_DM_REALADDR_AIE1 (0x8000 - LOG_MAX_LEN)
#define AIEML_CORE_LOG_DM_REALADDR_AIE2 (0x10000 - LOG_MAX_LEN)

typedef uint32_t (*Read32BitDataFunc)(int row, int col, uint64_t address, uint64_t inst);

#endif
