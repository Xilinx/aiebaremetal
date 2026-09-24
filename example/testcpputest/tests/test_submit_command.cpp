/**
 * @file test_submit_command.cpp
 * @brief Comprehensive unit tests for cert_host::cert_api::submit_command function
 *
 * This file contains comprehensive unit tests for the submit_command function in the
 * cert_host::cert_api class. The tests use stub infrastructure without hardware
 * dependencies. The submit_command function handles control code submission to the
 * AI Engine device through HSA queues with support for multiple execution paths
 * based on the number of columns.
 *
 * Test Areas:
 * - Input validation (zero columns, too many columns)
 * - Case 1: Single column execution path
 * - Case 3: 2-24 column execution path (uses level1 indirect entries)
 * - Case 4: >24 column execution path (uses level1 + level2 indirect entries)
 * - Allocation failure handling in all execution paths
 * - External buffer map population and handling
 * - Completion signal propagation (success and error codes)
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTest/MemoryLeakWarningPlugin.h"
#include "CppUTest/TestRegistry.h"

extern "C" {
#include "stubs/submit_cmd_stubs/stub_xaie_mem.h"
#include "stubs/submit_cmd_stubs/stub_barrier.h"
}
#include "stubs/submit_cmd_stubs/stub_cert_elf_loader.h"
// Note: HSA_MAX_LEVEL1_INDIRECT_ENTRIES is defined in host_queue.h (included via cert_host.h)

/*
 * Some third-party C headers in this tree define macros named `new` or
 * `nothrow`, which break usage of the C++ standard library containers.
 * Make sure they are not in scope before including <map>, <vector>, etc.
 */
#ifdef new
#undef new
#endif

#ifdef nothrow
#undef nothrow
#endif

#include <map>
#include <string_view>
#include <vector>
#include <cstdint>

#include "cert_host.h"  // for cert_host::cert_api

// HSA completion error code used only by tests; production headers
// do not define this symbol.
#ifndef HSA_COMP_ERROR_CODE
#define HSA_COMP_ERROR_CODE 5
#endif

using namespace cert_host;

/* Forward declaration for XAIE DevInst helper implemented in stubs/xaiengine.c */
extern "C" XAie_DevInst *Stub_XAie_CreateMinimalDevInst(void);

// Note: wakeup_cert/run_cert stubs not used - they conflict with real implementation

// Helper to poke private members of cert_api without modifying production code.
// Layout MUST match cert_host::cert_api exactly, including the public maps.
struct CertApiHack {
    // Public data members (must come first)
    std::map<int64_t, int64_t>  external_buffer_map;
    std::map<int64_t, uint64_t> external_buffer_addr_map;

    // Private data members (same order as in cert_host::cert_api)
    XAie_DevInst *DevInst;
    handshake_t   handshake;
    uint32_t      host_args_low;
    uint32_t      host_args_high;
    XAie_MemInst *host_args_meminst;
    XAie_MemInst *hsa_meminst;
    XAie_MemInst *comp_sig_meminst;
};

static void InitApiWithStubs(cert_api &api)
{
    // Reset XAIE and barrier stubs to a known state
    Stub_XAie_Reset();
    Stub_Barrier_Reset();
    Stub_AieMem_ResetAll();

    // Ensure DevInst is valid so wakeup/run loops are safe (or skipped)
    api.init_devinst(Stub_XAie_CreateMinimalDevInst());

    // Let the stubs own the actual XAie_MemInst objects
    static XAie_MemInst hsa_mem_inst{};
    static XAie_MemInst comp_mem_inst{};

    Stub_XAie_InitQueue(&hsa_mem_inst, &comp_mem_inst, HSA_QUEUE_ENTRY);

    // Wire cert_api's internal pointers to the same instances used by stubs
    CertApiHack *hack = reinterpret_cast<CertApiHack *>(&api);
    hack->hsa_meminst      = Stub_XAie_GetQueueMemInst();
    hack->comp_sig_meminst = Stub_XAie_GetCompMemInst();
}

static hsa_queue *GetQueue()
{
    XAie_MemInst *q_mem = Stub_XAie_GetQueueMemInst();
    return static_cast<hsa_queue *>(XAie_MemGetVAddr(q_mem));
}

static uint32_t *GetCompletionBuf()
{
    XAie_MemInst *c_mem = Stub_XAie_GetCompMemInst();
    return static_cast<uint32_t *>(XAie_MemGetVAddr(c_mem));
}
// Helper to call UUT
static int call_submit(cert_api &api,
    unsigned int cols,
    const std::vector<size_t> &sizes,
    uint32_t final_comp_sig)
{
StubCertElfLoader_SetColsLayout(cols, sizes);
g_stub_comp_sig_value = final_comp_sig;
char dummy[16] = {};
std::map<std::string_view, uint64_t> sym;
return api.submit_command(dummy, sizeof(dummy), sym);
}
/**
 * @brief Test group for cert_host::cert_api::submit_command functionality
 *
 * This test group validates the submit_command function which handles control code
 * submission to AI Engine devices. The function supports three execution paths:
 * - Case 1: Single column (direct submission)
 * - Case 3: 2-24 columns (level1 indirect packet structure)
 * - Case 4: >24 columns (level1 + level2 indirect packet structure)
 *
 * Test Areas:
 * - Input validation (column count bounds checking)
 * - Nominal execution paths for all three cases
 * - Memory allocation failure handling at each allocation point
 * - External buffer address map population
 * - Completion signal propagation and error handling
 *
 * The tests use stubs to simulate hardware behavior without requiring actual
 * AIE hardware or memory operations.
 */
TEST_GROUP(CertApiSubmitCommand)
{
    cert_api *api;

    /**
     * @brief Set up test fixtures before each test
     *
     * Initializes a new cert_api instance and configures it with stub
     * implementations for memory management, barrier operations, and
     * device instance. This ensures each test starts with a clean state.
     */
    void setup()
    {
        api = new cert_api();
        InitApiWithStubs(*api);
    }

    /**
     * @brief Clean up test fixtures after each test
     *
     * Deletes the cert_api instance to prevent memory leaks and prepare
     * for the next test. The stub infrastructure is reset automatically
     * by InitApiWithStubs in the next test's setup.
     */
    void teardown()
    {
        delete api;
    }
};

/**
 * @brief TC_001: Verify successful single column command submission (Case 1)
 *
 * This test validates the Case 1 execution path where submit_command handles
 * a single column of control code. The control code is allocated and directly
 * referenced in the XRT packet header without requiring indirect packet structures.
 */
TEST(CertApiSubmitCommand, TC_001_SingleColumnNominal)
{
    // Set up single column with 16 bytes of code
    StubCertElfLoader_SetColsLayout(1, {16});
    g_stub_comp_sig_value = HSA_COMP_SUCCESS;

    char dummy[16] = {};
    std::map<std::string_view, uint64_t> sym;

    int rc = api->submit_command(dummy, sizeof(dummy), sym);

    // Should succeed with single column
    CHECK_EQUAL(0, rc);
}



/**
 * @brief TC_002: Verify rejection of zero column count
 *
 * This test validates input validation logic that rejects invalid column counts.
 * When the control code contains zero columns, submit_command should reject the
 * request and return an error without attempting to process the command.
 */
TEST(CertApiSubmitCommand, TC_002_ColsZero)
{
    int rc = call_submit(*api, 0, {}, HSA_COMP_SUCCESS);
    CHECK_EQUAL(-1, rc);
}

/**
 * @brief TC_003: Verify rejection of excessive column count
 *
 * This test validates input validation logic that enforces the maximum column
 * count limit. The hardware supports a maximum of 24 columns, and submit_command
 * should reject requests exceeding this limit.
 */
TEST(CertApiSubmitCommand, TC_003_ColsTooLarge)
{
    int rc = call_submit(*api, 25, {1}, HSA_COMP_SUCCESS);
    CHECK_EQUAL(-1, rc);
}

/**
 * @brief TC_004: Verify handling of control code allocation failure in Case 1
 *
 * This test validates error handling when memory allocation fails during the
 * Case 1 (single column) execution path. The stub is configured to fail the
 * first allocation attempt for control code memory.
 */
TEST(CertApiSubmitCommand, TC_004_SingleColumnAllocFail)
{
    Stub_AieMem_SetAllocateFailIndex(0);
    int rc = call_submit(*api, 1, {64}, HSA_COMP_SUCCESS);
    CHECK_EQUAL(-1, rc);
}

/**
 * @brief TC_005: Verify propagation of completion error code in Case 1
 *
 * This test validates that submit_command correctly propagates non-success
 * completion codes back to the caller. When the hardware/stub indicates an
 * error via the completion signal, the function should return that specific
 * error code rather than a generic failure.
 */
TEST(CertApiSubmitCommand, TC_005_CompletionError)
{
    int rc = call_submit(*api, 1, {64}, HSA_COMP_ERROR_CODE);
    CHECK_EQUAL(HSA_COMP_ERROR_CODE, rc);
}

/**
 * @brief TC_006: Verify successful two-column command submission (Case 3)
 *
 * This test validates the Case 3 execution path where submit_command handles
 * 2-24 columns of control code. Indirect packet structures (level1) are used
 * to reference the control code for each column.
 */
TEST(CertApiSubmitCommand, TC_006_TwoColumnNominal)
{
    int rc = call_submit(*api, 2, {32, 48}, HSA_COMP_SUCCESS);
    CHECK_EQUAL(0, rc);
}

/**
 * @brief TC_007: Verify handling of entries allocation failure in Case 3
 *
 * This test validates error handling when the first memory allocation
 * (indirect packet entries buffer) fails during Case 3 execution.
 */
TEST(CertApiSubmitCommand, TC_007_TwoColumnEntryAllocFail)
{
    Stub_AieMem_SetAllocateFailIndex(0);
    int rc = call_submit(*api, 2, {32, 48}, HSA_COMP_SUCCESS);
    CHECK_EQUAL(-1, rc);
}

/**
 * @brief TC_008: Verify handling of control code allocation failure in Case 3
 *
 * This test validates error handling when the second memory allocation
 * (control code buffer) fails during Case 3 execution, after the first
 * allocation (entries buffer) has already succeeded.
 */
TEST(CertApiSubmitCommand, TC_008_TwoColumnCtrlAllocFail)
{
    // First allocation succeeds, second fails
    Stub_AieMem_SetAllocateFailIndex(1);
    int rc = call_submit(*api, 2, {32, 48}, HSA_COMP_SUCCESS);
    CHECK_EQUAL(-1, rc);
}

/**
 * @brief TC_009: Verify successful large column count submission (Case 4)
 *
 * This test validates the Case 4 execution path where submit_command handles
 * more columns than can fit in a single level of indirection. This requires
 * a two-level indirect packet structure (level1 + level2).
 */
TEST(CertApiSubmitCommand, TC_009_Case4Nominal)
{
    unsigned int cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1;
    std::vector<size_t> sizes(cols, 16);

    int rc = call_submit(*api, cols, sizes, HSA_COMP_SUCCESS);
    CHECK_EQUAL(0, rc);
}

/**
 * @brief TC_010: Verify Case 1 execution with external buffer mapping
 *
 * This test validates that submit_command correctly handles external buffer
 * address mappings during Case 1 execution. The external_buffer_map and
 * external_buffer_addr_map are populated before submission to verify they
 * don't interfere with normal operation.
 */
TEST(CertApiSubmitCommand, TC_010_Case1WithExternalBuffer)
{
    // Populate external buffer map before submit_command
    api->external_buffer_map[1] = 100;
    api->external_buffer_addr_map[100] = 0x12345678ULL;

    StubCertElfLoader_SetColsLayout(1, {16});
    g_stub_comp_sig_value = HSA_COMP_SUCCESS;

    char dummy[16] = {};
    std::map<std::string_view, uint64_t> sym;

    int rc = api->submit_command(dummy, sizeof(dummy), sym);
    CHECK_EQUAL(0, rc);
}

/**
 * @brief TC_011: Verify Case 3 execution with external buffer mapping
 *
 * This test validates that submit_command correctly handles external buffer
 * address mappings during Case 3 execution. This ensures that the indirect
 * packet structure creation doesn't interfere with external buffer handling.
 */
TEST(CertApiSubmitCommand, TC_011_Case3WithExternalBuffer)
{
    // Populate external buffer map
    api->external_buffer_map[1] = 200;
    api->external_buffer_addr_map[200] = 0xABCDEF00ULL;

    int rc = call_submit(*api, 2, {32, 48}, HSA_COMP_SUCCESS);
    CHECK_EQUAL(0, rc);
}

/**
 * @brief TC_012: Verify Case 4 execution with external buffer mapping
 *
 * This test validates that submit_command correctly handles external buffer
 * address mappings during Case 4 execution with two-level indirect packet
 * structures. This is the most complex execution path.
 */
TEST(CertApiSubmitCommand, TC_012_Case4WithExternalBuffer)
{
    // Populate external buffer map
    api->external_buffer_map[1] = 300;
    api->external_buffer_addr_map[300] = 0xDEADBEEFULL;

    unsigned int cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1;
    std::vector<size_t> sizes(cols, 16);

    int rc = call_submit(*api, cols, sizes, HSA_COMP_SUCCESS);
    CHECK_EQUAL(0, rc);
}

/**
 * @brief TC_013: Verify handling of entries allocation failure in Case 4
 *
 * This test validates error handling when the first memory allocation
 * (combined level1 + level2 entries buffer) fails during Case 4 execution.
 * Case 4 allocates both levels of indirection in a single buffer.
 */
TEST(CertApiSubmitCommand, TC_013_Case4EntryAllocFail)
{
    Stub_AieMem_SetAllocateFailIndex(0); // Fail first allocation (all entries)
    unsigned int cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1;
    std::vector<size_t> sizes(cols, 16);

    int rc = call_submit(*api, cols, sizes, HSA_COMP_SUCCESS);
    CHECK_EQUAL(-1, rc);
}

/**
 * @brief TC_014: Verify handling of control code allocation failure in Case 4
 *
 * This test validates error handling when the second memory allocation
 * (control code buffer) fails during Case 4 execution, after the first
 * allocation (combined entries buffer) has already succeeded.
 */
TEST(CertApiSubmitCommand, TC_014_Case4CtrlCodeAllocFail)
{
    Stub_AieMem_SetAllocateFailIndex(1); // Fail second allocation (ctrl code)
    unsigned int cols = HSA_MAX_LEVEL1_INDIRECT_ENTRIES + 1;
    std::vector<size_t> sizes(cols, 16);

    int rc = call_submit(*api, cols, sizes, HSA_COMP_SUCCESS);
    CHECK_EQUAL(-1, rc);
}
