#include <stdint.h>
#include <xaiengine.h>

#define XAIE_OK                0
#pragma pack(1)


// Module types
#define XAIE_CORE_MOD 0
#define XAIE_MEM_MOD  1
#define XAIE_PL_MOD   2

// Constants for testing
#define TEST_NUM_ROWS 5
#define TEST_NUM_COLS 8
#define TEST_COL_MIN 0
#define TEST_COL_MAX (TEST_NUM_COLS - 1)

// Event groups - used in the for loop range
#define XAIE_EVENT_GROUP_ERRORS_1_CORE       0  // Made up value for testing
#define XAIE_EVENT_GROUP_STREAM_SWITCH_CORE  100 // Made up value for testing
#define XAIE_EVENT_GROUP_ERRORS_PL          0 // Made up value for testing
#define XAIE_EVENT_GROUP_STREAM_SWITCH_PL   100 // Made up value for testing
#define XAIE_EVENT_GROUP_ERRORS_MEM         0   // Made up value for testing
#define XAIE_EVENT_GROUP_BROADCAST_MEM      100 // Made up value for testing
#define XAIE_EVENT_GROUP_ERRORS_MEM_TILE    0    // Made up value for testing
#define XAIE_EVENT_GROUP_BROADCAST_MEM_TILE 100 // Made up value for testing

// Convenience typedefs
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
// #define __AIESIM__ 1
inline u8 g_TileType = 0;
u8 Stub_GetTTypefromLoc(XAie_DevInst* DevInst, XAie_LocType Loc);
AieRC mock_Wakeup(XAie_DevInst *DevInst, XAie_LocType Loc,const struct XAie_UcMod *UcMod);
AieRC mock_Wakeup_fail(XAie_DevInst *DevInst, XAie_LocType Loc,const struct XAie_UcMod *UcMod);
AieRC mock_GetCoreStatus(XAie_DevInst *DevInst, XAie_LocType Loc,u32 *CoreStatus, const struct XAie_UcMod *UcMod);
AieRC mock_GetCoreStatusfail(XAie_DevInst *DevInst, XAie_LocType Loc,u32 *CoreStatus, const struct XAie_UcMod *UcMod);

// Test helpers for firmware completion simulation
extern uint32_t* g_test_completion_ptr;
extern void* g_test_comp_sig_meminst;
extern int g_test_sync_call_count;
extern int g_test_completion_threshold;
extern uint32_t g_test_completion_value;
// Create a mock of XAie_DevOps that uses our stub function
static XAie_DeviceOps  s_MockDevOps = {
	.GetTTypefromLoc = Stub_GetTTypefromLoc
};

static XAie_UcMod  Mock_XAie_UcMod = {
	.Wakeup = mock_Wakeup
};
static XAie_UcMod  Mock_XAie_UcModstatus = {
	.GetCoreStatus = mock_GetCoreStatus
};
static XAie_UcMod  Mock_XAie_UcModstatus_fail = {
	.GetCoreStatus = mock_GetCoreStatusfail
};
static XAie_UcMod  Mock_XAie_UcMod_fail = {
	.Wakeup = mock_Wakeup_fail
};
static XAie_UcMod  Mock_XAie_UcMod_full = {
	.Wakeup = mock_Wakeup,
	.GetCoreStatus = mock_GetCoreStatus
};
static XAie_PerfMod  Mock_XAie_PerfMod = {
	.MaxCounterVal = 1
};
void Stub_XAie_SetTileType(u8 TileType);

#ifndef PROFILING_API_STUB_H
#define PROFILING_API_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Control stub behavior - enable or disable profiling success
 * @param enable_success true = profiling calls succeed, false = profiling calls fail
 */
void set_profiling_stub_mode(bool enable_success);

/**
 * @brief Reset the profiling counter to initial value
 */
void reset_profiling_counter();

#ifdef __cplusplus
}
#endif

#endif // PROFILING_API_STUB_H
