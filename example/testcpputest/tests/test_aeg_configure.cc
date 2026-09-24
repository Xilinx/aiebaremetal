/**
 * @file test_aeg_configure.cc
 * @brief CppUTest test suite for ConfigureParser and AEG configuration helpers
 *
 * This file contains unit tests for:
 *  - Parsing AIE configuration via ConfigureParser (GMIOs, PLIOs, RTPs, buffers).
 *  - Accessor APIs like get_gmio_config(), get_plio_config(), get_graph_config(), etc.
 *  - Printing of various configuration structures (driver, graph, GMIO, PLIO, buffers).
 *  - Partition helper function get_partition_start_column() for column range queries.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

 #include "aeg_configure.h"
 #include "CppUTest/TestHarness.h"
 #include "CppUTest/CommandLineTestRunner.h"
 #include "CppUTestExt/MockSupport_c.h"
 #include "test_comparator.h"

 /**
  * @brief Tests for ConfigureParser class and configuration APIs
  */
 TEST_GROUP(ConfigureParserTests)
 {
	 ConfigureParser *cfg;
	 std::shared_ptr<XAieMem> mem;
	 XAie_DevInst devInst;

	 /**
	  * @brief Common setup for ConfigureParserTests
	  */
	 void setup()
	 {
		 cfg = new ConfigureParser();
		 RegisterXAieLocTypeComparator();
		 devInst.NumRows = 3;
		 mock_c()->installCopier("u8", u8Copier);
		 mem = std::make_shared<XAieMem>(&devInst);
	 }

	 /**
	  * @brief Common teardown for ConfigureParserTests
	  */
	 void teardown()
	 {
		 delete cfg;
		 mock_c()->removeAllComparatorsAndCopiers();

		 mock_c()->clear();
		 mock_c()->checkExpectations();
	 }
 };

 /**
  * @brief TC01: Verify ConfigureParser default constructor
  */
 TEST(ConfigureParserTests, TC01_Constructor)
 {
	 ConfigureParser *cfg;
	 cfg = new ConfigureParser();
	 CHECK(cfg != nullptr);
	 delete cfg;
 }

 /**
  * @brief TC02: Verify ConfigureGmios creates GMIO API successfully
  */
 TEST(ConfigureParserTests, TC02_ConfigureGmios)
 {
	 XAie_DevInst dev;
	 dev.NumCols = 2;
	 dev.NumRows = 2;
	 dev.StartCol = 0;
	 aeg_err  err;
	 XAie_LocType testLoc;
	 testLoc.Col = 0;
	 testLoc.Row = 0;

	 u8 FirstCallOutput = 1;
	 u8 secondCallOutput = 2;

	 mock_c()->expectOneCall("XAie_DmaDescInit")
		 //  ->withPointerParameters("DevInst", &devInst)
		 ->ignoreOtherParameters()
		 ->andReturnIntValue(XAIE_OK);
	 mock_c()->expectOneCall("XAie_DmaDescInit")
		 //  ->withPointerParameters("DevInst", &devInst)
		 ->ignoreOtherParameters()
		 //  ->withParameterOfType("XAie_LocType", "Loc", &testLoc)
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

	 err = cfg->ConfigureGmios(mem);
	 CHECK(err == aeg_err::ok);
	 cfg->printGMIOsConfig();

	 auto gmio_api = cfg->get_gmio_api_impl("gradf.in");
	 CHECK(gmio_api != nullptr);
 }

 /**
  * @brief TC03: Verify ConfigureExternalBuffers creates external buffer API
  */
 TEST(ConfigureParserTests, TC03_ConfigureExternalBuffers)
 {
	 aeg_err  err;
	 err = cfg->ConfigureExternalBuffers(mem);
	 CHECK(err == aeg_err::ok);

	 auto ext_buffer = cfg->get_externalbuffer_api_impl("EG.ddrin");
	 CHECK(ext_buffer != nullptr);
 }

 /**
  * @brief Stub abort implementation used by ConfigureParser tests
  */
 extern "C" void abort()
 {
 }

 /**
  * @brief TC04: Verify get_externalbuffer_api_impl with invalid name
  */
 TEST(ConfigureParserTests, TC04_GetExternalBufferApiImpl_InvalidName)
 {
	 auto buffer = cfg->get_externalbuffer_api_impl("graph");
	 CHECK(buffer == nullptr);
 }

 /**
  * @brief TC05: Verify get_rtp_config with invalid RTP name
  */
 TEST(ConfigureParserTests, TC05_GetRTPConfig_InvalidName)
 {
	 auto buffer = cfg->get_rtp_config("graph");
	 CHECK(buffer == nullptr);
 }

 /**
  * @brief TC06: Verify get_gmio_api_impl with invalid GMIO name
  */
 TEST(ConfigureParserTests, TC06_GetGmioApiImpl_InvalidName)
 {
	 auto buffer = cfg->get_gmio_api_impl("graph");
	 CHECK(buffer == nullptr);
 }

 /**
  * @brief TC07: Verify get_graph_config with invalid graph name
  */
 TEST(ConfigureParserTests, TC07_GetGraphConfig_InvalidName)
 {
	 auto buffer = cfg->get_graph_config("graph");

	 CHECK(buffer == nullptr);
 }

 /**
  * @brief TC08: Verify get_gmio_config with invalid GMIO name
  */
 TEST(ConfigureParserTests, TC08_GetGmioConfig_InvalidName)
 {
	 abr::gmio_config *gmconfig_1 = cfg->get_gmio_config("graph");
	 CHECK(gmconfig_1 == nullptr);
 }

 /**
  * @brief TC09: Verify get_plio_config_logicname with invalid logical name
  */
 TEST(ConfigureParserTests, TC09_GetPlioConfigLogicName_InvalidName)
 {
	 auto buffer = cfg->get_plio_config_logicname("graph");
	 CHECK(buffer == nullptr);
 }

 /**
  * @brief TC10: Verify get_plio_config with invalid PLIO name
  */
 TEST(ConfigureParserTests, TC10_GetPlioConfig_InvalidName)
 {
	 abr::plio_config *plio = cfg->get_plio_config("graph");
	 CHECK(plio == nullptr);
 }

 /**
  * @brief TC11: Verify get_externalbuffer_config with invalid name
  */
 TEST(ConfigureParserTests, TC11_GetExternalBufferConfig_InvalidName)
 {
	 auto buffer = cfg->get_externalbuffer_config("graph");
	 CHECK(true);
 }

 /**
  * @brief TC12: Verify get_shared_buffer_config with invalid name
  */
 TEST(ConfigureParserTests, TC12_GetSharedBufferConfig_InvalidName)
 {
	 auto buffer = cfg->get_shared_buffer_config("graph");
	 CHECK(true);
 }


 /**
  * @brief TC13: Verify get_driver_config and printDriverConfig
  */
 TEST(ConfigureParserTests, TC13_GetAndPrintDriverConfig)
 {
	 abr::driver_config &driver_config = cfg->get_driver_config();
	 CHECK_EQUAL(1, driver_config.hw_gen); // AIE (changed from 2/AIEML to cover aeg_aiebaremetal.cc lines 843-849)
	 cfg->printDriverConfig();
	 CHECK(true);
 }

 /**
  * @brief TC14: Verify get_gmio_config and printGMIOsConfig
  */
 TEST(ConfigureParserTests, TC14_GetAndPrintGMIOsConfig)
 {
	 abr::gmio_config *gmconfig_1 = cfg->get_gmio_config("gradf.in");
	 CHECK(gmconfig_1->name == "gradf.in");
	 cfg->printGMIOsConfig();
	 CHECK(true);
 }

 /**
  * @brief TC15: Verify get_aiecompiler_config returns expected values
  */
 TEST(ConfigureParserTests, TC15_GetAieCompilerConfig)
 {
	 abr::aiecompiler_config &aiecompilerconfig = cfg->get_aiecompiler_config();
	 CHECK_EQUAL(true, aiecompilerconfig.broadcast_enable_core);
 }

 /**
  * @brief TC16: Verify get_graph_config and printGraphConfig
  */
 TEST(ConfigureParserTests, TC16_GetAndPrintGraphConfig)
 {
	 abr::graph_config *aiegraphconfig = cfg->get_graph_config("gradf2");
	 CHECK(aiegraphconfig != nullptr);
	 CHECK_EQUAL(0, aiegraphconfig->id);
	 cfg->printGraphConfig();
	 CHECK(true);
 }
 /**
  * @brief TC17: Verify get_plio_config and printPLIOsConfig
  */
 TEST(ConfigureParserTests, TC17_GetAndPrintPLIOsConfig)
 {
	 const abr::plio_config *plio = cfg->get_plio_config("gradf2.pin");
	 CHECK(plio->name == "gradf2.pin");
	 cfg->printPLIOsConfig();
	 CHECK(true);
 }

 /**
  * @brief TC18: Verify get_plio_config_logicname with valid logical name
  */
 TEST(ConfigureParserTests, TC18_GetPlioConfigLogicName_ValidName)
 {
	 abr::plio_config *plio = cfg->get_plio_config_logicname("plioin1");
	 CHECK(plio->logicalName == "plioin1");
 }

 /**
  * @brief TC19: Verify get_externalbuffer_config and printExternalbufferConfig
  */
 TEST(ConfigureParserTests, TC19_GetAndPrintExternalBufferConfig)
 {
	 abr::external_buffer_config *ext_buffer = cfg->get_externalbuffer_config("EG.ddrin");
	 CHECK(ext_buffer->name == "EG.ddrin");
	 cfg->printExternalbufferConfig();
	 CHECK(true);
 }

 /**
  * @brief TC20: Verify get_shared_buffer_config and printSharedBufferConfig
  */
 TEST(ConfigureParserTests, TC20_GetAndPrintSharedBufferConfig)
 {
	 abr::shared_buffer_config *shr_buffer = cfg->get_shared_buffer_config("EG.input");
	 CHECK(shr_buffer->name == "EG.input");
	 cfg->printSharedBufferConfig();
	 CHECK(true);
 }

 /**
  * @brief TC21: Verify isRTPorSharedBuffer identifies RTP correctly
  */
 TEST(ConfigureParserTests, TC21_IsRTPorSharedBuffer_RTP)
 {
	 int ret;
	 ret = cfg->isRTPorSharedBuffer("gradf.sq.in[1]");
	 CHECK(ret == 0);
 }

 /**
  * @brief TC22: Verify isRTPorSharedBuffer identifies shared buffer correctly
  */
 TEST(ConfigureParserTests, TC22_IsRTPorSharedBuffer_SharedBuffer)
 {
	 int ret;
	 ret = cfg->isRTPorSharedBuffer("EG.input");
	 CHECK(ret == 1);
 }

 /**
  * @brief TC23: Verify isRTPorSharedBuffer handles null pointer
  */
 TEST(ConfigureParserTests, TC23_IsRTPorSharedBuffer_NullPointer)
 {
	 int ret;
	 char *name;
	 ret = cfg->isRTPorSharedBuffer(name);
	 CHECK(ret == -1);
 }

 /**
  * @brief TC24: Verify get_rtp_config and printRTPsConfig
  */
 TEST(ConfigureParserTests, TC24_GetAndPrintRTPsConfig)
 {
	 abr::rtp_config *rtp_buffer = cfg->get_rtp_config("gradf.sq.in[1]");
	 CHECK(rtp_buffer->portName == "gradf.sq.in[1]");
	 cfg->printRTPsConfig();
	 CHECK(true);
 }
 /**
  * @brief Tests for get_partition_start_column helper function
  */
 TEST_GROUP(GetPartitionStartColumnTestGroup)
 {
	 std::vector<abr::partition_config> partitions;

	 /**
	  * @brief Setup for GetPartitionStartColumnTestGroup
	  */
	 void setup()
	 {
	 }

	 /**
	  * @brief Teardown for GetPartitionStartColumnTestGroup
	  */
	 void teardown()
	 {
		 partitions.clear();
		 mock_c()->clear();
		 mock_c()->checkExpectations();
	 }
 };

 /**
  * @brief TC25: Verify behavior with empty partitions list
  */
 TEST(GetPartitionStartColumnTestGroup, TC25_EmptyPartitionsList)
 {
	 int column = 10;
	 CHECK_EQUAL(default_start_column, get_partition_start_column(partitions, column));
 }

 /**
  * @brief TC26: Verify column falls inside partition range
  */
 TEST(GetPartitionStartColumnTestGroup, TC26_ColumnInRange)
 {
	 partitions = {
		 {0, 5},   // Covers columns 0-4
		 {5, 10},  // Covers columns 5-14
	 };
	 int column = 7;
	 CHECK_EQUAL(5, get_partition_start_column(partitions, column));
 }

 /**
  * @brief TC27: Verify behavior with negative column index
  */
 TEST(GetPartitionStartColumnTestGroup, TC27_NegativeColumn)
 {
	 partitions = {
		 {0, 5},   // Covers columns 0-4
	 };
	 int column = -1;
	 CHECK_EQUAL(default_start_column, get_partition_start_column(partitions, column));
 }

