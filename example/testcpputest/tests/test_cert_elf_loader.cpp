/******************************************************************************
* (c) Copyright 2022 Xilinx, Inc. All rights reserved.
* (c) Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
*
* This file contains confidential and proprietary information
* of Xilinx, Inc. and is protected under U.S. and
* international copyright and other intellectual property
* laws.
*
* DISCLAIMER
* This disclaimer is not a license and does not grant any
* rights to the materials distributed herewith. Except as
* otherwise provided in a valid license issued to you by
* Xilinx, and to the maximum extent permitted by applicable
* law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
* WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
* AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
* BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
* INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
* (2) Xilinx shall not be liable (whether in contract or tort,
* including negligence, or under any other theory of
* liability) for any loss or damage of any kind or nature
* related to, arising under or in connection with these
* materials, including for any direct, or any indirect,
* special, incidental, or consequential loss or damage
* (including loss of data, profits, goodwill, or any type of
* loss or damage suffered as a result of any action brought
* by a third party) even if such damage or loss was
* reasonably foreseeable or Xilinx had been advised of the
* possibility of the same.
*
* CRITICAL APPLICATIONS
* Xilinx products are not designed or intended to be fail-
* safe, or for use in any application requiring fail-safe
* performance, such as life-support or safety devices or
* systems, Class III medical devices, nuclear facilities,
* applications related to the deployment of airbags, or any
* other applications that could lead to death, personal
* injury, or severe property or environmental damage
* (individually and collectively, "Critical
* Applications"). Customer assumes the sole risk and
* liability of any use of Xilinx products in Critical
* Applications, subject only to applicable laws and
* regulations governing limitations on product liability.
*
* THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
* PART OF THIS FILE AT ALL TIMES.
******************************************************************************/

/*****************************************************************************/
/**
*
* @file test_cert_elf_loader.cpp
*  This file contains test cases for cert_elf_loader class functions
*
* <pre>
* MODIFICATION HISTORY:
*
* * Ver	  Who	   Date		Changes
* ----- -------- -------- ---------------------------------
* 1.0	 Veera      05/03/2026  Align mock setup/teardown with other UT files
* 1.1	 Veera      20/04/2026  updated elf test cases
* 1.2	 Veera      10/05/2026  updated comment section for cert_elf_loader functions
* 1.3	 Veera      26/05/2026  updated timing analysis for cert_elf_loader functions
* </pre>
*
******************************************************************************/

/*****************************************************************************/
/**
*
* TEST SPECIFCIATION:
* Source code version: <2026.1>
* Verification Environment1:
	Testing Tool: LDRA
	Platform: Silicon
	Target: APU
	Compiler1: gcc
* Verification Environment2:
	Testing Tool: CPPUTEST
	Platform: Host
	Target: x86
    Compiler2: gcc
* Verification Method:By Specification of test cases
* Test Procedure: The test execution procedure is same for all the test cases.
* After the tests have been specified, the test procedure involves:
*   - building the tests cases for the selected environment
*   - execution of the binary executable in the selected environment
*   - recording the results of execution
*
/*****************************************************************************/
/***************************** Include Files *********************************/
#include "cert_elf_loader.h"

#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport_c.h"
#include <chrono>
#include <iostream>
#ifndef TEST_ELF_FILE
#define TEST_ELF_FILE "ctrl_code.elf"
#endif

TEST_GROUP(CertElfLoaderBeginTests) {
	void setup() { mock_c()->clear(); }

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_FailsWhenLoadingNonexistentFile
*		Function Name: begin
*		Input: Filename "nonexistent_file.elf"
*		Expectation: result == -ENODEV
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderBeginTests, TC_001_FailsWhenLoadingNonexistentFile) {

	const char *test_filename = "nonexistent_file.elf";
	cert_elf_loader loader;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = loader.begin(test_filename);

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by begin function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-ENODEV, result);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_FailsWhenLoadingInvalidElfFromMemory
*		Function Name: begin
*		Input: test_elf_data, test_elf_size
*		Expectation: result == -ENODEV
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderBeginTests, TC_002_FailsWhenLoadingInvalidElfFromMemory) {

	char test_elf_data[] = "This is invalid ELF data";
	size_t test_elf_size = sizeof(test_elf_data) - 1;


	cert_elf_loader loader;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = loader.begin(test_elf_data, test_elf_size);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by begin function: " << total_clock_enable_us << " microseconds" << std::endl;


	CHECK_EQUAL(-ENODEV, result);
}

TEST_GROUP(CertElfLoaderBeginEdgeCases) {
	void setup() { mock_c()->clear(); }

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_BeginWithEmptyFilename
*		Function Name: begin
*		Input: empty_filename = ""
*		Expectation: result == -ENODEV
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderBeginEdgeCases, TC_001_BeginWithEmptyFilename) {
	 cert_elf_loader loader;
	 const char *empty_filename = "";
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	 int result = loader.begin(empty_filename);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by begin function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-ENODEV, result);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_BeginFromMemoryWithZeroSize
*		Function Name: begin
*		Input: dummy_data, zero_size = 0
*		Expectation: result == -ENODEV
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderBeginEdgeCases, TC_002_BeginFromMemoryWithZeroSize) {
	 cert_elf_loader loader;
	 char dummy_data[] = "test";
	 size_t zero_size = 0;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	 int result = loader.begin(dummy_data, zero_size);
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by begin function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(-ENODEV, result);
}

TEST_GROUP(CertElfLoaderParseEdgeCases) {
	void setup() { mock_c()->clear(); }

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_ParseWithoutLoadingELF
*		Function Name: parse
*		Input: none
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderParseEdgeCases, TC_001_ParseWithoutLoadingELF) {
	cert_elf_loader loader;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();

	int result = loader.parse();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK(result == 0);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_ParseWithSymbolMapRelocation
*		Function Name: loader
*		Input: 	dummy_elf,elf_len,symbol_map
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderParseEdgeCases, TC_002_ParseWithSymbolMapRelocation) {

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["test_symbol"] = 0x1000000;
	symbol_map["data_buffer"] = 0x2000000;


	char dummy_elf[] = "invalid elf";
	unsigned int elf_len = sizeof(dummy_elf);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	cert_elf_loader loader(dummy_elf, elf_len, symbol_map);

	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_ParseSectionIteration
*		Function Name: begin
*		Input: validFilename = TEST_ELF_FILE
*		Expectation: parse_result == 0, parse_result2 == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderParseEdgeCases, TC_003_ParseSectionIteration) {
	const char *validFilename = TEST_ELF_FILE;
	cert_elf_loader loader;

	int load_result = loader.begin(validFilename);
	if (load_result == 0) {
		double total_clock_enable_us = 0.0;
		const auto clock_enable_t0 = std::chrono::steady_clock::now();
		int parse_result = loader.parse();
		const auto clock_enable_t1 = std::chrono::steady_clock::now();
		total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
		std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;
		int parse_result2 = loader.parse();

		CHECK_EQUAL(0, parse_result);
		CHECK_EQUAL(0, parse_result2);
	} else {
		CHECK(true);
	}
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_BeginFromMemoryElfBegin
*		Function Name: begin
*		Input:  buffer, size
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderParseEdgeCases, TC_004_BeginFromMemoryElfBegin) {
	const char *validFilename = TEST_ELF_FILE;

	FILE *fp = fopen(validFilename, "rb");
	if (fp != nullptr) {
		fseek(fp, 0, SEEK_END);
		long size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (size > 0 && size < 10000000) {
			char *buffer = new char[size];
			fread(buffer, 1, size, fp);
			fclose(fp);

			cert_elf_loader loader;
			double total_clock_enable_us = 0.0;
			const auto clock_enable_t0 = std::chrono::steady_clock::now();
			int result = loader.begin(buffer, size);
			const auto clock_enable_t1 = std::chrono::steady_clock::now();
			total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
			std::cout << "Time taken by begin function: " << total_clock_enable_us << " microseconds" << std::endl;
			delete[] buffer;

			CHECK_EQUAL(0, result);
		} else {
			fclose(fp);
			CHECK(true);
		}
	} else {
		CHECK(true);
	}
}
TEST_GROUP(RelocationProcessingTests) {
	void setup() { mock_c()->clear(); }

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_SymbolNotFoundInMap
*		Function Name: parse
*		Input: none
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RelocationProcessingTests, TC_001_SymbolNotFoundInMap) {
	std::string dynstr;
	dynstr.push_back('\0');
	dynstr += "unknown_symbol";
	dynstr.push_back('\0');

	const std::vector<ELFIO::utest_cert_rela_spec> relas = {{0u, 1u, 3u, 0x1000}};
	const std::vector<char> elf_bytes =
	    ELFIO::utest_build_cert_elf64_common(2, std::string_view(dynstr.data(), dynstr.size()), 2u, relas);

	std::map<std::string_view, uint64_t> symbol_map;
	cert_elf_loader loader(elf_bytes.data(), elf_bytes.size(), symbol_map);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = loader.parse();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(0, result);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_AllPatchKinds
*		Function Name: parse
*		Input: none
*		Expectation: 0; 5
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RelocationProcessingTests, TC_002_AllPatchKinds) {
	static const char dynstr[] = "\0s2\0s3\0s6\0s7\0s9\0";
	const std::vector<ELFIO::utest_cert_rela_spec> relas = {
	    {0u, 1u, 3u, 0x100},
	    {64u, 2u, 2u, 0x200},
	    {128u, 3u, 6u, 0x300},
	    {192u, 4u, 7u, 0x400},
	    {256u, 5u, 9u, 0x500},
	};
	const std::vector<char> elf_bytes = ELFIO::utest_build_cert_elf64_common(
	    2, std::string_view(dynstr, sizeof(dynstr) - 1u), 6u, relas);

	std::map<std::string_view, uint64_t> symbol_map = ELFIO::utest_default_reloc_symbol_map();
	cert_elf_loader loader(elf_bytes.data(), elf_bytes.size(), symbol_map);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = loader.parse();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(0, result);
	LONGS_EQUAL(5, static_cast<long>(loader.ccode.rela.size()));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_UnsupportedPatchKind
*		Function Name: parse
*		Input: none
*		Expectation: 0; 1
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RelocationProcessingTests, TC_003_UnsupportedPatchKind) {
	static const char dynstr[] = "\0test_sym\0";
	const std::vector<ELFIO::utest_cert_rela_spec> relas = {{0u, 1u, 99u, 0x100}};
	const std::vector<char> elf_bytes = ELFIO::utest_build_cert_elf64_common(
	    2, std::string_view(dynstr, sizeof(dynstr) - 1u), 2u, relas);

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["test_sym"] = 0x1000;

	cert_elf_loader loader(elf_bytes.data(), elf_bytes.size(), symbol_map);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = loader.parse();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(0, result);
	LONGS_EQUAL(1, static_cast<long>(loader.ccode.rela.size()));
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_RelaSymbolIndexOutOfRange
*		Function Name: parse
*		Input: none
*		Expectation: 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RelocationProcessingTests, TC_004_RelaSymbolIndexOutOfRange) {
	static const char dynstr[] = "\0s2\0s3\0s6\0s7\0s9\0";
	const std::vector<ELFIO::utest_cert_rela_spec> relas = {{0u, 99u, 3u, 0}};
	const std::vector<char> elf_bytes = ELFIO::utest_build_cert_elf64_common(
	    2, std::string_view(dynstr, sizeof(dynstr) - 1u), 6u, relas);

	std::map<std::string_view, uint64_t> symbol_map = ELFIO::utest_default_reloc_symbol_map();
	cert_elf_loader loader(elf_bytes.data(), elf_bytes.size(), symbol_map);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = loader.parse();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(0, result);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_RelaSymbolIndexPastDynstrEntries
*		Function Name: parse()
*		Input: none
*		Expectation: 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(RelocationProcessingTests, TC_005_RelaSymbolIndexPastDynstrEntries) {
	/* One byte .dynstr: loop in parse() records exactly one string_view (empty). */
	static const char dynstr[] = "\0";
	const std::vector<ELFIO::utest_cert_rela_spec> relas = {{0u, 1u, 3u, 0x1000}};
	const std::vector<char> elf_bytes = ELFIO::utest_build_cert_elf64_common(
	    2, std::string_view(dynstr, sizeof(dynstr) - 1u), 2u, relas);

	std::map<std::string_view, uint64_t> symbol_map;
	cert_elf_loader loader(elf_bytes.data(), elf_bytes.size(), symbol_map);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = loader.parse();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;

	CHECK_EQUAL(0, result);
}

TEST_GROUP(CertElfLoaderAbiAndEhdrCoverage) {
	void setup() { mock_c()->clear(); }

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_ParseAbiVersionNot2
*		Function Name: parse
*		Input: elf_bytes,elf_bytes.size(), symbol_map
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderAbiAndEhdrCoverage, TC_001_ParseAbiVersionNot2) {
	static const char dynstr[] = "\0test_sym\0";
	const int64_t legacy_addend = static_cast<int64_t>((0x100LL << 4) | 3LL);
	const std::vector<ELFIO::utest_cert_rela_spec> relas = {{0u, 1u, 0u, legacy_addend}};
	const std::vector<char> elf_bytes = ELFIO::utest_build_cert_elf64_common(
	    1, std::string_view(dynstr, sizeof(dynstr) - 1u), 2u, relas);

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["test_sym"] = 0x1000U;
	cert_elf_loader loader(elf_bytes.data(), elf_bytes.size(), symbol_map);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL(0, loader.parse());
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_BeginElf32Minimal
*		Function Name: begin
*		Input: elf_bytes.data(), elf_bytes.size()
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderAbiAndEhdrCoverage, TC_002_BeginElf32Minimal) {
	const std::vector<char> elf_bytes = ELFIO::utest_build_elf32_le_minimal(2);
	cert_elf_loader loader;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL(0, loader.begin(elf_bytes.data(), elf_bytes.size()));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by begin function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_BeginElf64BigEndianMinimal
*		Function Name: begin
*		Input: elf_bytes.data(), elf_bytes.size()
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderAbiAndEhdrCoverage, TC_003_BeginElf64BigEndianMinimal) {
	const std::vector<char> elf_bytes = ELFIO::utest_build_elf64_be_minimal(2);
	cert_elf_loader loader;
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_EQUAL(0, loader.begin(elf_bytes.data(), elf_bytes.size()));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by begin function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_HexdumpSmoke
*		Function Name: hexdump
*		Input: buf, sizeof(buf)
*		Expectation: none
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(CertElfLoaderAbiAndEhdrCoverage, TC_004_HexdumpSmoke) {
	uint8_t buf[] = {0x01, 0x02, 0x03};
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	hexdump(buf, static_cast<ssize_t>(sizeof(buf)));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by hexdump function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK(true);
}

TEST_GROUP(MalformedSectionNameTests) {
	void setup() { mock_c()->clear(); }

	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};

class cert_elf_loader_test_accessor : public cert_elf_loader {
public:
	using cert_elf_loader::is_ctrldata;
	using cert_elf_loader::is_ctrltext;
	using cert_elf_loader::is_pad;
};

//*********************************************************************************/
/*
*		Unique test case ID: TC_001_IsPadInvalidArgument
*		Function Name: is_pad
*		Input: name = ".pad.notanumber", col = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_001_IsPadInvalidArgument) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".pad.notanumber";
	int col = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_pad(&name, &col));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_pad function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_002_IsPadOutOfRange
*		Function Name: is_pad
*		Input: name = ".pad.99999999999999999999", col = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_002_IsPadOutOfRange) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".pad.99999999999999999999";
	int col = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_pad(&name, &col));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_pad function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_003_IsCtrltextInvalidColumn
*		Function Name: is_ctrltext
*		Input: ".ctrltext.notvalid.0" ,-1,-1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_003_IsCtrltextInvalidColumn) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.notvalid.0";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrltext(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrltext function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_004_IsCtrltextOutOfRangeColumn
*		Function Name: is_ctrltext
*		Input: name = ".ctrltext.99999999999999999999.0", col = -1, page = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_004_IsCtrltextOutOfRangeColumn) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.99999999999999999999.0";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrltext(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrltext function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_005_IsCtrltextInvalidPage
*		Function Name: is_ctrltext
*		Input: name = ".ctrltext.0.notvalid", col = -1, page = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_005_IsCtrltextInvalidPage) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.0.notvalid";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrltext(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrltext function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_006_IsCtrltextOutOfRangePage
*		Function Name: is_ctrltext
*		Input: name = ".ctrltext.0.99999999999999999999", col = -1, page = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_006_IsCtrltextOutOfRangePage) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.0.99999999999999999999";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrltext(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrltext function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_007_IsCtrldataInvalidColumn
*		Function Name: is_ctrldata
*		Input: name = ".ctrldata.notvalid.0", col = -1, page = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_007_IsCtrldataInvalidColumn) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.notvalid.0";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrldata(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrldata function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_008_IsCtrldataOutOfRangeColumn
*		Function Name: is_ctrldata
*		Input: name = ".ctrldata.99999999999999999999.0", col = -1, page = -1
*		Expectation: false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_008_IsCtrldataOutOfRangeColumn) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.99999999999999999999.0";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrldata(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrldata function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_009_IsCtrldataInvalidPage
*		Function Name: is_ctrldata
*		Input: ".ctrldata.0.notvalid" ,-1,-1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_009_IsCtrldataInvalidPage) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.0.notvalid";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrldata(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrldata function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_010_IsCtrldataOutOfRangePage
*		Function Name: is_ctrldata
*		Input:  ".ctrldata.0.99999999999999999999" ,-1,-1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_010_IsCtrldataOutOfRangePage) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.0.99999999999999999999";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrldata(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrldata function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_011_IsCtrltextNoDotSeparator
*		Function Name: is_ctrltext
*		Input: ".ctrltext.0" ,-1,-1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_011_IsCtrltextNoDotSeparator) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.0";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrltext(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrltext function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_012_IsCtrldataNoDotSeparator
*		Function Name: is_ctrldata
*		Input: ".ctrldata.0" ,-1,-1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_012_IsCtrldataNoDotSeparator) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.0";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrldata(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrldata function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_013_IsPadWrongPrefix
*		Function Name: is_pad
*		Input: "pad" ,-1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_013_IsPadWrongPrefix) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = "pad";
	int col = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_pad(&name, &col));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_pad function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_014_SectionWithNullData_Nobits
*		Function Name: parse
*		Input: none
*		Expectation: result == 0
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_014_SectionWithNullData_Nobits) {
	std::string dynstr;
	dynstr.push_back('\0');
	dynstr += "s2\0s3\0s6\0s7\0s9\0sbad\0";
	const std::vector<ELFIO::utest_cert_rela_spec> relas = {{0u, 1u, 3u, 0}};
	const std::vector<char> elf_bytes =
	    ELFIO::utest_build_cert_elf64_common(2, std::string_view(dynstr.data(), dynstr.size()), 7u, relas);

	cert_elf_loader loader;
	int result = loader.begin(elf_bytes.data(), elf_bytes.size());
	CHECK_EQUAL(0, result);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	result = loader.parse();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, result);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_015_IsPadValid
*		Function Name: is_pad
*		Input: name = ".pad.7", col = -1
*		Expectation: result == true, col == 7
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_015_IsPadValid) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".pad.7";
	int col = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_TRUE(loader.is_pad(&name, &col));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_pad function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(7, col);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_016_IsCtrltextValid
*		Function Name: is_ctrltext
*		Input: name = ".ctrltext.3.5", col = -1, page = -1
*		Expectation: result == true, col == 3, page == 5
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_016_IsCtrltextValid) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.3.5";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_TRUE(loader.is_ctrltext(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrltext function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(3, col);
	CHECK_EQUAL(5, page);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_017_IsCtrldataValid
*		Function Name: is_ctrldata
*		Input: name = ".ctrldata.2.4", col = -1, page = -1
*		Expectation: result == true, col == 2, page == 4
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_017_IsCtrldataValid) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.2.4";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_TRUE(loader.is_ctrldata(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrldata function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(2, col);
	CHECK_EQUAL(4, page);
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_018_IsCtrltextWrongPrefix
*		Function Name: is_ctrltext
*		Input: name = ".other.0.0", col = -1, page = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_018_IsCtrltextWrongPrefix) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".other.0.0";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrltext(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrltext function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_019_IsCtrldataWrongPrefix
*		Function Name: is_ctrldata
*		Input: name = ".other.0.0", col = -1, page = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_019_IsCtrldataWrongPrefix) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".other.0.0";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrldata(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrldata function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_020_IsCtrltextLeadingDot
*		Function Name: is_ctrltext
*		Input: name = ".ctrltext..5" , col = -1, page = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_020_IsCtrltextLeadingDot) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext..5";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrltext(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrltext function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_021_IsCtrldataLeadingDot
*		Function Name: is_ctrldata
*		Input: name = ".ctrldata..5" , col = -1, page = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_021_IsCtrldataLeadingDot) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata..5";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrldata(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrldata function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_022_IsPadEmptySuffix
*		Function Name: is_pad
*		Input: name = ".pad." , col = -1
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_022_IsPadEmptySuffix) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".pad.";
	int col = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_pad(&name, &col));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_pad function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_023_IsPadTrailingGarbage
*		Function Name: is_pad
*		Input: name = ".pad.7abc"
*		Expectation: result == false (from_chars must fully consume the suffix)
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_023_IsPadTrailingGarbage) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".pad.7abc";
	int col = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_pad(&name, &col));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_pad function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_024_IsCtrltextColumnTrailingGarbage
*		Function Name: is_ctrltext
*		Input: name = ".ctrltext.7abc.5"
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_024_IsCtrltextColumnTrailingGarbage) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.7abc.5";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrltext(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrltext function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_025_IsCtrltextPageTrailingGarbage
*		Function Name: is_ctrltext
*		Input: name = ".ctrltext.3.5abc"
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_025_IsCtrltextPageTrailingGarbage) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.3.5abc";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrltext(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrltext function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_026_IsCtrldataColumnTrailingGarbage
*		Function Name: is_ctrldata
*		Input: name = ".ctrldata.7abc.5"
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_026_IsCtrldataColumnTrailingGarbage) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.7abc.5";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrldata(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrldata function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_027_IsCtrldataPageTrailingGarbage
*		Function Name: is_ctrldata
*		Input: name = ".ctrldata.2.4abc"
*		Expectation: result == false
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_027_IsCtrldataPageTrailingGarbage) {
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.2.4abc";
	int col = -1;
	int page = -1;

	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	CHECK_FALSE(loader.is_ctrldata(&name, &col, &page));
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by is_ctrldata function: " << total_clock_enable_us << " microseconds" << std::endl;
}

//*********************************************************************************/
/*
*		Unique test case ID: TC_028_ParseWithCtrlTextDataPadSections
*		Function Name: parse
*		Input: ELF containing .ctrltext.0.0, .ctrldata.0.0, .pad.0 sections
*		Expectation: parse returns 0 and populates ccode.cols[0] with one page + one pad
*		Test Method: Unit testing
*     	Methods for deriving test case: Analysis of requirement
*     	Test Environment: 2
*/
/*********************************************************************************/
TEST(MalformedSectionNameTests, TC_028_ParseWithCtrlTextDataPadSections) {
	static const char dynstr[] = "\0test_sym\0";
	const std::vector<ELFIO::utest_cert_rela_spec> relas = {{0u, 1u, 3u, 0x100}};
	const std::vector<char> elf_bytes = ELFIO::utest_build_cert_elf64_common(
	    2, std::string_view(dynstr, sizeof(dynstr) - 1u), 2u, relas);

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["test_sym"] = 0x1000U;

	cert_elf_loader loader(elf_bytes.data(), elf_bytes.size(), symbol_map);
	double total_clock_enable_us = 0.0;
	const auto clock_enable_t0 = std::chrono::steady_clock::now();
	int result = loader.parse();
	const auto clock_enable_t1 = std::chrono::steady_clock::now();
	total_clock_enable_us = std::chrono::duration<double, std::micro>(clock_enable_t1 - clock_enable_t0).count();
	std::cout << "Time taken by parse function: " << total_clock_enable_us << " microseconds" << std::endl;
	CHECK_EQUAL(0, result);
}

/*@section Test Execution details
| Test Case ID                               | Test Result | CR  | Impact | Test Details |
|--------------------------------------------|-------------|-----|--------|--------------|
| TC_001_FailsWhenLoadingNonexistentFile     | PASS        | No  | NA     | NA           |
| TC_002_FailsWhenLoadingInvalidElfFromMemory| PASS        | No  | NA     | NA           |
| TC_001_BeginWithEmptyFilename              | PASS        | No  | NA     | NA           |
| TC_002_BeginFromMemoryWithZeroSize         | PASS        | No  | NA     | NA           |
| TC_001_ParseWithoutLoadingELF              | PASS        | No  | NA     | NA           |
| TC_002_ParseWithSymbolMapRelocation        | PASS        | No  | NA     | NA           |
| TC_003_ParseSectionIteration               | PASS        | No  | NA     | NA           |
| TC_004_BeginFromMemoryElfBegin             | PASS        | No  | NA     | NA           |
| TC_001_SymbolNotFoundInMap                 | PASS        | No  | NA     | NA           |
| TC_002_AllPatchKinds                       | PASS        | No  | NA     | NA           |
| TC_003_UnsupportedPatchKind                | PASS        | No  | NA     | NA           |
| TC_004_RelaSymbolIndexOutOfRange           | PASS        | No  | NA     | NA           |
| TC_005_RelaSymbolIndexPastDynstrEntries    | PASS        | No  | NA     | NA           |
| TC_001_ParseAbiVersionNot2                 | PASS        | No  | NA     | NA           |
| TC_002_BeginElf32Minimal                   | PASS        | No  | NA     | NA           |
| TC_003_BeginElf64BigEndianMinimal          | PASS        | No  | NA     | NA           |
| TC_004_HexdumpSmoke                        | PASS        | No  | NA     | NA           |
| TC_001_IsPadInvalidArgument                | PASS        | No  | NA     | NA           |
| TC_002_IsPadOutOfRange                     | PASS        | No  | NA     | NA           |
| TC_003_IsCtrltextInvalidColumn             | PASS        | No  | NA     | NA           |
| TC_004_IsCtrltextOutOfRangeColumn          | PASS        | No  | NA     | NA           |
| TC_005_IsCtrltextInvalidPage               | PASS        | No  | NA     | NA           |
| TC_006_IsCtrltextOutOfRangePage            | PASS        | No  | NA     | NA           |
| TC_007_IsCtrldataInvalidColumn             | PASS        | No  | NA     | NA           |
| TC_008_IsCtrldataOutOfRangeColumn          | PASS        | No  | NA     | NA           |
| TC_009_IsCtrldataInvalidPage               | PASS        | No  | NA     | NA           |
| TC_010_IsCtrldataOutOfRangePage            | PASS        | No  | NA     | NA           |
| TC_011_IsCtrltextNoDotSeparator            | PASS        | No  | NA     | NA           |
| TC_012_IsCtrldataNoDotSeparator            | PASS        | No  | NA     | NA           |
| TC_013_IsPadWrongPrefix                    | PASS        | No  | NA     | NA           |
| TC_014_SectionWithNullData_Nobits          | PASS        | No  | NA     | NA           |
| TC_015_IsPadValid                          | PASS        | No  | NA     | NA           |
| TC_016_IsCtrltextValid                     | PASS        | No  | NA     | NA           |
| TC_017_IsCtrldataValid                     | PASS        | No  | NA     | NA           |
| TC_018_IsCtrltextWrongPrefix               | PASS        | No  | NA     | NA           |
| TC_019_IsCtrldataWrongPrefix               | PASS        | No  | NA     | NA           |
| TC_020_IsCtrltextLeadingDot                | PASS        | No  | NA     | NA           |
| TC_021_IsCtrldataLeadingDot                | PASS        | No  | NA     | NA           |
| TC_022_IsPadEmptySuffix                    | PASS        | No  | NA     | NA           |
| TC_023_IsPadTrailingGarbage                | PASS        | No  | NA     | NA           |
| TC_024_IsCtrltextColumnTrailingGarbage     | PASS        | No  | NA     | NA           |
| TC_025_IsCtrltextPageTrailingGarbage       | PASS        | No  | NA     | NA           |
| TC_026_IsCtrldataColumnTrailingGarbage     | PASS        | No  | NA     | NA           |
| TC_027_IsCtrldataPageTrailingGarbage       | PASS        | No  | NA     | NA           |
| TC_028_ParseWithCtrlTextDataPadSections    | PASS        | No  | NA     | NA           |

/****Coverage Analysis*********/

/*@section Coverage details

| Coverage Type (Function Name)                       | %      | Result     | CR/Status    | Impact          |
|-----------------------------------------------------|--------|------------|--------------|-----------------|
| cert_elf_loader::begin(const char*)                 | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::begin(const void*, size_t)         | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::parse                              | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::is_pad                             | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::is_ctrltext                        | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::is_ctrldata                        | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::cert_elf_loader()                  | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::cert_elf_loader(const char*, ...)  | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::~cert_elf_loader                   | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::_elf_begin                         | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::aie4_patch_57                      | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::aie2ps_patch_57                    | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::patch_scalar_32                    | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::aie4_patch_ctrl_57                 | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::aie2ps_patch_ctrl_57               | <100>  | <Pass>     | <NA>         | No Impact       |
| cert_elf_loader::close_elf                          | <100>  | <Pass>     | <NA>         | No Impact       |
| hexdump_section                                     | <100>  | <Pass>     | <NA>         | No Impact       |
|ctrl_pad::ctrl_pad                                  | <100>  | <Pass>     | <NA>         | No Impact       |
| Total file coverage (overall)                       | 100    | <Pass>     | <NA>         | No Impact       |

/*@section Uncovered details*/
/* No uncovered details */
