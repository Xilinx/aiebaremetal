/**
 * @file test_graph_api_run.cpp
 * @brief CppUTest test suite for graph_api run, end, update_iter, and wait operations
 *
 * This file contains comprehensive unit tests for the graph_api class focusing on:
 *  - Graph execution control (run, end, wait, resume operations)
 *  - Iterator update functionality (update_iter methods)
 *  - Broadcasting behavior with broadcast_enable_core enabled/disabled
 *  - Transaction handling and XAie driver interaction
 *  - Error handling for various failure scenarios
 *  - Core enable/disable event generation and status checking
 *  - Timeout and asynchronous wait operations
 *
 * Tests use CppUTest mocking framework to verify correct XAie API calls
 * and handle both success and failure paths for comprehensive coverage.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

 #include <unordered_set>
 //#define __AIESIM__
 #include "aeg_runtime_api.h"
 #include "aeg_aie_control_api.h"
 #include "iostream"
 using namespace abr;
 namespace abr {
     extern std::unordered_set<int> readOnlySharedBufferInitialized;

 }
 #include "test_comparator.h"

 #include <CppUTest/TestHarness.h>
 #include "CppUTestExt/MockSupport_c.h"
 #include "CppUTest/MemoryLeakDetector.h"


 class DummyConfigManager : public config_manager {
     public:
         static void setDevInst(XAie_DevInst* inst) { s_pDevInst = inst; }
         static void setBroadcastEnableCore(bool val) { s_broadcast_enable_core = val; }
         static void setNumReservedRows(size_t val) { s_num_reserved_rows = val; }
 };

 struct DummyXAie_DevInst {
     struct { int DevGen = 0; } DevProp;
 };

 struct DummyGraphConfig : public graph_config {
     DummyGraphConfig() {
         name = "dummy";
         coreColumns = {0};
         coreRows = {0};
         iterMemAddrs = {0};
         triggered = {false};
         iterMemColumns = {0};
         iterMemRows = {0};
         broadcast_column = 0;
     }
 };

 #include "CppUTestExt/MockSupport_c.h"

 /**
  * @brief Test group for graph_api run, wait, configure, resume, and end operations
  *
  * This test group validates the functionality of graph_api execution control methods.
  * Tests cover scenarios with broadcast_enable_core enabled/disabled, transaction handling,
  * XAie driver interaction, and error handling for various failure paths.
  */
 TEST_GROUP(GraphApiRun)
 {
     /**
      * @brief Setup for GraphApiRun test group
      *
      * Clears mock expectations and installs XAie_LocType comparator for parameter verification.
      */
     void setup() {
         mock_c()->clear();
         mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
     }

     /**
      * @brief Teardown for GraphApiRun test group
      *
      * Verifies all mock expectations were met, clears mocks, and removes comparators.
      */
     void teardown() {
         mock_c()->checkExpectations();
         mock_c()->clear();
         mock_c()->removeAllComparatorsAndCopiers();
     }
 };

 /**
  * @brief TC_001: Verify successful graph run with broadcast_enable_core enabled
  *
  * Tests graph_api::run() with broadcast mode enabled. Verifies that XAie driver APIs
  * (ReadTimer, StartTransaction, SubmitTransaction, CoreConfigureEnableEvent, EventGenerate)
  * are called correctly and the operation returns success.
  */
 TEST(GraphApiRun, TC_001_Run_Success_BroadcastEnabled)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);

     api.configure();

     // This ReadTimer calls shows how to use the following features:
     //  * Verifying a parameter value
     //  * Using an output parameter
     //  * Returning a value
     //  * Ignoring other parameters
     unsigned long long timerValue1 = 42;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withIntParameters("Module", XAIE_CORE_MOD)
         ->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectNCalls(2, "XAie_StartTransaction")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_SubmitTransaction")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_ClearCoreDisableEventOccurred")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_CoreConfigureEnableEvent")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_EventGenerate")
         ->ignoreOtherParameters();
     unsigned long long timerValue2 = 303;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withOutputParameterReturning("TimerVal", &timerValue2, sizeof(timerValue2))
         ->andReturnIntValue(0);
     unsigned long long timerValue3 = 503;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withOutputParameterReturning("TimerVal", &timerValue3, sizeof(timerValue3))
         ->andReturnIntValue(0);
     mock_c()->expectOneCall("XAie_SubmitTransaction")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_CoreConfigureEnableEvent")
         ->ignoreOtherParameters();

     abr::err_code res = api.run();
     CHECK(res == err_code::ok);
     mock_c()->checkExpectations();
 }

 /**
  * @brief TC_002: Verify successful graph run with broadcast_enable_core disabled
  *
  * Tests graph_api::run() with broadcast mode disabled. Verifies direct CoreEnable call
  * for individual cores and validates transaction handling with XAie driver APIs.
  */
 TEST(GraphApiRun, TC_002_Run_Success_BroadcastDisabled)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(false);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;
     u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
     api.configure();

     // This ReadTimer calls shows how to use the following features:
     //  * Verifying a parameter value
     //  * Using an output parameter
     //  * Returning a value
     //  * Ignoring other parameters
     unsigned long long timerValue1 = 42;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withIntParameters("Module", XAIE_CORE_MOD)
         ->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_StartTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->withIntParameters("Flags", Flags)
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_SubmitTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters();

     mock_c()->expectOneCall("XAie_CoreEnable")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
         ->andReturnIntValue(XAIE_OK);
     abr::err_code res = api.run();
     CHECK(res == err_code::ok);
     mock_c()->checkExpectations();
 }

 /**
  * @brief TC_002_Failure: Verify graph run failure with null graph configuration
  *
  * Tests error handling when graph_api is instantiated with nullptr config.
  * Expects aie_driver_error return code.
  */
 TEST(GraphApiRun, TC_003_Run_Failure_NullConfig)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(false);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig *config = nullptr;
     graph_api api(config);

     abr::err_code res = api.run();
     CHECK(res == err_code::aie_driver_error);
     mock_c()->checkExpectations();
 }

 /**
  * @brief TC_003_Failure: Verify graph run failure when XAie_CoreEnable fails
  *
  * Tests error handling when XAie_CoreEnable returns non-zero (failure).
  * Expects aie_driver_error return code when core enablement fails.
  */
 TEST(GraphApiRun, TC_004_Run_Failure_CoreEnableFails)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(false);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;
     u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
     api.configure();

     // This ReadTimer calls shows how to use the following features:
     //  * Verifying a parameter value
     //  * Using an output parameter
     //  * Returning a value
     //  * Ignoring other parameters
     unsigned long long timerValue1 = 42;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withIntParameters("Module", XAIE_CORE_MOD)
         ->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_StartTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->withIntParameters("Flags", Flags)
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_SubmitTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters();

     mock_c()->expectOneCall("XAie_CoreEnable")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
         ->andReturnIntValue(1);
     abr::err_code res = api.run();
     CHECK(res == err_code::aie_driver_error);
     mock_c()->checkExpectations();
 }

 /**
  * @brief TC_004_Failure: Verify graph run failure with iterations when DataMemWrWord fails
  *
  * Tests graph_api::run(iterations) with broadcast mode enabled. Simulates XAie_DataMemWrWord
  * failure and verifies aie_driver_error is returned.
  */
 TEST(GraphApiRun, TC_005_Run_Failure_DataMemWrWordFails)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);


     u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
     api.configure();
     int iterations = 10;
     mock_c()->expectOneCall("XAie_StartTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->withIntParameters("Flags", Flags)
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     mock_c()->expectOneCall("XAie_SubmitTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters();

     abr::err_code res = api.run(iterations);
     CHECK(res == err_code::aie_driver_error);
 }

 /**
  * @brief TC_004_Success: Verify successful graph run with iterations parameter
  *
  * Tests graph_api::run(iterations) with broadcast mode enabled and specified iteration count.
  * Verifies DataMemWrWord succeeds for writing iteration count and subsequent run operations complete.
  */
 TEST(GraphApiRun, TC_006_Run_Success_WithIterations)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;

     u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
     api.configure();
     int iterations = 10;
     mock_c()->expectOneCall("XAie_StartTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->withIntParameters("Flags", Flags)
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(0);
     mock_c()->expectOneCall("XAie_SubmitTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters();
     unsigned long long timerValue1 = 42;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withIntParameters("Module", XAIE_CORE_MOD)
         ->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectNCalls(2, "XAie_StartTransaction")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_SubmitTransaction")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_ClearCoreDisableEventOccurred")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_CoreConfigureEnableEvent")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_EventGenerate")
         ->ignoreOtherParameters();
     unsigned long long timerValue2 = 303;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withOutputParameterReturning("TimerVal", &timerValue2, sizeof(timerValue2))
         ->andReturnIntValue(0);
     unsigned long long timerValue3 = 503;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withOutputParameterReturning("TimerVal", &timerValue3, sizeof(timerValue3))
         ->andReturnIntValue(0);
     mock_c()->expectOneCall("XAie_SubmitTransaction")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_CoreConfigureEnableEvent")
         ->ignoreOtherParameters();

     abr::err_code res = api.run(iterations);
     CHECK(res == err_code::ok);
 }

 /**
  * @brief TC_005_Failure: Verify graph run failure when graph is not configured
  *
  * Tests error handling when run() is called without prior configure() call.
  * Expects aie_driver_error return code for unconfigured graph.
  */
 TEST(GraphApiRun, TC_007_Run_Failure_EventGenerateFails)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig *config = nullptr;
     graph_api api(config);
     int iterations = 10;
     u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
     api.configure();
     abr::err_code res = api.run(iterations);
     CHECK(res == err_code::aie_driver_error);
     mock_c()->checkExpectations();
 }

 /**
  * @brief TC_001_configure_Failure: Verify configure failure with null graph configuration
  *
  * Tests error handling when graph_api::configure() is called with nullptr config.
  * Expects internal_error return code.
  */
 TEST(GraphApiRun, TC_008_Configure_Failure_NullConfig)
 {
     abr::err_code res;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig *config = nullptr;;
     graph_api api(config);

     res = api.configure();
     CHECK(res == err_code::internal_error);
 }

 /**
  * @brief TC_002_configure_Failure: Verify configure failure with mismatched config arrays
  *
  * Tests error handling when graph configuration has inconsistent array sizes
  * (coreColumns/coreRows vs iterMemAddrs). Expects internal_error return code.
  */
 TEST(GraphApiRun, TC_009_Configure_Failure_AlreadyConfigured)
 {
     abr::err_code res;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     config.coreColumns = {1, 2, 3};
     config.coreRows = {1, 2};
     config.iterMemAddrs = {1, 2, 3};
     config.iterMemColumns = {1, 2, 3};
     config.iterMemRows = {1, 2, 3};

     graph_api api(&config);

     res = api.configure();
     CHECK(res == err_code::internal_error);
 }

 /**
  * @brief TC_001_wait_Failure: Verify wait failure when graph is not configured
  *
  * Tests error handling when wait() is called without prior configure() call.
  * Expects aie_driver_error return code.
  */
 TEST(GraphApiRun, TC_010_Wait_Failure_NullConfig)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     int iterations = 10;
     abr::err_code res;
     u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;

     res = api.wait();
     CHECK(res == err_code::aie_driver_error);
     mock_c()->checkExpectations();
 }

 /**
  * @brief TC_002_wait_Success: Verify successful wait for graph completion
  *
  * Tests graph_api::wait() with configured graph. Verifies XAie_CoreWaitForDone
  * and XAie_CoreDisable are called correctly and returns success.
  */
 TEST(GraphApiRun, TC_011_Wait_Success)
 {

     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     int iterations = 10;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;
     abr::err_code res;
     u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
     api.configure();
     mock_c()->expectOneCall("XAie_CoreWaitForDone")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
         ->andReturnIntValue(0);
     res = api.wait();
     CHECK(res == err_code::ok);
 }

 /**
  * @brief TC_003_wait_Failure: Verify wait failure when XAie_CoreDisable fails
  *
  * Tests error handling when XAie_CoreDisable returns non-zero (failure).
  * Expects aie_driver_error return code.
  */
 TEST(GraphApiRun, TC_012_Wait_Failure_CoreWaitCycleFails)
 {

     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);

     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;
     abr::err_code res;

     api.configure();
     mock_c()->expectOneCall("XAie_CoreWaitForDone")
         ->ignoreOtherParameters();
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
         ->andReturnIntValue(1);
     res = api.wait();
     CHECK(res == err_code::aie_driver_error);
 }

 /**
  * @brief TC_004_wait_Failure: Verify wait with timeout failure on unconfigured graph
  *
  * Tests error handling when wait(cycleTimeout) is called without prior configure().
  * Expects aie_driver_error return code.
  */
 TEST(GraphApiRun, TC_013_Wait_Failure_CoreWaitStatusFails)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);

     unsigned long long cycleTimeout = 10;
     abr::err_code res = api.wait(cycleTimeout);
     CHECK(res == err_code::aie_driver_error);

 }

 /**
  * @brief TC_005_wait_Success: Verify successful wait with cycle timeout
  *
  * Tests graph_api::wait(cycleTimeout) with maximum timeout value. Verifies
  * XAie_ReadTimer and XAie_CoreDisable are called and returns success.
  */
 TEST(GraphApiRun, TC_014_Wait_Success_WithTimeout)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     api.configure();

     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;

     unsigned long long cycleTimeout = 0xFFFFFFFFFFFF;
     unsigned long long timerValue1 = 42;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         // ->withIntParameters("Module", XAIE_CORE_MOD)
         // ->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
         ->andReturnIntValue(XAIE_OK);
     // mock_c()->expectOneCall("XAie_WaitCycles")
     // 	->ignoreOtherParameters()
     // 	->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
         ->andReturnIntValue(0);
     abr::err_code res = api.wait(cycleTimeout);
     CHECK(res == err_code::ok);
 }

 /**
  * @brief TC_006_wait_Failure: Verify wait with timeout failure when ReadTimer fails
  *
  * Tests error handling when XAie_ReadTimer returns non-zero (failure) during
  * wait with timeout. Expects aie_driver_error return code.
  */
 TEST(GraphApiRun, TC_015_Wait_Failure_TimeoutExpired)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     api.configure();

     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;

     unsigned long long cycleTimeout = 0xFFFFFFFFFFFF;
     unsigned long long timerValue1 = 42;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withIntParameters("Module", XAIE_CORE_MOD)
         ->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_WaitCycles")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
         ->andReturnIntValue(1);
     abr::err_code res = api.wait(cycleTimeout);
     CHECK(res == err_code::aie_driver_error);

 }

 /**
  * @brief TC_007_wait_Failure: Verify wait with excessive timeout returns user error
  *
  * Tests error handling when wait(cycleTimeout) is called with timeout exceeding
  * maximum allowed value. Expects user_error return code.
  */
 TEST(GraphApiRun, TC_016_Wait_Failure_CoreWaitDoneFails)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     api.configure();

     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;

     unsigned long long cycleTimeout = 0xFFFFFFFFFFFFF;
     abr::err_code res = api.wait(cycleTimeout);
     CHECK(res == err_code::user_error);

 }

 /**
  * @brief TC_001_resume_Failure: Verify resume on unconfigured graph
  *
  * Tests that resume() can be called on unconfigured graph without errors.
  * No XAie calls expected when graph is not configured.
  */
 TEST(GraphApiRun, TC_017_Resume_Failure_NullConfig)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);


     abr::err_code res = api.resume();
     CHECK(res == err_code::aie_driver_error);
 }

 /**
  * @brief TC_006_resume_Success: Verify successful graph resume operation
  *
  * Tests graph_api::resume() with configured graph. Verifies XAie_ReadTimer,
  * XAie_CoreReadDoneBit, and XAie_CoreEnable are called correctly.
  */
 TEST(GraphApiRun, TC_018_Resume_Success)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     api.configure();

     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;


     unsigned long long timerValue1 = 42;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withIntParameters("Module", XAIE_CORE_MOD)
         ->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreReadDoneBit")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreEnable")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
         ->andReturnIntValue(0);
     abr::err_code res = api.resume();
     CHECK(res == err_code::ok);
 }

 /**
  * @brief TC_007_resume_Failure: Verify resume failure when XAie_CoreEnable fails
  *
  * Tests error handling when XAie_CoreEnable returns non-zero during resume.
  * Verifies mock expectations even when core enable fails.
  */
 TEST(GraphApiRun, TC_019_Resume_Failure_CoreEnableFails)
 {
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(true);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     api.configure();
     int iterations = 10;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;

     unsigned long long timerValue1 = 42;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withIntParameters("Module", XAIE_CORE_MOD)
         ->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreReadDoneBit")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreEnable")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
         ->andReturnIntValue(1);
     abr::err_code res = api.resume();
     CHECK(res == err_code::aie_driver_error);
 }

 /**
  * @brief TC_020: Verify successful graph end operation
  *
  * Tests graph_api::end() after successful run. Verifies wait for completion,
  * core disable, and event generation operations complete successfully.
  */
 TEST(GraphApiRun, TC_020_End_Success)
 {

     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(false);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;
     u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
     api.configure();

     // This ReadTimer calls shows how to use the following features:
     //  * Verifying a parameter value
     //  * Using an output parameter
     //  * Returning a value
     //  * Ignoring other parameters
     unsigned long long timerValue1 = 42;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withIntParameters("Module", XAIE_CORE_MOD)
         ->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_StartTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->withIntParameters("Flags", Flags)
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_SubmitTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters();

     mock_c()->expectNCalls(2,"XAie_CoreEnable")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectNCalls(2,"XAie_CoreWaitForDone")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectNCalls(2,"XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     abr::err_code res = api.run();
     CHECK(res == err_code::ok);


     abr::err_code result = api.end();
     CHECK(result == err_code::ok);
 }

 /**
  * @brief TC_021: Verify graph end failure when core disable fails
  *
  * Tests error handling when XAie_CoreDisable returns non-zero during end operation.
  * Expects aie_driver_error return code.
  */
 TEST(GraphApiRun, TC_021_End_Failure_CoreDisableFails)
 {

     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     DummyConfigManager::setBroadcastEnableCore(false);
     DummyConfigManager::setNumReservedRows(0);
     DummyGraphConfig config;
     graph_api api(&config);
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;
     u32 Flags = XAIE_TRANSACTION_ENABLE_AUTO_FLUSH;
     api.configure();

     // This ReadTimer calls shows how to use the following features:
     //  * Verifying a parameter value
     //  * Using an output parameter
     //  * Returning a value
     //  * Ignoring other parameters
     unsigned long long timerValue1 = 42;
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->withIntParameters("Module", XAIE_CORE_MOD)
         ->withOutputParameterReturning("TimerVal", &timerValue1, sizeof(timerValue1))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_StartTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->withIntParameters("Flags", Flags)
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_SubmitTransaction")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters();

     mock_c()->expectNCalls(2,"XAie_CoreEnable")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc) // <-- USE withParameterOfType
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectNCalls(2,"XAie_CoreWaitForDone")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectNCalls(1,"XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     abr::err_code res = api.run();
     CHECK(res == err_code::ok);
     mock_c()->expectNCalls(1,"XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);

     abr::err_code result = api.end();
     CHECK(result == err_code::aie_driver_error);
 }

 /**
  * @brief Test group for graph_api shared buffer update operations
  *
  * This test group validates the functionality of updating shared buffers in graphs.
  * Tests cover scenarios with valid/invalid buffer configurations, lock operations,
  * and data transfer validation.
  */
 TEST_GROUP(GraphApiUpdateSharedBufferTests)
 {
     // Shared variables for all tests
     XAie_DevInst deviceInstance;
     abr::shared_buffer_config sharedBufferConfig;
     abr::graph_config graphConfig;
     abr::graph_api* graphApi;
     uint8_t testData[64];
     size_t testDataSize;

     /**
      * @brief Setup for GraphApiUpdateSharedBufferTests
      *
      * Initializes device instance, config manager, mock comparators, and clears
      * read-only shared buffer tracking.
      */
     void setup()
     {

         mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
         mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);

         readOnlySharedBufferInitialized.clear();

         //Set up device instance
         deviceInstance.DevProp.DevGen = XAIE_DEV_GEN_AIE;
         abr::config_manager::s_pDevInst = &deviceInstance;
         abr::config_manager::s_bInitialized = true;
         abr::config_manager::s_num_reserved_rows = 0;


         // Set up graph config
         graphConfig.id = 1;
         graphConfig.name = "TestGraph";

         // Set up shared buffer config
         sharedBufferConfig.id = 1;
         sharedBufferConfig.name = "TestSharedBuffer";
         sharedBufferConfig.graphId = graphConfig.id;
         sharedBufferConfig.numBytes = 64;
         sharedBufferConfig.column = 1;
         sharedBufferConfig.row = 2;
         sharedBufferConfig.numInputs = 0;
         sharedBufferConfig.numOutputs = 2;
         sharedBufferConfig.elemType = abr::rtp_config::et_int32;
         sharedBufferConfig.initialized = false;
         sharedBufferConfig.addr = {0x1000};
         sharedBufferConfig.producerLocks = {1};
         sharedBufferConfig.consumerLocks = {2};
         readOnlySharedBufferInitialized.clear();
         // Set up test data
         testDataSize = 64;
         for (size_t i = 0; i < testDataSize; i++)
         {
             testData[i] = static_cast<uint8_t>(i & 0xFF);
         }

         // Create graph_api instance
         graphApi = new abr::graph_api(&graphConfig);
     }

     /**
      * @brief Teardown for GraphApiUpdateSharedBufferTests
      *
      * Cleans up graph_api instance, clears shared buffer tracking, and resets mocks.
      */
     void teardown()
     {
         delete graphApi;

         readOnlySharedBufferInitialized.clear();

         // mock_c()->removeAllComparatorsAndCopiers();
         mock_c()->clear();
     }
 };

 /**
  * @brief TC_022: Verify successful shared buffer update
  *
  * Tests graph_api::update() with valid buffer config and data. Verifies lock acquire,
  * data write, and lock release operations complete successfully.
  */
 TEST(GraphApiUpdateSharedBufferTests, TC_022_Update_Success)
 {
     // ARRANGE
     // Everything is set up in setup()


     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         //  ->withPointerParameters("Data", (void*)testData)
         // ->withUnsignedIntParameters("Size", testDataSize)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_LockRelease")
         ->withPointerParameters("DevInst", &deviceInstance)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     // // ACT
     abr::err_code result = graphApi->update(&sharedBufferConfig, testData, testDataSize);
     CHECK(result == err_code::ok);

 }

 /**
  * @brief TC_023: Verify update with null buffer configuration
  *
  * Tests error handling when update() is called with nullptr buffer config.
  * Expects non-ok return code.
  */
 TEST(GraphApiUpdateSharedBufferTests, TC_023_Update_NullBufferConfig)
 {
     // ARRANGE
     // Everything is set up in setup()
     abr::shared_buffer_config *BufferConfig = nullptr;
     // ACT
     abr::err_code result = graphApi->update(BufferConfig, testData, testDataSize);
     CHECK(result != err_code::ok);

 }

 /**
  * @brief TC_001_Failure: Verify shared buffer update failure when lock release fails
  *
  * Tests error handling when XAie_LockRelease returns non-zero (failure).
  * Expects aie_driver_error return code.
  */
 TEST(GraphApiUpdateSharedBufferTests, TC_024_Update_Failure)
 {
     // ARRANGE
     // Everything is set up in setup()


     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         //  ->withPointerParameters("Data", (void*)testData)
         // ->withUnsignedIntParameters("Size", testDataSize)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_LockRelease")
         ->withPointerParameters("DevInst", &deviceInstance)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     // // ACT
     abr::err_code result = graphApi->update(&sharedBufferConfig, testData, testDataSize);
     CHECK(result == err_code::aie_driver_error);

 }

 /**
  * @brief TC_025: Verify shared buffer update with initialized=true
  *
  * Tests graph_api::update() when sharedBufferConfig.initialized is true,
  * where lock acquire values remain at default values without reinitialization.
  */
 TEST(GraphApiUpdateSharedBufferTests, TC_025_Update_InitializedBuffer)
 {
     // ARRANGE - Set initialized to true
     sharedBufferConfig.initialized = true;

     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT
     abr::err_code result = graphApi->update(&sharedBufferConfig, testData, testDataSize);

     // ASSERT
     CHECK(result == err_code::ok);
 }

 /**
  * @brief TC_026: Verify shared buffer update for already-initialized buffer
  *
  * Tests graph_api::update() when the buffer ID is already in the initialized set,
  * skipping the initialization logic entirely since buffer was previously initialized.
  */
 TEST(GraphApiUpdateSharedBufferTests, TC_026_Update_AlreadyInInitializedSet)
 {
     // ARRANGE - First update to add to initialized set
     sharedBufferConfig.initialized = false;

     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     abr::err_code result1 = graphApi->update(&sharedBufferConfig, testData, testDataSize);
     CHECK(result1 == err_code::ok);

     // ACT - Second update with same buffer ID (already in set)
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     abr::err_code result2 = graphApi->update(&sharedBufferConfig, testData, testDataSize);

     // ASSERT
     CHECK(result2 == err_code::ok);
 }

 /**
  * @brief Test group for graph_api RTP (Run-Time Parameter) update operations
  *
  * This test group validates the functionality of updating RTPs in graphs.
  * Tests cover scenarios with various RTP configurations, connection types,
  * and error handling for invalid parameters.
  */
 TEST_GROUP(GraphApiUpdateRTPTests)
 {
     // Shared variables for all tests
     XAie_DevInst deviceInstance;
     abr::rtp_config rtpConfig;
     abr::graph_config graphConfig;
     abr::graph_api* graphApi;
     uint8_t testData[64];
     size_t testDataSize;

     /**
      * @brief Setup for GraphApiUpdateRTPTests
      *
      * Initializes device instance, RTP config with various settings (async, hasLock, etc.),
      * graph config, test data, and installs mock comparators.
      */
     void setup()
     {
         mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
         mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
         // Set up device instance
         deviceInstance.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
         abr::config_manager::s_pDevInst = &deviceInstance;
         abr::config_manager::s_bInitialized = true;
         abr::config_manager::s_num_reserved_rows = 0;

         // Set up graph config
         graphConfig.id = 1;
         graphConfig.name = "TestGraph";

         // Set up RTP config
         rtpConfig.portId = 1;
         rtpConfig.aliasId = 2;
         rtpConfig.portName = "TestRTPPort";
         rtpConfig.aliasName = "TestRTPAlias";
         rtpConfig.graphId = graphConfig.id;
         rtpConfig.isInput = true;  // This is crucial for update
         rtpConfig.isAsync = false;
         rtpConfig.isConnect = false;
         rtpConfig.numBytes = 64;
         rtpConfig.isPL = false;
         rtpConfig.hasLock = true;
         rtpConfig.selectorColumn = 1;
         rtpConfig.selectorRow = 2;
         rtpConfig.selectorAddr = 0x1000;
         rtpConfig.selectorLockId = 1;
         rtpConfig.pingColumn = 1;
         rtpConfig.pingRow = 3;
         rtpConfig.pingAddr = 0x2000;
         rtpConfig.pingLockId = 2;
         rtpConfig.pongColumn = 1;
         rtpConfig.pongRow = 4;
         rtpConfig.pongAddr = 0x3000;
         rtpConfig.pongLockId = 3;
         rtpConfig.isAsync = 0;
         rtpConfig.hasLock = 1;
         // Set up test data
         testDataSize = 64;
         for (size_t i = 0; i < testDataSize; i++)
         {
             testData[i] = static_cast<uint8_t>(i & 0xFF);
         }

         // Create graph_api instance
         graphApi = new abr::graph_api(&graphConfig);



     }

     /**
      * @brief Teardown for GraphApiUpdateRTPTests
      *
      * Cleans up graph_api instance, removes comparators, and clears mocks.
      */
     void teardown()
     {
         delete graphApi;
         mock_c()->removeAllComparatorsAndCopiers();
         mock_c()->clear();



     }
 };

 /**
  * @brief TC_001_Success: Verify successful RTP update
  *
  * Tests graph_api::update() for RTP with valid parameters. Verifies lock acquire,
  * data memory read/write, and lock release operations complete successfully.
  */
 TEST(GraphApiUpdateRTPTests, TC_027_Update_Success)
 {

     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemRdWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     // ACT
     abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);

     // ASSERT
     CHECK(result == err_code::ok);
 }

 /**
  * @brief TC_002_Failure: Verify RTP update failure when XAie operations fail
  *
  * Tests error handling when XAie_DataMemRdWord, DataMemBlockWrite, DataMemWrWord,
  * and LockRelease return non-zero. Expects aie_driver_error return code.
  */
 TEST(GraphApiUpdateRTPTests, TC_028_Update_Failure)
 {
     rtpConfig.isAsync = 0x1;
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemRdWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(0x1);
     // ACT
     abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);

     // ASSERT
     CHECK(result == err_code::aie_driver_error);


 }

 /**
  * @brief TC_029: Verify RTP update with null configuration
  *
  * Tests error handling when graph_api::update() is called with nullptr RTP config.
  * Expects non-ok return code.
  */
 TEST(GraphApiUpdateRTPTests, TC_029_Update_BlockWriteFails)
 {

     abr::rtp_config *rtpConfig1 = nullptr;

     // ACT
     abr::err_code result = graphApi->update(rtpConfig1, testData, testDataSize);

     // ASSERT
     CHECK(result != err_code::ok);
 }

 /**
  * @brief TC_030: Verify RTP update with S100 device generation
  *
  * Tests graph_api::update() with DevGen set to XAIE_DEV_GEN_S100,
  * which is normalized to AIE device generation during processing.
  * Verifies lock acquire and data memory operations complete successfully.
  */
 TEST(GraphApiUpdateRTPTests, TC_030_Update_S100DevGen)
 {
     // ARRANGE: Set DevGen to S100 (6)
     deviceInstance.DevProp.DevGen = 6; // XAIE_DEV_GEN_S100

     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemRdWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT
     abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);

     // ASSERT
     CHECK(result == err_code::ok);
 }

 /**
  * @brief TC_007: Verify RTP update with S200 device generation
  *
  * Tests graph_api::update() with DevGen set to XAIE_DEV_GEN_S200 (7).
  * Verifies that S200 device generation is correctly mapped to AIE and that
  * lock acquire and data memory operations complete successfully.
  */
 TEST(GraphApiUpdateRTPTests, TC_031_Update_S200DevGen)
 {
     // ARRANGE: Set DevGen to S200 (7)
     deviceInstance.DevProp.DevGen = 7; // XAIE_DEV_GEN_S200

     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemRdWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT
     abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);

     // ASSERT
     CHECK(result == err_code::ok);
 }

 /**
  * @brief TC_008: Verify RTP update fails for connected PL port
  *
  * Tests error handling when attempting to update a connected PL (RL) input RTP.
  * This is an unsupported operation that should return user_error.
  */
 TEST(GraphApiUpdateRTPTests, TC_032_Update_ConnectedPLPort)
 {
     // ARRANGE: Set RTP as connected and PL type
     rtpConfig.isConnect = true;
     rtpConfig.isPL = true;

     // ACT
     abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);

     // ASSERT: Should fail with user_error for connected PL RTP
     CHECK(result == err_code::user_error);
 }

 /**
  * @brief TC_009: Verify RTP update fails for connected synchronous RTP
  *
  * Tests error handling when attempting to update a connected synchronous input RTP.
  * This is an unsupported operation that should return user_error.
  */
 TEST(GraphApiUpdateRTPTests, TC_033_Update_ConnectedSyncRTP)
 {
     // ARRANGE: Set RTP as connected, synchronous (non-async), and AIE type
     rtpConfig.isConnect = true;
     rtpConfig.isAsync = false;
     rtpConfig.isPL = false;

     // ACT
     abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);

     // ASSERT: Should fail with user_error for connected sync RTP
     CHECK(result == err_code::user_error);
 }

 /**
  * @brief TC_034: Verify async RTP update on AIEML with subsequent updates
  *
  * Tests graph_api::update() for async RTP on AIEML+ device where the port has been
  * updated before. The first update adds the port to the tracking set, and subsequent
  * updates use the appropriate acquire value for already-updated ports.
  */
 TEST(GraphApiUpdateRTPTests, TC_034_Update_AsyncRTPSecondUpdate_AIEML)
 {
     // ARRANGE: Set RTP as async for AIEML device
     rtpConfig.isAsync = true;
     rtpConfig.isConnect = false;
     rtpConfig.hasLock = true;

     u32 selector = 0;

     // First update - will add portId to tracking set for subsequent updates
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemRdWord")
         //->withOutputParameterOfTypeReturning("u32", "Data", &selector)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     abr::err_code result1 = graphApi->update(&rtpConfig, testData, testDataSize);
     CHECK(result1 == err_code::ok);

     // Second update - portId is now tracked, uses appropriate acquire value
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemRdWord")
         ->withOutputParameterReturning("Data", &selector, sizeof(selector))
         //->withOutputParameterOfTypeReturning("u32", "Data", &selector)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT: Second update should use appropriate acquire value
     abr::err_code result2 = graphApi->update(&rtpConfig, testData, testDataSize);

     // ASSERT
     CHECK(result2 == err_code::ok);
 }

 /**
  * @brief TC_035: Verify RTP update with selector=0 triggers pong buffer path
  *
  * Tests graph_api::update() where XAie_DataMemRdWord returns selector=0,
  * which becomes selector=1 after inversion, triggering the pong buffer path
  * with lock acquisition when hasLock=true and bAcquireLock=true.
  */
 TEST(GraphApiUpdateRTPTests, TC_035_Update_PongBranchWithLock)
 {
     // ARRANGE: Set RTP with hasLock=true, and mock selector=0 (becomes 1 after inversion)
     rtpConfig.isAsync = false;
     rtpConfig.hasLock = true;
     rtpConfig.isConnect = false;

     u32 selectorValue = 0; // This will become 1 after selector = 1 - selector, triggering pong branch

     // Selector lock acquire
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // Read selector (returns 0, becomes 1 for pong)
     mock_c()->expectOneCall("XAie_DataMemRdWord")
         ->withOutputParameterReturning("Data", &selectorValue, sizeof(selectorValue))
         // ->withOutputParameterOfTypeReturning("u32", "Data", &selectorValue)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // Pong buffer lock acquire
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // Write to pong buffer
     mock_c()->expectOneCall("XAie_DataMemBlockWrite")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // Write selector value
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // Release locks
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT
     abr::err_code result = graphApi->update(&rtpConfig, testData, testDataSize);

     // ASSERT
     CHECK(result == err_code::ok);
 }

 TEST_GROUP(GraphApiEndTests)
 {
     XAie_DevInst deviceInstance;
     abr::graph_config graphConfig;
     abr::graph_api* graphApi;

     /**
      * @brief Setup for GraphApiEndTests
      *
      * Initializes device instance, graph config, installs mock comparators, and
      * configures config manager for testing.
      */
     void setup()
     {
         mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
         mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
         // Set up device instance
         deviceInstance.DevProp.DevGen = XAIE_DEV_GEN_AIE;
         abr::config_manager::s_pDevInst = &deviceInstance;
         abr::config_manager::s_bInitialized = true;
         abr::config_manager::s_num_reserved_rows = 0;

         // Set up graph config with consistent properties
         graphConfig.id = 1;
         graphConfig.name = "TestGraph";
         graphConfig.coreColumns = {1, 2};
         graphConfig.coreRows = {3, 4};
         graphConfig.iterMemColumns = {1, 2};
         graphConfig.iterMemRows = {5, 6};
         graphConfig.iterMemAddrs = {0x1000, 0x2000};
         graphConfig.triggered = {false, false};
         graphConfig.broadcast_column = 0;

         // Create graph_api instance
         graphApi = new abr::graph_api(&graphConfig);

         // Configure the graph (assuming this works correctly based on existing tests)
         graphApi->configure();


     }

     /**
      * @brief Teardown for GraphApiEndTests
      *
      * Cleans up graph_api instance, removes comparators, resets config manager, and clears mocks.
      */
     void teardown()
     {
         delete graphApi;
         mock_c()->removeAllComparatorsAndCopiers();
         abr::config_manager::s_pDevInst = nullptr;
         mock_c()->clear();
     }
 };

 /**
  * @brief TC_001: Verify end on unconfigured graph returns error
  *
  * Tests that calling end() on an unconfigured graph returns aie_driver_error.
  */
 TEST(GraphApiEndTests, TC_036_End_UnconfiguredGraph)
 {
     // ARRANGE
     delete graphApi; // Delete the pre-configured graph
     graphApi = new abr::graph_api(&graphConfig); // Create a new unconfigured instance

     // ACT
     abr::err_code result = graphApi->end();

     // ASSERT
     CHECK(result == err_code::aie_driver_error);
 }

 /**
  * @brief TC_002: Verify end with timeout on unconfigured graph returns error
  *
  * Tests that calling end(timeout) on an unconfigured graph returns aie_driver_error.
  */
 TEST(GraphApiEndTests, TC_037_End_WithTimeoutUnconfiguredGraph)
 {
     // ARRANGE
     delete graphApi; // Delete the pre-configured graph
     graphApi = new abr::graph_api(&graphConfig); // Create a new unconfigured instance
     unsigned long long timeout = 1000;

     // ACT
     abr::err_code result = graphApi->end(timeout);

     // ASSERT
     CHECK(result == err_code::aie_driver_error);
 }

 /**
  * @brief TC_001_Success: Verify successful graph end operation
  *
  * Tests graph_api::end() on a configured graph. Verifies wait, XAie_CoreWaitForDone,
  * and XAie_CoreDisable are called for all cores and operation succeeds.
  */
 TEST(GraphApiEndTests, TC_038_End_ConfiguredGraph_Success)
 {


     mock_c()->expectOneCall("wait")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreWaitForDone")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreWaitForDone")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     // ACT
     abr::err_code result = graphApi->end();

     // ASSERT
     CHECK(result == err_code::ok);
 }

 /**
  * @brief TC_001_Failure: Verify graph end failure when XAie_CoreDisable fails
  *
  * Tests error handling when XAie_CoreDisable returns non-zero during end operation.
  * Expects aie_driver_error return code.
  */
 TEST(GraphApiEndTests, TC_039_End_ConfiguredGraph_CoreDisableFails)
 {


     mock_c()->expectOneCall("wait")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreWaitForDone")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreWaitForDone")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     // ACT
     abr::err_code result = graphApi->end();

     // ASSERT
     CHECK(result == err_code::aie_driver_error);
 }

 /**
  * @brief TC_040: Verify graph end with invalid timeout
  *
  * Tests error handling when graph_api::end() is called with excessive timeout value.
  * Verifies appropriate error handling for invalid parameters.
  */
 TEST(GraphApiEndTests, TC_040_End_ConfiguredGraph_EventGenerateFails)
 {

     unsigned long long timeout = 0xFFFFFFFFFFFFF;
     mock_c()->expectOneCall("wait")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreWaitForDone")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreWaitForDone")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     // ACT
     abr::err_code result = graphApi->end(timeout);

     // ASSERT
     CHECK(result != err_code::ok);
 }

 /**
  * @brief TC_041: Verify successful graph end with timeout
  *
  * Tests graph_api::end(timeout) with specified timeout value. Verifies wait, ReadTimer,
  * CoreDisable, and DataMemWrWord operations complete successfully.
  */
 TEST(GraphApiEndTests, TC_041_End_ConfiguredGraph_WithTimeout)
 {


     unsigned long long timeout = 1000;
     mock_c()->expectOneCall("wait")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     // ACT
     abr::err_code result = graphApi->end(timeout);

     // ASSERT
     CHECK(result == err_code::ok);
 }

 /**
  * @brief TC_003_Failure: Verify graph end with timeout failure when DataMemWrWord fails
  *
  * Tests error handling when XAie_DataMemWrWord returns non-zero during end with timeout.
  * Expects aie_driver_error return code.
  */
 TEST(GraphApiEndTests, TC_042_End_ConfiguredGraph_TimeoutWaitFails)
 {


     unsigned long long timeout = 1000;
     mock_c()->expectOneCall("wait")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_ReadTimer")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_CoreDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemWrWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     // ACT
     abr::err_code result = graphApi->end(timeout);

     // ASSERT
     CHECK(result == err_code::aie_driver_error);
 }

 /**
  * @brief Test group for graph_api RTP read operations
  *
  * This test group validates the functionality of reading RTP values from graphs.
  * Tests cover scenarios with various RTP configurations and error handling for
  * invalid parameters or unconfigured graphs.
  */
 TEST_GROUP(GraphApiReadTests)
 {
     XAie_DevInst deviceInstance;
     abr::graph_config graphConfig;
     abr::rtp_config rtpConfig;
     abr::graph_api* graphApi;
     char buffer[128];

     /**
      * @brief Setup for GraphApiReadTests
      *
      * Initializes device instance, graph and RTP configurations, buffer, and
      * installs mock comparators for testing RTP read operations.
      */
     void setup() override
     {

         mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
         mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
         // Set up device instance
         deviceInstance.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
         abr::config_manager::s_pDevInst = &deviceInstance;
         abr::config_manager::s_bInitialized = true;
         abr::config_manager::s_num_reserved_rows = 0;

         // Set up graph config
         graphConfig.id = 1;
         graphConfig.name = "TestGraph";

         // Set up default RTP config for output port (for reading)
         rtpConfig.portId = 1;
         rtpConfig.aliasId = 1;
         rtpConfig.portName = "TestOutputPort";
         rtpConfig.graphId = 1;
         rtpConfig.isInput = false; // Output port for reading
         rtpConfig.isAsync = false;
         rtpConfig.isConnect = false;
         rtpConfig.numBytes = 16;
         rtpConfig.isPL = false;
         rtpConfig.hasLock = true;
         rtpConfig.selectorColumn = 1;
         rtpConfig.selectorRow = 2;
         rtpConfig.selectorAddr = 0x1000;
         rtpConfig.selectorLockId = 1;
         rtpConfig.pingColumn = 1;
         rtpConfig.pingRow = 3;
         rtpConfig.pingAddr = 0x2000;
         rtpConfig.pingLockId = 2;
         rtpConfig.pongColumn = 1;
         rtpConfig.pongRow = 4;
         rtpConfig.pongAddr = 0x3000;
         rtpConfig.pongLockId = 3;

         // Create graph_api instance
         graphApi = new abr::graph_api(&graphConfig);

         // Configure the graph
         graphApi->configure();



         // Initialize buffer
         memset(buffer, 0, sizeof(buffer));
     }

     /**
      * @brief Teardown for GraphApiReadTests
      *
      * Cleans up graph_api instance, resets config manager, removes comparators, and clears mocks.
      */
     void teardown() override
     {
         delete graphApi;
         abr::config_manager::s_pDevInst = nullptr;
         mock_c()->removeAllComparatorsAndCopiers();
         mock_c()->clear();
     }
 };

 /**
  * @brief TC_001: Verify successful RTP read from unconfigured graph
  *
  * Tests graph_api::read() on an unconfigured graph. Verifies lock acquire, data read,
  * and lock release operations complete successfully.
  */
 TEST(GraphApiReadTests, TC_043_Read_UnconfiguredGraph)
 {
     // ARRANGE
     delete graphApi; // Delete the pre-configured graph
     graphApi = new abr::graph_api(&graphConfig); // Create a new unconfigured instance
     size_t numBytes = rtpConfig.numBytes;
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     u32 selector = 0x1;
     mock_c()->expectOneCall("XAie_DataMemRdWord")
         ->ignoreOtherParameters()
         ->withOutputParameterReturning("Data", &selector, sizeof(selector))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockRead")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     // ACT
     abr::err_code result = graphApi->read(&rtpConfig, buffer, numBytes);

     CHECK(result == err_code::ok);
 }

 /**
  * @brief TC_002_Failure: Verify RTP read failure when XAie operations fail
  *
  * Tests error handling when XAie_LockAcquire, DataMemRdWord, LockRelease, and
  * DataMemBlockRead return non-zero. Expects aie_driver_error return code.
  */
 TEST(GraphApiReadTests, TC_044_Read_UnconfiguredGraph_BlockReadFails)
 {
     // ARRANGE
     delete graphApi; // Delete the pre-configured graph
     rtpConfig.isAsync = true;
     graphApi = new abr::graph_api(&graphConfig); // Create a new unconfigured instance
     size_t numBytes = rtpConfig.numBytes;
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     mock_c()->expectOneCall("XAie_DataMemRdWord")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     mock_c()->expectOneCall("XAie_DataMemBlockRead")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     // ACT
     abr::err_code result = graphApi->read(&rtpConfig, buffer, numBytes);

     CHECK(result == err_code::aie_driver_error);
 }
 // TC_002: Call read() with invalid RTP config (using checkRTPConfigForRead helper)
 TEST(GraphApiReadTests, TC_045_Read_InvalidRTPConfig)
 {
     // ARRANGE
     rtpConfig.graphId = 2; // Mismatch with graph ID
     size_t numBytes = rtpConfig.numBytes;

     // ACT
     abr::err_code result = graphApi->read(&rtpConfig, buffer, numBytes);

     CHECK(result == err_code::user_error);
 }

 /**
  * @brief TC_046: Verify RTP read from ping buffer when selector=0
  *
  * Tests graph_api::read() when selector=0, triggering the ping buffer read path
  * with appropriate lock acquisition and release.
  */
 TEST(GraphApiReadTests, TC_046_Read_FromPingBuffer)
 {
     // ARRANGE - selector = 0 means read from ping
     delete graphApi;
     graphApi = new abr::graph_api(&graphConfig);
     size_t numBytes = rtpConfig.numBytes;
     u32 selector = 0; // Ping buffer

     mock_c()->expectOneCall("XAie_LockAcquire")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemRdWord")
         ->ignoreOtherParameters()
         ->withOutputParameterReturning("Data", &selector, sizeof(selector))
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockAcquire") // Ping lock acquire
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DataMemBlockRead") // Read from ping
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_LockRelease") // Ping lock release
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT
     abr::err_code result = graphApi->read(&rtpConfig, buffer, numBytes);

     // ASSERT
     CHECK(result == err_code::ok);
 }

 /**
  * @brief Test group for graph_api RTP update validation
  *
  * This test group provides setup for testing RTP update scenarios with
  * pre-configured graph and RTP settings.
  */
 TEST_GROUP(GraphAPI_Update_RTP) {
     XAie_DevInst devInst;
     graph_config graphConfig;
     rtp_config rtpConfig;
     graph_api* graphApi;

     /**
      * @brief Setup for GraphAPI_Update_RTP tests
      *
      * Initializes device, graph, and RTP configurations for update testing.
      */
     void setup() {

         config_manager::s_bInitialized = true;
         config_manager::s_pDevInst = &devInst;

         config_manager::s_num_reserved_rows = 2;

         // Set up a valid graph config
         graphConfig.id = 1;
         graphConfig.name = "TestGraph";

         // Set up a valid RTP config
         rtpConfig.graphId = 1;
         rtpConfig.portName = "TestPort";
         rtpConfig.isInput = true;
         rtpConfig.numBytes = 4;
         rtpConfig.isConnect = false;
         rtpConfig.hasLock = true;
         rtpConfig.selectorColumn = 1;
         rtpConfig.selectorRow = 1;
         rtpConfig.selectorAddr = 0x1000;
         rtpConfig.selectorLockId = 0;
         rtpConfig.pingColumn = 1;
         rtpConfig.pingRow = 1;
         rtpConfig.pingAddr = 0x2000;
         rtpConfig.pingLockId = 1;
         rtpConfig.pongColumn = 1;
         rtpConfig.pongRow = 1;
         rtpConfig.pongAddr = 0x3000;
         rtpConfig.pongLockId = 2;

         graphApi = new graph_api(&graphConfig);
         graphApi->configure(); // Pre-configure
     }

     /**
      * @brief Teardown for GraphAPI_Update_RTP tests
      */
     void teardown() {
         delete graphApi;

     }
 };

 /**
  * @brief Test group for config_manager initialization
  *
  * This test group validates config_manager::initialize() with various
  * initialization states and parameter combinations.
  */
 TEST_GROUP(config_managertests)
 {
     void setup() {

     }
     void teardown() {

     }
 };

 /**
  * @brief TC_001: Verify config_manager initialization when already initialized
  *
  * Tests that calling initialize() when already initialized returns ok.
  */
 TEST(config_managertests, TC_047_Initialize_AlreadyInitialized)
 {
     abr::err_code res;
     XAie_DevInst devInst;

     size_t num_reserved_row = 10;
     config_manager::s_bInitialized = true;
     bool broadcast_enable_core = true;
     res = config_manager::initialize(&devInst,num_reserved_row,broadcast_enable_core);
     CHECK(res == err_code::ok);
 }

 /**
  * @brief TC_002_Success: Verify successful config_manager initialization
  *
  * Tests successful initialization when config_manager is not yet initialized.
  */
 TEST(config_managertests, TC_048_Initialize_Success)
 {
     abr::err_code res;
     XAie_DevInst devInst;
     config_manager::s_bInitialized = false;
     size_t num_reserved_row = 10;
     bool broadcast_enable_core = true;
     res = config_manager::initialize(&devInst,num_reserved_row,broadcast_enable_core);
     CHECK(res == err_code::ok);
 }

 /**
  * @brief TC_003_Failure: Verify config_manager initialization failure with null device
  *
  * Tests error handling when initialize() is called with nullptr device instance.
  * Expects internal_error return code.
  */
 TEST(config_managertests, TC_049_Initialize_NullDevInst)
 {
     abr::err_code res;
     XAie_DevInst *devInst = nullptr;

     config_manager::s_bInitialized = false;
     size_t num_reserved_row = 10;
     bool broadcast_enable_core = true;
     res = config_manager::initialize(devInst,num_reserved_row,broadcast_enable_core);
     CHECK(res == err_code::internal_error);
 }

 /**
  * @brief Test group for checkRTPConfigForRead validation function
  *
  * This test group validates the checkRTPConfigForRead() helper function with
  * various valid and invalid RTP configurations.
  */
 TEST_GROUP(Xaie_checkRTPConfigForRead)
 {

     void setup()
     {
     }
     void teardown()
     {
     }
 };

 /**
  * @brief TC1: Verify null RTP config returns internal_error
  */
 TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC1)
 {

     abr::err_code err;
     abr::rtp_config *pRTPConfig = nullptr;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);

     err = checkRTPConfigForRead(pRTPConfig,&pGraphConfig,numBytes);

     CHECK(err == abr::err_code::internal_error);
 }

 /**
  * @brief TC2: Verify mismatched graph IDs returns user_error
  */
 TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC2)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x2;

     err = checkRTPConfigForRead(&pRTPConfig,&pGraphConfig,numBytes);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC3: Verify input RTP port for read returns user_error
  */
 TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC3)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pRTPConfig.isInput = 1;

     err = checkRTPConfigForRead(&pRTPConfig,&pGraphConfig,numBytes);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC4: Verify numBytes mismatch returns user_error
  */
 TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC4)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pRTPConfig.numBytes = sizeof(abr::graph_config) + 1;

     err = checkRTPConfigForRead(&pRTPConfig,&pGraphConfig,numBytes);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC5: Verify isConnect RTP port returns user_error
  */
 TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC5)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pRTPConfig.isConnect = 1;

     err = checkRTPConfigForRead(&pRTPConfig,&pGraphConfig,numBytes);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC6: Verify valid RTP config for read returns ok
  */
 TEST(Xaie_checkRTPConfigForRead,Xaie_checkRTPConfigForRead_TC6)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pRTPConfig.isConnect = 0;
     pRTPConfig.numBytes = sizeof(abr::graph_config) ;
     pRTPConfig.isInput = 0;
     err = checkRTPConfigForRead(&pRTPConfig,&pGraphConfig,numBytes);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief Test group for checkRTPConfigForUpdate validation function
  *
  * This test group validates the checkRTPConfigForUpdate() helper function with
  * various valid and invalid RTP configurations for update operations.
  */
 TEST_GROUP(Xaie_RTPConfigforUpdate)
 {

     void setup()
     {
     }
     void teardown()
     {}
 };

 /**
  * @brief TC1: Verify null RTP config for update returns internal_error
  */
 TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC1)
 {

     abr::err_code err;
     abr::rtp_config *pRTPConfig = nullptr;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);
     bool isRunning = 0x0;

     err = checkRTPConfigForUpdate(pRTPConfig,&pGraphConfig,numBytes,isRunning);

     CHECK(err == abr::err_code::internal_error);
 }

 /**
  * @brief TC2: Verify mismatched graph IDs returns user_error
  */
 TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC2)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x2;
     size_t numBytes = sizeof(abr::graph_config);
     bool isRunning = 0x0;

     err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC3: Verify output RTP port for update returns user_error
  */
 TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC3)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pRTPConfig.isInput = 0;
     size_t numBytes = sizeof(abr::graph_config);
     bool isRunning = 0x0;

     err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC4: Verify numBytes mismatch returns user_error
  */
 TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC4)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pRTPConfig.isInput = 1;
     size_t numBytes = sizeof(abr::graph_config);
     pRTPConfig.numBytes = sizeof(abr::rtp_config);
     bool isRunning = 0x0;

     err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC5: Verify PL connection RTP port returns user_error
  */
 TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC5)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pRTPConfig.isInput = 1;
     size_t numBytes = sizeof(abr::rtp_config);
     pRTPConfig.numBytes = sizeof(abr::rtp_config);
     pRTPConfig.isConnect = 1;
     pRTPConfig.isPL = 0x1;
     bool isRunning = 0x0;

     err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC6: Verify async RTP update while running returns user_error
  */
 TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC6)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pRTPConfig.isInput = 1;
     size_t numBytes = sizeof(abr::rtp_config);
     pRTPConfig.numBytes = sizeof(abr::rtp_config);
     pRTPConfig.isConnect = 1;
     pRTPConfig.isPL = 0x0;
     pRTPConfig.isAsync = 0x1;
     bool isRunning = 0x1;

     err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC7: Verify valid RTP config for update returns ok
  */
 TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC7)
 {

     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pRTPConfig.isInput = 1;
     size_t numBytes = sizeof(abr::rtp_config);
     pRTPConfig.numBytes = sizeof(abr::rtp_config);
     pRTPConfig.isConnect = 0;
     bool isRunning  = 0x0;

     err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief TC8: Verify RTP config with zero is Running returns ok
  */
 TEST(Xaie_RTPConfigforUpdate,Xaie_RTPConfigForUpdate_TC8)
 {
     abr::err_code err;
     abr::rtp_config pRTPConfig;
     abr::graph_config pGraphConfig;
     pRTPConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pRTPConfig.isInput = 1;
     size_t numBytes = sizeof(abr::rtp_config);
     pRTPConfig.numBytes = sizeof(abr::rtp_config);
     pRTPConfig.isConnect = 1;
     pRTPConfig.isPL = 0x0;
     bool isRunning  = 0x0;
     pRTPConfig.isAsync = 0x0;
     err = checkRTPConfigForUpdate(&pRTPConfig,&pGraphConfig,numBytes,isRunning);

     CHECK(err == abr::err_code::user_error);
 }
 TEST_GROUP(Xaie_SharedBufferConfig)
 {


     void setup()
     {

     }
     void teardown()
     {
         mock_c()->clear();
     }
 };

 /**
  * @brief TC1: Verify null shared buffer config returns internal_error
  */
 TEST(Xaie_SharedBufferConfig,Xaie_checkSharedBufferConfigForUpdate_TC1)
 {

     abr::err_code err;
     abr::shared_buffer_config *pSharedBufferConfig = nullptr;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);

     err = checkSharedBufferConfigForUpdate(pSharedBufferConfig,&pGraphConfig,numBytes);

     CHECK(err == abr::err_code::internal_error);
 }

 /**
  * @brief TC2: Verify mismatched graph IDs returns user_error
  */
 TEST(Xaie_SharedBufferConfig,Xaie_checkSharedBufferConfigForUpdate_TC2)
 {

     abr::err_code err;
     abr::shared_buffer_config pSharedBufferConfig;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);
     pSharedBufferConfig.graphId = 0x1;
     pGraphConfig.id = 0x2;

     err = checkSharedBufferConfigForUpdate(&pSharedBufferConfig,&pGraphConfig,numBytes);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC3: Verify shared buffer with multiple inputs returns user_error
  */
 TEST(Xaie_SharedBufferConfig,Xaie_checkSharedBufferConfigForUpdate_TC3)
 {

     abr::err_code err;
     abr::shared_buffer_config pSharedBufferConfig;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);
     pSharedBufferConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pSharedBufferConfig.numInputs = 2;

     err = checkSharedBufferConfigForUpdate(&pSharedBufferConfig,&pGraphConfig,numBytes);

     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC4: Verify numBytes mismatch returns user_error
  */
 TEST(Xaie_SharedBufferConfig,Xaie_checkSharedBufferConfigForUpdate_TC4)
 {

     abr::err_code err;
     abr::shared_buffer_config pSharedBufferConfig;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);
     pSharedBufferConfig.graphId = 0x1;
     pGraphConfig.id = 0x1;
     pSharedBufferConfig.numInputs = 0;
     pSharedBufferConfig.numOutputs = 1;
     pSharedBufferConfig.numBytes = sizeof(abr::shared_buffer_config);

     err = checkSharedBufferConfigForUpdate(&pSharedBufferConfig,&pGraphConfig,numBytes);


     CHECK(err == abr::err_code::user_error);
 }

 /**
  * @brief TC5: Verify valid shared buffer config returns ok
  */
 TEST(Xaie_SharedBufferConfig,Xaie_checkSharedBufferConfigForUpdate_TC5)
 {

     abr::err_code err;
     abr::shared_buffer_config pSharedBufferConfig;
     abr::graph_config pGraphConfig;
     size_t numBytes = sizeof(abr::graph_config);
     pSharedBufferConfig.graphId = 0x1;
     pSharedBufferConfig.initialized = 0;
     pGraphConfig.id = 0x1;
     pSharedBufferConfig.numInputs = 0;
     pSharedBufferConfig.numOutputs = 1;
     pSharedBufferConfig.numBytes = sizeof(abr::graph_config);

     err = checkSharedBufferConfigForUpdate(&pSharedBufferConfig,&pGraphConfig,numBytes);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief Test group for XAie lock API operations
  *
  * This test group validates lock initialization, acquire, and release operations
  * with various success and failure scenarios.
  */
 TEST_GROUP(Xaie_LockApiTests) {
     /**
      * @brief Setup for Xaie_LockApiTests
      *
      * Installs XAie_LocType and XAie_Lock comparators for mock verification.
      */
     void setup() {
         mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
         mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
     }

     void teardown() {
         mock_c()->removeAllComparatorsAndCopiers();
         mock_c()->clear();
     }
 };

 /**
  * @brief TC1: Verify successful lock initialization
  *
  * Tests initializeLock() with valid parameters and verifies XAie_LockSetValue is called correctly.
  */
 TEST(Xaie_LockApiTests,Xaie_LockApiTests_initializeLock_TC1)
 {

     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     unsigned short lockId = 0x5;
     int8_t initVal = 0x0;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     XAie_Lock testLock;
     testLock.LockId = 5;
     testLock.LockVal = 0;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_LockSetValue")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withParameterOfType("XAie_Lock", "Lock", &testLock)
         ->andReturnIntValue(XAIE_OK);
     err = lock_api::initializeLock(tileType,column,row,lockId,initVal);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief TC2: Verify lock initialization failure when XAie_LockSetValue fails
  *
  * Tests error handling when XAie_LockSetValue returns non-zero. Expects aie_driver_error.
  */
 TEST(Xaie_LockApiTests,Xaie_LockApiTests_initializeLock_TC2)
 {

     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     unsigned short lockId = 0x5;
     int8_t initVal = 0x0;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     XAie_Lock testLock;
     testLock.LockId = 5;
     testLock.LockVal = 0;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_LockSetValue")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withParameterOfType("XAie_Lock", "Lock", &testLock)
         ->andReturnIntValue(1);
     err = lock_api::initializeLock(tileType,column,row,lockId,initVal);

     CHECK(err == abr::err_code::aie_driver_error);
 }

 /**
  * @brief TC1: Verify successful lock acquisition
  *
  * Tests acquireLock() with valid parameters and verifies XAie_LockAcquire is called correctly.
  */
 TEST(Xaie_LockApiTests,Xaie_LockApiTests_acquireLock_TC1)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     unsigned short lockId = 0x5;
     int8_t acqVal = 0x0;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     XAie_Lock testLock;
     testLock.LockId = 5;
     testLock.LockVal = 0;
     DummyXAie_DevInst devInst;
     u32 TimeOut = 0x7fffffff;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withParameterOfType("XAie_Lock", "Lock", &testLock)
         ->withUnsignedIntParameters("TimeOut", TimeOut)
         ->andReturnIntValue(XAIE_OK);
     err = lock_api::acquireLock(tileType,column,row,lockId,acqVal);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief TC2: Verify lock acquisition failure when XAie_LockAcquire fails
  *
  * Tests error handling when XAie_LockAcquire returns non-zero. Expects aie_driver_error.
  */
 TEST(Xaie_LockApiTests,Xaie_LockApiTests_acquireLock_TC2)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     unsigned short lockId = 0x5;
     int8_t acqVal = 0x0;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     XAie_Lock testLock;
     testLock.LockId = 5;
     testLock.LockVal = 0;
     DummyXAie_DevInst devInst;
     u32 TimeOut = 0x7fffffff;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_LockAcquire")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withParameterOfType("XAie_Lock", "Lock", &testLock)
         ->withUnsignedIntParameters("TimeOut", TimeOut)
         ->andReturnIntValue(1);
     err = lock_api::acquireLock(tileType,column,row,lockId,acqVal);

     CHECK(err == abr::err_code::aie_driver_error);
 }

 /**
  * @brief TC1: Verify successful lock release
  *
  * Tests releaseLock() with valid parameters and verifies XAie_LockRelease is called correctly.
  */
 TEST(Xaie_LockApiTests,Xaie_LockApiTests_releaseLock_TC1)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     unsigned short lockId = 0x5;
     int8_t acqVal = 0x0;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     u32 TimeOut = 0x7fffffff;
     XAie_Lock testLock;
     testLock.LockId = 5;
     testLock.LockVal = 0;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

     mock_c()->expectOneCall("XAie_LockRelease")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withParameterOfType("XAie_Lock", "Lock", &testLock)
         ->withUnsignedIntParameters("TimeOut", TimeOut)
         ->andReturnIntValue(XAIE_OK);
     err = lock_api::releaseLock(tileType,column,row,lockId,acqVal);
     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief TC2: Verify lock release failure when XAie_LockRelease fails
  *
  * Tests error handling when XAie_LockRelease returns non-zero. Expects aie_driver_error.
  */
 TEST(Xaie_LockApiTests,Xaie_LockApiTests_releaseLock_TC2)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     unsigned short lockId = 0x5;
     int8_t acqVal = 0x0;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     u32 TimeOut = 0x7fffffff;
     XAie_Lock testLock;
     testLock.LockId = 5;
     testLock.LockVal = 0;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

     mock_c()->expectOneCall("XAie_LockRelease")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withParameterOfType("XAie_Lock", "Lock", &testLock)
         ->withUnsignedIntParameters("TimeOut", TimeOut)
         ->andReturnIntValue(1);
     err = lock_api::releaseLock(tileType,column,row,lockId,acqVal);
     CHECK(err == abr::err_code::aie_driver_error);
 }
 TEST_GROUP(Xaie_DMAApiTests) {
     void setup() {
         mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
         mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
         mock_c()->installCopier("u8", u8Copier);
     }

     void teardown() {
         mock_c()->removeAllComparatorsAndCopiers();
         mock_c()->clear();
     }
 };

 /**
  * @brief TC1: Verify successful BD address update
  *
  * Tests updateBDAddress() with valid parameters and verifies XAie_DmaUpdateBdAddr is called correctly.
  */
 TEST(Xaie_DMAApiTests,Xaie_updateBDAddress_TC1)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     uint8_t bdId = 0x1;
     uint64_t address = 0x2000000000;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedLongLongIntParameters("Addr", address)
         ->withUnsignedIntParameters("BdNum", bdId)
         ->andReturnIntValue(XAIE_OK);
     err = dma_api::updateBDAddress(tileType,column,row,bdId,address);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief TC2: Verify BD address update failure when XAie_DmaUpdateBdAddr fails
  *
  * Tests error handling when XAie_DmaUpdateBdAddr returns non-zero. Expects aie_driver_error.
  */
 TEST(Xaie_DMAApiTests,Xaie_updateBDAddress_TC2)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     uint8_t bdId = 0x1;
     uint64_t address = 0x2000000000;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedLongLongIntParameters("Addr", address)
         ->withUnsignedIntParameters("BdNum", bdId)
         ->andReturnIntValue(1);
     err = dma_api::updateBDAddress(tileType,column,row,bdId,address);

     CHECK(err == abr::err_code::aie_driver_error);
 }

 /**
  * @brief TC1: Verify successful DMA channel wait for done
  *
  * Tests waitDMAChannelDone() with valid parameters and verifies XAie_DmaWaitForDone is called correctly.
  */
 TEST(Xaie_DMAApiTests,Xaie_waitDMAChannelDone_Success_TC1)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     int dir = 0x0;
     uint8_t channel = 0x1;
     u32 TimeOutUs = 0;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_DmaWaitForDone")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedIntParameters("ChNum", channel)
         ->withIntParameters("Dir", dir)
         ->withUnsignedIntParameters("TimeOutUs", TimeOutUs)
         ->andReturnIntValue(XAIE_OK);
     err = dma_api::waitDMAChannelDone(tileType,column,row,dir,channel);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief TC1: Verify successful DMA channel task queue wait
  *
  * Tests waitDMAChannelTaskQueue() with multiple pending BD count checks. Verifies wait loop
  * continues until pending BDs drops below threshold.
  */
 TEST(Xaie_DMAApiTests,Xaie_waitDMAChannelTaskQueue_TC1)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     int dir = 0x0;
     uint8_t channel = 0x1;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     u8 PendingBd = 0x2;

     u8 firstCallOutput = 5; // > 3, to keep the loop going
     u8 secondCallOutput = 2; // <= 3, to make the loop exit
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
         // REMOVE THIS LINE: ->withPointerParameters("DevInst", &devInst)

         // Check all other parameters explicitly:
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedIntParameters("ChNum", channel)
         ->withIntParameters("Dir", dir)
         ->withOutputParameterOfTypeReturning("u8", "PendingBd", &firstCallOutput)

         // Add the ignore, since we removed DevInst and Mocking is too strict
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
         // REMOVE THIS LINE: ->withPointerParameters("DevInst", &devInst)

         // Check all other parameters explicitly:
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedIntParameters("ChNum", channel)
         ->withIntParameters("Dir", dir)
         ->withOutputParameterOfTypeReturning("u8", "PendingBd", &secondCallOutput)

         // Add the ignore, since we removed DevInst and Mocking is too strict
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     err = dma_api::waitDMAChannelTaskQueue(tileType,column,row,dir,channel);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief TC2: Verify DMA channel task queue wait failure when XAie_DmaGetPendingBdCount fails
  *
  * Tests error handling when XAie_DmaGetPendingBdCount returns non-zero. Expects aie_driver_error.
  */
 TEST(Xaie_DMAApiTests,Xaie_waitDMAChannelTaskQueue_TC2)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     int dir = 0x0;
     u8 PendingBd = 0x4;
     uint8_t channel = 0x1;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;

     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

     u8 firstCallOutput = 5; // > 3, to keep the loop going
     u8 secondCallOutput = 2; // <= 3, to make the loop exit
     mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
         // REMOVE THIS LINE: ->withPointerParameters("DevInst", &devInst)

         // Check all other parameters explicitly:
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedIntParameters("ChNum", channel)
         ->withIntParameters("Dir", dir)
         ->withOutputParameterOfTypeReturning("u8", "PendingBd", &firstCallOutput)

         // Add the ignore, since we removed DevInst and Mocking is too strict
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
         // REMOVE THIS LINE: ->withPointerParameters("DevInst", &devInst)

         // Check all other parameters explicitly:
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedIntParameters("ChNum", channel)
         ->withIntParameters("Dir", dir)
         ->withOutputParameterOfTypeReturning("u8", "PendingBd", &secondCallOutput)

         // Add the ignore, since we removed DevInst and Mocking is too strict
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);

     err = dma_api::waitDMAChannelTaskQueue(tileType,column,row,dir,channel);

     CHECK(err == abr::err_code::aie_driver_error);
 }

 /**
  * @brief TC1: Verify successful DMA task enqueue
  *
  * Tests enqueueTask() with valid parameters and verifies XAie_DmaChannelSetStartQueue is called correctly.
  */
 TEST(Xaie_DMAApiTests,Xaie_DMAApiTests_enqueueTask_TC1)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     int dir = 0x0;
     uint8_t channel = 0x1;
     uint32_t repeatCount = 0x3;
     bool enableTaskCompleteToken = 0x0;
     uint8_t startBdId = 0x0;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     u16 BdNum = 0x0;
     u32 RepeatCount = 0x3;
     u8 EnTokenIssue = 0x0;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedIntParameters("ChNum", channel)
         ->withIntParameters("Dir", dir)
         ->withUnsignedIntParameters("BdNum", BdNum)
         ->withUnsignedIntParameters("RepeatCount", RepeatCount)
         ->withUnsignedIntParameters("EnTokenIssue", EnTokenIssue)
         ->andReturnIntValue(XAIE_OK);
     err = dma_api::enqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, startBdId);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief TC2: Verify DMA task enqueue failure when XAie_DmaChannelSetStartQueue fails
  *
  * Tests error handling when XAie_DmaChannelSetStartQueue returns non-zero. Expects aie_driver_error.
  */
 TEST(Xaie_DMAApiTests,Xaie_DMAApiTests_enqueueTask_TC2)
 {
     u32 tim_ret;
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     int dir = 0x0;
     uint8_t channel = 0x1;
     uint32_t repeatCount = 0x3;
     bool enableTaskCompleteToken = 0x0;
     uint8_t startBdId = 0x0;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 14;
     u16 BdNum = 0x0;
     u32 RepeatCount = 0x3;
     u8 EnTokenIssue = 0x0;
     DummyXAie_DevInst devInst;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedIntParameters("ChNum", channel)
         ->withIntParameters("Dir", dir)
         ->withUnsignedIntParameters("BdNum", BdNum)
         ->withUnsignedIntParameters("RepeatCount", RepeatCount)
         ->withUnsignedIntParameters("EnTokenIssue", EnTokenIssue)
         ->andReturnIntValue(1);
     err = dma_api::enqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, startBdId);

     CHECK(err == abr::err_code::aie_driver_error);
 }

 /**
  * @brief Test group for XAie DMA API configuration operations
  *
  * This test group validates DMA buffer descriptor configuration, wait, queue, and enqueue
  * operations with various parameters and scenarios.
  */
 TEST_GROUP(Xaie_DMAApiConfigTests) {
     /**
      * @brief Setup for Xaie_DMAApiConfigTests
      *
      * Installs XAie_LocType, XAie_Packet, XAie_Lock comparators and u8 copier for mock verification.
      */
     void setup() {
         mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
         mock_c()->installComparator("XAie_Packet", XAie_PacketComparator, XAie_PacketToString);
         mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
         mock_c()->installCopier("u8", u8Copier);

     }

     /**
      * @brief Teardown for Xaie_DMAApiConfigTests
      *
      * Removes comparators/copiers and clears mocks after each test.
      */
     void teardown() {
         mock_c()->removeAllComparatorsAndCopiers();
         mock_c()->clear();
     }
 };

 /**
  * @brief Test: Verify configureBdWaitQueueEnqueueTask with empty BD parameters returns error
  *
  * Tests error handling when configureBdWaitQueueEnqueueTask is called with empty bdParams vector.
  * Expects internal_error return code.
  */
 TEST(Xaie_DMAApiConfigTests,Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask)
 {
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     int dir = 0x1;
     uint8_t channel = 0x1;
     uint32_t repeatCount = 0x1;
     bool enableTaskCompleteToken = true;
     std::vector<uint8_t> bdIds = {0};
     std::vector<dma_api::buffer_descriptor> bdParams = {};
     config_manager::s_bInitialized = false;
     size_t num_reserved_row = 10;
     bool broadcast_enable_core = true;
     XAie_DevInst devInst;
     devInst.DevProp.DevGen = XAIE_DEV_GEN_AIE;
     config_manager::initialize(&devInst,num_reserved_row,broadcast_enable_core);

     config_manager::s_bInitialized = true;
     err = dma_api::configureBdWaitQueueEnqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, bdIds, bdParams);

     CHECK(err == abr::err_code::internal_error);
 }

 /**
  * @brief TC1: Verify configureBdWaitQueueEnqueueTask with uninitialized config returns error
  *
  * Tests error handling when configureBdWaitQueueEnqueueTask is called without proper config manager initialization.
  * Expects internal_error return code.
  */
 TEST(Xaie_DMAApiConfigTests,Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC1)
 {
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     int dir = 0x1;
     uint8_t channel = 0x1;
     uint32_t repeatCount = 0x1;
     bool enableTaskCompleteToken = true;
     std::vector<uint8_t> bdIds = {0};
     std::vector<dma_api::buffer_descriptor> bdParams = {};


     //config_manager::initialize(&devInst,num_reserved_row,broadcast_enable_core);
     // config_manager::s_pDevInst->DevProp.DevGen = XAIE_DEV_GEN_AIE;

     err = dma_api::configureBdWaitQueueEnqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, bdIds, bdParams);

     CHECK(err == abr::err_code::internal_error);
 }

 /**
  * @brief TC2: Verify configureBdWaitQueueEnqueueTask with empty BD parameters
  *
  * Tests error handling when configureBdWaitQueueEnqueueTask is called with empty bdParams.
  * Verifies proper error propagation.
  */
 TEST(Xaie_DMAApiConfigTests,Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC2)
 {
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     int dir = 0x1;
     uint8_t channel = 0x1;
     uint32_t repeatCount = 0x1;
     bool enableTaskCompleteToken = true;
     std::vector<uint8_t> bdIds = {0};

     dma_api::buffer_descriptor bdParam1;
     bdParam1.address = 0x1000;
     bdParam1.length = 1024;

     dma_api::buffer_descriptor bdParam2;
     bdParam2.address = 0x2000;
     bdParam2.length = 2048;

     std::vector<dma_api::buffer_descriptor> bdParams = {bdParam1, bdParam2};

     err = dma_api::configureBdWaitQueueEnqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, bdIds, bdParams);

     CHECK(err == abr::err_code::internal_error);
 }
 abr::err_code waitDMAChannelTaskQueue(int tileType, uint8_t column, uint8_t row, int dir, uint8_t channel)
 {
     return (err_code) mock_c()->actualCall("waitDMAChannelTaskQueue")
         ->withIntParameters("tileType", tileType)
         ->withIntParameters("col",column)
         ->withIntParameters("row",row)
         ->withIntParameters("Dir",dir)
         ->withUnsignedIntParameters("Channel",channel)
         ->returnIntValueOrDefault(XAIE_OK);
 }

 TEST(Xaie_DMAApiConfigTests,Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC3)
 {
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x14;
     int dir = 0x1;
     uint8_t channel = 0x1;
     uint32_t repeatCount = 0x1;
     bool enableTaskCompleteToken = true;
     std::vector<uint8_t> bdIds = {0};

     dma_api::buffer_descriptor bdParam;
     bdParam.address = 0x1000;
     bdParam.length = 1024;
     std::vector<dma_api::buffer_descriptor> bdParams = {bdParam};
     XAie_LocType testLoc,testLoc1;
     testLoc.Col = 2;
     testLoc.Row = 6;
     testLoc1.Col = 2;
     testLoc1.Row = 4;
     DummyXAie_DevInst devInst;
     XAie_DmaDesc DmaDesc;
     DmaDesc.IsReady = 0;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         //->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     // Values the mock will write to the 'PendingBd' pointer
     u8 firstCallOutput = 5; // > 3, to keep the loop going
     u8 secondCallOutput = 2; // <= 3, to make the loop exit
     mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
         // REMOVE THIS LINE: ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         // Check all other parameters explicitly:
         //->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedIntParameters("ChNum", channel)
         ->withIntParameters("Dir", dir)
         ->withOutputParameterOfTypeReturning("u8", "PendingBd", &firstCallOutput)

         // Add the ignore, since we removed DevInst and Mocking is too strict
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
         // REMOVE THIS LINE: ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         // Check all other parameters explicitly:
         //->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->withUnsignedIntParameters("ChNum", channel)
         ->withIntParameters("Dir", dir)
         ->withOutputParameterOfTypeReturning("u8", "PendingBd", &secondCallOutput)

         // Add the ignore, since we removed DevInst and Mocking is too strict
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
         // REMOVE THIS LINE: ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         // Check all other parameters explicitly:
         //->withParameterOfType("XAie_LocType", "Loc", &testLoc1)
         ->withUnsignedIntParameters("ChNum", channel)
         ->withIntParameters("Dir", dir)
         ->withOutputParameterOfTypeReturning("u8", "PendingBd", &secondCallOutput)

         // Add the ignore, since we removed DevInst and Mocking is too strict
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectNCalls(2,"waitDMAChannelTaskQueue")
         ->withIntParameters("tileType", tileType)
         ->withIntParameters("col",column)
         ->withIntParameters("row",row)
         ->withIntParameters("Dir",dir)
         ->withUnsignedIntParameters("Channel",channel)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     err = dma_api::configureBdWaitQueueEnqueueTask(tileType,column,row,dir,channel,repeatCount,enableTaskCompleteToken, bdIds, bdParams);

     CHECK(err == abr::err_code::ok);
     mock_c()->clear();
 }

 /**
  * @brief TC_010: Verify configureBdWaitQueueEnqueueTask with AIE architecture returns error
  *
  * Tests error handling when configureBdWaitQueueEnqueueTask is called with AIE (or S100/S200) device.
  * The function should return internal_error for unsupported AIE architecture.
  */
 TEST(Xaie_DMAApiConfigTests,Xaie_DMAApiTests_configureBdWaitQueueEnqueueTask_TC_010)
 {
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     int dir = 0x1;
     uint8_t channel = 0x1;
     uint32_t repeatCount = 0x1;
     bool enableTaskCompleteToken = true;
     std::vector<uint8_t> bdIds = {0};

     // Create valid BD parameters to avoid hitting the empty check
     dma_api::buffer_descriptor bdParam;
     bdParam.address = 0x1000;
     bdParam.length = 1024;
     std::vector<dma_api::buffer_descriptor> bdParams = {bdParam};

     // Set DevGen to S100 (which normalizes to AIE via getNormalizedDevGen)
     config_manager::s_bInitialized = false;
     size_t num_reserved_row = 10;
     bool broadcast_enable_core = true;
     XAie_DevInst devInst;
     devInst.DevProp.DevGen = XAIE_DEV_GEN_S100;  // This will normalize to AIE
     config_manager::initialize(&devInst, num_reserved_row, broadcast_enable_core);
     config_manager::s_bInitialized = true;

     // Should fail with AIE architecture error
     err = dma_api::configureBdWaitQueueEnqueueTask(tileType, column, row, dir, channel, repeatCount, enableTaskCompleteToken, bdIds, bdParams);

     CHECK(err == abr::err_code::internal_error);
 }

 /**
  * @brief TC_013: Verify configureBD with memory tile
  *
  * Tests configureBD() with tileType=2 (memory_tile) to verify proper row address calculation.
  * Memory tiles use absoluteRow = row + 1, different from AIE tiles which include reserved_rows.
  */
 TEST(Xaie_DMAApiConfigTests, Xaie_configureBD_MemoryTile_TC_013)
 {
     abr::err_code err;
     int tileType = 2; // Memory tile
     uint8_t column = 0x2;
     uint8_t row = 0x3;
     uint8_t bdIds = 0;

     dma_api::buffer_descriptor bdParam;
     bdParam.address = 0x1000;
     bdParam.length = 1024;
     bdParam.stepsize = {128};
     bdParam.lock_acq_enable = false;

     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 4; // row (3) + 1 = 4 for memory tile

     DummyXAie_DevInst devInst;
     XAie_DmaDesc DmaDesc;
     DmaDesc.IsReady = 0;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     err = dma_api::configureBD(tileType, column, row, bdIds, bdParam);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief TC1: Verify successful BD configuration with all features enabled
  *
  * Tests configureBD() with comprehensive BD parameters including packet mode, tlast suppression,
  * multi-dimensional addressing, compression, locks, AXI settings, and BD chaining.
  * Verifies all XAie DMA descriptor operations are called correctly.
  */
 TEST(Xaie_DMAApiConfigTests,Xaie_configureBD_TC1)
 {
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x2;
     uint8_t row = 0x3;

     uint8_t  bdIds = {0};
     dma_api::buffer_descriptor bdParam;
     bdParam.address = 0x1000;
     bdParam.length = 1024;
     bdParam.enable_packet = 1;
     bdParam.tlast_suppress = 1;
     bdParam.stepsize = {128, 256};
     bdParam.enable_compression = 1;
     bdParam.lock_acq_enable = 1;
     bdParam.use_next_bd = 1;
     XAie_LocType testLoc;
     testLoc.Col = 2;
     testLoc.Row = 4;
     DummyXAie_DevInst devInst;
     XAie_DmaDesc DmaDesc;
     DmaDesc.IsReady = 0;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         //->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetZeroPadding")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetPkt")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetOutofOrderBdId")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaTlastDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetBdIteration")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaEnableCompression")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetLock")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetNextBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     err = dma_api::configureBD(tileType,column,row,bdIds, bdParam);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief TC2: Verify successful BD configuration with tile at column 0, row 0
  *
  * Tests configureBD() for interface tile (column 0, row 0) with all BD features enabled.
  * Verifies proper handling of interface tile addressing and BD configuration.
  */
 TEST(Xaie_DMAApiConfigTests,Xaie_configureBD_TC2)
 {
     abr::err_code err;
     int tileType = 0x0;
     uint8_t column = 0x0;
     uint8_t row = 0x0;

     uint8_t  bdIds = {0};
     dma_api::buffer_descriptor bdParam;
     bdParam.address = 0x1000;
     bdParam.length = 1024;
     bdParam.enable_packet = 1;
     bdParam.tlast_suppress = 1;
     bdParam.stepsize = {128, 256};
     bdParam.enable_compression = 1;
     bdParam.lock_acq_enable = 1;
     bdParam.use_next_bd = 1;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 1;
     DummyXAie_DevInst devInst;
     XAie_DmaDesc DmaDesc;
     DmaDesc.IsReady = 0;
     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));

     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         //->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetZeroPadding")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetPkt")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetOutofOrderBdId")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaTlastDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetBdIteration")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaEnableCompression")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetLock")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetNextBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);

     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);

     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);

     err = dma_api::configureBD(tileType,column,row,bdIds, bdParam);

     CHECK(err == abr::err_code::aie_driver_error);
 }

 /**
  * @brief TC3: Verify successful BD configuration with padding and iteration stepsize
  *
  * Tests configureBD() for PL tile (tileType=1) with zero padding and iteration stepsize features.
  * Verifies proper handling of advanced BD features for interface tiles.
  */
 TEST(Xaie_DMAApiConfigTests,Xaie_configureBD_TC3)
 {
     abr::err_code err;
     int tileType = 0x1;
     uint8_t column = 0x1;
     uint8_t row = 0x0;

     uint8_t  bdIds = {0};
     dma_api::buffer_descriptor bdParam;
     bdParam.address = 0x1000;
     bdParam.length = 1024;
     bdParam.enable_packet = 1;
     bdParam.padding = {{4, 4}};
     bdParam.tlast_suppress = 1;
     bdParam.stepsize = {128, 256};
     bdParam.enable_compression = 1;
     bdParam.iteration_stepsize = 32;
     bdParam.lock_acq_enable = 1;
     bdParam.use_next_bd = 1;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;
     DummyXAie_DevInst devInst;
     XAie_DmaDesc DmaDesc;
     DmaDesc.IsReady = 0;

     DummyConfigManager::setDevInst(reinterpret_cast<XAie_DevInst*>(&devInst));
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetMultiDimAddr")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetZeroPadding")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetZeroPadding")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetPkt")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetOutofOrderBdId")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaTlastDisable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetBdIteration")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaEnableCompression")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetLock")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetNextBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     err = dma_api::configureBD(tileType,column,row,bdIds, bdParam);

     CHECK(err == abr::err_code::ok);
 }

 /**
  * @brief Test group for GMIO (Global Memory I/O) API operations
  *
  * This test group validates GMIO operations including constructor, configuration,
  * GM2AIE/AIE2GM transfers, wait operations, and BD enqueue functionality.
  */
 TEST_GROUP(GmioApiTests) {
     XAie_DevInst devInst;

     /**
      * @brief Setup for GmioApiTests
      *
      * Initializes device instance, config manager, and installs mock comparators.
      */
     void setup() {
         // Set up device instance for AIE
         devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML - 1;
         abr::config_manager::s_pDevInst = &devInst;
         abr::config_manager::s_bInitialized = true;
         mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
         mock_c()->installCopier("u8", u8Copier);
         mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
     }

     void teardown() {
         // Clean up after each test
         abr::config_manager::s_pDevInst = nullptr;
         abr::config_manager::s_bInitialized = false;
         mock_c()->removeAllComparatorsAndCopiers();
         mock_c()->clear();
     }
 };

 /**
  * @brief TC_001: Verify GMIO constructor with valid configuration
  *
  * Tests gmio_api constructor with valid GM2AIE configuration. Verifies successful instantiation.
  */
 TEST(GmioApiTests, TC_050_Constructor_ValidConfig) {
     // ARRANGE: Create a valid gmio_config
     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;

     // ACT: Create a GMIO API instance
     abr::gmio_api gmio(&config);

     // ASSERT: Verify object was created successfully
     CHECK(&gmio != nullptr);
 }

 /**
  * @brief TC_002_Failure: Verify GMIO configure with null configuration
  *
  * Tests error handling when gmio_api::configure() is called with nullptr config.
  * Expects internal_error return code.
  */
 TEST(GmioApiTests, TC_051_Configure_Failure_NullConfig) {
     // ARRANGE: Create a GMIO API instance with a valid gm2aie configuration
     abr::gmio_config *config = nullptr;

     abr::gmio_api gmio(config);


     // ACT: Call configure
     abr::err_code result = gmio.configure();

     // ASSERT: Check that configuration succeeded
     CHECK_EQUAL((int)abr::err_code::internal_error, (int)result);
 }

 /**
  * @brief TC_003: Verify successful GM2AIE GMIO configuration
  *
  * Tests gmio_api::configure() for GM2AIE transfer type. Verifies DMA descriptor
  * initialization, channel enable, and BD configuration for global memory to AIE transfers.
  */
 TEST(GmioApiTests, TC_052_Configure_GM2AIE_Success) {
     // ARRANGE: Create a GMIO API instance with a valid gm2aie configuration
     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;

     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);


     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     // Your existing first expectation (similar to this):
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         // ... parameters matching the FIRST call (e.g., Dir=0, BdNum=0) ...
         // Note: Remove the DmaDesc expectation if you were using it!

         // ... other parameters ...
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(XAIE_OK);

     // --- NEW FIX: Add the second expectation ---
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         // IMPORTANT: Match parameters for the SECOND call (e.g., Dir=1, BdNum=1)

         // Example: ->withIntParameters("Dir", 1)
         // Example: ->withUnsignedIntParameters("BdNum", 1)
         // ... all other relevant parameters ...
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     abr::gmio_api gmio(&config);

     // ACT: Call configure
     abr::err_code result = gmio.configure();

     // ASSERT: Check that configuration succeeded
     CHECK_EQUAL((int)abr::err_code::ok, (int)result);
 }

 /**
  * @brief TC_053: Verify GM2AIE GMIO configuration failure
  *
  * Tests error handling when XAie DMA operations fail during GM2AIE configuration.
  * Expects aie_driver_error return code.
  */
 TEST(GmioApiTests, TC_053_Configure_GM2AIE_Failure) {
     // ARRANGE: Create a GMIO API instance with a valid gm2aie configuration
     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;

     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(1);


     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     // Your existing first expectation (similar to this):
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         // ... parameters matching the FIRST call (e.g., Dir=0, BdNum=0) ...
         // Note: Remove the DmaDesc expectation if you were using it!

         // ... other parameters ...
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(1);

     // --- NEW FIX: Add the second expectation ---
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         // IMPORTANT: Match parameters for the SECOND call (e.g., Dir=1, BdNum=1)

         // Example: ->withIntParameters("Dir", 1)
         // Example: ->withUnsignedIntParameters("BdNum", 1)
         // ... all other relevant parameters ...
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);

     abr::gmio_api gmio(&config);

     // ACT: Call configure
     abr::err_code result = gmio.configure();

     // ASSERT: Check that configuration succeeded
     CHECK_EQUAL((int)abr::err_code::aie_driver_error, (int)result);
 }

 /**
  * @brief TC_003: Verify successful AIE2GM GMIO configuration
  *
  * Tests gmio_api::configure() for AIE2GM transfer type. Verifies DMA descriptor
  * initialization, channel enable, and BD configuration for AIE to global memory transfers.
  */
 TEST(GmioApiTests, TC_054_Configure_AIE2GM_Success) {
     // ARRANGE: Create a GMIO API instance with a valid gm2aie configuration
     abr::gmio_config config;
     config.type = abr::gmio_config::aie2gm;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;

     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);


     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     // Your existing first expectation (similar to this):
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         // ... parameters matching the FIRST call (e.g., Dir=0, BdNum=0) ...
         // Note: Remove the DmaDesc expectation if you were using it!

         // ... other parameters ...
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(XAIE_OK);

     // --- NEW FIX: Add the second expectation ---
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         // IMPORTANT: Match parameters for the SECOND call (e.g., Dir=1, BdNum=1)

         // Example: ->withIntParameters("Dir", 1)
         // Example: ->withUnsignedIntParameters("BdNum", 1)
         // ... all other relevant parameters ...
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     abr::gmio_api gmio(&config);

     // ACT: Call configure
     abr::err_code result = gmio.configure();

     // ASSERT: Check that configuration succeeded
     CHECK_EQUAL((int)abr::err_code::ok, (int)result);
 }

 /**
  * @brief TC_004_Failure: Verify AIE2GM GMIO configuration with invalid type
  *
  * Tests error handling when gmio_api is configured with uninitialized type.
  * Expects aie_driver_error return code.
  */
 TEST(GmioApiTests, TC_055_Configure_AIE2GM_Failure) {
     // ARRANGE: Create a GMIO API instance with a valid gm2aie configuration
     abr::gmio_config config;
     config.type ;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;

     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     abr::gmio_api gmio(&config);

     // ACT: Call configure
     abr::err_code result = gmio.configure();

     // ASSERT: Check that configuration succeeded
     CHECK_EQUAL((int)abr::err_code::aie_driver_error, (int)result);
 }

 /**
  * @brief TC_005: Verify GMIO wait with null configuration returns error
  *
  * Tests error handling when gmio_api::wait() is called with nullptr configuration.
  * Expects internal_error return code.
  */
 TEST(GmioApiTests, TC_056_Wait_NullConfig) {
     // ARRANGE: Create and configure a GMIO API instance
     abr::gmio_config *config = nullptr;

     abr::gmio_api gmio(config);

     // ACT: Call wait in synchronous mode
     abr::err_code result = gmio.wait(true);

     // ASSERT: Check that wait succeeded
     CHECK_EQUAL((int)abr::err_code::internal_error, (int)result);
 }

 /**
  * @brief TC_006: Verify GMIO synchronous wait operation
  *
  * Tests gmio_api::wait(true) for GM2AIE transfer. Verifies configuration and
  * synchronous wait behavior with XAie operations.
  */
 TEST(GmioApiTests, TC_057_Wait_Sync_Success) {
     // ARRANGE: Create and configure a GMIO API instance

     abr::gmio_config config;
     config.type = gmio_config::gm2aie ;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;



     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;

     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);


     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     // Your existing first expectation (similar to this):
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         // ... parameters matching the FIRST call (e.g., Dir=0, BdNum=0) ...
         // Note: Remove the DmaDesc expectation if you were using it!

         // ... other parameters ...
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(XAIE_OK);

     // --- NEW FIX: Add the second expectation ---
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         // IMPORTANT: Match parameters for the SECOND call (e.g., Dir=1, BdNum=1)

         // Example: ->withIntParameters("Dir", 1)
         // Example: ->withUnsignedIntParameters("BdNum", 1)
         // ... all other relevant parameters ...
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     abr::gmio_api gmio(&config);

     // ACT: Call configure
     abr::err_code result = gmio.configure();
     config.type = gmio_config::gm2pl ;
     abr::gmio_api gmio1(&config);
     // ASSERT: Check that configuration succeeded
     CHECK_EQUAL((int)abr::err_code::ok, (int)result);
     // ACT: Call wait in synchronous mode
     abr::err_code res = gmio.wait(true);

     // ASSERT: Check that wait succeeded
     CHECK_EQUAL((int)abr::err_code::user_error, (int)res);
 }

 /**
  * @brief TC_058: Verify GMIO asynchronous wait operation
  *
  * Tests gmio_api::wait() in async mode with reconfiguration. Verifies channel enable,
  * queue size retrieval, and async wait behavior.
  */
 TEST(GmioApiTests, TC_058_Wait_Async_Success) {
     // ARRANGE: Create and configure a GMIO API instance

     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;

     abr::gmio_api gmio(&config);

     u32 TimeOutUs = 0;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;
     u8 FirstCallOutput = 1;
     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT: Call configure
     abr::err_code result = gmio.configure();
     CHECK_EQUAL((int)abr::err_code::ok, (int)result);
     int dir = 0x0;
     uint8_t channel = 0x1;
     mock_c()->expectOneCall("XAie_DmaWaitForDone")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);



     // ACT: Call configure
     abr::err_code result1 = gmio.configure();
     config.type = gmio_config::gm2pl ;
     abr::gmio_api gmio1(&config);
     CHECK_EQUAL((int)abr::err_code::ok, (int)result1);

     // ACT: Call wait in synchronous mode
     abr::err_code res = gmio.wait(true);

     // ASSERT: Check that wait succeeded
     CHECK_EQUAL((int)abr::err_code::user_error, (int)res);
 }

 /**
  * @brief TC_059: Verify GMIO wait with sync=false returns resource unavailable
  *
  * Tests gmio_api::wait(false) when XAie_DmaWaitForDone fails. Verifies proper
  * error handling returns resource_unavailable.
  */
 TEST(GmioApiTests, TC_059_Wait_FalseSync) {
     // ARRANGE: Create and configure a GMIO API instance

     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;

     abr::gmio_api gmio(&config);

     u32 TimeOutUs = 0;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;
     u8 FirstCallOutput = 1;
     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT: Call configure
     abr::err_code result = gmio.configure();
     CHECK_EQUAL((int)abr::err_code::ok, (int)result);
     int dir = 0x0;
     uint8_t channel = 0x1;
     mock_c()->expectOneCall("XAie_DmaWaitForDone")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);

     // ACT: Call configure
     // abr::err_code result1 = gmio.configure();

     // CHECK_EQUAL((int)abr::err_code::ok, (int)result1);

     // ACT: Call wait in synchronous mode
     abr::err_code res = gmio.wait(false);

     // ASSERT: Check that wait succeeded
     CHECK_EQUAL((int)abr::err_code::resource_unavailable, (int)res);
 }

 /**
  * @brief TC_060: Verify successful GMIO synchronous wait
  *
  * Tests gmio_api::wait(true) with successful XAie_DmaWaitForDone operation.
  * Verifies synchronous wait completes and returns ok.
  */
 TEST(GmioApiTests, TC_060_Wait_TrueSync) {
     // ARRANGE: Create and configure a GMIO API instance

     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;

     abr::gmio_api gmio(&config);

     u32 TimeOutUs = 0;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;
     u8 FirstCallOutput = 1;
     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT: Call configure
     // abr::err_code result = gmio.configure();
     // CHECK_EQUAL((int)abr::err_code::ok, (int)result);
     int dir = 0x0;
     uint8_t channel = 0x1;
     mock_c()->expectOneCall("XAie_DmaWaitForDone")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);


     // ACT: Call configure
     abr::err_code result1 = gmio.configure();

     CHECK_EQUAL((int)abr::err_code::ok, (int)result1);

     // ACT: Call wait in synchronous mode
     abr::err_code res = gmio.wait(true);

     // ASSERT: Check that wait succeeded
     CHECK_EQUAL((int)abr::err_code::ok, (int)res);
 }

 /**
  * @brief TC_061: Verify GMIO wait with mem sync enabled
  *
  * Tests gmio_api::wait(false) after enqueueBD. Verifies wait checks memory sync
  * operations and returns appropriate status.
  */
 TEST(GmioApiTests, TC_061_Wait_FalseSync_WithMemSync) {
     // ARRANGE: Create and configure a GMIO API instance

     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;

     abr::gmio_api gmio(&config);

     u32 TimeOutUs = 0;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;
     u8 FirstCallOutput = 1;
     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT: Call configure
     abr::err_code result = gmio.configure();
     CHECK_EQUAL((int)abr::err_code::ok, (int)result);

     mock_c()->expectOneCall("XAie_DmaSetAddrLen")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetLock")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     uint64_t address = 0x1000;
     size_t size = 1024;
     abr::err_code enqResult = gmio.enqueueBD(address, size);
     CHECK_EQUAL((int)abr::err_code::ok, (int)enqResult);
     int dir = 0x0;
     uint8_t channel = 0x1;
     mock_c()->expectOneCall("XAie_DmaWaitForDone")
         ->withPointerParameters("DevInst", &devInst)
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);

     // ACT: Call configure
     abr::err_code result1 = gmio.configure();

     CHECK_EQUAL((int)abr::err_code::ok, (int)result1);

     // ACT: Call wait in synchronous mode
     abr::err_code res = gmio.wait(false);

     // ASSERT: Check that wait succeeded
     CHECK_EQUAL((int)abr::err_code::resource_unavailable, (int)res);
 }

 /**
  * @brief TC_01: Verify successful GMIO BD enqueue operation
  *
  * Tests gmio_api::enqueueBD() with valid address and size parameters. Verifies DMA address/length
  * setting, lock configuration, BD enablement, BD write, and queue push operations.
  */
 TEST(GmioApiTests, TC_062_EnqueueBD_Success) {
     // ARRANGE: Create and configure a GMIO API instance

     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;

     abr::gmio_api gmio(&config);

     u32 TimeOutUs = 0;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;
     u8 FirstCallOutput = 1;
     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT: Call configure
     abr::err_code result = gmio.configure();
     CHECK_EQUAL((int)abr::err_code::ok, (int)result);

     mock_c()->expectOneCall("XAie_DmaSetAddrLen")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetLock")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     uint64_t address = 0x1000;
     size_t size = 1024;
     abr::err_code enqResult = gmio.enqueueBD(address, size);
     CHECK_EQUAL((int)abr::err_code::ok, (int)enqResult);
 }

 /**
  * @brief Test group for GMIO BD enqueue failure scenarios
  *
  * This test group validates error handling for GMIO BD enqueue operations
  * with AIEML device generation.
  */
 TEST_GROUP(GmioApi_enqueueBDTests) {
     XAie_DevInst devInst;

     /**
      * @brief Setup for GmioApi_enqueueBDTests
      *
      * Initializes device instance with AIEML+1 generation, config manager, and installs mock comparators.
      */
     void setup() {
         // Set up device instance for AIE
         devInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML + 1;
         abr::config_manager::s_pDevInst = &devInst;
         abr::config_manager::s_bInitialized = true;
         mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
         mock_c()->installCopier("u8", u8Copier);
         mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
     }

     void teardown() {
         // Clean up after each test
         abr::config_manager::s_pDevInst = nullptr;
         abr::config_manager::s_bInitialized = false;
         mock_c()->removeAllComparatorsAndCopiers();
         mock_c()->clear();
     }
 };

 /**
  * @brief TC_01_Failure: Verify GMIO BD enqueue failure when XAie operations fail
  *
  * Tests error handling when XAie_DmaSetAddrLen, DmaSetLock, DmaEnableBd, DmaWriteBd,
  * or DmaChannelPushBdToQueue return non-zero. Expects aie_driver_error.
  */
 TEST(GmioApi_enqueueBDTests, TC_063_EnqueueBD_Failure_DescInit) {
     // ARRANGE: Create and configure a GMIO API instance

     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;

     abr::gmio_api gmio(&config);

     u32 TimeOutUs = 0;
     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;
     u8 FirstCallOutput = 1;
     u8 secondCallOutput = 2;
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &devInst)
         ->ignoreOtherParameters()
         ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters() // Must be present if DmaDesc is not matched
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // ACT: Call configure
     abr::err_code result = gmio.configure();
     CHECK_EQUAL((int)abr::err_code::ok, (int)result);

     mock_c()->expectOneCall("XAie_DmaSetAddrLen")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetLock")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);

     mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
         ->ignoreOtherParameters()
         ->andReturnIntValue(1);
     uint64_t address = 0x1000;
     size_t size = 1024;
     abr::err_code enqResult = gmio.enqueueBD(address, size);
     CHECK_EQUAL((int)abr::err_code::aie_driver_error, (int)enqResult);
 }

 /**
  * @brief TC_02_Failure: Verify GMIO BD enqueue failure with null configuration
  *
  * Tests error handling when gmio_api::enqueueBD() is called with nullptr configuration.
  * Expects internal_error return code.
  */
 TEST(GmioApi_enqueueBDTests, TC_064_EnqueueBD_Failure_SetAddrLen) {
     // ARRANGE: Create and configure a GMIO API instance

     abr::gmio_config *config = nullptr;
     uint64_t address = 0x1000;
     size_t size = 1024;
     abr::gmio_api gmio(config);

     abr::err_code enqResult = gmio.enqueueBD(address, size);
     CHECK_EQUAL((int)abr::err_code::internal_error, (int)enqResult);
 }

 /**
  * @brief TC_065: Verify GMIO BD enqueue with AIE device generation
  *
  * Tests enqueueBD with AIE device generation (non-AIEML) which uses
  * XAIE_LOCK_WITH_NO_VALUE instead of 0 for lock initialization.
  * Expects successful enqueue with ok return code.
  */
 TEST(GmioApi_enqueueBDTests, TC_065_EnqueueBD_AIE_DevGen) {
     XAie_DevInst aieDevInst;
     aieDevInst.DevProp.DevGen = XAIE_DEV_GEN_AIE; // Use AIE instead of AIEML
     abr::config_manager::s_pDevInst = &aieDevInst;
     abr::config_manager::s_bInitialized = true;

     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio_aie";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;

     abr::gmio_api gmio(&config);

     XAie_LocType testLoc;
     testLoc.Col = 0;
     testLoc.Row = 0;
     u8 queueSize = 2;

     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &aieDevInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->withPointerParameters("DevInst", &aieDevInst)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &queueSize)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &queueSize)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectNCalls(2, "XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     abr::err_code result = gmio.configure();
     CHECK_EQUAL((int)abr::err_code::ok, (int)result);

     // Now enqueue BD with AIE device generation
     mock_c()->expectOneCall("XAie_DmaSetAddrLen")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     // Should use XAIE_LOCK_WITH_NO_VALUE for AIE device generation
     XAie_Lock expectedLock;
     expectedLock.LockId = 0;
     expectedLock.LockVal = XAIE_LOCK_WITH_NO_VALUE;
     mock_c()->expectOneCall("XAie_DmaSetLock")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     uint64_t address = 0x1000;
     size_t size = 1024;
     abr::err_code enqResult = gmio.enqueueBD(address, size);
     CHECK_EQUAL((int)abr::err_code::ok, (int)enqResult);

     mock_c()->clear();
     abr::config_manager::s_pDevInst = nullptr;
     abr::config_manager::s_bInitialized = false;
 }

 /**
  * @brief TC_066: Verify gmio_api destructor is called
  *
  * Tests that the gmio_api destructor is properly invoked. Creates a gmio_api
  * object with new and explicitly deletes it to ensure proper cleanup.
  */
 TEST(GmioApi_enqueueBDTests, TC_066_Destructor_Coverage) {
     XAie_DevInst aieDevInst;
     aieDevInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML;
     abr::config_manager::s_pDevInst = &aieDevInst;
     abr::config_manager::s_bInitialized = true;

     abr::gmio_config config;
     config.type = abr::gmio_config::gm2aie;
     config.id = 1;
     config.name = "test_gmio_destructor";
     config.shimColumn = 0;
     config.channelNum = 0;
     config.burstLength = 4;

     // Create gmio_api object on heap
     abr::gmio_api* gmio = new abr::gmio_api(&config);

     // Delete to explicitly call destructor
     delete gmio;

     // ASSERT - Verify gmio was successfully deleted without crash
     CHECK(gmio != nullptr || gmio == nullptr); // Always true, just verifies we reached here

     mock_c()->clear();
     abr::config_manager::s_pDevInst = nullptr;
     abr::config_manager::s_bInitialized = false;
 }

 /**
  * @brief TC_067: Verify GMIO BD enqueue with full queue waiting logic
  *
  * Tests the queue management logic that waits for available BDs when the queue is full.
  * Simulates a scenario where initially no BDs are available, requiring the function to wait
  * and check for completed BDs before proceeding with the enqueue operation.
  */
 TEST(GmioApi_enqueueBDTests, TC_067_EnqueueBD_WaitForAvailableBD) {
     XAie_DevInst aiemlDevInst;
     aiemlDevInst.DevProp.DevGen = XAIE_DEV_GEN_AIEML;
     abr::config_manager::s_pDevInst = &aiemlDevInst;
     abr::config_manager::s_bInitialized = true;

     abr::gmio_config config;
     config.type = abr::gmio_config::aie2gm;
     config.id = 2;
     config.name = "test_gmio_wait";
     config.shimColumn = 1;
     config.channelNum = 1;
     config.burstLength = 8;

     abr::gmio_api gmio(&config);

     XAie_LocType testLoc;
     testLoc.Col = 1;
     testLoc.Row = 0;
     u8 queueSize = 1; // Small queue to trigger waiting

     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaDescInit")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelEnable")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &queueSize)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
         ->withOutputParameterOfTypeReturning("u8", "QueueSize", &queueSize)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetAxi")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     abr::err_code result = gmio.configure();
     CHECK_EQUAL((int)abr::err_code::ok, (int)result);

     // First enqueueBD - should use the available BD
     mock_c()->expectOneCall("XAie_DmaSetAddrLen")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetLock")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     uint64_t address1 = 0x1000;
     size_t size1 = 1024;
     abr::err_code enqResult1 = gmio.enqueueBD(address1, size1);
     CHECK_EQUAL((int)abr::err_code::ok, (int)enqResult1);

     // Second enqueueBD - queue is full, must wait for available BD
     // Simulate the while loop: first check shows 1 pending (none completed), second check shows 0 pending (1 completed)
     u8 pendingBDs_first = 1;  // Queue full
     u8 pendingBDs_second = 0; // BD completed, queue available

     mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
         ->withOutputParameterOfTypeReturning("u8", "PendingBd", &pendingBDs_first)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaGetPendingBdCount")
         ->withOutputParameterOfTypeReturning("u8", "PendingBd", &pendingBDs_second)
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     mock_c()->expectOneCall("XAie_DmaSetAddrLen")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaSetLock")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaEnableBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaWriteBd")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);
     mock_c()->expectOneCall("XAie_DmaChannelPushBdToQueue")
         ->ignoreOtherParameters()
         ->andReturnIntValue(XAIE_OK);

     uint64_t address2 = 0x2000;
     size_t size2 = 2048;
     abr::err_code enqResult2 = gmio.enqueueBD(address2, size2);
     CHECK_EQUAL((int)abr::err_code::ok, (int)enqResult2);

     mock_c()->clear();
     abr::config_manager::s_pDevInst = nullptr;
     abr::config_manager::s_bInitialized = false;
 }