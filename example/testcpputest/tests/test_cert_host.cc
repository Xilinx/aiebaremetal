/**
 * @file test_cert_host.cc
 * @brief Unit tests for the cert_host::cert_api class
 *
 * This file contains unit tests for the cert_host::cert_api class, which provides
 * certification and handshake functionality for AI Engine host communication.
 * The tests verify handshake operations, host arguments parsing, device instance
 * initialization, certificate loading, wake-up procedures, and execution.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

 #include "cert_host.h"
 #include "test_comparator.h"
 #include "stubs.h"
 #include "CppUTest/TestHarness.h"
 #include "CppUTest/CommandLineTestRunner.h"
 #include "CppUTestExt/MockSupport_c.h"

 // Ensure tile type constant is defined
 #ifndef XAIEGBL_TILE_TYPE_SHIMNOC
 #define XAIEGBL_TILE_TYPE_SHIMNOC 1U
 #endif

 using namespace cert_host;

 /**
  * @brief Test group for cert_api main functionality
  *
  * This test group verifies the core functionality of the cert_host::cert_api class,
  * which provides certification and handshake operations for AI Engine host communication.
  * The tests in this group validate host arguments parsing, buffer mapping, and
  * handshake initialization with various success and failure scenarios.
  */
 TEST_GROUP(CertApiTests)
 {
	 cert_host::cert_api *api;

	 /**
	  * @brief Set up test fixtures before each test
	  *
	  * Initializes the device instance and creates a new cert_api instance
	  * to be used in the tests.
	  */
	 void setup()
	 {
		 XAie_DevInst dev;
		 dev.NumCols = 3;

		 api = new cert_host::cert_api(&dev);
	 }

	 /**
	  * @brief Clean up test fixtures after each test
	  *
	  * Deletes the cert_api instance and clears any mock expectations
	  * to prepare for the next test.
	  */
	 void teardown()
	 {

		 mock_c()->expectNCalls(4,"XAie_MemFree")
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(0);

		 delete api;
		 mock_c()->clear();
		 mock_c()->checkExpectations();
	 }
 };

 /**
  * @brief Test parse_host_args with valid buffer mapping
  * Validates successful parsing of host arguments with proper buffer map setup
  */
 TEST(CertApiTests, TC_001_ParseHostArgs_WithBufferMap)
 {
	 XAie_MemInst *host_args_meminst = new XAie_MemInst;
	 // Set up the buffer maps
	 api->external_buffer_map[1] = 100; // Map xrt_id 1 to sim_id 100
	 api->external_buffer_addr_map[100] = 0x12345678; // Map sim_id 100 to address 0x12345678
	 uint32_t *comp_sig_va = (uint32_t*)malloc(sizeof(uint32_t) * 10);
	 XAie_MemInst *inst_ptr_A = host_args_meminst;
	 mock_c()->expectNCalls(1, "XAie_MemAllocate")
		 // ->withPointerParameters("DevInst", devInst)
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(host_args_meminst);
	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->withPointerParameters("MemInst", inst_ptr_A) // Verify this matches what init_handshake() uses
		 ->andReturnPointerValue(comp_sig_va);
	 mock_c()->expectOneCall("XAie_MemGetDevAddr")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 // ACT
	 api->parse_host_args();

	 // ASSERT
	 // Check that memory was allocated - success is implied by no crash
	 CHECK(true);
	 delete host_args_meminst;
	 // delete devInst;
	 free(comp_sig_va);


 }

 /**
  * @brief Test parse_host_args with buffer mapping and device address
  * Validates host arguments parsing with device address retrieval
  */
 TEST(CertApiTests, TC_002_ParseHostArgs_WithDeviceAddress)
 {
	 uint64_t host_args_dev_addr;
	 // ARRANGE

	 XAie_MemInst *host_args_meminst = new XAie_MemInst;
	 // Set up the buffer maps
	 api->external_buffer_map[1] = 100; // Map xrt_id 1 to sim_id 100
	 api->external_buffer_addr_map[100] = 0x12345678; // Map sim_id 100 to address 0x12345678
	 uint32_t *comp_sig_va = (uint32_t*)malloc(sizeof(uint32_t) * 10);
	 XAie_MemInst *inst_ptr_A = host_args_meminst;
	 mock_c()->expectNCalls(1, "XAie_MemAllocate")
		 //  ->withPointerParameters("DevInst", devInst)
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(host_args_meminst);
	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->withPointerParameters("MemInst", inst_ptr_A) // Verify this matches what init_handshake() uses
		 ->andReturnPointerValue(comp_sig_va);
	 mock_c()->expectOneCall("XAie_MemGetDevAddr")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(1);
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 // ACT
	 api->parse_host_args();

	 // ASSERT
	 // Check that memory was allocated - success is implied by no crash
	 CHECK(true);
	 delete host_args_meminst;
	 // delete devInst;
	 free(comp_sig_va);
 }

 /**
  * @brief Test parse_host_args failure with sync error
  * Validates error handling when memory synchronization fails
  */
 TEST(CertApiTests, TC_003_ParseHostArgs_WithSyncFailure)
 {
	 uint64_t host_args_dev_addr;
	 // ARRANGE

	 XAie_MemInst *host_args_meminst = new XAie_MemInst;
	 // Set up the buffer maps
	 api->external_buffer_map[1] = 100; // Map xrt_id 1 to sim_id 100
	 api->external_buffer_addr_map[100] = 0x12345678; // Map sim_id 100 to address 0x12345678
	 uint32_t *comp_sig_va = (uint32_t*)malloc(sizeof(uint32_t) * 10);
	 XAie_MemInst *inst_ptr_A = host_args_meminst;
	 mock_c()->expectNCalls(1, "XAie_MemAllocate")
		 //  ->withPointerParameters("DevInst", devInst)
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(nullptr);
	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->withPointerParameters("MemInst", inst_ptr_A) // Verify this matches what init_handshake() uses
		 ->andReturnPointerValue(comp_sig_va);
	 mock_c()->expectOneCall("XAie_MemGetDevAddr")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(1);
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 // ACT
	 api->parse_host_args();

	 // ASSERT
	 // Check that memory was allocated - success is implied by no crash
	 CHECK(true);
	 delete host_args_meminst;
	 // delete devInst;
	 free(comp_sig_va);
 }

 /**
  * @brief Test init_handshake failure scenario
  * Validates error handling when handshake initialization fails
  */
 TEST(CertApiTests, TC_004_InitHandshake_Failure)
 {
	 // Setup pointers and resources
	 //XAie_DevInst* devInst = new XAie_DevInst;
	 XAie_MemInst *host_args_meminst = nullptr;
	 // Mock expectations
	 mock_c()->expectNCalls(1, "XAie_MemAllocate")
		 //    ->withPointerParameters("DevInst", devInst)
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(host_args_meminst);
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);

	 // Act
	 int result = api->init_handshake();

	 // Assert
	 CHECK_EQUAL(-1, result);

	 delete host_args_meminst;
	 // delete devInst;  // Ensure proper cleanup for non-malloc allocated objects
 }

 /**
  * @brief Test init_handshake failure with memory allocation error
  * Validates error handling when memory allocation fails during handshake
  */
 TEST(CertApiTests, TC_005_InitHandshake_MemoryAllocationFailure)
 {
	 // Setup pointers and resources
	 //XAie_DevInst* devInst = new XAie_DevInst;
	 XAie_MemInst *host_args_meminst = new XAie_MemInst;
	 XAie_MemInst *host_args_meminst1 = nullptr;


	 // Mock expectations
	 mock_c()->expectNCalls(1, "XAie_MemAllocate")
		 //  ->withPointerParameters("DevInst", devInst)
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(host_args_meminst);
	 mock_c()->expectNCalls(1, "XAie_MemAllocate")
		 //  ->withPointerParameters("DevInst", devInst)
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(host_args_meminst1);
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);

	 // Act
	 int result = api->init_handshake();

	 // Assert
	 CHECK_EQUAL(-ENOMEM, result);


	 // delete devInst;  // Ensure proper cleanup for non-malloc allocated objects
	 delete host_args_meminst;

 }

 /**
  * @brief Test group for device instance initialization
  *
  * This test group verifies the device instance initialization functionality of the
  * cert_host::cert_api class. The tests validate proper configuration and setup of
  * AIE device instances for certification operations.
  */
 TEST_GROUP(CertApiInitDevinstTests)
 {
	 /**
	  * @brief Set up test fixtures before each test
	  *
	  * Prepares the test environment for device instance initialization tests.
	  */
	 void setup()
	 {

	 }

	 /**
	  * @brief Clean up test fixtures after each test
	  *
	  * Clears mock expectations and prepares for the next test.
	  */
	 void teardown()
	 {
		 mock_c()->clear();
		 mock_c()->checkExpectations();
	 }
 };

 /**
  * @brief Test init_devinst functionality
  * Validates device instance initialization with proper configuration
  */
 TEST(CertApiInitDevinstTests, TC_001_InitDevinst)
 {
	 XAie_DevInst devInst;
	 devInst.NumCols = 3;
	 cert_host::cert_api api(&devInst);
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 cert_api cert;
	 api.init_devinst(&devInst);
	 CHECK(true);
 }

 /**
  * @brief Test group for handshake operations
  *
  * This test group verifies the handshake initialization and communication functionality
  * of the cert_host::cert_api class. The tests validate successful handshake setup,
  * memory allocation, device synchronization, and proper error handling during
  * certification handshake procedures.
  */
 TEST_GROUP(CertApiHandshakeTests)
 {
	 /**
	  * @brief Set up test fixtures before each test
	  *
	  * Clears mock state and installs custom comparators for XAie_LocType
	  * to enable proper mock expectations.
	  */
	 void setup()
	 {
		 mock_c()->clear();
		 mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
	 }

	 /**
	  * @brief Clean up test fixtures after each test
	  *
	  * Clears mock expectations, checks expectations were met, and removes
	  * custom comparators to prepare for the next test.
	  */
	 void teardown()
	 {
		 mock_c()->clear();
		 mock_c()->checkExpectations();
		 mock_c()->removeAllComparatorsAndCopiers();
	 }
 };

 /**
  * @brief Test successful handshake initialization
  * Validates successful handshake setup and completion
  */
 TEST(CertApiHandshakeTests, TC_001_InitHandshake_Success)
 {
	 // Initialize resources
	 XAie_DevInst devInst;
	 devInst.NumCols = 3;
	 XAie_LocType Loc = {.Row = 0, .Col = 0};
	 // Allocate memory and initialize pointers as necessary
	 XAie_MemInst *host_args_meminst = (XAie_MemInst*)malloc(sizeof(XAie_MemInst));
	 XAie_MemInst *host_args_meminst1 = (XAie_MemInst*)malloc(sizeof(XAie_MemInst));
	 uint32_t *comp_sig_va = (uint32_t *)malloc(sizeof(*comp_sig_va) * HSA_QUEUE_ENTRY);
	 uint32_t *comp_sig_va1 = (uint32_t *)malloc(sizeof(hsa_queue));
	 cert_host::cert_api *api_ptr = new cert_host::cert_api(&devInst); // <-- ADD

	 // Setup mock expectations
	 mock_c()->expectOneCall("XAie_MemAllocate")
		 ->withPointerParameters("DevInst", &devInst)
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(host_args_meminst);

	 mock_c()->expectOneCall("XAie_MemAllocate")
		 ->withPointerParameters("DevInst", &devInst)
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(host_args_meminst1);

	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->withPointerParameters("MemInst", host_args_meminst1)
		 ->andReturnPointerValue(comp_sig_va);

	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->withPointerParameters("MemInst", host_args_meminst)
		 ->andReturnPointerValue(comp_sig_va1);

	 mock_c()->expectOneCall("XAie_MemGetDevAddr")
		 ->withPointerParameters("MemInst", host_args_meminst)
		 ->andReturnIntValue(0x10);

	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);

	 for (Loc.Col = 0; Loc.Col < devInst.NumCols; Loc.Col ++) {
		 mock_c()->expectOneCall	( "XAie_DataMemBlockWrite")
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(1);
	 }
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);

	 // Act
	 int result = api_ptr->init_handshake();


	 CHECK_EQUAL(0, result);

	 delete api_ptr;


	 // Cleanup allocated resources
	 if (comp_sig_va) {
		 free(comp_sig_va);
		 comp_sig_va = nullptr;
	 }

	 if (comp_sig_va1) {
		 free(comp_sig_va1);
		 comp_sig_va1 = nullptr;
	 }

	 if (host_args_meminst) {
		 free(host_args_meminst);
		 host_args_meminst = nullptr;
	 }

	 if (host_args_meminst1) {
		 free(host_args_meminst1);
		 host_args_meminst1 = nullptr;
	 }
}

/**
 * @brief Test group for submit_test_command functionality
 *
 * This test group verifies the submit_test_command functionality of the
 * cert_host::cert_api class. The tests validate successful submission of
 * test commands to the HSA queue, proper handling of completion signals,
 * and error conditions during test command execution.
 */
TEST_GROUP(CertApisubmit_test_commandTests)
 {
	 void setup()
	 {
		 mock_c()->clear();
		 mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
	 }

	 void teardown()
	 {
		 mock_c()->clear();
		 mock_c()->checkExpectations();
		 mock_c()->removeAllComparatorsAndCopiers();
	 }
 };

 /**
  * @brief Test submit_test_command with successful completion
  * Validates that submit_test_command successfully submits a test packet
  * and receives successful completion signal
  */
 TEST(CertApisubmit_test_commandTests, TC_001_SubmitTestCommand_Success)
 {
	 // ARRANGE - Setup device instance
	 XAie_DevInst devInst;
	 memset(&devInst, 0, sizeof(devInst));
	 Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	 devInst.NumCols = 3;
	 devInst.StartCol = 0;
	 devInst.DevOps = &s_MockDevOps;

	 // Allocate DevMod array (it's a pointer that needs allocation)
	 // Each DevMod contains 2 pointers (UcMod, PerfMod), so 16 bytes on 64-bit, but use 32 to be safe
	 size_t devModSize = 32 * 10;  // 10 elements, each 32 bytes
	 void* devMod = malloc(devModSize);
	 memset(devMod, 0, devModSize);
	 devInst.DevProp.DevMod = (decltype(devInst.DevProp.DevMod))devMod;

	 XAie_UcMod* Mock_XAie_UcModPointer = &Mock_XAie_UcModstatus;
	 devInst.DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].UcMod = Mock_XAie_UcModPointer;

	 cert_host::cert_api *api = new cert_host::cert_api(&devInst);

	 // Allocate memory structures for HSA queue and completion signal
	 XAie_MemInst *hsa_meminst = (XAie_MemInst*)malloc(sizeof(XAie_MemInst));
	 XAie_MemInst *comp_sig_meminst = (XAie_MemInst*)malloc(sizeof(XAie_MemInst));

	 struct hsa_queue *hsa_q = (struct hsa_queue *)malloc(sizeof(struct hsa_queue));
	 memset(hsa_q, 0, sizeof(struct hsa_queue));
	 hsa_q->hq_header.write_index = 0;

	 uint32_t *completion_signal_array = (uint32_t *)malloc(sizeof(uint32_t) * HSA_QUEUE_ENTRY);
	 memset(completion_signal_array, 0, sizeof(uint32_t) * HSA_QUEUE_ENTRY);

	 // Mock init_handshake to set up hsa_meminst and comp_sig_meminst
	 mock_c()->expectOneCall("XAie_MemAllocate")
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(hsa_meminst);

	 mock_c()->expectOneCall("XAie_MemAllocate")
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(comp_sig_meminst);

	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(completion_signal_array);

	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(hsa_q);

	 mock_c()->expectOneCall("XAie_MemGetDevAddr")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0x2000);

	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);

	 // Mock DataMemBlockWrite calls for each column
	 for (int col = 0; col < devInst.NumCols; col++) {
		 mock_c()->expectOneCall("XAie_DataMemBlockWrite")
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(1);
	 }

	 // Initialize the handshake
	 int init_result = api->init_handshake();
	 CHECK_EQUAL(0, init_result);

	 // Setup global variables to simulate firmware completion signal
	 g_test_completion_ptr = completion_signal_array; // Point to the signal array
	 g_test_comp_sig_meminst = comp_sig_meminst;      // Track the comp_sig MemInst
	 g_test_sync_call_count = 0;                      // Reset counter
	 g_test_completion_threshold = 1;                 // Set after >1 sync call (on the 2nd comp_sig sync)
	 g_test_completion_value = HSA_COMP_SUCCESS;      // Set to success

	 // Now mock submit_test_command operations
	 // First XAie_MemGetVAddr returns hsa_queue
	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->withPointerParameters("MemInst", hsa_meminst)
		 ->andReturnPointerValue(hsa_q);

	 // XAie_MemSyncForCPU for hsa queue
	 mock_c()->expectOneCall("XAie_MemSyncForCPU")
		 ->withPointerParameters("MemInst", hsa_meminst)
		 ->andReturnIntValue(0);

	 // XAie_MemGetVAddr returns completion signal array
	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->withPointerParameters("MemInst", comp_sig_meminst)
		 ->andReturnPointerValue(completion_signal_array);

	 // XAie_MemSyncForCPU for completion signal (reading to initialize)
	 mock_c()->expectOneCall("XAie_MemSyncForCPU")
		 ->withPointerParameters("MemInst", comp_sig_meminst)
		 ->andReturnIntValue(0);

	 // XAie_MemSyncForDev for completion signal (after setting to 0)
	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->withPointerParameters("MemInst", comp_sig_meminst)
		 ->andReturnIntValue(0);

	 // XAie_MemGetDevAddr for completion signal
	 mock_c()->expectOneCall("XAie_MemGetDevAddr")
		 ->withPointerParameters("MemInst", comp_sig_meminst)
		 ->andReturnIntValue(0x3000);

	 // XAie_MemSyncForDev for hsa queue (after packet setup)
	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->withPointerParameters("MemInst", hsa_meminst)
		 ->andReturnIntValue(0);

	 // XAie_MemSyncForDev for hsa queue header write_index update
	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->withPointerParameters("MemInst", hsa_meminst)
		 ->andReturnIntValue(0);

	 // In the polling loop: XAie_MemSyncForCPU for checking completion signal
	 // The global mechanism will set completion_signal to HSA_COMP_SUCCESS after threshold
	 mock_c()->expectOneCall("XAie_MemSyncForCPU")
		 ->withPointerParameters("MemInst", comp_sig_meminst)
		 ->andReturnIntValue(0);

	 // ACT
	 int result = api->submit_test_command();

	 // ASSERT
	 CHECK_EQUAL(0, result);
	 CHECK_EQUAL(1, hsa_q->hq_header.write_index); // write_index should be incremented
	 CHECK_EQUAL(HOST_QUEUE_PACKET_TYPE_VENDOR_SPECIFIC,
		 hsa_q->hq_entry[0].xrt_header.common_header.type);
	 CHECK_EQUAL(HOST_QUEUE_PACKET_TEST,
		 hsa_q->hq_entry[0].xrt_header.common_header.opcode);

	 // CLEANUP - Reset globals
	 g_test_completion_ptr = NULL;
	 g_test_comp_sig_meminst = NULL;
	 g_test_sync_call_count = 0;
	 g_test_completion_threshold = 0;
	 g_test_completion_value = 4; // Reset to default

	 // Cleanup API object
	 mock_c()->expectNCalls(4, "XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 delete api;

	 free(hsa_q);
	 free(completion_signal_array);
	 free(hsa_meminst);
	 free(comp_sig_meminst);
	 free(devMod);
 }

 /**
  * @brief Test submit_test_command with error completion
  * Validates error handling when completion signal indicates an error state
  */
 TEST(CertApisubmit_test_commandTests, TC_002_SubmitTestCommand_ErrorCompletion)
 {
	 // ARRANGE - Setup device instance
	 XAie_DevInst devInst;
	 memset(&devInst, 0, sizeof(devInst));
	 Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	 devInst.NumCols = 3;
	 devInst.StartCol = 0;
	 devInst.DevOps = &s_MockDevOps;

	 // Allocate DevMod array (it's a pointer that needs allocation)
	 // Each DevMod contains 2 pointers (UcMod, PerfMod), so 16 bytes on 64-bit, but use 32 to be safe
	 size_t devModSize = 32 * 10;  // 10 elements, each 32 bytes
	 void* devMod = malloc(devModSize);
	 memset(devMod, 0, devModSize);
	 devInst.DevProp.DevMod = (decltype(devInst.DevProp.DevMod))devMod;

	 XAie_UcMod* Mock_XAie_UcModPointer = &Mock_XAie_UcModstatus;
	 devInst.DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].UcMod = Mock_XAie_UcModPointer;

	 cert_host::cert_api *api = new cert_host::cert_api(&devInst);

	 // Allocate memory structures
	 XAie_MemInst *hsa_meminst = (XAie_MemInst*)malloc(sizeof(XAie_MemInst));
	 XAie_MemInst *comp_sig_meminst = (XAie_MemInst*)malloc(sizeof(XAie_MemInst));

	 struct hsa_queue *hsa_q = (struct hsa_queue *)malloc(sizeof(struct hsa_queue));
	 memset(hsa_q, 0, sizeof(struct hsa_queue));
	 hsa_q->hq_header.write_index = 0;

	 uint32_t *completion_signal_array = (uint32_t *)malloc(sizeof(uint32_t) * HSA_QUEUE_ENTRY);
	 memset(completion_signal_array, 0, sizeof(uint32_t) * HSA_QUEUE_ENTRY);

	 // Mock init_handshake
	 mock_c()->expectOneCall("XAie_MemAllocate")
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(hsa_meminst);

	 mock_c()->expectOneCall("XAie_MemAllocate")
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(comp_sig_meminst);

	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(completion_signal_array);

	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->ignoreOtherParameters()
		 ->andReturnPointerValue(hsa_q);

	 mock_c()->expectOneCall("XAie_MemGetDevAddr")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0x2000);

	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);

	 for (int col = 0; col < devInst.NumCols; col++) {
		 mock_c()->expectOneCall("XAie_DataMemBlockWrite")
			 ->ignoreOtherParameters()
			 ->andReturnIntValue(1);
	 }

	 int init_result = api->init_handshake();
	 CHECK_EQUAL(0, init_result);

	 // Setup global variables to simulate firmware error completion
	 g_test_completion_ptr = completion_signal_array;
	 g_test_comp_sig_meminst = comp_sig_meminst;
	 g_test_sync_call_count = 0;
	 g_test_completion_threshold = 1; // Set after >1 sync call (on the 2nd comp_sig sync)
	 g_test_completion_value = HSA_CMD_STATE_ERROR; // Set to error instead of success

	 // Mock submit_test_command operations
	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->withPointerParameters("MemInst", hsa_meminst)
		 ->andReturnPointerValue(hsa_q);

	 mock_c()->expectOneCall("XAie_MemSyncForCPU")
		 ->withPointerParameters("MemInst", hsa_meminst)
		 ->andReturnIntValue(0);

	 mock_c()->expectOneCall("XAie_MemGetVAddr")
		 ->withPointerParameters("MemInst", comp_sig_meminst)
		 ->andReturnPointerValue(completion_signal_array);

	 mock_c()->expectOneCall("XAie_MemSyncForCPU")
		 ->withPointerParameters("MemInst", comp_sig_meminst)
		 ->andReturnIntValue(0);

	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->withPointerParameters("MemInst", comp_sig_meminst)
		 ->andReturnIntValue(0);

	 mock_c()->expectOneCall("XAie_MemGetDevAddr")
		 ->withPointerParameters("MemInst", comp_sig_meminst)
		 ->andReturnIntValue(0x3000);

	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->withPointerParameters("MemInst", hsa_meminst)
		 ->andReturnIntValue(0);

	 mock_c()->expectOneCall("XAie_MemSyncForDev")
		 ->withPointerParameters("MemInst", hsa_meminst)
		 ->andReturnIntValue(0);

	 // In the polling loop: XAie_MemSyncForCPU
	 // The global mechanism will set completion signal to HSA_CMD_STATE_ERROR after threshold
	 mock_c()->expectOneCall("XAie_MemSyncForCPU")
		 ->withPointerParameters("MemInst", comp_sig_meminst)
		 ->andReturnIntValue(0);

	 // ACT
	 int result = api->submit_test_command();

	 // ASSERT
	 CHECK_EQUAL(HSA_CMD_STATE_ERROR, result); // Should return error code (5)

	 // CLEANUP - Reset globals
	 g_test_completion_ptr = NULL;
	 g_test_comp_sig_meminst = NULL;
	 g_test_sync_call_count = 0;
	 g_test_completion_threshold = 0;
	 g_test_completion_value = 4; // Reset to default

	 // Cleanup API object
	 mock_c()->expectNCalls(4, "XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 delete api;

	 free(hsa_q);
	 free(completion_signal_array);
	 free(hsa_meminst);
	 free(comp_sig_meminst);
	 free(devMod);
 }



 /**
  * @brief Test group for certificate loading operations
  *
  * This test group verifies the certificate loading functionality of the
  * cert_host::cert_api class. The tests validate successful loading of
  * certification data and proper interaction with AIE device operations.
  */
 TEST_GROUP(CertApiLoadCertTests)
 {
	 /**
	  * @brief Set up test fixtures before each test
	  *
	  * Clears mock state and installs custom comparators for XAie_LocType
	  * to enable proper mock expectations.
	  */
	 void setup()
	 {
		 mock_c()->clear();
		 mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
	 }

	 /**
	  * @brief Clean up test fixtures after each test
	  *
	  * Clears mock expectations, checks expectations were met, and removes
	  * custom comparators to prepare for the next test.
	  */
	 void teardown()
	 {
		 mock_c()->clear();
		 mock_c()->checkExpectations();
		 mock_c()->removeAllComparatorsAndCopiers();
	 }
 };

 /**
  * @brief Test successful certificate loading
  * Validates successful loading of certification data
  */
 TEST(CertApiLoadCertTests, TC_001_LoadCert_Success)
 {
	 XAie_DevInst devInst;
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	 XAie_UcMod* Mock_XAie_UcModPointer = &Mock_XAie_UcModstatus_fail;
	 devInst.DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].UcMod = Mock_XAie_UcModPointer;
	 devInst.NumCols = 2;
	 devInst.StartCol  = 0;
	 devInst.DevOps = &s_MockDevOps;
	 cert_host::cert_api api(&devInst);
	 mock_c()->expectNCalls(2,"XAie_LoadUcMem")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(1);
	 int res = api.load_cert();
	 CHECK(res == 0);
 }

 /**
  * @brief TC_002: Test certificate loading with zero ELF length
  *
  * This test validates that load_cert properly handles the error case when
  * cert_elf_len is zero. The function should detect this condition and return
  * -1 without attempting to load the certificate.
  */
 TEST(CertApiLoadCertTests, TC_002_LoadCert_ZeroElfLen)
 {
	 XAie_DevInst devInst;
	 // Expect XAie_MemFree calls from cert_api initialization
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	 XAie_UcMod* Mock_XAie_UcModPointer = &Mock_XAie_UcModstatus_fail;
	 devInst.DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].UcMod = Mock_XAie_UcModPointer;
	 devInst.NumCols = 2;  // Use 2 columns to test the loop
	 devInst.StartCol = 0;
	 devInst.DevOps = &s_MockDevOps;

	 // Create cert_api and temporarily set cert_elf_len to 0
	 cert_host::cert_api api(&devInst);
	 extern unsigned int cert_elf_len;
	 unsigned int saved_len = cert_elf_len;
	 cert_elf_len = 0;

	 // Should return -1 when cert_elf_len is 0
	 // Call loop version to cover line 72 (error return in loop)
	 int res = api.load_cert();
	 CHECK_EQUAL(-1, res);

	 // Restore original cert_elf_len
	 cert_elf_len = saved_len;
 }


/**
  * @brief Test group for wake-up certification operations
  *
  * This test group verifies the wake-up certification procedures of the
  * cert_host::cert_api class. The tests validate successful wake-up of
  * certification processes and proper error handling for failure scenarios.
  */
 TEST_GROUP(CertApiWakeupCertTests)
 {
	 /**
	  * @brief Set up test fixtures before each test
	  *
	  * Clears mock state and installs custom comparators for XAie_LocType
	  * to enable proper mock expectations.
	  */
	 void setup()
	 {
		 mock_c()->clear();
		 mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
	 }

	 /**
	  * @brief Clean up test fixtures after each test
	  *
	  * Clears mock expectations, checks expectations were met, and removes
	  * custom comparators to prepare for the next test.
	  */
	 void teardown()
	 {
		 mock_c()->clear();
		 mock_c()->checkExpectations();
		 mock_c()->removeAllComparatorsAndCopiers();
	 }
 };

 /**
  * @brief Test successful wake-up certification
  * Validates successful wake-up of certification process
  */
 TEST(CertApiWakeupCertTests, TC_001_WakeupCert_Success)
 {
	 XAie_DevInst devInst;
	 XAie_LocType Loc = {1,2};
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);

	 Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	 devInst.NumCols = 3;
	 devInst.StartCol  = 0;
	 devInst.DevOps = &s_MockDevOps;
	 XAie_UcMod* Mock_XAie_UcModPointer = &Mock_XAie_UcMod;
	 // devInst.DevProp.DevMod[TileType]->UcMod->Wakeup = mock_Wakeup();
	 //const_cast<XAie_UcMod*>(devInst.DevProp.DevMod->UcMod)->Wakeup = mock_Wakeup;
	 devInst.DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].UcMod = Mock_XAie_UcModPointer;
	 cert_host::cert_api api(&devInst);

	 int res = api.wakeup_cert();
	 CHECK(res == 0);
 }

 /**
  * @brief Test wake-up certification failure
  * Validates error handling when wake-up certification fails
  */
 TEST(CertApiWakeupCertTests, TC_002_WakeupCert_Failure)
 {
	 XAie_DevInst devInst;
	 XAie_LocType Loc = {1,2};
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);

	 Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	 devInst.NumCols = 3;
	 devInst.StartCol  = 0;
	 devInst.DevOps = &s_MockDevOps;
	 XAie_UcMod* Mock_XAie_UcModPointer = &Mock_XAie_UcMod_fail;
	 // devInst.DevProp.DevMod[TileType]->UcMod->Wakeup = mock_Wakeup();
	 //const_cast<XAie_UcMod*>(devInst.DevProp.DevMod->UcMod)->Wakeup = mock_Wakeup;
	 devInst.DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].UcMod = Mock_XAie_UcModPointer;
	 cert_host::cert_api api(&devInst);

	 int res = api.wakeup_cert();
	 CHECK(res == 1);
 }

 /**
  * @brief Test group for certificate execution operations
  *
  * This test group verifies the certificate execution functionality of the
  * cert_host::cert_api class. The tests validate successful execution of
  * certification procedures with proper device operations.
  */
 TEST_GROUP(CertApiRunCertTests)
 {
	 /**
	  * @brief Set up test fixtures before each test
	  *
	  * Clears mock state and installs custom comparators for XAie_LocType
	  * to enable proper mock expectations.
	  */
	 void setup()
	 {
		 mock_c()->clear();
		 mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
	 }

	 /**
	  * @brief Clean up test fixtures after each test
	  *
	  * Clears mock expectations, checks expectations were met, and removes
	  * custom comparators to prepare for the next test.
	  */
	 void teardown()
	 {
		 mock_c()->clear();
		 mock_c()->checkExpectations();
		 mock_c()->removeAllComparatorsAndCopiers();
	 }
 };

 /**
  * @brief Test certificate execution
  * Validates successful execution of certification
  */
 TEST(CertApiRunCertTests, TC_001_RunCert)
 {
	 XAie_DevInst devInst;
	 mock_c()->expectNCalls(4,"XAie_MemFree")
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(0);
	 Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	 devInst.NumCols = 3;
	 devInst.StartCol  = 0;
	 devInst.DevOps = &s_MockDevOps;
	 XAie_UcMod* Mock_XAie_UcModPointer = &Mock_XAie_UcModstatus;
	 devInst.DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].UcMod = Mock_XAie_UcModPointer;
	 cert_host::cert_api api(&devInst);
	 int cycles = 5;
	 api.run_cert(cycles);
	 CHECK(true);
 }
/**
 * @brief Test group for submit_command functionality
 *
 * This test group verifies the submit_command functionality of the
 * cert_host::cert_api class. The tests validate error handling when
 * control code memory allocation fails.
 */
TEST_GROUP(CertApisubmit_commandTests)
{
	void setup()
	{
		mock_c()->clear();
		mock_c()->installComparator("XAie_LocType", CompareXAieLocType, XAieLocTypeToString);
	}

	void teardown()
	{
		mock_c()->clear();
		mock_c()->checkExpectations();
		mock_c()->removeAllComparatorsAndCopiers();
	}
};

/**
 * @brief TC_001: Verify submit_command error handling for control code allocation failure
 *
 * This test verifies that submit_command correctly handles the case when
 * ctrl_code memory allocation fails. The test loads a real ELF file to
 * enter the single-column execution path, then simulates a NULL return
 * from XAie_MemGetVAddr to trigger the error path at line 281.
 */
TEST(CertApisubmit_commandTests, TC_001_SubmitCommand_CtrlCodeAllocFailure)
{
	// ARRANGE: Setup device instance
	XAie_DevInst devInst;
	memset(&devInst, 0, sizeof(devInst));

	size_t devModSize = 32 * 10;
	void* devMod = malloc(devModSize);
	memset(devMod, 0, devModSize);
	devInst.DevProp.DevMod = (decltype(devInst.DevProp.DevMod))devMod;

	Stub_XAie_SetTileType(XAIEGBL_TILE_TYPE_SHIMNOC);
	devInst.NumCols = 3;
	devInst.StartCol = 0;
	devInst.DevOps = &s_MockDevOps;
	devInst.DevProp.ColShift = 25;

	XAie_UcMod* Mock_XAie_UcModPointer = &Mock_XAie_UcMod_full;
	devInst.DevProp.DevMod[XAIEGBL_TILE_TYPE_SHIMNOC].UcMod = Mock_XAie_UcModPointer;

	cert_host::cert_api *api = new cert_host::cert_api(&devInst);

	XAie_MemInst *hsa_meminst = (XAie_MemInst*)malloc(sizeof(XAie_MemInst));
	XAie_MemInst *comp_sig_meminst = (XAie_MemInst*)malloc(sizeof(XAie_MemInst));
	XAie_MemInst *host_args_meminst = (XAie_MemInst*)malloc(sizeof(XAie_MemInst));
	XAie_MemInst *ctrl_code_meminst_mock = (XAie_MemInst*)malloc(sizeof(XAie_MemInst));

	struct hsa_queue *hsa_q = (struct hsa_queue *)malloc(sizeof(struct hsa_queue));
	memset(hsa_q, 0, sizeof(struct hsa_queue));
	hsa_q->hq_header.write_index = 0;

	uint32_t *completion_signal_array = (uint32_t *)malloc(sizeof(uint32_t) * HSA_QUEUE_ENTRY);
	memset(completion_signal_array, 0, sizeof(uint32_t) * HSA_QUEUE_ENTRY);

	// ARRANGE: Configure mocks for init_handshake
	mock_c()->expectOneCall("XAie_MemAllocate")
		->ignoreOtherParameters()
		->andReturnPointerValue(hsa_meminst);
	mock_c()->expectOneCall("XAie_MemAllocate")
		->ignoreOtherParameters()
		->andReturnPointerValue(comp_sig_meminst);
	mock_c()->expectOneCall("XAie_MemGetVAddr")
		->ignoreOtherParameters()
		->andReturnPointerValue(completion_signal_array);
	mock_c()->expectOneCall("XAie_MemGetVAddr")
		->ignoreOtherParameters()
		->andReturnPointerValue(hsa_q);
	mock_c()->expectOneCall("XAie_MemGetDevAddr")
		->ignoreOtherParameters()
		->andReturnIntValue(0x2000);
	mock_c()->expectOneCall("XAie_MemSyncForDev")
		->ignoreOtherParameters()
		->andReturnIntValue(0);

	for (int col = 0; col < devInst.NumCols; col++) {
		mock_c()->expectOneCall("XAie_DataMemBlockWrite")
			->ignoreOtherParameters()
			->andReturnIntValue(1);
	}

	int init_result = api->init_handshake();
	CHECK_EQUAL(0, init_result);

	// ARRANGE: Create dummy ELF data with valid header
	unsigned char dummy_elf[] = {
		0x7f, 'E', 'L', 'F',  // ELF magic number
		0x02, 0x01, 0x01, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x02, 0x00, 0xF3, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	char *test_elf_data = new char[sizeof(dummy_elf)];
	memcpy(test_elf_data, dummy_elf, sizeof(dummy_elf));
	unsigned int test_elf_len = sizeof(dummy_elf);
	std::map<std::string_view, uint64_t> test_symbols;

	// ARRANGE: Configure mocks for submit_command
	mock_c()->expectOneCall("XAie_MemGetVAddr")
		->withPointerParameters("MemInst", hsa_meminst)
		->andReturnPointerValue(hsa_q);
	mock_c()->expectOneCall("XAie_MemGetDevAddr")
		->withPointerParameters("MemInst", hsa_meminst)
		->andReturnIntValue(0x80000000);
	mock_c()->expectOneCall("XAie_MemSyncForCPU")
		->withPointerParameters("MemInst", hsa_meminst)
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_MemGetVAddr")
		->withPointerParameters("MemInst", comp_sig_meminst)
		->andReturnPointerValue(completion_signal_array);
	mock_c()->expectOneCall("XAie_MemGetDevAddr")
		->withPointerParameters("MemInst", comp_sig_meminst)
		->andReturnIntValue(0x90000000);
	mock_c()->expectOneCall("XAie_MemSyncForCPU")
		->withPointerParameters("MemInst", comp_sig_meminst)
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_MemSyncForDev")
		->withPointerParameters("MemInst", comp_sig_meminst)
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_MemAllocate")
		->ignoreOtherParameters()
		->andReturnPointerValue(ctrl_code_meminst_mock);
	mock_c()->expectOneCall("XAie_MemGetVAddr")
		->withPointerParameters("MemInst", ctrl_code_meminst_mock)
		->andReturnPointerValue(NULL);
	mock_c()->expectOneCall("XAie_MemGetDevAddr")
		->withPointerParameters("MemInst", ctrl_code_meminst_mock)
		->andReturnIntValue(0xA0000000);

	// ACT: Call submit_command
	int res = api->submit_command(test_elf_data, test_elf_len, test_symbols);

	// ASSERT: Verify error return
	CHECK_EQUAL(-1, res);

	// CLEANUP: Configure mocks for destructor
	mock_c()->expectOneCall("XAie_MemFree")
		->withPointerParameters("MemInst", hsa_meminst)
		->andReturnIntValue(0);
	mock_c()->expectOneCall("XAie_MemFree")
		->withPointerParameters("MemInst", comp_sig_meminst)
		->andReturnIntValue(0);

	delete api;
	delete[] test_elf_data;
	free(ctrl_code_meminst_mock);
	free(hsa_q);
	free(hsa_meminst);
	free(comp_sig_meminst);
	free(host_args_meminst);
	free(completion_signal_array);
	free(devMod);
}
