// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef __CONFIG__
#define __CONFIG__
// #define HW_TEST
#if defined(HW_TEST)
#define WIN_SIZE 256 //
#define DATA_LENGTH (1200 * 1200)
#else
#define WIN_SIZE 32 //
#define DATA_LENGTH 128
#endif
#endif
