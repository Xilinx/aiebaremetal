// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
// (c) Copyright 2024 Advanced Micro Devices, Inc. All Rights reserved.

#ifndef __TRANSPOSE_BASE_H__
#define __TRANSPOSE_BASE_H__

#ifndef TRANSPOSE_LP_SIZE
#define TRANSPOSE_LP_SIZE 7
#endif

#ifndef TRANSPOSE_0213_LP_SIZE
#define TRANSPOSE_0213_LP_SIZE 4
#endif

enum TYPE_ODR {
    ODR_012 = 0,
    ODR_021,    // 1
    ODR_120,    // 2
    ODR_102,    // 3
    ODR_210,    // 4
    ODR_201,    // 5
    ODR_0312,   // 6
    ODR_0231,   // 7
    ODR_0213,   // 8
};

#endif