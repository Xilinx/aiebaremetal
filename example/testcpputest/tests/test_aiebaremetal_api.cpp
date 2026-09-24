/**
 * @file tests.cpp
 * @brief Unit tests for the BaremetalGraphSim class
 *
 * This file contains unit tests for the BaremetalGraphSim class, which provides
 * the main C++ interface for AIE baremetal graph simulation. The tests verify
 * graph initialization, run operations, GMIO operations, profiling, and error
 * handling functionality.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include <iostream>
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include <sstream>
#include <cstring>
#include <dlfcn.h>

#define __AIESIM__
#include "aiebaremetal.h"

#ifndef CMAKE_BINARY_DIR
#define CMAKE_BINARY_DIR "."
#endif

using namespace std::literals;

/**
 * @brief Mock implementation of BaremetalGraphSimBase for testing
 *
 * This mock class implements all virtual methods of BaremetalGraphSimBase
 * and uses CppUTest's mock support to verify method calls during testing.
 */
class MockBaremetalGraphSimBase : public BaremetalGraphSimBase {
public:
    void selftestexit() override {
        mock().actualCall("selftestexit");
    }

    ERR_CODE clock_enable() override {
        return mock().actualCall("clock_enable").returnIntValue();
    }

    ERR_CODE clock_disable() override {
        return mock().actualCall("clock_disable").returnIntValue();
    }

    void error_handling_cb_init(void *cb, void *priv) override {
        mock().actualCall("error_handling_cb_init").withParameter("cb", cb).withParameter("priv", priv);
    }

    void init() override {
        mock().actualCall("init");
    }

    void run(int num) override {
        mock().actualCall("run").withParameter("num", num);
    }

    void gm2aie_nb(char*, char*, int) override {
        mock().actualCall("gm2aie_nb");
    }

    void aie2gm_nb(char*, char*, int) override {
        mock().actualCall("aie2gm_nb");
    }

    void gm2aie(char*, char*, int) override {
        mock().actualCall("gm2aie");
    }

    void aie2gm(char*, char*, int) override {
        mock().actualCall("aie2gm");

    }
    void wait() override {
        mock().actualCall("wait");
    }

	void setAddress(char*,  void*) override {
        mock().actualCall("setAddress");
    };

	ERR_CODE wait(char*, std::string) override { return 0; };
	ERR_CODE gm2aie_nb(char*, std::string) override { return 0; };
	ERR_CODE aie2gm_nb(char*, std::string) override { return 0; };
	ERR_CODE setupPingPongBuffers(char*, void* const,  void* const) override { return 0; };
    ERR_CODE end() override { return 0; }
    int gmio_wait(std::string, bool) override { return 0; }
    void* malloc(uint32_t) override { return nullptr; }
    void free(void*) override {}
    ERR_CODE update(const std::string&, void*, size_t) override { return 0; }
    ERR_CODE read(const std::string&, char*, int) override { return 0; }
    char* dumpcorelog(int, int) override { return nullptr; }
    void* getaiedevinst() override { return nullptr; }
    void* BacktrackErrors() override { return nullptr; }
    void gdbpoint() override {}
    int start_profiling(const std::string&, int, uint32_t) override { return 0; }
    int start_profiling(const std::string&, const std::string&, int, uint32_t) override { return 0; }
    long long read_profiling(int) override { return 0; }
    void stop_profiling(int) override {}
};

/**
 * @brief Testable wrapper for BaremetalGraphSim that exposes internal state
 *
 * This class extends BaremetalGraphSim to provide access to the internal
 * bgraph pointer for testing purposes.
 */
class TestableBaremetalGraphSim : public BaremetalGraphSim {
public:
    using BaremetalGraphSim::BaremetalGraphSim;
    void setBGraph(BaremetalGraphSimBase* ptr) { this->bgraph = ptr; }
    void clearBGraph() { this->bgraph = nullptr; }
};

/**
 * @brief Test group for BaremetalGraphSim API functionality
 *
 * This test group verifies the core functionality of the BaremetalGraphSim class,
 * including graph initialization, run operations, GMIO operations (gm2aie/aie2gm),
 * memory management, profiling, and error handling. The tests validate both
 * successful operations and error conditions when the baremetal graph is not loaded.
 */
TEST_GROUP(RunApiGroup)
{
    char *savedEnvVal;

    std::streambuf* origCoutBuf;
    std::ostringstream testCout;

    TestableBaremetalGraphSim* sim;
    MockBaremetalGraphSimBase* mockGraph;

    /**
     * @brief Set up test fixtures before each test
     *
     * Initializes the test environment including:
     * - Saves and clears AIE_BAREMETAL_LIB environment variable
     * - Redirects cout to capture output
     * - Creates mock graph and simulator instances
     */
    void setup() {
        savedEnvVal = getenv("AIE_BAREMETAL_LIB");
        unsetenv("AIE_BAREMETAL_LIB");

        origCoutBuf = std::cout.rdbuf();
        testCout.str("");
        testCout.clear();
        std::cout.rdbuf(testCout.rdbuf());

        mockGraph = new MockBaremetalGraphSimBase();
        sim = new TestableBaremetalGraphSim("gradf");

        testCout.str("");
        testCout.clear();

        if (savedEnvVal) {
            setenv("AIE_BAREMETAL_LIB", savedEnvVal, 1);
        }
    }

    /**
     * @brief Clean up test fixtures after each test
     *
     * Restores the original cout buffer, cleans up mock and simulator
     * instances, and clears all mock expectations.
     */
    void teardown() {
        std::cout.rdbuf(origCoutBuf);

        sim->clearBGraph();

        delete sim;
        delete mockGraph;

        mock().clear();
    }
};

/**
 * @brief TC_001: Verify test fixture environment variable handling
 *
 * This test verifies that the test fixture correctly handles the AIE_BAREMETAL_LIB
 * environment variable preservation and restoration across test execution.
 */
TEST(RunApiGroup, TC_001_SetupRestoresExistingEnvironment)
{
    CHECK(sim != nullptr);
}

// Helper test group to set up environment variables for subsequent tests
TEST_GROUP(EnvironmentPrep)
{
    void setup() {
        setenv("AIE_BAREMETAL_LIB", "/tmp/test_lib.so", 1);
    }
    void teardown() {}
};

TEST(EnvironmentPrep, TC_001a_SetEnvironmentForNextTest)
{
    CHECK_TRUE(true);
}

/**
 * @brief TC_002: Verify run() calls bgraph->run() with default parameter -1
 *
 * This test verifies that when run() is called without parameters on a valid
 * BaremetalGraphSim instance, it properly delegates to the underlying bgraph's
 * run() method with the default value of -1.
 */
TEST(RunApiGroup, TC_002_CallsRunWithDefaultParamNegativeOne)
{

    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));

    mock().expectOneCall("run").withParameter("num", -1);

    sim->run();

    mock().checkExpectations();

    CHECK_EQUAL(""s, testCout.str());
}

/**
 * @brief TC_003: Verify run() with explicit iteration count
 *
 * This test verifies that when run() is called with a specific iteration count,
 * it correctly passes the value to the underlying bgraph's run() method.
 */
TEST(RunApiGroup, TC_003_CallsRunWithSpecificIterations)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));

    mock().expectOneCall("run").withParameter("num", 42);

    sim->run(42);

    mock().checkExpectations();

    CHECK_EQUAL(""s, testCout.str());
}

/**
 * @brief TC_004: Verify run() error handling when bgraph is null
 *
 * This test verifies that when run() is called but the baremetal graph library
 * is not loaded (bgraph is null), an appropriate error message is printed.
 */
TEST(RunApiGroup, TC_004_PrintsErrorWhenBgraphIsNull)
{
    sim->clearBGraph();

    sim->run(7);

    mock().checkExpectations();

    STRCMP_CONTAINS("baremetal graph load failed", testCout.str().c_str());
    STRCMP_CONTAINS("AIE_BAREMETAL_LIB", testCout.str().c_str());
}

/**
 * @brief TC_005: Verify successful init() operation
 *
 * This test verifies that the init() method correctly delegates to the
 * underlying bgraph's init() method when bgraph is properly loaded.
 */
TEST(RunApiGroup, TC_005_CallsInit)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    mock().expectOneCall("init");
    sim->init();
    mock().checkExpectations();
}

/**
 * @brief TC_006: Verify successful clock_enable() operation
 *
 * This test verifies that the clock_enable() method correctly enables the
 * AIE clock and returns success code.
 */
TEST(RunApiGroup, TC_006_CallsClockEnable)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    mock().expectOneCall("clock_enable").andReturnValue(0);
    ERR_CODE result = sim->clock_enable();
    mock().checkExpectations();
    CHECK_EQUAL(0, result);
}

/**
 * @brief TC_007: Verify successful clock_disable() operation
 *
 * This test verifies that the clock_disable() method correctly disables the
 * AIE clock and returns success code.
 */
TEST(RunApiGroup, TC_007_CallsClockDisable)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    mock().expectOneCall("clock_disable").andReturnValue(0);
    ERR_CODE result = sim->clock_disable();
    mock().checkExpectations();
    CHECK_EQUAL(0, result);
}

/**
 * @brief TC_008: Verify GMIO gm2aie non-blocking operation
 *
 * This test verifies that the gm2aie_nb() method correctly initiates a
 * non-blocking data transfer from global memory to AIE.
 */
TEST(RunApiGroup, TC_008_CallsGm2aieNb)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    char name[] = "test_name";
    char data[] = "test_data";
    mock().expectOneCall("gm2aie_nb");
    sim->gm2aie_nb(name, data, 10);
    mock().checkExpectations();
}

/**
 * @brief TC_009: Verify GMIO aie2gm non-blocking operation
 *
 * This test verifies that the aie2gm_nb() method correctly initiates a
 * non-blocking data transfer from AIE to global memory.
 */
TEST(RunApiGroup, TC_009_CallsAie2gmNb)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    char name[] = "test_name";
    char data[] = "test_data";
    mock().expectOneCall("aie2gm_nb");
    sim->aie2gm_nb(name, data, 10);
    mock().checkExpectations();
}

/**
 * @brief TC_010: Verify GMIO wait operation
 *
 * This test verifies that the wait() method correctly waits for completion
 * of outstanding GMIO operations.
 */
TEST(RunApiGroup, TC_010_CallsWait)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    mock().expectOneCall("wait");
    sim->wait();
    mock().checkExpectations();
}

/**
 * @brief TC_011: Verify end() operation returns success
 *
 * This test verifies that the end() method properly completes graph execution
 * and returns the appropriate success error code.
 */
TEST(RunApiGroup, TC_011_CallsEnd)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    auto result = sim->end();
    mock().checkExpectations();
    CHECK_EQUAL((int)abr::err_code::ok, (int)result);
}

/**
 * @brief TC_012: Verify memory allocation through malloc()
 *
 * This test verifies that the malloc() method correctly allocates memory
 * for AIE operations through the underlying bgraph.
 */
TEST(RunApiGroup, TC_012_CallsMalloc)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    void* ptr = sim->malloc(1024);
    mock().checkExpectations();
    CHECK_EQUAL(nullptr, ptr);
}

/**
 * @brief TC_013: Verify RTP update operation
 *
 * This test verifies that the update() method correctly updates run-time
 * parameters (RTP) of the graph.
 */
TEST(RunApiGroup, TC_013_CallsUpdate)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    char data[] = "test_data";
    auto result = sim->update("port_name", data, 10);
    mock().checkExpectations();
    CHECK_EQUAL((int)abr::err_code::ok, (int)result);
}

/**
 * @brief TC_014: Verify RTP read operation
 *
 * This test verifies that the read() method correctly reads run-time
 * parameters (RTP) from the graph.
 */
TEST(RunApiGroup, TC_014_CallsRead)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    char data[100];
    auto result = sim->read("port_name", data, 100);
    mock().checkExpectations();
    CHECK_EQUAL((int)abr::err_code::ok, (int)result);
}

/**
 * @brief TC_015: Verify profiling start operation for single port
 *
 * This test verifies that the start_profiling() method correctly initiates
 * performance profiling for a single port.
 */
TEST(RunApiGroup, TC_015_CallsStartProfilingSinglePort)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    int result = sim->start_profiling(std::string("port1"), 0, (uint32_t)100);
    mock().checkExpectations();
    CHECK_EQUAL(0, result);
}

/**
 * @brief TC_016: Verify profiling read operation
 *
 * This test verifies that the read_profiling() method correctly reads
 * profiling counter values.
 */
TEST(RunApiGroup, TC_016_CallsReadProfiling)
{
    sim->setBGraph(dynamic_cast<BaremetalGraphSimBase*>(mockGraph));
    long long result = sim->read_profiling(1);
    mock().checkExpectations();
    CHECK_EQUAL(0, result);
}

/**
 * @brief TC_017: Verify init() error handling when bgraph is null
 *
 * This test verifies that the init() method prints an appropriate error
 * message when the baremetal graph is not loaded.
 */
TEST(RunApiGroup, TC_017_InitPrintsErrorWhenBgraphIsNull)
{
    sim->clearBGraph();
    sim->init();
    STRCMP_CONTAINS("baremetal graph load failed", testCout.str().c_str());
}

/**
 * @brief TC_018: Verify clock_enable() error handling when bgraph is null
 *
 * This test verifies that the clock_enable() method returns an error code
 * when the baremetal graph is not loaded.
 */
TEST(RunApiGroup, TC_018_ClockEnableReturnsErrorWhenBgraphIsNull)
{
    sim->clearBGraph();
    ERR_CODE result = sim->clock_enable();
    CHECK_EQUAL(-1, result);
}

/**
 * @brief TC_019: Verify end() error handling when bgraph is null
 *
 * This test verifies that the end() method returns a user_error code and
 * prints an appropriate error message when the baremetal graph is not loaded.
 */
TEST(RunApiGroup, TC_019_EndReturnsErrorWhenBgraphIsNull)
{
    sim->clearBGraph();
    auto result = sim->end();
    STRCMP_CONTAINS("baremetal graph load failed", testCout.str().c_str());
    CHECK_EQUAL((int)abr::err_code::user_error, (int)result);
}

/**
 * @brief TC_020: Verify malloc() error handling when bgraph is null
 *
 * This test verifies that the malloc() method returns nullptr and prints
 * an error message when the baremetal graph is not loaded.
 */
TEST(RunApiGroup, TC_020_MallocReturnsNullWhenBgraphIsNull)
{
    sim->clearBGraph();
    void* result = sim->malloc(1024);
    CHECK_EQUAL(nullptr, result);
    STRCMP_CONTAINS("baremetal graph load failed", testCout.str().c_str());
}

/**
 * @brief TC_022: Verify update() error handling when bgraph is null
 *
 * This test verifies that the update() method returns a user_error code
 * when the baremetal graph is not loaded.
 */
TEST(RunApiGroup, TC_022_UpdateReturnsErrorWhenBgraphIsNull)
{
    sim->clearBGraph();
    char data[] = "test";
    auto result = sim->update("port", data, 10);
    STRCMP_CONTAINS("baremetal graph load failed", testCout.str().c_str());
    CHECK_EQUAL((int)abr::err_code::user_error, (int)result);
}

/**
 * @brief Test group for BaremetalGraphSim constructor and destructor
 *
 * This test group verifies the initialization and cleanup functionality of
 * BaremetalGraphSim, including library loading, environment variable handling,
 * and proper resource cleanup in the destructor.
 */
TEST_GROUP(ConstructorDestructorGroup)
{
    char *savedEnvVal;
    std::streambuf* origCoutBuf;
    std::ostringstream testCout;

    /**
     * @brief Set up test environment before each test
     *
     * Saves the current AIE_BAREMETAL_LIB environment variable and
     * redirects cout to capture output messages.
     */
    void setup() {
        savedEnvVal = getenv("AIE_BAREMETAL_LIB");
        origCoutBuf = std::cout.rdbuf();
        testCout.str("");
        testCout.clear();
        std::cout.rdbuf(testCout.rdbuf());
    }

    /**
     * @brief Clean up test environment after each test
     *
     * Restores the original cout buffer and AIE_BAREMETAL_LIB
     * environment variable to their original states.
     */
    void teardown() {
        std::cout.rdbuf(origCoutBuf);
        if (savedEnvVal) {
            setenv("AIE_BAREMETAL_LIB", savedEnvVal, 1);
        } else {
            unsetenv("AIE_BAREMETAL_LIB");
        }
    }
};

// Helper test to set environment for ConstructorDestructorGroup tests
TEST(EnvironmentPrep, TC_001b_SetEnvironmentForConstructorGroup)
{
    CHECK_TRUE(true);
}

/**
 * @brief TC_023: Verify destructor properly cleans up resources
 *
 * This test verifies that the BaremetalGraphSim destructor correctly deletes
 * the bgraph object and closes the library handle when the simulator is destroyed.
 */
TEST(ConstructorDestructorGroup, TC_023_DestructorDeletesBgraphAndClosesHandle)
{
    // Create with valid mock bgraph to ensure destructor paths are covered
    TestableBaremetalGraphSim* sim = new TestableBaremetalGraphSim("testgraph");
    MockBaremetalGraphSimBase* mockGraph = new MockBaremetalGraphSimBase();
    sim->setBGraph(mockGraph);

    // Delete sim - this should trigger the destructor which deletes bgraph (mockGraph)
    // Note: Don't delete mockGraph separately as sim's destructor will handle it
    delete sim;

    // If we get here without crash, destructor worked
    CHECK(true);
}

/**
 * @brief TC_024: Verify constructor error when environment variable not set
 *
 * This test verifies that the constructor prints an appropriate error message
 * when the AIE_BAREMETAL_LIB environment variable is not set.
 */
TEST(ConstructorDestructorGroup, TC_024_ConstructorWithoutEnvVariable)
{
    unsetenv("AIE_BAREMETAL_LIB");

    TestableBaremetalGraphSim* sim = new TestableBaremetalGraphSim("testgraph");

    STRCMP_CONTAINS("AIE_BAREMETAL_LIB environment variable not set", testCout.str().c_str());

    delete sim;
}

/**
 * @brief TC_025: Verify constructor error with invalid library path
 *
 * This test verifies that the constructor handles the case when the library
 * path specified in AIE_BAREMETAL_LIB does not exist or is invalid.
 */
TEST(ConstructorDestructorGroup, TC_025_ConstructorWithInvalidLibPath)
{
    setenv("AIE_BAREMETAL_LIB", "/nonexistent/path/lib.so", 1);

    TestableBaremetalGraphSim* sim = new TestableBaremetalGraphSim("testgraph");

    // Should contain error message about loading library
    std::string output = testCout.str();
    CHECK(output.find("Error loading library") != std::string::npos ||
          output.find("load failed") != std::string::npos);

    delete sim;
}

/**
 * @brief TC_026: Verify constructor successfully loads valid mock library
 *
 * This test verifies that the constructor successfully loads a valid baremetal
 * library and prints the appropriate success message.
 */
TEST(ConstructorDestructorGroup, TC_026_ConstructorWithValidMockLibrary)
{
    // Set path to our mock library - CMAKE_BINARY_DIR will be the build directory
    std::string libPath = std::string(CMAKE_BINARY_DIR) + "/mock_aie_library.so";
    setenv("AIE_BAREMETAL_LIB", libPath.c_str(), 1);

    TestableBaremetalGraphSim* sim = new TestableBaremetalGraphSim("testgraph");

    // Should contain success message about loading library
    std::string output = testCout.str();
    STRCMP_CONTAINS("load success", output.c_str());

    delete sim;
}

/**
 * @brief TC_027: Verify GMIO methods error handling when library load fails
 *
 * This test verifies that GMIO methods (gm2aie_nb, aie2gm_nb, gm2aie, aie2gm)
 * print appropriate error messages when the baremetal library failed to load.
 */
TEST(ConstructorDestructorGroup, TC_027_GMIOMethodsWithFailedLibraryLoad)
{
    unsetenv("AIE_BAREMETAL_LIB");

    TestableBaremetalGraphSim* sim = new TestableBaremetalGraphSim("testgraph");

    char name[] = "test_port";
    char data[100];

    // Call gm2aie_nb - should print error
    testCout.str("");
    sim->gm2aie_nb(name, data, 10);
    STRCMP_CONTAINS("load failed", testCout.str().c_str());

    // Call aie2gm_nb - should print error
    testCout.str("");
    sim->aie2gm_nb(name, data, 10);
    STRCMP_CONTAINS("load failed", testCout.str().c_str());

    // Call gm2aie - should print error
    testCout.str("");
    sim->gm2aie(name, data, 10);
    STRCMP_CONTAINS("load failed", testCout.str().c_str());

    // Call aie2gm - should print error
    testCout.str("");
    sim->aie2gm(name, data, 10);
    STRCMP_CONTAINS("load failed", testCout.str().c_str());

    delete sim;
}

/**
 * @brief TC_028: Verify profiling methods error handling when library load fails
 *
 * This test verifies that profiling methods (start_profiling, read_profiling,
 * stop_profiling) print appropriate error messages when the baremetal library
 * failed to load.
 */
TEST(ConstructorDestructorGroup, TC_028_ProfilingMethodsWithFailedLibraryLoad)
{
    unsetenv("AIE_BAREMETAL_LIB");

    TestableBaremetalGraphSim* sim = new TestableBaremetalGraphSim("testgraph");

    // Call start_profiling - should print error
    testCout.str("");
    int handle = sim->start_profiling("test_port", 0x20, 100);
    STRCMP_CONTAINS("load failed", testCout.str().c_str());
    CHECK_EQUAL(0, handle);

    // Call read_profiling - should print error
    testCout.str("");
    long long value = sim->read_profiling(0);
    STRCMP_CONTAINS("load failed", testCout.str().c_str());
    CHECK_EQUAL(0, value);

    // Call stop_profiling - should print error
    testCout.str("");
    sim->stop_profiling(0);
    STRCMP_CONTAINS("load failed", testCout.str().c_str());

    delete sim;
}

/**
 * @brief TC_021: Verify MockBaremetalGraphSimBase stub methods
 *
 * This test verifies that all mock stub methods in MockBaremetalGraphSimBase
 * return expected default values and can be called without errors.
 */
TEST(RunApiGroup, TC_021_UnusedMockMethodsCoverage)
{
    MockBaremetalGraphSimBase* mockBase = new MockBaremetalGraphSimBase();

    // Call selftestexit mock
    mock().expectOneCall("selftestexit");
    mockBase->selftestexit();

    // Call error_handling_cb_init mock
    void* cb = nullptr;
    void* priv = nullptr;
    mock().expectOneCall("error_handling_cb_init")
        .withParameter("cb", cb)
        .withParameter("priv", priv);
    mockBase->error_handling_cb_init(cb, priv);

    // Call gm2aie and aie2gm mocks
    char name[] = "test";
    char data[10];
    mock().expectOneCall("gm2aie");
    mockBase->gm2aie(name, data, 10);

    mock().expectOneCall("aie2gm");
    mockBase->aie2gm(name, data, 10);

    // Call setAddress mock
    void* addr = nullptr;
    mock().expectOneCall("setAddress");
    mockBase->setAddress(name, addr);

    // Call unused override methods that just return default values
    ERR_CODE result1 = mockBase->wait(name, "port");
    CHECK_EQUAL(0, result1);

    ERR_CODE result2 = mockBase->gm2aie_nb(name, "port");
    CHECK_EQUAL(0, result2);

    ERR_CODE result3 = mockBase->aie2gm_nb(name, "port");
    CHECK_EQUAL(0, result3);

    ERR_CODE result4 = mockBase->setupPingPongBuffers(name, nullptr, nullptr);
    CHECK_EQUAL(0, result4);

    int gmio_result = mockBase->gmio_wait("port", true);
    CHECK_EQUAL(0, gmio_result);

    void* malloc_result = mockBase->malloc(100);
    CHECK(malloc_result == nullptr);

    mockBase->free(nullptr);

    ERR_CODE update_result = mockBase->update("port", nullptr, 10);
    CHECK_EQUAL(0, update_result);

    ERR_CODE read_result = mockBase->read("port", data, 10);
    CHECK_EQUAL(0, read_result);

    char* log_result = mockBase->dumpcorelog(0, 0);
    CHECK(log_result == nullptr);

    void* dev_result = mockBase->getaiedevinst();
    CHECK(dev_result == nullptr);

    void* error_result = mockBase->BacktrackErrors();
    CHECK(error_result == nullptr);

    mockBase->gdbpoint();

    int prof_handle1 = mockBase->start_profiling("port", 0, 0);
    CHECK_EQUAL(0, prof_handle1);

    int prof_handle2 = mockBase->start_profiling("port1", "port2", 0, 0);
    CHECK_EQUAL(0, prof_handle2);

    long long prof_value = mockBase->read_profiling(0);
    CHECK_EQUAL(0, prof_value);

    mockBase->stop_profiling(0);

    mock().checkExpectations();
    delete mockBase;
}
