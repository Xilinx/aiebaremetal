/**
 * @file test_comparator.cpp
 * @brief Custom comparators and helper functions for CppUTest.
 *
 * This file defines helper functions and custom comparators used by the
 * CppUTest-based unit tests for the AIE bare-metal project. The comparators
 * extend the default CppUTest comparison mechanisms to support project-
 * specific types such as XAie_Lock, XAie_Packet, and XAie_LocType.
 *
 * The utilities in this file are typically registered with the CppUTest
 * mock framework so that complex types can be compared and rendered in a
 * human-readable form when expectations fail.
 */
#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport_c.h"
#include "test_comparator.h"

/**
 * @brief Copy a single u8 value from source to destination.
 *
 * This helper is intended to be used as a custom copier function when
 * configuring comparators or mock expectations that operate on u8 values.
 * It performs a byte-wise copy using memcpy and assumes that @ref u8 is
 * a one-byte unsigned integer type.
 *
 * @param dst Pointer to the destination buffer where the value is copied.
 * @param src Pointer to the source buffer from which the value is read.
 */
void u8Copier(void* dst, const void* src)
{
	// A u8 is a single unsigned 8-bit integer (usually unsigned char).
	// Standard library memcpy is the safest way to copy the memory.
	// The size of u8 is assumed to be 1 byte, but we use sizeof(u8) for safety.
	memcpy(dst, src, sizeof(u8));
}

/**
 * @brief Comparator function for XAie_Lock objects.
 *
 * Compares two @ref XAie_Lock objects based on their LockId and LockVal
 * members. This function is suitable for use with CppUTest's custom
 * comparator registration APIs.
 *
 * @param object1 Pointer to the first XAie_Lock instance to compare.
 * @param object2 Pointer to the second XAie_Lock instance to compare.
 *
 * @return 1 if both locks have equal LockId and LockVal, otherwise 0.
 */
int XAie_LockComparator(const void* object1, const void* object2)
{
	const XAie_Lock* loc1 = (const XAie_Lock*)object1;
	const XAie_Lock* loc2 = (const XAie_Lock*)object2;
	return (loc1->LockId == loc2->LockId && loc1->LockVal == loc2->LockVal) ? 1 : 0;
}

/**
 * @brief Convert an XAie_Lock object to a human-readable string.
 *
 * Formats the contents of an @ref XAie_Lock instance into a static buffer
 * for diagnostic or comparison failure messages.
 *
 * @param object Pointer to the XAie_Lock to stringify.
 *
 * @return Pointer to a static, null-terminated string representing the lock.
 *         The buffer is reused on each call and is not thread-safe.
 */
const char* XAie_LockToString(const void* object)
{
	static char buffer[32];
	const XAie_Lock* loc = (const XAie_Lock*)object;
	sprintf(buffer, "XAie_Lock{LockId=%d, LockVal=%d}", loc->LockId, loc->LockVal);
	return buffer;
}

/**
 * @brief Comparator function for XAie_Packet objects.
 *
 * Compares two @ref XAie_Packet instances. The intent is to compare
 * packet identifier and type fields so that packet expectations in tests
 * can be validated.
 *
 * @param object1 Pointer to the first XAie_Packet instance.
 * @param object2 Pointer to the second XAie_Packet instance.
 *
 * @return 1 if the compared fields are equal, otherwise 0.
 */
int XAie_PacketComparator(const void* object1, const void* object2)
{
	const XAie_Packet* loc1 = (const XAie_Packet*)object1;
	const XAie_Packet* loc2 = (const XAie_Packet*)object2;
	return (loc1->PktId == loc2->PktId && loc1->PktType == loc2->PktType) ? 1 : 0;
}

/**
 * @brief Convert an XAie_Packet object to a human-readable string.
 *
 * Formats the contents of an @ref XAie_Packet instance into a static buffer
 * for use in debug output and comparator mismatch messages.
 *
 * @param object Pointer to the XAie_Packet to stringify.
 *
 * @return Pointer to a static, null-terminated string representing the packet.
 *         The buffer is reused on each call and is not thread-safe.
 */
const char* XAie_PacketToString(const void* object)
{
	static char buffer[32];
	const XAie_Packet* loc = (const XAie_Packet*)object;
	sprintf(buffer, "XAie_Packet{PktId=%d, PktType=%d}", loc->PktId, loc->PktType);
	return buffer;
}

/**
 * @brief Comparator implementation for XAie_LocType structures.
 *
 * Performs a member-wise comparison of two @ref XAie_LocType instances.
 * Currently, the comparison checks the Col and Row members and can be
 * extended if additional fields must be validated.
 *
 * @param expected Pointer to the expected XAie_LocType value.
 * @param actual   Pointer to the actual XAie_LocType value observed.
 *
 * @return 1 if all compared members are equal, otherwise 0.
 */
int CompareXAieLocType(const void* expected, const void* actual)
{
	const XAie_LocType* expectedLoc = (const XAie_LocType*)expected;
	const XAie_LocType* actualLoc   = (const XAie_LocType*)actual;

	// Perform member-by-member comparison
	if (expectedLoc->Col != actualLoc->Col) {
		return 0; // Not equal
	}

	if (expectedLoc->Row != actualLoc->Row) {
		return 0; // Not equal
	}

	// Add more fields if your struct has additional members to validate.

	return 1; // Equal
}

/**
 * @brief Convert an XAie_LocType object to a human-readable string.
 *
 * Provides a formatted representation of an @ref XAie_LocType instance,
 * including column and row. This is primarily used in test diagnostics
 * and comparator failure messages.
 *
 * @param object Pointer to the XAie_LocType instance to stringify.
 *
 * @return Pointer to a static, null-terminated string describing the value.
 *         The buffer is reused on each call and is not thread-safe.
 */
const char* XAieLocTypeToString(const void* object)
{
	const XAie_LocType* loc = (const XAie_LocType*)object;
	static char buffer[128];
	snprintf(buffer, sizeof(buffer), "XAie_LocType(Col: %u, Row: %u)", loc->Col, loc->Row);
	return buffer;
}

/**
 * @brief Register a custom comparator for XAie_LocType with CppUTest mocks.
 *
 * Installs a comparator and value-to-string function for the type name
 * "XAie_LocType" into the CppUTest C mock framework. Once this function
 * is called, mock expectations on XAie_LocType values can use the
 * registered comparator and will produce readable failure messages.
 */
void RegisterXAieLocTypeComparator()
{
	mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
}
