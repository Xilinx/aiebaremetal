// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport_c.h"

#include "xaiengine.h"

void u8Copier(void* dst, const void* src);
int XAie_LockComparator(const void* object1, const void* object2);
const char* XAie_LockToString(const void* object) ;
int XAie_PacketComparator(const void* object1, const void* object2);
const char* XAie_PacketToString(const void* object) ;
void RegisterXAieLocTypeComparator();
const char* XAieLocTypeToString(const void *object) ;
int CompareXAieLocType(const void *expected, const void *actual);
