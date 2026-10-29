// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport_c.h"
#include "test_comparator.h"

void u8Copier(void* dst, const void* src)
{

	memcpy(dst, src, sizeof(u8));
}

int XAie_LockComparator(const void* object1, const void* object2)
{
	const XAie_Lock* loc1 = (const XAie_Lock*)object1;
	const XAie_Lock* loc2 = (const XAie_Lock*)object2;
	return (loc1->LockId == loc2->LockId && loc1->LockVal == loc2->LockVal) ? 1 : 0;
}

const char* XAie_LockToString(const void* object)
{
	static char buffer[32];
	const XAie_Lock* loc = (const XAie_Lock*)object;
	sprintf(buffer, "XAie_Lock{LockId=%d, LockVal=%d}", loc->LockId, loc->LockVal);
	return buffer;
}

int XAie_PacketComparator(const void* object1, const void* object2)
{
	const XAie_Packet* loc1 = (const XAie_Packet*)object1;
	const XAie_Packet* loc2 = (const XAie_Packet*)object2;
	return (loc1->PktId == loc2->PktId && loc1->PktType == loc2->PktType) ? 1 : 0;
}

const char* XAie_PacketToString(const void* object)
{
	static char buffer[32];
	const XAie_Packet* loc = (const XAie_Packet*)object;
	sprintf(buffer, "XAie_Packet{PktId=%d, PktType=%d}", loc->PktId, loc->PktType);
	return buffer;
}

int CompareXAieLocType(const void* expected, const void* actual)
{
	const XAie_LocType* expectedLoc = (const XAie_LocType*)expected;
	const XAie_LocType* actualLoc   = (const XAie_LocType*)actual;

	if (expectedLoc->Col != actualLoc->Col) {
		return 0;
	}

	if (expectedLoc->Row != actualLoc->Row) {
		return 0;
	}

	return 1;
}

const char* XAieLocTypeToString(const void* object)
{
	const XAie_LocType* loc = (const XAie_LocType*)object;
	static char buffer[128];
	snprintf(buffer, sizeof(buffer), "XAie_LocType(Col: %u, Row: %u)", loc->Col, loc->Row);
	return buffer;
}

void RegisterXAieLocTypeComparator()
{
	mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
}
