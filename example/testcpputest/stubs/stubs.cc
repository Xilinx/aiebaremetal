/**
 * @file stubs.cc
 * @brief Test stub implementations for XAie / GMIO related APIs.
 *
 * This file provides simple, deterministic stub functions used by the
 * CppUTest test suites under example/testcpputest. The stubs replace
 * selected XAie driver and helper APIs so that unit tests can exercise
 * higher-level logic (wake-up, core status, tile-type handling) without
 * requiring real hardware or the full XAie library.
 *
 * All functions here are intentionally minimal and controllable from tests:
 *  - Stub_GetTTypefromLoc / Stub_XAie_SetTileType: global tile-type control.
 *  - mock_Wakeup / mock_Wakeup_fail: always-success / always-fail wakeup.
 *  - mock_GetCoreStatus / mock_GetCoreStatusfail: always-success / always-fail
 *    core status queries.
 */
#include "stubs.h"

// Globals for simulating firmware completion signal in tests
uint32_t* g_test_completion_ptr = NULL;
void* g_test_comp_sig_meminst = NULL;
int g_test_sync_call_count = 0;
int g_test_completion_threshold = 0;
uint32_t g_test_completion_value = 4; // Default to HSA_COMP_SUCCESS

/**
 * @brief Return the current stubbed tile type for a given device/location.
 *
 * This stub ignores the XAie device instance and tile location parameters
 * and simply returns the global variable g_TileType. It is used by tests
 * to simulate XAie_GetTTypefromLoc() behavior in a controlled way.
 *
 * @param DevInst  Pointer to the XAie device instance (unused in stub).
 * @param Loc      Tile location (column/row), unused in this stub.
 *
 * @return u8      The current value of g_TileType.
 */
u8 Stub_GetTTypefromLoc(XAie_DevInst* DevInst, XAie_LocType Loc)
{
	(void)DevInst;  // Unused parameter
	(void)Loc;      // Unused parameter

	return g_TileType;
}

/**
 * @brief Set the global stub tile type used by Stub_GetTTypefromLoc().
 *
 * Test cases call this helper to configure the value that will be returned
 * by Stub_GetTTypefromLoc(), allowing different tile-type scenarios to be
 * exercised without touching real hardware.
 *
 * @param TileType  Tile type value to store in g_TileType.
 */
void Stub_XAie_SetTileType(u8 TileType)
{
	g_TileType = TileType;
}

/**
 * @brief Stub implementation of a “wakeup” operation that always succeeds.
 *
 * This function mimics a XAie core wakeup routine and is used in unit tests
 * as a happy-path implementation. All input parameters are ignored and the
 * function simply returns XAIE_OK cast to AieRC.
 *
 * @param DevInst  Pointer to XAie device instance (unused).
 * @param Loc      Tile location (unused).
 * @param UcMod    Pointer to microcode module descriptor (unused).
 *
 * @return AieRC   XAIE_OK to indicate success.
 */
AieRC mock_Wakeup(XAie_DevInst *DevInst, XAie_LocType Loc, const struct XAie_UcMod *UcMod)
{
	(void)DevInst;  // Unused parameter
	(void)Loc;      // Unused parameter
	// (void)UcMod;    // Unused parameter

	return (AieRC)XAIE_OK;
}

/**
 * @brief Stub implementation of a “wakeup” operation that always fails.
 *
 * This is the negative-path counterpart to mock_Wakeup(). It ignores all
 * input parameters and always returns XAIE_ERR, allowing tests to verify
 * error-handling behavior when wakeup fails.
 *
 * @param DevInst  Pointer to XAie device instance (unused).
 * @param Loc      Tile location (unused).
 * @param UcMod    Pointer to microcode module descriptor (unused).
 *
 * @return AieRC   XAIE_ERR to indicate failure.
 */
AieRC mock_Wakeup_fail(XAie_DevInst *DevInst, XAie_LocType Loc, const struct XAie_UcMod *UcMod)
{
	(void)DevInst;  // Unused parameter
	(void)Loc;      // Unused parameter
	// (void)UcMod;    // Unused parameter

	return (AieRC)XAIE_ERR;
}

/**
 * @brief Stub implementation of a core-status query that always succeeds.
 *
 * This function stands in for a real XAie core status read API. All
 * parameters are ignored; it does not modify the CoreStatus pointer and
 * simply returns XAIE_OK cast to AieRC. Tests use this to model a
 * successful status read.
 *
 * @param DevInst    Pointer to XAie device instance (unused).
 * @param Loc        Tile location (unused).
 * @param CoreStatus Output pointer for status (unused and not written).
 * @param UcMod      Pointer to microcode module descriptor (unused).
 *
 * @return AieRC     XAIE_OK to represent a successful status read.
 */
AieRC mock_GetCoreStatus(XAie_DevInst *DevInst,
		XAie_LocType Loc,
		u32 *CoreStatus,
		const struct XAie_UcMod *UcMod)
{
	(void)DevInst;    // Unused parameter
	(void)Loc;        // Unused parameter
	(void)CoreStatus; // Unused parameter
	(void)UcMod;      // Unused parameter

	return (AieRC)XAIE_OK;
}

/**
 * @brief Stub implementation of a core-status query that always fails.
 *
 * Negative-path version of mock_GetCoreStatus(). It ignores all parameters
 * and always returns XAIE_ERR. Tests use this to validate how higher-level
 * code responds when reading core status fails.
 *
 * @param DevInst    Pointer to XAie device instance (unused).
 * @param Loc        Tile location (unused).
 * @param CoreStatus Output pointer for status (unused and not written).
 * @param UcMod      Pointer to microcode module descriptor (unused).
 *
 * @return AieRC     XAIE_ERR to represent a failed status read.
 */
AieRC mock_GetCoreStatusfail(XAie_DevInst *DevInst,
		XAie_LocType Loc,
		u32 *CoreStatus,
		const struct XAie_UcMod *UcMod)
{
	(void)DevInst;    // Unused parameter
	(void)Loc;        // Unused parameter
	(void)CoreStatus; // Unused parameter
	(void)UcMod;      // Unused parameter

	return (AieRC)XAIE_ERR;
}
