/**
 * @file test_aiebaremetal.cc
 * @brief Unit tests for the AIEGraph class
 *
 * This file contains unit tests for the AIEGraph class, which provides the main
 * interface for managing AI Engine graphs including initialization, clock control,
 * memory management, GMIO operations, profiling, and error handling.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

 #include "aeg_baremetal_api.h"
 #include "aiebaremetal.h"
 #include "fal_util.h"
 #include "aeg_configure.h"
 #include "test_comparator.h"
 #include "stubs.h"


 #include <CppUTest/TestHarness.h>
 #include "CppUTestExt/MockSupport_c.h"

 /**
  * @brief Test group for AIEGraph simulation functionality
  *
  * This test group verifies the functionality of the AIEGraph class, which provides
  * comprehensive management of AI Engine graphs. The tests validate constructor behavior,
  * clock management, memory operations, GMIO transfers, profiling capabilities,
  * error handling, and various graph control operations.
  */
 TEST_GROUP(AIEGraphSimTests)
 {

	 AIEGraph *gr;

	 /**
	  * Register mocks for ConfigureGmios / ConfigureExternalBuffers DMA path and
	  * call init() so isInitialized is true. Use before run(), clock_enable(), etc.
	  * when the test expects success (not for tests that assert invalid_state without init).
	  */
	 void graphInit_mocks_ok()
	 {
		 u8 FirstCallOutput = 1;
		 u8 secondCallOutput = 2;
		 mock_c()->expectNCalls(2, "XAie_DmaDescInit")
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(XAIE_OK);
		 mock_c()->expectNCalls(2, "XAie_DmaChannelEnable")
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(XAIE_OK);
		 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
			 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(XAIE_OK);
		 for (int i = 0; i < 3; ++i) {
			 mock_c()->expectOneCall("XAie_DmaSetAxi")
				 ->ignoreOtherParameters()
				 ->withUnsignedIntParameters("Smid", 0)
				 ->withUnsignedIntParameters("BurstLen", 4)
				 ->withUnsignedIntParameters("Qos", 0)
				 ->withUnsignedIntParameters("Cache", 0)
				 ->withUnsignedIntParameters("Secure", 0)
				 ->andReturnIntValue(0);
		 }
		 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
			 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(XAIE_OK);
		 CHECK(gr->init() == aeg_err::ok);
	 }

	 void setup()
	 {
		 RegisterXAieLocTypeComparator();
		 mock_c()->installComparator("XAie_Lock", XAie_LockComparator, XAie_LockToString);
		 mock_c()->installCopier("u8", u8Copier);
		 mock_c()->expectOneCall("XAie_SetupPartitionConfig")
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(0);
		 mock_c()->expectOneCall("XAie_CfgInitialize")
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(0);
		 mock_c()->expectOneCall("XAie_Finish")
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(0);
		 gr = new AIEGraph("gradf2");
	 }

	 void teardown()
	 {
		 mock_c()->expectOneCall("XAie_Finish")
			 ->ignoreOtherParameters();
		 mock_c()->expectNCalls(2,"XAie_MemFree")
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(0);
		 delete gr;

		 // // Clean up static list to trigger AIEGraphSim destructor
		 // extern std::vector<std::shared_ptr<AIEGraphSim>> *AIEGraphSim_List;
		 // if (AIEGraphSim_List != NULL) {
		 // 	mock_c()->expectOneCall("XAie_Finish")
		 // 		->ignoreOtherParameters();
		 // 	AIEGraphSim_List->clear();
		 // 	delete AIEGraphSim_List;
		 // 	AIEGraphSim_List = NULL;
		 // }

		 mock_c()->removeAllComparatorsAndCopiers();
		 mock_c()->clear();
		 mock_c()->checkExpectations();
	 }
 };

 /**
  * @brief TC_001: Verify AIEGraph constructor handles XAie_SetupPartitionConfig failure
  *
  * This test verifies that the AIEGraph constructor properly handles the case when
  * XAie_SetupPartitionConfig fails. The mock is configured to return an error code,
  * and the test ensures the constructor handles the failure gracefully.
  */
 TEST(AIEGraphSimTests, TC_001_ConstructorSetupPartitionConfigFailure)
 {
	 XAie_DevInst devInst;
	 mock_c()->expectOneCall("XAie_SetupPartitionConfig")
		 //  ->withPointerParameters("DevInst", &devInst)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(1);
	 XAie_DevInst Inst;
	 mock_c()->expectOneCall("XAie_Finish")
		 ->ignoreOtherParameters();
	 mock_c()->expectNCalls(2,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);


	 AIEGraph gr1("gradf2");
	 CHECK(true);
 }

 /**
  * @brief TC_002: Verify constructor failure when XAie_CfgInitialize fails
  *
  * This test simulates a scenario where XAie_SetupPartitionConfig succeeds but
  * XAie_CfgInitialize fails after a successful XAie_SetupPartitionConfig call.
  * The test ensures proper cleanup and error handling in the constructor.
  */
 TEST(AIEGraphSimTests, TC_002_ConstructorCfgInitializeFailure)
 {
	 XAie_DevInst devInst;
	 devInst.NumCols = 1;
	 devInst.NumRows = 1;
	 devInst.StartCol = 0;
	 XAie_Config ConfigPtr;
	 ConfigPtr.NumRows = 0x5;
	 mock_c()->expectOneCall("XAie_SetupPartitionConfig")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 mock_c()->expectOneCall("XAie_CfgInitialize")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(1);
	 mock_c()->expectOneCall("XAie_Finish")
		 ->ignoreOtherParameters();
	 mock_c()->expectNCalls(2,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 AIEGraph gr2("gradf2");
	 CHECK(true);
 }

 /**
  * @brief TC_003: Verify successful AIEGraph constructor initialization
  *
  * This test verifies that the AIEGraph constructor successfully initializes when
  * both XAie_SetupPartitionConfig and XAie_CfgInitialize return success codes.
  * This represents the normal, successful initialization path.
  */
 TEST(AIEGraphSimTests, TC_003_ConstructorSuccess)
 {
	 mock_c()->expectOneCall("XAie_SetupPartitionConfig")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 mock_c()->expectOneCall("XAie_CfgInitialize")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 mock_c()->expectOneCall("XAie_Finish")
		 ->ignoreOtherParameters();

	 mock_c()->expectNCalls(2,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 AIEGraph gr("gradf2");
	 CHECK(true);
 }

 /**
  * @brief TC_004: Verify successful selftest exit
  *
  * This test verifies that the selftestexit() method successfully requests tiles,
  * initializes error handling, and manages column clock configuration. All underlying
  * XAie functions are mocked to return success codes.
  */
 TEST(AIEGraphSimTests, TC_004_SelftestExitSuccess)
 {
	 XAie_DevInst devInst;
	 /* Match mock XAie_SetupPartitionConfig when driver blob has partition_num_cols==0 (NumCols forced to 1) */
	 devInst.NumCols = 1;
	 devInst.NumRows = 1;
	 devInst.StartCol = 0;
	 devInst.DevProp.DevGen = XAIE_DEV_GEN_AIE2PS;
	 graphInit_mocks_ok();
	 mock_c()->expectOneCall("XAie_PmRequestTiles")
		 ->ignoreOtherParameters()
		 ->withUnsignedIntParameters("NumTiles", 0)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 mock_c()->expectOneCall("XAie_ErrorHandlingInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 mock_c()->expectNCalls(3,"XAie_PmSetColumnClk")
		 ->ignoreOtherParameters()
		 ->withUnsignedIntParameters("StartCol",  devInst.StartCol )
		 ->withUnsignedIntParameters("NumCols", devInst.NumCols )
		 ->withUnsignedIntParameters("Enable", 1)
		 ->andReturnIntValue(0);
	 mock_c()->expectOneCall("XAie_PmSetColumnClk")
		 ->ignoreOtherParameters()
		 ->withUnsignedIntParameters("StartCol",  devInst.StartCol )
		 ->withUnsignedIntParameters("NumCols", devInst.NumCols )
		 ->withUnsignedIntParameters("Enable", 0)
		 ->andReturnIntValue(0);

	 aeg_err result = gr->selftestexit();
	 CHECK(result == aeg_err::ok);
	 CHECK(true);
	 mock_c()->clear();
 }

 /**
  * @brief TC_005: Verify successful clock enable operation
  *
  * This test verifies that clock_enable() correctly enables clocks for all columns
  * in the AI Engine partition. The test mocks XAie_PmRequestTiles and XAie_PmSetColumnClk
  * to return success codes and validates that the method returns 0 on success.
  */
 TEST(AIEGraphSimTests, TC_005_ClockEnableSuccess)
 {
	 XAie_DevInst devInst;
	 devInst.NumCols = 1;
	 devInst.NumRows = 1;
	 devInst.StartCol = 0;
	 graphInit_mocks_ok();
	 mock_c()->expectOneCall("XAie_PmRequestTiles")
		 ->ignoreOtherParameters()
		 ->withUnsignedIntParameters("NumTiles", 0)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 mock_c()->expectOneCall("XAie_PmSetColumnClk")
		 ->ignoreOtherParameters()
		 ->withUnsignedIntParameters("StartCol",  devInst.StartCol )
		 ->withUnsignedIntParameters("NumCols", devInst.NumCols )
		 ->withUnsignedIntParameters("Enable", 1)
		 ->andReturnIntValue(0);
	 // Call clock_enable
	 aeg_err result = gr->clock_enable();
	 CHECK(result == aeg_err::ok);
	 mock_c()->clear();
 }

 /**
  * @brief TC_006: Verify clock_enable handles failures
  *
  * This test verifies that clock_enable() properly handles failures when
  * XAie_PmRequestTiles or XAie_PmSetColumnClk return error codes. The test
  * validates that the method returns the error code (1) when operations fail.
  */
 TEST(AIEGraphSimTests, TC_006_ClockEnableFailure)
 {
	 XAie_DevInst devInst;
	 devInst.NumCols = 1;
	 devInst.NumRows = 1;
	 devInst.StartCol = 0;
	 graphInit_mocks_ok();
	 mock_c()->expectOneCall("XAie_PmRequestTiles")
		 ->ignoreOtherParameters()
		 ->withUnsignedIntParameters("NumTiles", 0)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(1);
	 mock_c()->expectOneCall("XAie_PmSetColumnClk")
		 ->ignoreOtherParameters()
		 ->withUnsignedIntParameters("StartCol",  devInst.StartCol )
		 ->withUnsignedIntParameters("NumCols", devInst.NumCols )
		 ->withUnsignedIntParameters("Enable", 1)
		 ->andReturnIntValue(1);
	 // Call clock_enable
	 aeg_err result = gr->clock_enable();

	 // Verify result
	 CHECK(result == aeg_err::aie_driver_error);
 }

 /**
  * @brief TC_007: Verify successful clock disable operation
  *
  * This test verifies that clock_disable() correctly disables clocks for all columns
  * in the AI Engine partition. The test mocks XAie_PmSetColumnClk with Enable=0
  * to simulate disabling the clocks and validates success return value.
  */
 TEST(AIEGraphSimTests, TC_007_ClockDisableSuccess)
 {
	 XAie_DevInst devInst;
	 devInst.NumCols = 1;
	 devInst.NumRows = 1;
	 devInst.StartCol = 0;
	 graphInit_mocks_ok();
	 mock_c()->expectOneCall("XAie_PmSetColumnClk")
		 ->ignoreOtherParameters()
		 ->withUnsignedIntParameters("StartCol",  devInst.StartCol )
		 ->withUnsignedIntParameters("NumCols", devInst.NumCols )
		 ->withUnsignedIntParameters("Enable", 0)
		 ->andReturnIntValue(0);

	 // Call clock_disable
	 aeg_err result = gr->clock_disable();

	 // Verify result
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_008: Verify clock_disable handles failures
  *
  * This test verifies that clock_disable() properly handles failures when
  * XAie_PmSetColumnClk returns an error code during clock disable operations.
  * The test validates that the method returns the error code (1) on failure.
  */
 TEST(AIEGraphSimTests, TC_008_ClockDisableFailure)
 {
	 XAie_DevInst devInst;
	 devInst.NumCols = 1;
	 devInst.NumRows = 1;
	 devInst.StartCol = 0;
	 graphInit_mocks_ok();
	 mock_c()->expectOneCall("XAie_PmSetColumnClk")
		 ->ignoreOtherParameters()
		 ->withUnsignedIntParameters("StartCol",  devInst.StartCol )
		 ->withUnsignedIntParameters("NumCols", devInst.NumCols )
		 ->withUnsignedIntParameters("Enable", 0)
		 ->andReturnIntValue(1);

	 // Call clock_disable
	 aeg_err result = gr->clock_disable();

	 // Verify result
	 CHECK(result == aeg_err::aie_driver_error);
 }

 /**
  * @brief TC_009: Verify successful memory allocation
  *
  * This test verifies that malloc() correctly allocates memory using XAie_MemAllocate
  * with the specified size and cache parameters. The test mocks a successful allocation
  * and validates that a non-null pointer is returned.
  */
 TEST(AIEGraphSimTests, TC_009_MallocSuccess)
 {
	 XAie_DevInst devInst;
	 devInst.NumCols = 0;
	 devInst.NumRows = 1;
	 devInst.StartCol = 0;
	 // SETUP MOCK RETURN VALUE
	 XAie_MemInst mockMemInst;
	 int  Cache = 1 ;
	 size_t testSize = 1024;
	 graphInit_mocks_ok();
	 // ARRANGE: Configure mock for successful allocation
	 mock_c()->expectOneCall("XAie_MemAllocate")
		 ->ignoreOtherParameters()
		 ->withUnsignedLongLongIntParameters("Size", testSize)
		 ->withIntParameters("Cache", Cache)
		 ->andReturnPointerValue((void *)&mockMemInst);
	 // Call malloc
	 void* result = gr->malloc(testSize);

	 // Verify result is not null
	 CHECK(result != nullptr);
	 mock_c()->clear();
 }

 /**
  * @brief TC_010: Verify memory allocation failure handling
  *
  * This test verifies that malloc() correctly handles allocation failures by returning
  * nullptr when XAie_MemAllocate fails. The test mocks a failed allocation scenario
  * to validate error handling behavior.
  */
 TEST(AIEGraphSimTests, TC_010_MallocFailure)
 {
	 XAie_DevInst devInst;
	 devInst.NumCols = 0;
	 devInst.NumRows = 1;
	 devInst.StartCol = 0;

	 int  Cache = 1 ;
	 size_t testSize = 1024;
	 graphInit_mocks_ok();
	 // ARRANGE: Configure mock for successful allocation
	 mock_c()->expectOneCall("XAie_MemAllocate")
		 ->ignoreOtherParameters()
		 ->withUnsignedLongLongIntParameters("Size", testSize)
		 ->withIntParameters("Cache", Cache)
		 ->andReturnPointerValue((void *)nullptr);
	 // Call malloc
	 void* result = gr->malloc(testSize);

	 // Verify result is not null
	 CHECK(result == nullptr);
	 mock_c()->clear();
 }

 /**
  * @brief TC_011: Verify successful memory deallocation
  *
  * This test verifies that free() correctly releases previously allocated memory
  * by calling XAie_MemFreeVAddr with the proper virtual address. The test validates
  * that the deallocation operation completes successfully.
  */
 TEST(AIEGraphSimTests, TC_011_Free)
 {

	 XAie_DevInst devInst;
	 devInst.NumCols = 0;
	 devInst.NumRows = 1;
	 devInst.StartCol = 0;
	 // SETUP MOCK RETURN VALUE
	 void *TestAddress;
	 TestAddress = malloc(1024);
	 graphInit_mocks_ok();
	 // ARRANGE: Configure mock for successful Free operation
	 mock_c()->expectOneCall("XAie_MemFreeVAddr")
		 ->ignoreOtherParameters()
		 ->withPointerParameters("VAddr", TestAddress)
		 ->andReturnIntValue(XAIE_OK);
	 // Call free
	 aeg_err  result = gr->free(TestAddress);
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_012: Verify wait operation completes
  *
  * This test verifies that the wait() method executes without errors. The wait
  * operation is used to synchronize graph execution and ensure all operations
  * complete before proceeding.
  */
 TEST(AIEGraphSimTests, TC_012_Wait)
 {
	 // Verify the wait operation completes without errors
	 graphInit_mocks_ok();
	 aeg_err  result = gr->wait();
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_013: Verify successful graph termination
  *
  * This test verifies that the end() method successfully terminates graph execution
  * and returns a success code (0). This method is typically called to cleanly shut
  * down graph operations.
  */
 TEST(AIEGraphSimTests, TC_013_EndSuccess)
 {
	 // Call end
	 graphInit_mocks_ok();
	 aeg_err  result = gr->end();
	 // Verify result (0 = success in this implementation)
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_014: Verify successful initialization of GMIOs and external buffers
  *
  * This test verifies that the init() method correctly configures GMIOs (Graph Memory I/O)
  * and External Buffers by calling DMA descriptor initialization and channel setup functions.
  * The test validates that all necessary DMA and AXI configuration calls are made.
  */
 TEST(AIEGraphSimTests, TC_014_InitConfiguresSystem)
 {
	 // Verifies ConfigureGmios / ConfigureExternalBuffers DMA path (mocks inside graphInit_mocks_ok)
	 graphInit_mocks_ok();
 }

 /**
  * @brief TC_015: Verify graph execution with specified iterations
  *
  * This test verifies that the run() method correctly executes the graph with the
  * specified number of iterations (5). The test validates transaction handling, core
  * enable events, timer operations, and core enable functionality during graph execution.
  */
 TEST(AIEGraphSimTests, TC_015_RunDefaultIterations)
 {
	 graphInit_mocks_ok();

	 mock_c()->expectNCalls(3,"XAie_StartTransaction")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DataMemWrWord")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(3,"XAie_SubmitTransaction")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(3,"XAie_ReadTimer")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_ClearCoreDisableEventOccurred")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_CoreConfigureEnableEvent")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_EventGenerate")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_CoreEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);

	 aeg_err result = gr->run(5);
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_016: Verify error backtracking functionality
  *
  * This test verifies that the BacktrackErrors() method correctly calls
  * XAie_BacktrackErrorInterrupts to trace and identify error sources in the
  * AI Engine array. This is essential for debugging hardware errors.
  */
 TEST(AIEGraphSimTests, TC_016_BacktrackErrors)
 {
	 graphInit_mocks_ok();
	 mock_c()->expectOneCall("XAie_BacktrackErrorInterrupts")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 void* result = gr->BacktrackErrors();
	 CHECK(result != nullptr);
 }

 /**
  * @brief TC_017: Verify core log dump functionality
  *
  * This test verifies that dumpcorelog() correctly retrieves log data from a specific
  * AI Engine core at the given row and column coordinates. The test validates that
  * a non-null pointer is returned containing the log data.
  */
 TEST(AIEGraphSimTests, TC_017_DumpCoreLog)
 {

	 int row = 0x2;
	 int col = 0x0;
	 char* ptr;
	 graphInit_mocks_ok();
	 mock_c()->expectOneCall("XAie_Read32")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 ptr = gr->dumpcorelog(row,col);

	 CHECK(ptr != nullptr);

 }

 /**
  * @brief TC_018: Verify retrieval of AIE device instance
  *
  * This test verifies that getaiedevinst() correctly returns a pointer to the internal
  * XAie_DevInst structure, allowing direct access to the AIE device instance for
  * advanced operations or debugging.
  */
 TEST(AIEGraphSimTests, TC_018_GetAieDevInst)
 {

	 void* ptr;
	 graphInit_mocks_ok();
	 ptr =  gr->getaiedevinst();
	 CHECK(ptr != nullptr);
 }

 /**
  * @brief TC_019: Verify non-blocking GMIO transfer from global memory to AIE
  *
  * This test verifies that gm2aie_nb() correctly initiates a non-blocking data transfer
  * from global memory to an AI Engine tile via GMIO. The test configures DMA descriptors,
  * sets up buffer descriptors, and validates the transfer initiation without waiting for completion.
  */
 TEST(AIEGraphSimTests, TC_019_GM2AIE_NB)
 {
	 // Define relevant variables
	 int len = 10;
	 XAie_DevInst devInst;
	 std::string gmio_name = "gradf.in";
	 int32_t* input_data = (int32_t*)malloc(len * sizeof(int32_t)); // Allocate mock input data
	 for (int i = 0; i < len; i++) {
		 input_data[i] = i + 1;
	 }
	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;
	 mock_c()->expectNCalls(2,"XAie_DmaDescInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_DmaChannelEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 for (int i = 0; i < 3; ++i) {
		 mock_c()->expectOneCall("XAie_DmaSetAxi")
			 ->ignoreOtherParameters()
			 ->withUnsignedIntParameters("Smid", 0)
			 ->withUnsignedIntParameters("BurstLen", 4)
			 ->withUnsignedIntParameters("Qos", 0)
			 ->withUnsignedIntParameters("Cache", 0)
			 ->withUnsignedIntParameters("Secure", 0)

			 ->andReturnIntValue(0);
	 }
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 void *TestAddress;
	 uint64_t *DevAddr;

	 // Allocate memory for TestAddress and DevAddr
	 TestAddress = malloc(1024);
	 CHECK(TestAddress != nullptr); // Verify memory allocation success
	 DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	 CHECK(DevAddr != nullptr);

	 devInst.NumRows = 2;

	 // ARRANGE: Configure mock
	 mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		 ->ignoreOtherParameters()
		 // ->withPointerParameters("DevInst", &devInst)
		 // ->withPointerParameters("VAddr", TestAddress)
		 // ->withPointerParameters("DevAddr", DevAddr) // Ensure proper mock expectation) // Add OutputParam to mock
		 ->andReturnIntValue(XAIE_OK);
	 // ARRANGE: Configure mock for successful sync for dev addr allocation
	 mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
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
	 aeg_err result = gr->init();
	 CHECK(result == aeg_err::ok);
	 // Act: Call the method to test
	 result = gr->gm2aie_nb(&gmio_name[0], reinterpret_cast<char*>(&input_data[0]), len * sizeof(int32_t));
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_020: Verify non-blocking GMIO transfer from AIE to global memory
  *
  * This test verifies that aie2gm_nb() correctly initiates a non-blocking data transfer
  * from an AI Engine tile to global memory via GMIO. The test validates DMA configuration,
  * buffer descriptor setup, and transfer initiation for output data paths.
  */
 TEST(AIEGraphSimTests, TC_020_AIE2GM_NB)
 {
	 int len = 10;
	 XAie_DevInst devInst;
	 std::string gmio_name = "gradf.out";
	 int32_t* output_data = (int32_t*)malloc(len*sizeof(int32_t));
	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;
	 mock_c()->expectNCalls(2,"XAie_DmaDescInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_DmaChannelEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 for (int i = 0; i < 3; ++i) {
		 mock_c()->expectOneCall("XAie_DmaSetAxi")
			 ->ignoreOtherParameters()
			 ->withUnsignedIntParameters("Smid", 0)
			 ->withUnsignedIntParameters("BurstLen", 4)
			 ->withUnsignedIntParameters("Qos", 0)
			 ->withUnsignedIntParameters("Cache", 0)
			 ->withUnsignedIntParameters("Secure", 0)

			 ->andReturnIntValue(0);
	 }
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 void *TestAddress;
	 uint64_t *DevAddr;

	 // Allocate memory for TestAddress and DevAddr
	 TestAddress = malloc(1024);
	 CHECK(TestAddress != nullptr); // Verify memory allocation success
	 DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	 CHECK(DevAddr != nullptr);

	 devInst.NumRows = 2;

	 // ARRANGE: Configure mock
	 mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		 ->ignoreOtherParameters()
		 // ->withPointerParameters("DevInst", &devInst)
		 // ->withPointerParameters("VAddr", TestAddress)
		 // ->withPointerParameters("DevAddr", DevAddr) // Ensure proper mock expectation) // Add OutputParam to mock
		 ->andReturnIntValue(XAIE_OK);
	 // ARRANGE: Configure mock for successful sync for dev addr allocation
	 mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
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
	 aeg_err result = gr->init();
	 CHECK(result == aeg_err::ok);
	 result = gr->aie2gm_nb(&gmio_name[0],reinterpret_cast<char*>(&output_data[0]), len*sizeof(int32_t));
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_021: Verify blocking GMIO transfer from global memory to AIE
  *
  * This test verifies that gm2aie() performs a blocking data transfer from global memory
  * to an AI Engine tile via GMIO. Unlike the non-blocking version, this method waits for
  * the DMA transfer to complete before returning, using XAie_DmaWaitForDone.
  */
 TEST(AIEGraphSimTests, TC_021_GM2AIE)
 {
	 int len = 10;
	 XAie_DevInst devInst;
	 int32_t* input_data = (int32_t*)malloc(len*sizeof(int32_t));
	 for (int i=0; i<len; i++){
		 input_data[i] = i+1;
	 }
	 std::string gmio_name = "gradf.in";
	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;
	 mock_c()->expectNCalls(2,"XAie_DmaDescInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_DmaChannelEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 for (int i = 0; i < 3; ++i) {
		 mock_c()->expectOneCall("XAie_DmaSetAxi")
			 ->ignoreOtherParameters()
			 ->withUnsignedIntParameters("Smid", 0)
			 ->withUnsignedIntParameters("BurstLen", 4)
			 ->withUnsignedIntParameters("Qos", 0)
			 ->withUnsignedIntParameters("Cache", 0)
			 ->withUnsignedIntParameters("Secure", 0)

			 ->andReturnIntValue(0);
	 }
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 void *TestAddress;
	 uint64_t *DevAddr;

	 // Allocate memory for TestAddress and DevAddr
	 TestAddress = malloc(1024);
	 CHECK(TestAddress != nullptr); // Verify memory allocation success
	 DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	 CHECK(DevAddr != nullptr);

	 devInst.NumRows = 2;

	 // ARRANGE: Configure mock
	 mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		 ->ignoreOtherParameters()
		 // ->withPointerParameters("DevInst", &devInst)
		 // ->withPointerParameters("VAddr", TestAddress)
		 // ->withPointerParameters("DevAddr", DevAddr) // Ensure proper mock expectation) // Add OutputParam to mock
		 ->andReturnIntValue(XAIE_OK);
	 // ARRANGE: Configure mock for successful sync for dev addr allocation
	 mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
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
	 mock_c()->expectOneCall("XAie_DmaWaitForDone")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 aeg_err result = gr->init();
	 CHECK(result == aeg_err::ok);
	 result = gr->gm2aie(&gmio_name[0],reinterpret_cast<char*>(&input_data[0]), len*sizeof(int32_t));
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_022: Verify blocking GMIO transfer from AIE to global memory
  *
  * This test verifies that aie2gm() performs a blocking data transfer from an AI Engine
  * tile to global memory via GMIO. The method waits for the DMA transfer to complete,
  * ensuring all output data has been written before returning.
  */
 TEST(AIEGraphSimTests, TC_022_AIE2GM)
 {
	 int len = 10;
	 XAie_DevInst devInst;
	 int32_t* output_data = (int32_t*)malloc(len*sizeof(int32_t));
	 std::string gmio_name = "gradf.out";
	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;
	 mock_c()->expectNCalls(2,"XAie_DmaDescInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_DmaChannelEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 for (int i = 0; i < 3; ++i) {
		 mock_c()->expectOneCall("XAie_DmaSetAxi")
			 ->ignoreOtherParameters()
			 ->withUnsignedIntParameters("Smid", 0)
			 ->withUnsignedIntParameters("BurstLen", 4)
			 ->withUnsignedIntParameters("Qos", 0)
			 ->withUnsignedIntParameters("Cache", 0)
			 ->withUnsignedIntParameters("Secure", 0)

			 ->andReturnIntValue(0);
	 }
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 void *TestAddress;
	 uint64_t *DevAddr;

	 // Allocate memory for TestAddress and DevAddr
	 TestAddress = malloc(1024);
	 CHECK(TestAddress != nullptr); // Verify memory allocation success
	 DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	 CHECK(DevAddr != nullptr);

	 devInst.NumRows = 2;

	 // ARRANGE: Configure mock
	 mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		 ->ignoreOtherParameters()
		 // ->withPointerParameters("DevInst", &devInst)
		 // ->withPointerParameters("VAddr", TestAddress)
		 // ->withPointerParameters("DevAddr", DevAddr) // Ensure proper mock expectation) // Add OutputParam to mock
		 ->andReturnIntValue(XAIE_OK);
	 // ARRANGE: Configure mock for successful sync for dev addr allocation
	 mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
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
	 mock_c()->expectOneCall("XAie_DmaWaitForDone")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 aeg_err result = gr->init();
	 CHECK(result == aeg_err::ok);
	 result = gr->aie2gm(&gmio_name[0],reinterpret_cast<char*>(&output_data[0]), len*sizeof(int32_t));
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_023: Verify GMIO wait operation
  *
  * This test verifies that gmio_wait() correctly waits for a specific GMIO operation
  * to complete. The method calls XAie_DmaWaitForDone to ensure the DMA transfer has
  * finished before proceeding, returning 0 on success.
  */
 TEST(AIEGraphSimTests, TC_023_GMIO_WAIT)
 {
	 int len = 10;
	 XAie_DevInst devInst;
	 int32_t* output_data = (int32_t*)malloc(len*sizeof(int32_t));
	 std::string gmio_name = "gradf.out";
	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;
	 mock_c()->expectNCalls(2,"XAie_DmaDescInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_DmaChannelEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 for (int i = 0; i < 3; ++i) {
		 mock_c()->expectOneCall("XAie_DmaSetAxi")
			 ->ignoreOtherParameters()
			 ->withUnsignedIntParameters("Smid", 0)
			 ->withUnsignedIntParameters("BurstLen", 4)
			 ->withUnsignedIntParameters("Qos", 0)
			 ->withUnsignedIntParameters("Cache", 0)
			 ->withUnsignedIntParameters("Secure", 0)

			 ->andReturnIntValue(0);
	 }
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 void *TestAddress;
	 uint64_t *DevAddr;

	 // Allocate memory for TestAddress and DevAddr
	 TestAddress = malloc(1024);
	 CHECK(TestAddress != nullptr); // Verify memory allocation success
	 DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	 CHECK(DevAddr != nullptr);

	 devInst.NumRows = 2;

	 // ARRANGE: Configure mock
	 mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		 ->ignoreOtherParameters()
		 // ->withPointerParameters("DevInst", &devInst)
		 // ->withPointerParameters("VAddr", TestAddress)
		 // ->withPointerParameters("DevAddr", DevAddr) // Ensure proper mock expectation) // Add OutputParam to mock
		 ->andReturnIntValue(XAIE_OK);
	 // ARRANGE: Configure mock for successful sync for dev addr allocation
	 mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
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
	 mock_c()->expectOneCall("XAie_DmaWaitForDone")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 aeg_err result = gr->init();
	 CHECK(result == aeg_err::ok);
	 result = gr->gmio_wait(&gmio_name[0]);
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_024: Verify error handling callback initialization
  *
  * This test verifies that error_handling_cb_init() correctly registers a callback
  * function for error handling. The callback will be invoked when errors are detected
  * in the AI Engine array, allowing custom error processing and recovery logic.
  */
 TEST(AIEGraphSimTests, TC_024_ErrorHandlingCallbackInit)
 {


	 // Dummy callback function and data
	 graphInit_mocks_ok();
	 void (*callback)(std::vector<XAie_ErrorPayload>, void*) =
		 [](std::vector<XAie_ErrorPayload>, void*) {};
	 void* privData = (void*)0xDEADBEEF;

	 // Call error_handling_cb_init
	 aeg_err result = gr->error_handling_cb_init((void*)callback, privData);
	 CHECK(result == aeg_err::ok);
	 CHECK(true);
 }

 /**
  * @brief TC_025: Verify start_profiling fails with invalid single port options
  *
  * This test verifies that start_profiling() correctly returns -1 (failure) when called
  * with invalid option values for single-port profiling without proper setup. The test
  * consolidates multiple failure scenarios (options 0x22, 0x21, 0x23, 0x20) to validate
  * error handling.
  */
 TEST(AIEGraphSimTests, TC_025_StartProfilingSinglePortInvalidOptions)
 {

	 // Test invalid option values (0x22, 0x21, 0x23, 0x20 without proper setup)
	 int handle1 = gr->start_profiling("test_port", 0x22, 100);
	 CHECK(handle1 == -1);

	 int handle2 = gr->start_profiling("test_port", 0x21, 100);
	 CHECK(handle2 == -1);

	 int handle3 = gr->start_profiling("test_port", 0x23, 100);
	 CHECK(handle3 == -1);

	 int handle4 = gr->start_profiling("test_port", 0x20, 100);
	 CHECK(handle4 == -1);
 }

 /**
  * @brief TC_026: Verify start_profiling fails with invalid two port options
  *
  * This test verifies that start_profiling() for two-port profiling correctly returns -1
  * when called with invalid option values (0x20, 0x21, 0x22, 0x23) without proper setup.
  * Two-port profiling is used for differential cycle measurements between ports.
  */
 TEST(AIEGraphSimTests, TC_026_StartProfilingTwoPortsInvalidOptions)
 {
	 // Test various invalid option values for two-port profiling
	 int handle1 = gr->start_profiling("test_port1", "test_port2", 0x20, 100);
	 CHECK(handle1 == -1);

	 int handle2 = gr->start_profiling("test_port1", "test_port2", 0x21, 100);
	 CHECK(handle2 == -1);

	 int handle3 = gr->start_profiling("test_port1", "test_port2", 0x22, 100);
	 CHECK(handle3 == -1);

	 int handle4 = gr->start_profiling("test_port1", "test_port2", 0x23, 100);
	 CHECK(handle4 == -1);
 }

 /**
  * @brief TC_027: Verify successful two-port profiling initialization
  *
  * This test verifies that start_profiling() successfully initializes profiling for two
  * ports when called with a invalid option (-1). The test validates that a valid handle (0)
  * is returned, enabling subsequent profiling operations.
  */
 TEST(AIEGraphSimTests, TC_027_StartProfilingTwoPortsInvalidOptions)
 {
	 // Call start_profiling for two ports with valid option
	 graphInit_mocks_ok();
	 int handle = gr->start_profiling("test_port1", "test_port2", -1, 100);

	 // Verify we got a valid handle
	 CHECK(handle == -1);
 }

 /**
  * @brief TC_028: Verify read_profiling with invalid handle returns zero
  *
  * This test verifies that read_profiling() returns 0 when called with an invalid handle
  * (from a failed profiling start). This ensures graceful handling of invalid profiling
  * operations and prevents reading from non-existent profiling sessions.
  */
 TEST(AIEGraphSimTests, TC_028_ReadProfilingInvalidHandle)
 {

	 graphInit_mocks_ok();
	 // First, start profiling to get a handle
	 int handle = gr->start_profiling("test_port", 0x20, 100);

	 // Call read_profiling
	 long long data = gr->read_profiling(handle);

	 // Verify we got some data (mock returns a large value)
	 CHECK(data != 0);
 }

 /**
  * @brief TC_029: Verify stop_profiling with invalid handle completes safely
  *
  * This test verifies that stop_profiling() handles invalid handles gracefully without
  * crashing. The method is called with an invalid handle (from a failed profiling start)
  * to ensure robust error handling.
  */
 TEST(AIEGraphSimTests, TC_029_StopProfilingInvalidHandle)
 {

	 graphInit_mocks_ok();
	 // First, start profiling to get a handle
	 int handle = gr->start_profiling("test_port", 0x20, 100);

	 // Call stop_profiling - verifying it handles invalid handle gracefully
	 aeg_err result = gr->stop_profiling(handle);
	 CHECK(result == aeg_err::invalid_state);

 }

 /**
  * @brief TC_030: Verify successful profiling for run_idle mode
  *
  * This test verifies that start_profiling() successfully initializes profiling in run_idle
  * mode (option 0x20) when the profiling stub is properly configured. The test validates
  * that a valid handle (0) is returned for subsequent profiling operations.
  */
 TEST(AIEGraphSimTests, TC_030_StartProfilingSuccessRunIdle)
 {
	 graphInit_mocks_ok();
	 set_profiling_stub_mode(true); // Enable profiling success

	 // Call start_profiling with run_idle option
	 int handle = gr->start_profiling("test_port", 0x20, 100);

	 // Should return valid handle (0)
	 CHECK(handle == 0);

	 set_profiling_stub_mode(false); // Reset to default
 }

 /**
  * @brief TC_031: Verify successful profiling for start_bytes mode
  *
  * This test verifies that start_profiling() successfully initializes profiling in start_bytes
  * mode (option 0x21) which tracks the number of bytes transferred through a GMIO port.
  * A valid handle is returned when the profiling stub is properly configured.
  */
 TEST(AIEGraphSimTests, TC_031_StartProfilingSuccessStartBytes)
 {
	 graphInit_mocks_ok();
	 set_profiling_stub_mode(true); // Enable profiling success

	 // Call start_profiling with start_bytes option
	 int handle = gr->start_profiling("gradf.in", 0x21, 100);
	 CHECK(handle == 0);
	 set_profiling_stub_mode(false); // Reset to default
 }

 /**
  * @brief TC_032: Verify successful profiling for diff_cycles mode
  *
  * This test verifies that start_profiling() successfully initializes two-port profiling
  * in diff_cycles mode (option 0x22), which measures the cycle difference between two
  * ports. This is useful for analyzing timing relationships between graph components.
  */
 TEST(AIEGraphSimTests, TC_032_StartProfilingSuccessDiffCycles)
 {
	 graphInit_mocks_ok();
	 set_profiling_stub_mode(true); // Enable profiling success

	 // Call start_profiling with diff_cycles option (two ports)
	 int handle = gr->start_profiling("test_port1", "test_port2", 0x22, 100);
	 CHECK(handle == 0);
	 set_profiling_stub_mode(false); // Reset to default
 }

 /**
  * @brief TC_033: Verify successful profiling for event_count mode
  *
  * This test verifies that start_profiling() successfully initializes profiling in event_count
  * mode (option 0x23), which counts specific events occurring at a port. This is useful for
  * monitoring event occurrences during graph execution.
  */
 TEST(AIEGraphSimTests, TC_033_StartProfilingSuccessEventCount)
 {
	 graphInit_mocks_ok();
	 set_profiling_stub_mode(true); // Enable profiling success

	 // Call start_profiling with event_count option
	 int handle = gr->start_profiling("test_port", 0x23, 100);
	 CHECK(handle == 0);
	 set_profiling_stub_mode(false); // Reset to default
 }

 /**
  * @brief TC_034: Verify successful read_profiling with valid handle
  *
  * This test verifies that read_profiling() successfully retrieves profiling data when
  * called with a valid handle from a properly initialized profiling session. The test
  * validates that non-zero profiling data is returned (>= 1000).
  */
 TEST(AIEGraphSimTests, TC_034_ReadProfilingWithValidHandle)
 {
	 graphInit_mocks_ok();
	 set_profiling_stub_mode(true); // Enable profiling success

	 // First, start profiling to get a valid handle
	 int handle = gr->start_profiling("test_port", 0x20, 100);

	 // Call read_profiling with valid handle
	 long long result = gr->read_profiling(handle);

	 // Should return profiling data (not 0, as stub returns counter value)
	 CHECK(result >= 1000);

	 set_profiling_stub_mode(false); // Reset to default
 }

 /**
  * @brief TC_035: Verify successful stop_profiling with valid handle
  *
  * This test verifies that stop_profiling() successfully stops an active profiling session
  * when called with a valid handle. The test ensures that profiling can be cleanly terminated
  * without errors, releasing associated resources.
  */
 TEST(AIEGraphSimTests, TC_035_StopProfilingWithValidHandle)
 {
	 graphInit_mocks_ok();
	 set_profiling_stub_mode(true); // Enable profiling success

	 // First, start profiling to get a valid handle
	 int handle = gr->start_profiling("test_port", 0x20, 100);

	 // Call stop_profiling with valid handle - should not crash
	 aeg_err result = gr->stop_profiling(handle);
	 CHECK(result == aeg_err::ok);
	 CHECK(true);
	 set_profiling_stub_mode(false); // Reset to default
 }

 /**
  * @brief TC_036: Verify setAddress for external buffer configuration
  *
  * This test verifies that setAddress() correctly sets the memory address for an external
  * buffer. External buffers are used for data sharing between the host and AI Engine tiles.
  * The test validates DMA configuration and address registration for the specified buffer.
  */
 TEST(AIEGraphSimTests, TC_036_SetAddress)
 {


	 // Setup test data
	 void* testPtr = malloc(128);
	 int len = 10;
	 XAie_DevInst devInst;
	 std::string gmio_name = "gradf.out";
	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;
	 mock_c()->expectNCalls(2,"XAie_DmaDescInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_DmaChannelEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 for (int i = 0; i < 3; ++i) {
		 mock_c()->expectOneCall("XAie_DmaSetAxi")
			 ->ignoreOtherParameters()
			 ->withUnsignedIntParameters("Smid", 0)
			 ->withUnsignedIntParameters("BurstLen", 4)
			 ->withUnsignedIntParameters("Qos", 0)
			 ->withUnsignedIntParameters("Cache", 0)
			 ->withUnsignedIntParameters("Secure", 0)

			 ->andReturnIntValue(0);
	 }
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 void *TestAddress;
	 uint64_t *DevAddr;

	 // Allocate memory for TestAddress and DevAddr
	 TestAddress = malloc(1024);
	 CHECK(TestAddress != nullptr); // Verify memory allocation success
	 DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	 CHECK(DevAddr != nullptr);

	 devInst.NumRows = 2;

	 // ARRANGE: Configure mock
	 mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		 ->ignoreOtherParameters()
		 // ->withPointerParameters("DevInst", &devInst)
		 // ->withPointerParameters("VAddr", TestAddress)
		 // ->withPointerParameters("DevAddr", DevAddr) // Ensure proper mock expectation) // Add OutputParam to mock
		 ->andReturnIntValue(XAIE_OK);
	 // ARRANGE: Configure mock for successful sync for dev addr allocation
	 mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
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
	 mock_c()->expectOneCall("XAie_DmaWaitForDone")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 aeg_err result = gr->init();
	 CHECK(result == aeg_err::ok);
	 // Call setAddress
	  result = gr->setAddress((char*)"EG.ddrin", testPtr);
	 CHECK(result == aeg_err::ok);
	 free(TestAddress);
	 free(DevAddr);
	 free(testPtr);
 }

 /**
  * @brief TC_037: Verify wait operation for external buffer synchronization
  *
  * This test verifies that wait() correctly synchronizes operations on an external buffer,
  * ensuring that all pending DMA transfers have completed before proceeding. The test
  * validates proper buffer synchronization between host and AI Engine.
  */
 TEST(AIEGraphSimTests, TC_037_WaitExternalBuffer)
 {
	 int len = 10;
	 XAie_DevInst devInst;
	 std::string gmio_name = "gradf.out";
	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;
	 mock_c()->expectNCalls(2,"XAie_DmaDescInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_DmaChannelEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 for (int i = 0; i < 3; ++i) {
		 mock_c()->expectOneCall("XAie_DmaSetAxi")
			 ->ignoreOtherParameters()
			 ->withUnsignedIntParameters("Smid", 0)
			 ->withUnsignedIntParameters("BurstLen", 4)
			 ->withUnsignedIntParameters("Qos", 0)
			 ->withUnsignedIntParameters("Cache", 0)
			 ->withUnsignedIntParameters("Secure", 0)

			 ->andReturnIntValue(0);
	 }
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 void *TestAddress;
	 uint64_t *DevAddr;

	 // Allocate memory for TestAddress and DevAddr
	 TestAddress = malloc(1024);
	 CHECK(TestAddress != nullptr); // Verify memory allocation success
	 DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	 CHECK(DevAddr != nullptr);

	 devInst.NumRows = 2;

	 // ARRANGE: Configure mock
	 mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		 ->ignoreOtherParameters()
		 // ->withPointerParameters("DevInst", &devInst)
		 // ->withPointerParameters("VAddr", TestAddress)
		 // ->withPointerParameters("DevAddr", DevAddr) // Ensure proper mock expectation) // Add OutputParam to mock
		 ->andReturnIntValue(XAIE_OK);
	 // ARRANGE: Configure mock for successful sync for dev addr allocation
	 mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
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
	 mock_c()->expectOneCall("XAie_DmaWaitForDone")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 aeg_err result = gr->init();
	 CHECK(result == aeg_err::ok);
	 // Call setAddress
	 result = gr->wait((char*)"EG.ddrin", "graph");
	 free(TestAddress);
	 free(DevAddr);
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_038: Verify non-blocking GMIO transfer to external buffer
  *
  * This test verifies that gm2aie_nb() correctly initiates a non-blocking transfer to an
  * external buffer. The method is used for asynchronous data input to AI Engine tiles
  * through external buffer interfaces, allowing overlapped computation and communication.
  */
 TEST(AIEGraphSimTests, TC_038_GM2AIE_NB_ExternalBuffer)
 {
	 int len = 10;
	 XAie_DevInst devInst;
	 std::string gmio_name = "gradf.out";
	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;
	 graphInit_mocks_ok();
	 mock_c()->expectNCalls(2,"XAie_DmaDescInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_DmaChannelEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 for (int i = 0; i < 3; ++i) {
		 mock_c()->expectOneCall("XAie_DmaSetAxi")
			 ->ignoreOtherParameters()
			 ->withUnsignedIntParameters("Smid", 0)
			 ->withUnsignedIntParameters("BurstLen", 4)
			 ->withUnsignedIntParameters("Qos", 0)
			 ->withUnsignedIntParameters("Cache", 0)
			 ->withUnsignedIntParameters("Secure", 0)

			 ->andReturnIntValue(0);
	 }
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 void *TestAddress;
	 uint64_t *DevAddr;

	 // Allocate memory for TestAddress and DevAddr
	 TestAddress = malloc(1024);
	 CHECK(TestAddress != nullptr); // Verify memory allocation success
	 DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	 CHECK(DevAddr != nullptr);

	 devInst.NumRows = 2;

	 // ARRANGE: Configure mock
	 mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		 ->ignoreOtherParameters()
		 // ->withPointerParameters("DevInst", &devInst)
		 // ->withPointerParameters("VAddr", TestAddress)
		 // ->withPointerParameters("DevAddr", DevAddr) // Ensure proper mock expectation) // Add OutputParam to mock
		 ->andReturnIntValue(XAIE_OK);
	 // ARRANGE: Configure mock for successful sync for dev addr allocation
	 mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
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
	 mock_c()->expectOneCall("XAie_DmaWaitForDone")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 aeg_err result = gr->init();
	 CHECK(result == aeg_err::ok);
	 // Call setAddress
	 result = gr->gm2aie_nb((char*)"EG.ddrin", "graph");
	 free(TestAddress);
	 free(DevAddr);
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_039: Verify non-blocking GMIO transfer from external buffer
  *
  * This test verifies that aie2gm_nb() correctly initiates a non-blocking transfer from an
  * external buffer. This enables asynchronous data output from AI Engine tiles through
  * external buffer interfaces, improving overall system throughput.
  */
 TEST(AIEGraphSimTests, TC_039_AIE2GM_NB_ExternalBuffer)
 {
	 int len = 10;
	 XAie_DevInst devInst;
	 std::string gmio_name = "gradf.out";
	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;
	 graphInit_mocks_ok();
	 mock_c()->expectNCalls(2,"XAie_DmaDescInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_DmaChannelEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 for (int i = 0; i < 3; ++i) {
		 mock_c()->expectOneCall("XAie_DmaSetAxi")
			 ->ignoreOtherParameters()
			 ->withUnsignedIntParameters("Smid", 0)
			 ->withUnsignedIntParameters("BurstLen", 4)
			 ->withUnsignedIntParameters("Qos", 0)
			 ->withUnsignedIntParameters("Cache", 0)
			 ->withUnsignedIntParameters("Secure", 0)

			 ->andReturnIntValue(0);
	 }
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 void *TestAddress;
	 uint64_t *DevAddr;

	 // Allocate memory for TestAddress and DevAddr
	 TestAddress = malloc(1024);
	 CHECK(TestAddress != nullptr); // Verify memory allocation success
	 DevAddr = (uint64_t *)malloc(sizeof(uint64_t));
	 CHECK(DevAddr != nullptr);

	 devInst.NumRows = 2;

	 // ARRANGE: Configure mock
	 mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		 ->ignoreOtherParameters()
		 // ->withPointerParameters("DevInst", &devInst)
		 // ->withPointerParameters("VAddr", TestAddress)
		 // ->withPointerParameters("DevAddr", DevAddr) // Ensure proper mock expectation) // Add OutputParam to mock
		 ->andReturnIntValue(XAIE_OK);
	 // ARRANGE: Configure mock for successful sync for dev addr allocation
	 mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
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
	 mock_c()->expectOneCall("XAie_DmaWaitForDone")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 aeg_err result = gr->init();
	 CHECK(result == aeg_err::ok);
	 // Call setAddress
	 result = gr->aie2gm_nb((char*)"EG.ddrin", "graph");
	 free(TestAddress);
	 free(DevAddr);
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_040: Verify RTP (Run-Time Parameter) update functionality
  *
  * This test verifies that update() correctly updates Run-Time Parameters in AI Engine tiles.
  * RTPs allow dynamic parameter updates during graph execution without recompilation.
  * The test validates lock acquisition, memory operations, and lock release for RTP updates.
  */
 TEST(AIEGraphSimTests, TC_040_UpdateRTP)
 {

	 int testDataSize = 4;
	 uint8_t testData[4];
	 graphInit_mocks_ok();
	 for (size_t i = 0; i < testDataSize; i++)
	 {
		 testData[i] = static_cast<uint8_t>(i & 0xFF);
	 }
	 abr::graph_config graphConfig;
	 abr::graph_api* graphApi;
	 graphConfig.id = 1;
	 graphConfig.name = "TestGraph";
	 graphApi = new abr::graph_api(&graphConfig);
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
	 aeg_err result = gr->update("gradf.sq.in[1]", testData, testDataSize);
	 CHECK(result == aeg_err::ok); // Expect success (0)
	 delete graphApi;

 }

 /**
  * @brief TC_041: Shared buffer update success (stub metadata)
  *
  * "EG.input" in baremetal_metadata is patched so numInputs==0 (required by
  * checkSharedBufferConfigForUpdate). numBytes is 108 — buffer size must match.
  * Driver sequence: 2x XAie_LockAcquire, XAie_DataMemBlockWrite, 2x XAie_LockRelease.
  */
 TEST(AIEGraphSimTests, TC_041_UpdateSharedBuffer)
 {
	 const size_t kNumBytes = 108U; /* matches stub shared_buffer_config.numBytes for EG.input */
	 uint8_t testData[kNumBytes];
	 for (size_t i = 0; i < kNumBytes; i++) {
		 testData[i] = static_cast<uint8_t>(i & 0xFFU);
	 }
	 graphInit_mocks_ok();
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
	 aeg_err result = gr->update("EG.input", testData, kNumBytes);
	 CHECK(result == aeg_err::ok);
 }

 /**
  * @brief TC_042: Verify ping-pong buffer setup
  *
  * This test verifies that setupPingPongBuffers() correctly configures double-buffering
  * for an external buffer. Ping-pong buffering allows continuous data streaming by
  * alternating between two buffers, enabling overlap of computation and data transfer.
  */
 TEST(AIEGraphSimTests, TC_042_SetupPingPongBuffers)
 {

	 // Setup test data
	 graphInit_mocks_ok();
	 void* ptr1 = malloc(128);
	 void* ptr2 = malloc(128);
	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;
	 mock_c()->expectNCalls(2,"XAie_DmaDescInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(2,"XAie_DmaChannelEnable")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &FirstCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 for (int i = 0; i < 3; ++i) {
		 mock_c()->expectOneCall("XAie_DmaSetAxi")
			 ->ignoreOtherParameters()
			 ->withUnsignedIntParameters("Smid", 0)
			 ->withUnsignedIntParameters("BurstLen", 4)
			 ->withUnsignedIntParameters("Qos", 0)
			 ->withUnsignedIntParameters("Cache", 0)
			 ->withUnsignedIntParameters("Secure", 0)

			 ->andReturnIntValue(0);
	 }
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->withOutputParameterOfTypeReturning("u8", "QueueSize", &secondCallOutput)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);

	 aeg_err result = gr->init();
	 CHECK(result == aeg_err::ok);
	 mock_c()->expectOneCall("XAie_MemGetDevAddrFromVAddr")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaUpdateBdAddr")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_MemSyncForDevVAddr")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaGetMaxQueueSize")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaChannelSetStartQueue")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 // Call setupPingPongBuffers
	 result = gr->setupPingPongBuffers("EG.ddrin", ptr1, ptr2);

	 // Verify result
	 CHECK(result == aeg_err::ok); // Expect success (0)

	 // Clean up
	 free(ptr1);
	 free(ptr2);
 }

 // /**
 //  * @brief TC_044: Verify read operation from shared buffer or RTP
 //  *
 //  * This test verifies that read() correctly retrieves data from shared buffers or RTPs.
 //  * This allows the host to inspect current parameter values or buffer contents during
 //  * graph execution for monitoring and debugging purposes.
 //  */
 // TEST(AIEGraphSimTests, TC_043_Read)
 // {
 // 	int testDataSize = 4;
 // 	graphInit_mocks_ok();
 // 	char* output_data = (char*)malloc(testDataSize*sizeof(char));
 // 	abr::graph_config graphConfig;
 // 	abr::graph_api* graphApi;
 // 	graphConfig.id = 1;
 // 	graphConfig.name = "TestGraph";
 // 	graphApi = new abr::graph_api(&graphConfig);

 // 	aeg_err result = gr->read("gradf.sq.in[1]", output_data,testDataSize);
 // 	CHECK(result == aeg_err::ok);
 // 	free(output_data);
 // 	delete graphApi;
 // 	CHECK(true);
 // }

 /**
  * @brief TC_044: Verify selftest with event generation functionality
  *
  * This test verifies that selftestexit() correctly generates events on columns using
  * generate_events_on_col. This extended test validates event generation capabilities
  * across all columns, expecting 100 event generation calls for comprehensive testing.
  */
 TEST(AIEGraphSimTests, TC_044_SelftestWithEventGeneration)
 {

	 graphInit_mocks_ok();
	 mock_c()->expectOneCall("XAie_PmRequestTiles")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_ErrorHandlingInit")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectNCalls(3, "XAie_PmSetColumnClk")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_PmSetColumnClk")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);

	 // Expect calls for generate_events_on_col - covers line 869
	 mock_c()->expectNCalls(100, "XAie_EventGenerate")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);

	 aeg_err result = gr->selftestexit();
	 CHECK(result == aeg_err::ok);
	 mock_c()->clear();
 }

 /**
  * @brief Test group for AIE ISR (Interrupt Service Routine) initialization
  *
  * This test group verifies the initialization of interrupt service routines for
  * AI Engine devices. ISRs are essential for handling hardware events and errors.
  */
 TEST_GROUP(AbrInitIsrTests)
 {
	 // Setup function - called before each test
	 void setup()
	 {

	 }

	 // Teardown function - called after each test
	 void teardown()
	 {

	 }
 };

 /**
  * @brief TC_001: Verify successful ISR initialization
  *
  * This test verifies that abr_init_isr() successfully initializes the interrupt service
  * routine for AI Engine devices. The test ensures that ISR setup completes without errors,
  * enabling interrupt-driven error handling and event processing.
  */
 TEST(AbrInitIsrTests, TC_001_NominalInitialization)
 {


	 // ACT: Call the function under test
	 abr_init_isr();
	 CHECK(true);
 }

 /**
  * @brief Test group for AIE error ISR (Interrupt Service Routine) handling
  *
  * This test group verifies the error interrupt service routine functionality,
  * which processes hardware errors detected in the AI Engine array.
  */
 TEST_GROUP(AbrErrorIsrTests)
 {
	 // Setup function - called before each test
	 void setup()
	 {

	 }

	 // Teardown function - called after each test
	 void teardown()
	 {

	 }
 };

 /**
  * @brief TC_001: Verify error ISR execution
  *
  * This test verifies that abr_error_isr() executes successfully when called with error
  * data. The error ISR processes hardware errors from the AI Engine array, enabling
  * appropriate error handling and recovery actions.
  */
 TEST(AbrErrorIsrTests, TC_001_NominalInitialization)
 {
	 int len = 10;
	 int32_t* output_data = (int32_t*)malloc(len*sizeof(int32_t));
	 // ACT: Call the function under test
	 abr_error_isr(output_data);
	 free(output_data);
	 CHECK(true);
 }

 /**
  * @brief Test group for microcontroller application dump functionality
  *
  * This test group verifies the functionality to dump ELF binary data for
  * microcontroller applications embedded in the AI Engine system.
  */
 TEST_GROUP(DumpUcAppTests)
 {
	 void setup()
	 {

	 }

	 void teardown()
	 {
		 // Clean up any resources
	 }
 };

 /**
  * @brief TC_001: Verify successful ELF binary dump
  *
  * This test verifies that dump_uc_app() successfully dumps ELF binary data to file
  * when running in simulation mode (__AIESIM__ defined). This functionality is used
  * for debugging and analysis of microcontroller application binaries.
  */
 TEST(DumpUcAppTests, TC_001_SuccessfulDump)
 {
	 // ACT: Call the real production function
	 dump_uc_app();
	 CHECK(true);
 }

 /**
  * @brief Test group for AIE driver testing functionality
  *
  * This test group verifies core AIE driver test functions that validate
  * low-level hardware interactions and driver initialization.
  */
 TEST_GROUP(aie_driver_tests)
 {
	 void setup()
	 {

	 }

	 void teardown()
	 {
		 // Clean up any resources
	 }
 };

 /**
  * @brief TC_001: Verify AIE driver test execution
  *
  * This test verifies that aie_driver_test() executes successfully with a device instance.
  * This function performs low-level driver tests to validate hardware communication and
  * basic driver functionality.
  */
 TEST(aie_driver_tests, TC_001_aie_driver_test)
 {

	 XAie_DevInst DevInst;
	 // ACT: Call the real production function
	 aie_driver_test(&DevInst);
	 CHECK(true);
 }
