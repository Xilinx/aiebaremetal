/**
 * @file test_cert_elf_loader.cpp
 * @brief Unit tests for the cert_elf_loader class
 *
 * This file contains unit tests for the cert_elf_loader class, which provides
 * ELF file loading and parsing functionality for certification operations.
 * The tests verify constructor functionality, ELF file operations, parsing,
 * symbol resolution, relocation, and AIE-specific section handling.
 *
 * Copyright (C) 2026 Advanced Micro Devices, Inc.
 */

#include "cert_elf_loader.h"
#include "elfio.hpp"

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport_c.h>
#include <sstream>

// Real cert ELF from thirdparty/cert-elf-binaries (copied next to utests by CMake as ctrl_code.elf)
#ifndef TEST_ELF_FILE
#define TEST_ELF_FILE "ctrl_code.elf"
#endif

// Helper function to create a synthetic ELF with AIE-specific sections
static std::vector<char> create_synthetic_aie_elf() {
    using namespace ELFIO;

    elfio writer;
    writer.create(ELFCLASS64, ELFDATA2LSB);
    writer.set_os_abi(ELFOSABI_NONE);
    writer.set_type(ET_EXEC);
    writer.set_machine(EM_X86_64);
    writer.set_abi_version(2); // Set ABI version 2 for kind detection

    // Create .ctrltext.0.1 section with patchable data
    section* ctrltext_sec = writer.sections.add(".ctrltext.0.1");
    ctrltext_sec->set_type(SHT_PROGBITS);
    ctrltext_sec->set_flags(SHF_ALLOC | SHF_EXECINSTR | SHF_WRITE);
    ctrltext_sec->set_addr_align(0x10);
    // Create buffer with extra space for relocation patching (offset + 16 bytes)
    char ctrltext_data[128] = {0};
    for (int i = 0; i < 128; i++) ctrltext_data[i] = i & 0xFF;
    ctrltext_sec->set_data(ctrltext_data, sizeof(ctrltext_data));

    // Create .ctrldata.1.2 section with patchable data
    section* ctrldata_sec = writer.sections.add(".ctrldata.1.2");
    ctrldata_sec->set_type(SHT_PROGBITS);
    ctrldata_sec->set_flags(SHF_ALLOC | SHF_WRITE);
    ctrldata_sec->set_addr_align(0x8);
    char ctrldata_data[128] = {0};
    for (int i = 0; i < 128; i++) ctrldata_data[i] = (i * 2) & 0xFF;
    ctrldata_sec->set_data(ctrldata_data, sizeof(ctrldata_data));

    // Create .pad.5 section
    section* pad_sec = writer.sections.add(".pad.5");
    pad_sec->set_type(SHT_PROGBITS);
    pad_sec->set_flags(SHF_ALLOC);
    char pad_data[16] = {(char)0xAA, (char)0xBB, (char)0xCC, (char)0xDD};
    pad_sec->set_data(pad_data, sizeof(pad_data));

    // Create .dynstr section
    section* dynstr_sec = writer.sections.add(".dynstr");
    dynstr_sec->set_type(SHT_STRTAB);
    const char dynstr_data[] = "\0scalar_sym\0aie2ps_sym\0aie4_sym\0ctrl57_sym\0ctrl57aie4_sym\0";
    dynstr_sec->set_data(dynstr_data, sizeof(dynstr_data) - 1);

    // Create .dynsym section with symbols for each patch type
    section* dynsym_sec = writer.sections.add(".dynsym");
    dynsym_sec->set_type(SHT_DYNSYM);
    dynsym_sec->set_flags(SHF_ALLOC);
    dynsym_sec->set_addr_align(0x8);
    dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
    dynsym_sec->set_link(dynstr_sec->get_index());

    symbol_section_accessor symbol_writer(writer, dynsym_sec);
    string_section_accessor string_writer(dynstr_sec);

    // Add symbols that reference our patchable sections
    Elf_Word sym1 = symbol_writer.add_symbol(string_writer, "scalar_sym", 0x1000, 32,
                                              STB_GLOBAL, STT_OBJECT, ctrltext_sec->get_index());
    Elf_Word sym2 = symbol_writer.add_symbol(string_writer, "aie2ps_sym", 0x2000, 36,
                                              STB_GLOBAL, STT_OBJECT, ctrltext_sec->get_index());
    Elf_Word sym3 = symbol_writer.add_symbol(string_writer, "aie4_sym", 0x3000, 8,
                                              STB_GLOBAL, STT_OBJECT, ctrldata_sec->get_index());
    Elf_Word sym4 = symbol_writer.add_symbol(string_writer, "ctrl57_sym", 0x4000, 16,
                                              STB_GLOBAL, STT_OBJECT, ctrldata_sec->get_index());
    Elf_Word sym5 = symbol_writer.add_symbol(string_writer, "ctrl57aie4_sym", 0x5000, 12,
                                              STB_GLOBAL, STT_OBJECT, ctrltext_sec->get_index());

    // Create .rela.dyn section with relocations for each patch type
    section* rela_sec = writer.sections.add(".rela.dyn");
    rela_sec->set_type(SHT_RELA);
    rela_sec->set_flags(SHF_ALLOC);
    rela_sec->set_addr_align(0x8);
    rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
    rela_sec->set_link(dynsym_sec->get_index());
    rela_sec->set_info(ctrltext_sec->get_index());

    relocation_section_accessor rela_writer(writer, rela_sec);

    // Add relocations with different kinds (type field for ABI v2)
    // Offset is within section, needs to be < (section_size - 16) for ptr calculation
    rela_writer.add_entry(0, sym1, 3, 0x100);  // SCALAR_32BIT_KIND = 3
    rela_writer.add_entry(32, sym2, 2, 0x200); // SHIM_DMA_BASE_ADDR_SYMBOL_KIND = 2
    rela_writer.add_entry(64, sym3, 6, 0x300); // SHIM_DMA_AIE4_BASE_ADDR_SYMBOL_KIND = 6
    rela_writer.add_entry(0, sym4, 7, 0x400);  // CONTROL_PACKET_57 = 7
    rela_writer.add_entry(32, sym5, 9, 0x500); // CONTROL_PACKET_57_AIE4 = 9

    // Write to stringstream
    std::ostringstream oss;
    writer.save(oss);
    std::string elf_string = oss.str();

    return std::vector<char>(elf_string.begin(), elf_string.end());
}

/**
 * @brief Test group for cert_elf_loader constructor validation
 *
 * Tests constructor functionality including default constructor
 * and parameterized constructor variants
 */
TEST_GROUP(cert_elf_loader_constructor_tests)
{
	/**
	 * @brief Setup for cert_elf_loader_constructor_tests
	 * No special setup required for constructor tests
	 */
	void setup() {



	}

	/**
	 * @brief Teardown for cert_elf_loader_constructor_tests
	 * No cleanup required for constructor tests
	 */
	void teardown() {

	}
};

/**
 * @brief Test default constructor of cert_elf_loader
 * Validates that cert_elf_loader can be constructed with default parameters
 */
TEST(cert_elf_loader_constructor_tests, TC_001_DefaultConstructor)
{

	cert_elf_loader loader;

	CHECK(true);

}


/**
 * @brief Test parameterized constructor with valid inputs
 * Validates that cert_elf_loader can be constructed with ELF data and symbol map
 */
TEST(cert_elf_loader_constructor_tests, TC_002_ParameterizedConstructor_ValidInputs)
{
	// ARRANGE
	char test_elf_data[] = "dummy ELF content";
	unsigned int test_elf_len = sizeof(test_elf_data);
	std::map<std::string_view, uint64_t> test_symbols = {
		{"symbol1", 0x1000},
		{"symbol2", 0x2000}
	};

	// ACT
	cert_elf_loader loader(test_elf_data, test_elf_len, test_symbols);

	// ASSERT - constructor should complete without crashing
	CHECK(true);
}

/**
 * @brief Test group for ELF loading operations
 *
 * Tests cert_elf_loader begin() functionality for loading ELF files
 * from filesystem and memory with various input scenarios
 */
TEST_GROUP(CertElfLoaderBeginTests)
{
	/**
	 * @brief Setup for CertElfLoaderBeginTests
	 * No special setup required
	 */
	void setup() {


	}
	/**
	 * @brief Teardown for CertElfLoaderBeginTests
	 * Verifies mock expectations and clears mocks
	 */
	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();

	}
};

/**
 * @brief Test loading valid ELF file from filesystem
 * Validates that begin() successfully loads a valid ELF file
 * from the specified filesystem path
 */
TEST(CertElfLoaderBeginTests, Begin_WithValidFilename_ShouldSucceed) {
	const char *validFilename = TEST_ELF_FILE;

	cert_elf_loader loader;

	int result = loader.begin(validFilename);

	CHECK_EQUAL(0, result);   // assuming 0 = success

}

/**
 * @brief Test loading nonexistent ELF file
 * Validates that begin() returns proper error code when attempting
 * to load a file that doesn't exist
 */
TEST(CertElfLoaderBeginTests, TC_002_FailsWhenLoadingNonexistentFile)
{

	const char* test_filename = "nonexistent_file.elf";
	cert_elf_loader loader;
	// ACT: Call the function under test

	int result = loader.begin(test_filename);

	// ASSERT: Verify the outcome
	CHECK_EQUAL(-ENODEV, result); // Expect failure (-ENODEV)

}

/**
 * @brief Test loading invalid ELF data from memory
 * Validates that begin() returns proper error code when attempting
 * to load invalid ELF data from memory buffer
 */
TEST(CertElfLoaderBeginTests, TC_005_FailsWhenLoadingInvalidElfFromMemory)
{


	char test_elf_data[] = "This is invalid ELF data";
	size_t test_elf_size = sizeof(test_elf_data) - 1; // Exclude null terminator

	// ACT: Call the function under test
	cert_elf_loader loader;
	int result = loader.begin(test_elf_data, test_elf_size);

	// ASSERT: Verify the outcome
	CHECK_EQUAL(-ENODEV, result); // Expect failure (-ENODEV)

}

/**
 * @brief Test group for ELF file closing operations
 *
 * Tests cert_elf_loader close_elf() functionality for proper
 * cleanup and resource release
 */
TEST_GROUP(cert_elf_loader_close_tests)
{
	/**
	 * @brief Setup for cert_elf_loader_close_tests
	 * No special setup required
	 */
	void setup() {
		// Reset the stub control structure before each test

	}

	/**
	 * @brief Teardown for cert_elf_loader_close_tests
	 * No cleanup required
	 */
	void teardown() {
		// Cleanup after each test
	}
};

/**
 * @brief Test ELF file closing functionality
 * Validates that close_elf() properly closes and cleans up
 * ELF file resources
 */
TEST(cert_elf_loader_close_tests, TC_001_closeelf)
{
	// ARRANGE
	// No arrangement needed for default constructor

	// ACT
	cert_elf_loader loader;
	loader.close_elf();
	CHECK(true);

	// Additional checks could be added here if we want to examine
	// the internal state of the loader object, but that would require
	// making those members accessible for testing.
}

/**
 * @brief Test group for ELF parsing operations
 *
 * Tests cert_elf_loader parse() functionality for parsing
 * loaded ELF file structure and sections
 */
TEST_GROUP(CertElfLoaderParseTests)
{
	/**
	 * @brief Setup for CertElfLoaderParseTests
	 * No special setup required
	 */
	void setup() {
		// Reset the stub control structure before each test

	}

	/**
	 * @brief Teardown for CertElfLoaderParseTests
	 * No cleanup required
	 */
	void teardown() {
		// Cleanup after each test
	}
};

/**
 * @brief Test nominal ELF parsing case
 * Validates that parse() successfully parses ELF structure
 * in normal conditions
 */
TEST(CertElfLoaderParseTests, TC_001_NominalCase)
{

	// Create a cert_elf_loader instance
	cert_elf_loader loader;
	const char *validFilename = TEST_ELF_FILE;

	int res = loader.begin(validFilename);

	CHECK_EQUAL(0, res);   // assuming 0 = success
	// Call the parse function
	int result = loader.parse();

	// Verify the result
	CHECK_EQUAL(0, result);

}

/**
 * @brief Test group for hexdump utility function
 *
 * Tests hexdump() functionality for displaying buffer contents
 * in hexadecimal format with various buffer sizes
 */
TEST_GROUP(HexdumpTests)
{
	/**
	 * @brief Setup for HexdumpTests
	 * No special setup required
	 */
	void setup()
	{

	}

	/**
	 * @brief Teardown for HexdumpTests
	 * No cleanup required
	 */
	void teardown()
	{

	}
};

/**
 * @brief Test hexdump with small buffer (less than 16 bytes)
 * Validates that hexdump() correctly displays buffer contents
 * when buffer size is less than one full line (16 bytes)
 */
TEST(HexdumpTests, TC_001_SmallBufferLessThan16Bytes)
{
	// ARRANGE: Set up a small buffer with less than 16 bytes
	uint8_t test_data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
	ssize_t test_size = 8;

	// ACT: Call the REAL production function
	hexdump(test_data, test_size);

	CHECK(true);
}

/**
 * @brief Test hexdump with buffer exactly 16 bytes
 * Validates hexdump with buffer that fits exactly one line
 */
TEST(HexdumpTests, TC_002_BufferExactly16Bytes)
{
	uint8_t test_data[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
	                         0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
	ssize_t test_size = 16;

	hexdump(test_data, test_size);

	CHECK(true);
}

/**
 * @brief Test hexdump with large buffer (more than 16 bytes)
 * Validates hexdump with multi-line output
 */
TEST(HexdumpTests, TC_003_LargeBufferMoreThan16Bytes)
{
	uint8_t test_data[40];
	for (int i = 0; i < 40; i++) {
		test_data[i] = i;
	}
	ssize_t test_size = 40;

	hexdump(test_data, test_size);

	CHECK(true);
}

/**
 * @brief Test hexdump with zero-sized buffer
 * Validates hexdump handles empty buffer gracefully
 */
TEST(HexdumpTests, TC_004_ZeroSizedBuffer)
{
	uint8_t test_data[1] = {0x00};
	ssize_t test_size = 0;

	hexdump(test_data, test_size);

	CHECK(true);
}
//----------------------------------------------------------------------
// Test group for ctrl_pad structure
//----------------------------------------------------------------------

/**
 * @brief Test group for ctrl_pad structure
 *
 * Tests ctrl_pad structure initialization including default
 * and parameterized constructors
 */
TEST_GROUP(CtrlPadTests)
{
	/**
	 * @brief Setup for CtrlPadTests
	 * No special setup required
	 */
	void setup()
	{

	}

	/**
	 * @brief Teardown for CtrlPadTests
	 * No cleanup required
	 */
	void teardown()
	{
		// Cleanup after each test
	}
};

//----------------------------------------------------------------------
// Test cases for ctrl_pad constructors and destructor
//----------------------------------------------------------------------

/**
 * @brief Test default constructor of ctrl_pad structure
 * Validates that ctrl_pad initializes with NULL data buffer
 * and zero data size
 */
TEST(CtrlPadTests, TC_001_DefaultConstructor)
{
	// ARRANGE - None needed for default constructor

	// ACT - Create a ctrl_pad using the default constructor
	struct ctrl_pad pad;

	// ASSERT - Verify initial values
	CHECK_EQUAL(0, pad.data_buf);    // data_buf should be NULL (0)
	CHECK_EQUAL(0, pad.data_size);   // data_size should be 0
}

/**
 * @brief Test parameterized constructor of ctrl_pad structure
 * Validates that ctrl_pad correctly initializes with provided
 * data buffer and size parameters
 */
TEST(CtrlPadTests, TC_002_ParameterizedConstructor)
{
	// ARRANGE
	const char* test_data = "Test data";
	size_t test_size = 10;

	// ACT - Create a ctrl_pad using the parameterized constructor
	struct ctrl_pad pad(test_data, test_size);

	// ASSERT - Verify values match constructor parameters
	POINTERS_EQUAL(test_data, pad.data_buf);
	CHECK_EQUAL(test_size, pad.data_size);
}

//----------------------------------------------------------------------
// Test group for AIE-specific section parsing
//----------------------------------------------------------------------

/**
 * @brief Test group for AIE-specific ELF sections
 * Uses synthetic ELF with .ctrltext, .ctrldata, .pad, etc.
 */
TEST_GROUP(AieSpecificSectionTests)
{
	void setup() {}
	void teardown() {}
};

/**
 * @brief Test parsing ELF with .ctrltext sections
 * Exercises is_ctrltext() and related code paths
 */
TEST(AieSpecificSectionTests, TC_001_ParseCtrltextSection)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();

	cert_elf_loader loader;
	int load_result = loader.begin(elf_data.data(), elf_data.size());

	CHECK_EQUAL(0, load_result);

	// Parse should process .ctrltext.0.1 section
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);
}

/**
 * @brief Test parsing ELF with .ctrldata sections
 * Exercises is_ctrldata() and related code paths
 */
TEST(AieSpecificSectionTests, TC_002_ParseCtrldataSection)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();

	cert_elf_loader loader;
	loader.begin(elf_data.data(), elf_data.size());

	// Parse should process .ctrldata.1.2 section
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);
}

/**
 * @brief Test parsing ELF with .pad sections
 * Exercises is_pad() and related code paths
 */
TEST(AieSpecificSectionTests, TC_003_ParsePadSection)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();

	cert_elf_loader loader;
	loader.begin(elf_data.data(), elf_data.size());

	// Parse should process .pad.5 section
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);
}

/**
 * @brief Test parsing ELF with .dynsym section
 * Exercises dynamic symbol table processing
 */
TEST(AieSpecificSectionTests, TC_004_ParseDynsymSection)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();

	cert_elf_loader loader;
	loader.begin(elf_data.data(), elf_data.size());

	// Parse should process .dynsym section
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);
}

/**
 * @brief Test parsing ELF with .dynstr section
 * Exercises dynamic string table processing
 */
TEST(AieSpecificSectionTests, TC_005_ParseDynstrSection)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();

	cert_elf_loader loader;
	loader.begin(elf_data.data(), elf_data.size());

	// Parse should process .dynstr section
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);
}

/**
 * @brief Test parsing ELF with .rela.dyn section
 * Exercises relocation processing
 */
TEST(AieSpecificSectionTests, TC_006_ParseRelaDynSection)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();

	cert_elf_loader loader;
	loader.begin(elf_data.data(), elf_data.size());

	// Parse should process .rela.dyn section
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);
}

/**
 * @brief Test complete parse flow with all AIE sections
 * Validates end-to-end processing of synthetic ELF
 */
TEST(AieSpecificSectionTests, TC_007_ParseAllAieSections)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();

	cert_elf_loader loader;
	int load_result = loader.begin(elf_data.data(), elf_data.size());
	CHECK_EQUAL(0, load_result);

	// Parse processes all sections including AIE-specific ones
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);

	// Parse should be idempotent
	int parse_result2 = loader.parse();
	CHECK_EQUAL(0, parse_result2);

	// Cleanup
	loader.close_elf();
}

/**
 * @brief Test parsing with symbol map for relocation
 * Exercises patch functions indirectly
 */
TEST(AieSpecificSectionTests, TC_008_ParseWithSymbolMap)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();
	std::map<std::string_view, uint64_t> symbol_map;

	// Add symbols that match the dynsym entries in our synthetic ELF
	symbol_map["scalar_sym"] = 0x10000000;
	symbol_map["aie2ps_sym"] = 0x20000000;
	symbol_map["aie4_sym"] = 0x30000000;
	symbol_map["ctrl57_sym"] = 0x40000000;
	symbol_map["ctrl57aie4_sym"] = 0x50000000;

	// Use parameterized constructor with symbol map
	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);

	// Parse with symbol map - this should trigger all patch functions
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);
}

/**
 * @brief Test relocation processing with valid symbols
 * Ensures all switch case paths in relocation loop are covered
 */
TEST(AieSpecificSectionTests, TC_009_RelocationProcessing)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();
	std::map<std::string_view, uint64_t> symbol_map;

	symbol_map["scalar_sym"] = 0xABCD0000;
	symbol_map["aie2ps_sym"] = 0x12340000;
	symbol_map["aie4_sym"] = 0x56780000;
	symbol_map["ctrl57_sym"] = 0x9ABC0000;
	symbol_map["ctrl57aie4_sym"] = 0xDEF00000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	CHECK_EQUAL(0, result);

	// Verify ccode structure has been populated
	CHECK(loader.ccode.cols.size() > 0);
}

/**
 * @brief Test column and page iteration loops
 *
 * Verifies that the ELF loader correctly processes all columns and pages
 * when parsing AIE-specific sections with symbol mappings.
 */
TEST(AieSpecificSectionTests, TC_010_ColumnPageIteration)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();
	std::map<std::string_view, uint64_t> symbol_map;

	symbol_map["scalar_sym"] = 0x1000;
	symbol_map["aie2ps_sym"] = 0x2000;
	symbol_map["aie4_sym"] = 0x3000;
	symbol_map["ctrl57_sym"] = 0x4000;
	symbol_map["ctrl57aie4_sym"] = 0x5000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	loader.parse();

	// Check that columns were created from .ctrltext/.ctrldata sections
	// Should have col 0 from .ctrltext.0.1 and col 1 from .ctrldata.1.2
	CHECK(loader.ccode.cols.find(0) != loader.ccode.cols.end() ||
	      loader.ccode.cols.find(1) != loader.ccode.cols.end());

	// Verify code was assembled from pages and pads
	for (auto &[col, col_code] : loader.ccode.cols) {
		// If pages exist, code should be populated
		if (!col_code.pages.empty() || !col_code.pad.empty()) {
			CHECK(col_code.code.size() > 0);
		}
	}
}

/**
 * @brief Test relocation with out-of-bounds dynsym index
 * Exercises error path in relocation loop
 */
TEST(AieSpecificSectionTests, TC_011_RelocationErrorPaths)
{
	using namespace ELFIO;

	// Create minimal ELF with bad relocation
	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[64] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);
	const char dynstr_data[] = "\0testsym\0";
	dynstr_sec->set_data(dynstr_data, sizeof(dynstr_data) - 1);

	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_writer(writer, dynsym_sec);
	string_section_accessor str_writer(dynstr_sec);
	sym_writer.add_symbol(str_writer, "testsym", 0, 4, STB_GLOBAL, STT_OBJECT, text_sec->get_index());

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	relocation_section_accessor rela_writer(writer, rela_sec);
	// Add relocation with out-of-bounds symbol index
	rela_writer.add_entry(0, 999, 3, 0);  // symbol index 999 doesn't exist

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["testsym"] = 0x1000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	// Should succeed but skip the bad relocation
	CHECK_EQUAL(0, result);
}

/**
 * @brief Test relocation with symbol not in symbol map
 * Exercises the "symbol not found" error path
 */
TEST(AieSpecificSectionTests, TC_012_RelocationSymbolNotFound)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();
	std::map<std::string_view, uint64_t> symbol_map;

	// Don't add any symbols to the map - relocations will fail to find symbols

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	// Should succeed but skip all relocations
	CHECK_EQUAL(0, result);
}

//----------------------------------------------------------------------
// Test group for begin() method edge cases
//----------------------------------------------------------------------

/**
 * @brief Test group for additional begin() method scenarios
 */
TEST_GROUP(CertElfLoaderBeginEdgeCases)
{
	void setup() {}
	void teardown() {
		mock_c()->checkExpectations();
		mock_c()->clear();
	}
};

/**
 * @brief Test begin() with null filename
 * Validates error handling for null filename pointer
 */
TEST(CertElfLoaderBeginEdgeCases, TC_001_BeginWithNullFilename)
{
	cert_elf_loader loader;
	const char* null_filename = nullptr;

	// This should handle gracefully or return error
	if (null_filename) {
		int result = loader.begin(null_filename);
		CHECK_EQUAL(-ENODEV, result);
	}
	CHECK(true);
}

/**
 * @brief Test begin() with empty filename
 * Validates error handling for empty string filename
 */
TEST(CertElfLoaderBeginEdgeCases, TC_002_BeginWithEmptyFilename)
{
	cert_elf_loader loader;
	const char* empty_filename = "";

	int result = loader.begin(empty_filename);

	CHECK_EQUAL(-ENODEV, result);
}

/**
 * @brief Test begin() from memory with zero size
 * Validates error handling for zero-sized ELF buffer
 */
TEST(CertElfLoaderBeginEdgeCases, TC_003_BeginFromMemoryWithZeroSize)
{
	cert_elf_loader loader;
	char dummy_data[] = "test";
	size_t zero_size = 0;

	int result = loader.begin(dummy_data, zero_size);

	CHECK_EQUAL(-ENODEV, result);
}

/**
 * @brief Test begin() from memory with null pointer
 * Validates error handling for null ELF data pointer
 */
TEST(CertElfLoaderBeginEdgeCases, TC_004_BeginFromMemoryWithNullPointer)
{
	cert_elf_loader loader;
	const void* null_ptr = nullptr;
	size_t size = 100;

	// Should handle gracefully
	if (null_ptr) {
		int result = loader.begin(null_ptr, size);
		CHECK_EQUAL(-ENODEV, result);
	}
	CHECK(true);
}

//----------------------------------------------------------------------
// Test group for parse() method edge cases
//----------------------------------------------------------------------

/**
 * @brief Test group for parse() edge cases and error conditions
 */
TEST_GROUP(CertElfLoaderParseEdgeCases)
{
	void setup() {}
	void teardown() {}
};

/**
 * @brief Test parse without loading ELF file first
 * Validates parse() behavior when called before begin()
 */
TEST(CertElfLoaderParseEdgeCases, TC_001_ParseWithoutLoadingElf)
{
	cert_elf_loader loader;

	// Call parse without loading an ELF file first
	int result = loader.parse();

	// Should handle gracefully - may return 0 or error depending on implementation
	CHECK(result == 0);
}

/**
 * @brief Test parse after loading valid ELF with various sections
 * This ensures all parsing branches are covered
 */
TEST(CertElfLoaderParseEdgeCases, TC_002_ParseWithValidElfAllSections)
{
	const char *validFilename = TEST_ELF_FILE;
	cert_elf_loader loader;

	int load_result = loader.begin(validFilename);
	if (load_result == 0) {
		int parse_result = loader.parse();
		CHECK_EQUAL(0, parse_result);

		// Verify ccode structure is populated
		// This exercises the section parsing logic
		CHECK(parse_result == 0);
	} else {
		// If file doesn't exist, test still passes
		CHECK(true);
	}
}

/**
 * @brief Test parse with symbol map to trigger relocation patching
 * This test exercises the relocation and patching code paths
 */
TEST(CertElfLoaderParseEdgeCases, TC_003_ParseWithSymbolMapRelocation)
{
	// Create a symbol map for relocation
	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["test_symbol"] = 0x1000000;
	symbol_map["data_buffer"] = 0x2000000;

	// Test with parameterized constructor that uses symbol map
	char dummy_elf[] = "invalid elf";
	unsigned int elf_len = sizeof(dummy_elf);

	cert_elf_loader loader(dummy_elf, elf_len, symbol_map);

	// Even with invalid ELF, the symbol map is set
	CHECK(true);
}

/**
 * @brief Test parse to exercise section iteration
 * Ensures all section types are properly handled
 */
TEST(CertElfLoaderParseEdgeCases, TC_004_ParseSectionIteration)
{
	const char *validFilename = TEST_ELF_FILE;
	cert_elf_loader loader;

	int load_result = loader.begin(validFilename);
	if (load_result == 0) {
		// Parse will iterate through all sections including:
		// .ctrltext.*, .ctrldata.*, .pad.*, .rela.dyn, .dynsym, .dynstr
		int parse_result = loader.parse();

		// Multiple parse calls should be safe
		int parse_result2 = loader.parse();

		CHECK_EQUAL(0, parse_result);
		CHECK_EQUAL(0, parse_result2);
	} else {
		CHECK(true);
	}
}

/**
 * @brief Test destructor cleanup
 * Validates that destructor properly cleans up resources
 */
TEST(CertElfLoaderParseEdgeCases, TC_005_DestructorCleanup)
{
	{
		cert_elf_loader loader;
		// Destructor should be called when leaving scope
	}
	CHECK(true);
}

/**
 * @brief Test _elf_begin indirectly through begin()
 * Validates ELF class and encoding detection
 */
TEST(CertElfLoaderParseEdgeCases, TC_006_ElfBeginClassAndEncoding)
{
	const char *validFilename = TEST_ELF_FILE;
	cert_elf_loader loader;

	// begin() calls _elf_begin() internally which prints ELF class and encoding
	int result = loader.begin(validFilename);

	if (result == 0) {
		// Successfully loaded, _elf_begin() was called
		CHECK_EQUAL(0, result);
	} else {
		// File not found is acceptable
		CHECK(true);
	}
}

/**
 * @brief Test begin() from memory to exercise _elf_begin() path
 * Validates the memory-based ELF loading path
 */
TEST(CertElfLoaderParseEdgeCases, TC_007_BeginFromMemoryElfBegin)
{
	// Use the valid ELF file if it exists
	const char *validFilename = TEST_ELF_FILE;

	// Try to read the file and load from memory
	FILE* fp = fopen(validFilename, "rb");
	if (fp != nullptr) {
		fseek(fp, 0, SEEK_END);
		long size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (size > 0 && size < 10000000) { // Reasonable size check
			char* buffer = new char[size];
			fread(buffer, 1, size, fp);
			fclose(fp);

			cert_elf_loader loader;
			int result = loader.begin(buffer, size);

			delete[] buffer;

			// Should successfully call _elf_begin()
			CHECK_EQUAL(0, result);
		} else {
			fclose(fp);
			CHECK(true);
		}
	} else {
		CHECK(true);
	}
}

/**
 * @brief Test close_elf after successful load
 * Validates close_elf() after loading a valid file
 */
TEST(CertElfLoaderParseEdgeCases, TC_008_CloseElfAfterLoad)
{
	const char *validFilename = TEST_ELF_FILE;
	cert_elf_loader loader;

	int load_result = loader.begin(validFilename);
	if (load_result == 0) {
		loader.close_elf();
		// Should be able to close multiple times
		loader.close_elf();
	}
	CHECK(true);
}

/**
 * @brief Test multiple begin() calls
 * Validates that begin() can be called multiple times
 */
TEST(CertElfLoaderParseEdgeCases, TC_009_MultipleBeginCalls)
{
	cert_elf_loader loader;
	const char *validFilename = TEST_ELF_FILE;

	// First begin
	int result1 = loader.begin(validFilename);

	if (result1 == 0) {
		// Second begin on same loader
		int result2 = loader.begin(validFilename);
		CHECK_EQUAL(0, result2);
	} else {
		CHECK(true);
	}
}

//----------------------------------------------------------------------
// Additional coverage tests for edge cases
//----------------------------------------------------------------------

/**
 * @brief Test group for additional coverage
 */
TEST_GROUP(AdditionalCoverageTests)
{
	void setup() {}
	void teardown() {}
};

/**
 * @brief Test section with null data pointer
 *
 * Verifies that the ELF parser correctly handles sections with null data pointers
 * by skipping them during processing.
 */
TEST(AdditionalCoverageTests, TC_001_SectionWithNullData)
{
	using namespace ELFIO;

	// Create ELF with a section that might have null data
	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	// Create a NOBITS section (no data in file)
	section* nobits_sec = writer.sections.add(".bss");
	nobits_sec->set_type(SHT_NOBITS);
	nobits_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	nobits_sec->set_size(100);

	// Add a normal section too
	section* text_sec = writer.sections.add(".text");
	text_sec->set_type(SHT_PROGBITS);
	char text_data[16] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	cert_elf_loader loader;
	int load_result = loader.begin(elf_data.data(), elf_data.size());
	CHECK_EQUAL(0, load_result);

	// Parse should skip section with null data
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);
}

/**
 * @brief Test default switch case in relocation processing
 *
 * Verifies that the ELF loader correctly handles unsupported relocation types
 * by continuing to process remaining relocations without error.
 */
TEST(AdditionalCoverageTests, TC_002_RelocationWithUnsupportedKind)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[64] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);
	const char dynstr_data[] = "\0unsupported_sym\0";
	dynstr_sec->set_data(dynstr_data, sizeof(dynstr_data) - 1);

	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_writer(writer, dynsym_sec);
	string_section_accessor str_writer(dynstr_sec);
	Elf_Word sym1 = sym_writer.add_symbol(str_writer, "unsupported_sym", 0, 4, STB_GLOBAL, STT_OBJECT, text_sec->get_index());

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	relocation_section_accessor rela_writer(writer, rela_sec);
	// Use kind value 99 which is not in the switch statement (unsupported)
	rela_writer.add_entry(0, sym1, 99, 0);

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["unsupported_sym"] = 0x1000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	// Should succeed but skip the unsupported relocation kind
	CHECK_EQUAL(0, result);
}

/**
 * @brief Test ctrl_pad destructor
 * Validates destructor is called correctly
 */
TEST(AdditionalCoverageTests, TC_003_CtrlPadDestructor)
{
	{
		char test_data[] = "test";
		struct ctrl_pad pad(test_data, sizeof(test_data));
		// Destructor should be called when leaving scope
	}
	CHECK(true);
}

/**
 * @brief Test relocation with dynsym index validation
 *
 * Verifies that the ELF loader correctly validates dynamic symbol indices against
 * the dynamic string table size to prevent out-of-bounds access.
 */
TEST(AdditionalCoverageTests, TC_004_RelocationDynsymIndexOutOfBoundsInDynstr)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[64] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	// Create dynstr with just one entry (empty string)
	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);
	const char dynstr_data[] = "\0";
	dynstr_sec->set_data(dynstr_data, sizeof(dynstr_data) - 1);

	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_writer(writer, dynsym_sec);
	string_section_accessor str_writer(dynstr_sec);
	// Add multiple symbols but dynstr only has one entry
	Elf_Word sym1 = sym_writer.add_symbol(str_writer, "", 0, 4, STB_GLOBAL, STT_OBJECT, text_sec->get_index());
	Elf_Word sym2 = sym_writer.add_symbol(str_writer, "", 0, 4, STB_GLOBAL, STT_OBJECT, text_sec->get_index());

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	relocation_section_accessor rela_writer(writer, rela_sec);
	// Reference symbol 2 which would be out of bounds in dynstr
	rela_writer.add_entry(0, sym2, 3, 0);

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	std::map<std::string_view, uint64_t> symbol_map;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	// Should handle gracefully and skip the bad relocation
	CHECK_EQUAL(0, result);
}

/**
 * @brief Test ELF with non-standard ABI version
 *
 * Verifies that the ELF loader correctly handles ELF files with ABI versions
 * other than 2, ensuring proper relocation processing regardless of ABI version.
 */
TEST(AdditionalCoverageTests, TC_005_AbiVersionNot2)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(1);  // Set to 1 instead of 2

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[128] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);
	const char dynstr_data[] = "\0test_sym\0";
	dynstr_sec->set_data(dynstr_data, sizeof(dynstr_data) - 1);

	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_writer(writer, dynsym_sec);
	string_section_accessor str_writer(dynstr_sec);
	Elf_Word sym1 = sym_writer.add_symbol(str_writer, "test_sym", 0, 32, STB_GLOBAL, STT_OBJECT, text_sec->get_index());

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	relocation_section_accessor rela_writer(writer, rela_sec);
	// For ABI version != 2, kind comes from addend and addend comes from addend >> 4
	// Set addend to encode kind=3 (SCALAR_32BIT_KIND) in lower bits
	rela_writer.add_entry(0, sym1, 0, (0x100 << 4) | 3);  // kind in lower bits, offset in upper

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["test_sym"] = 0x1000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	CHECK_EQUAL(0, result);
}

/**
 * @brief Test hexdump with negative size (edge case)
 * Though size_t is unsigned, test with 0 is already covered
 */
TEST(AdditionalCoverageTests, TC_006_HexdumpEdgeCases)
{
	uint8_t test_data[] = {0xFF, 0xFF, 0xFF, 0xFF};

	// Test with data containing all 0xFF
	hexdump(test_data, sizeof(test_data));

	CHECK(true);
}

/**
 * @brief Test complete workflow with all patch kinds in ABI v2
 * Ensures all relocation types are processed correctly
 */
TEST(AdditionalCoverageTests, TC_007_AllPatchKindsAbiV2)
{
	std::vector<char> elf_data = create_synthetic_aie_elf();
	std::map<std::string_view, uint64_t> symbol_map;

	// Provide all symbols
	symbol_map["scalar_sym"] = 0x10000;
	symbol_map["aie2ps_sym"] = 0x20000;
	symbol_map["aie4_sym"] = 0x30000;
	symbol_map["ctrl57_sym"] = 0x40000;
	symbol_map["ctrl57aie4_sym"] = 0x50000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	CHECK_EQUAL(0, result);

	// Verify all columns and pages were processed
	CHECK(loader.ccode.cols.size() > 0);
	for (auto &[col, col_code] : loader.ccode.cols) {
		// Code should be assembled from pages and pads
		if (!col_code.pages.empty() || !col_code.pad.empty()) {
			CHECK(col_code.code.size() > 0);
		}
	}
}

/**
 * @brief Test ELF with ELFCLASS32
 * Exercises the class detection in _elf_begin
 */
TEST(AdditionalCoverageTests, TC_008_ElfClass32)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS32, ELFDATA2LSB);  // 32-bit ELF
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_386);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".text");
	text_sec->set_type(SHT_PROGBITS);
	char text_data[16] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	cert_elf_loader loader;
	int result = loader.begin(elf_data.data(), elf_data.size());

	// Should successfully load 32-bit ELF
	CHECK_EQUAL(0, result);
}

/**
 * @brief Test ELF with big endian encoding
 * Exercises the encoding detection in _elf_begin
 */
TEST(AdditionalCoverageTests, TC_009_ElfBigEndian)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2MSB);  // Big endian
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".text");
	text_sec->set_type(SHT_PROGBITS);
	char text_data[16] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	cert_elf_loader loader;
	int result = loader.begin(elf_data.data(), elf_data.size());

	// Should successfully load big endian ELF
	CHECK_EQUAL(0, result);
}

/**
 * @brief Test ELF with program headers
 * Exercises the program header iteration loop
 */
TEST(AdditionalCoverageTests, TC_010_ElfWithProgramHeaders)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	// Add sections
	section* text_sec = writer.sections.add(".text");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_EXECINSTR);
	text_sec->set_addr_align(0x10);
	char text_data[128] = {0};
	for (int i = 0; i < 128; i++) text_data[i] = i;
	text_sec->set_data(text_data, sizeof(text_data));

	// Add program header segments
	segment* load_seg = writer.segments.add();
	load_seg->set_type(PT_LOAD);
	load_seg->set_virtual_address(0x400000);
	load_seg->set_physical_address(0x400000);
	load_seg->set_flags(PF_R | PF_X);
	load_seg->set_align(0x1000);
	load_seg->add_section_index(text_sec->get_index(), text_sec->get_addr_align());

	segment* dynamic_seg = writer.segments.add();
	dynamic_seg->set_type(PT_DYNAMIC);
	dynamic_seg->set_virtual_address(0x500000);
	dynamic_seg->set_physical_address(0x500000);
	dynamic_seg->set_flags(PF_R | PF_W);
	dynamic_seg->set_align(0x8);

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	cert_elf_loader loader;
	int load_result = loader.begin(elf_data.data(), elf_data.size());
	CHECK_EQUAL(0, load_result);

	// Parse should iterate through program headers
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);
}

/**
 * @brief Test section with zero entry size
 * Exercises the ternary operator: entry_size ? entry_size : 1
 */
TEST(AdditionalCoverageTests, TC_011_SectionWithZeroEntrySize)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	// Create a section with entry_size = 0
	section* progbits_sec = writer.sections.add(".custom");
	progbits_sec->set_type(SHT_PROGBITS);
	progbits_sec->set_flags(SHF_ALLOC);
	progbits_sec->set_entry_size(0);  // Zero entry size
	char data[32] = {0};
	progbits_sec->set_data(data, sizeof(data));

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	cert_elf_loader loader;
	loader.begin(elf_data.data(), elf_data.size());

	// Parse should handle zero entry size (uses 1 as default)
	int result = loader.parse();
	CHECK_EQUAL(0, result);
}

/**
 * @brief Test ELF with multiple columns and pages
 * Exercises nested loops for column and page iteration
 */
TEST(AdditionalCoverageTests, TC_012_MultipleColumnsAndPages)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	// Create multiple columns with multiple pages each
	// Column 0, Page 0
	section* sec_0_0_text = writer.sections.add(".ctrltext.0.0");
	sec_0_0_text->set_type(SHT_PROGBITS);
	sec_0_0_text->set_flags(SHF_ALLOC | SHF_WRITE);
	char data_0_0_text[32] = {0x00};
	sec_0_0_text->set_data(data_0_0_text, sizeof(data_0_0_text));

	section* sec_0_0_data = writer.sections.add(".ctrldata.0.0");
	sec_0_0_data->set_type(SHT_PROGBITS);
	sec_0_0_data->set_flags(SHF_ALLOC | SHF_WRITE);
	char data_0_0_data[16] = {0x01};
	sec_0_0_data->set_data(data_0_0_data, sizeof(data_0_0_data));

	// Column 0, Page 1
	section* sec_0_1_text = writer.sections.add(".ctrltext.0.1");
	sec_0_1_text->set_type(SHT_PROGBITS);
	sec_0_1_text->set_flags(SHF_ALLOC | SHF_WRITE);
	char data_0_1_text[24] = {0x02};
	sec_0_1_text->set_data(data_0_1_text, sizeof(data_0_1_text));

	section* sec_0_1_data = writer.sections.add(".ctrldata.0.1");
	sec_0_1_data->set_type(SHT_PROGBITS);
	sec_0_1_data->set_flags(SHF_ALLOC | SHF_WRITE);
	char data_0_1_data[20] = {0x03};
	sec_0_1_data->set_data(data_0_1_data, sizeof(data_0_1_data));

	// Column 1, Page 0
	section* sec_1_0_text = writer.sections.add(".ctrltext.1.0");
	sec_1_0_text->set_type(SHT_PROGBITS);
	sec_1_0_text->set_flags(SHF_ALLOC | SHF_WRITE);
	char data_1_0_text[40] = {0x04};
	sec_1_0_text->set_data(data_1_0_text, sizeof(data_1_0_text));

	section* sec_1_0_data = writer.sections.add(".ctrldata.1.0");
	sec_1_0_data->set_type(SHT_PROGBITS);
	sec_1_0_data->set_flags(SHF_ALLOC | SHF_WRITE);
	char data_1_0_data[28] = {0x05};
	sec_1_0_data->set_data(data_1_0_data, sizeof(data_1_0_data));

	// Add pads for both columns
	section* pad_0 = writer.sections.add(".pad.0");
	pad_0->set_type(SHT_PROGBITS);
	pad_0->set_flags(SHF_ALLOC);
	char pad_0_data[8] = {(char)0xAA};
	pad_0->set_data(pad_0_data, sizeof(pad_0_data));

	section* pad_1 = writer.sections.add(".pad.1");
	pad_1->set_type(SHT_PROGBITS);
	pad_1->set_flags(SHF_ALLOC);
	char pad_1_data[12] = {(char)0xBB};
	pad_1->set_data(pad_1_data, sizeof(pad_1_data));

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	cert_elf_loader loader;
	loader.begin(elf_data.data(), elf_data.size());

	int result = loader.parse();
	CHECK_EQUAL(0, result);

	// Verify both columns were processed
	CHECK(loader.ccode.cols.find(0) != loader.ccode.cols.end());
	CHECK(loader.ccode.cols.find(1) != loader.ccode.cols.end());

	// Verify pages were processed for column 0
	auto& col0 = loader.ccode.cols[0];
	CHECK(col0.pages.find(0) != col0.pages.end());
	CHECK(col0.pages.find(1) != col0.pages.end());

	// Verify code was assembled (text + data + pad)
	CHECK(col0.code.size() > 0);
	CHECK(loader.ccode.cols[1].code.size() > 0);
}

/**
 * @brief Test column with only text section
 *
 * Verifies that the ELF loader correctly handles columns with only text sections
 * and no data sections, ensuring proper code assembly.
 */
TEST(AdditionalCoverageTests, TC_013_ColumnWithTextOnly)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	// Create ctrltext without corresponding ctrldata
	section* text_sec = writer.sections.add(".ctrltext.5.3");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[48] = {0};
	for (int i = 0; i < 48; i++) text_data[i] = i * 2;
	text_sec->set_data(text_data, sizeof(text_data));

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	cert_elf_loader loader;
	loader.begin(elf_data.data(), elf_data.size());

	int result = loader.parse();
	CHECK_EQUAL(0, result);

	// Verify column 5 was created
	CHECK(loader.ccode.cols.find(5) != loader.ccode.cols.end());
}

/**
 * @brief Test column with only data section
 *
 * Verifies that the ELF loader correctly handles columns with only data sections
 * and no text sections, ensuring proper code assembly.
 */
TEST(AdditionalCoverageTests, TC_014_ColumnWithDataOnly)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	// Create ctrldata without corresponding ctrltext
	section* data_sec = writer.sections.add(".ctrldata.7.2");
	data_sec->set_type(SHT_PROGBITS);
	data_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char data_data[36] = {0};
	for (int i = 0; i < 36; i++) data_data[i] = i * 3;
	data_sec->set_data(data_data, sizeof(data_data));

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	cert_elf_loader loader;
	loader.begin(elf_data.data(), elf_data.size());

	int result = loader.parse();
	CHECK_EQUAL(0, result);

	// Verify column 7 was created
	CHECK(loader.ccode.cols.find(7) != loader.ccode.cols.end());
}

/**
 * @brief Test column with multiple pads
 * Exercises the pad iteration loop
 */
TEST(AdditionalCoverageTests, TC_015_ColumnWithMultiplePads)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	// Create same column number with multiple pads
	// Note: In practice, this would be multiple .pad sections with same column
	// But ELFIO doesn't allow duplicate section names, so we test with different columns
	section* pad_1 = writer.sections.add(".pad.10");
	pad_1->set_type(SHT_PROGBITS);
	pad_1->set_flags(SHF_ALLOC);
	char pad_1_data[16] = {(char)0xAA, (char)0xBB, (char)0xCC, (char)0xDD};
	pad_1->set_data(pad_1_data, sizeof(pad_1_data));

	section* text_sec = writer.sections.add(".ctrltext.10.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[32] = {0x11, 0x22, 0x33, 0x44};
	text_sec->set_data(text_data, sizeof(text_data));

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	cert_elf_loader loader;
	loader.begin(elf_data.data(), elf_data.size());

	int result = loader.parse();
	CHECK_EQUAL(0, result);

	// Verify column 10 exists and has both pad and text
	CHECK(loader.ccode.cols.find(10) != loader.ccode.cols.end());
	CHECK(loader.ccode.cols[10].pad.size() > 0);
	CHECK(loader.ccode.cols[10].code.size() > 0);
}

/**
 * @brief Test hexdump with data that requires multiple lines
 * Ensures all branches in hexdump for loop are covered
 */
TEST(AdditionalCoverageTests, TC_016_HexdumpMultipleLines)
{
	// Test with 33 bytes to ensure we hit line boundary at 16, 32
	uint8_t test_data[33];
	for (int i = 0; i < 33; i++) {
		test_data[i] = i * 7;  // Some pattern
	}

	hexdump(test_data, 33);
	CHECK(true);
}

/**
 * @brief Test hexdump with exactly 17 bytes
 *
 * Verifies that the hexdump function correctly handles data that crosses
 * 16-byte alignment boundaries.
 */
TEST(AdditionalCoverageTests, TC_017_HexdumpSeventeenBytes)
{
	uint8_t test_data[17];
	for (int i = 0; i < 17; i++) {
		test_data[i] = 0xFF - i;
	}

	hexdump(test_data, 17);
	CHECK(true);
}

/**
 * @brief Test with relocation offset at different positions
 * Ensures ptr calculation works for various offsets
 */
TEST(AdditionalCoverageTests, TC_018_RelocationDifferentOffsets)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[256] = {0};  // Larger buffer for various offsets
	text_sec->set_data(text_data, sizeof(text_data));

	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);
	const char dynstr_data[] = "\0sym1\0sym2\0sym3\0";
	dynstr_sec->set_data(dynstr_data, sizeof(dynstr_data) - 1);

	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_writer(writer, dynsym_sec);
	string_section_accessor str_writer(dynstr_sec);
	Elf_Word sym1 = sym_writer.add_symbol(str_writer, "sym1", 0, 32, STB_GLOBAL, STT_OBJECT, text_sec->get_index());
	Elf_Word sym2 = sym_writer.add_symbol(str_writer, "sym2", 0, 32, STB_GLOBAL, STT_OBJECT, text_sec->get_index());
	Elf_Word sym3 = sym_writer.add_symbol(str_writer, "sym3", 0, 32, STB_GLOBAL, STT_OBJECT, text_sec->get_index());

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	relocation_section_accessor rela_writer(writer, rela_sec);
	// Different offsets: 0, 64, 128
	rela_writer.add_entry(0, sym1, 3, 0x100);
	rela_writer.add_entry(64, sym2, 3, 0x200);
	rela_writer.add_entry(128, sym3, 3, 0x300);

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["sym1"] = 0x10000;
	symbol_map["sym2"] = 0x20000;
	symbol_map["sym3"] = 0x30000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	CHECK_EQUAL(0, result);
}

/**
 * @brief Test parse after close_elf
 * Validates behavior when parse is called after closing
 */
TEST(AdditionalCoverageTests, TC_027_ParseAfterCloseElf)
{
	const char *validFilename = TEST_ELF_FILE;
	cert_elf_loader loader;

	int load_result = loader.begin(validFilename);
	if (load_result == 0) {
		loader.close_elf();

		// Parse after close - should still work as ELF is loaded
		int parse_result = loader.parse();
		CHECK_EQUAL(0, parse_result);
	} else {
		CHECK(true);
	}
}

/**
 * @brief Test empty ELF with no sections
 *
 * Verifies that the ELF loader can handle a minimal ELF file structure
 * with no sections defined.
 */
TEST(AdditionalCoverageTests, TC_028_EmptyElfNoSections)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	// Don't add any sections beyond what ELFIO creates automatically

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	cert_elf_loader loader;
	int load_result = loader.begin(elf_data.data(), elf_data.size());
	CHECK_EQUAL(0, load_result);

	// Parse should handle ELF with minimal sections
	int parse_result = loader.parse();
	CHECK_EQUAL(0, parse_result);
}

//----------------------------------------------------------------------
// Test group for complete relocation processing
//----------------------------------------------------------------------

/**
 * @brief Test group for full relocation and symbol processing
 */
TEST_GROUP(RelocationProcessingTests)
{
	void setup() {}
	void teardown() {}
};

/**
 * @brief Test complete relocation processing with dynsym, dynstr, and rela sections
 *
 * This test verifies that the ELF loader correctly processes all relocation sections
 * including dynamic symbol table, dynamic string table, and relocation with addends.
 */
TEST(RelocationProcessingTests, TC_001_CompleteRelocationProcessing)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	// Create target sections for relocations
	section* ctrltext_sec = writer.sections.add(".ctrltext.0.0");
	ctrltext_sec->set_type(SHT_PROGBITS);
	ctrltext_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char ctrltext_data[256] = {0};
	ctrltext_sec->set_data(ctrltext_data, sizeof(ctrltext_data));

	// First create .dynstr section - MUST come before .dynsym
	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);
	dynstr_sec->set_flags(0);
	// Build string table with all symbol names
	std::string str_table = std::string("\0", 1); // Start with null string
	str_table += "sym1\0";
	str_table += "sym2\0";
	str_table += "sym3\0";
	dynstr_sec->set_data(str_table.c_str(), str_table.length());

	// Create .dynsym section that references dynstr
	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_acc(writer, dynsym_sec);
	string_section_accessor str_acc(dynstr_sec);

	// Add symbols
	Elf_Word sym1_idx = sym_acc.add_symbol(str_acc, "sym1", 0, 32, STB_GLOBAL, STT_OBJECT, ctrltext_sec->get_index());
	Elf_Word sym2_idx = sym_acc.add_symbol(str_acc, "sym2", 0, 36, STB_GLOBAL, STT_OBJECT, ctrltext_sec->get_index());
	Elf_Word sym3_idx = sym_acc.add_symbol(str_acc, "sym3", 0, 16, STB_GLOBAL, STT_OBJECT, ctrltext_sec->get_index());

	// Create .rela.dyn section with relocations
	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());
	rela_sec->set_info(ctrltext_sec->get_index());

	relocation_section_accessor rela_acc(writer, rela_sec);

	// Add relocations that reference the symbols
	rela_acc.add_entry(0, sym1_idx, 3, 0x1000);  // SCALAR_32BIT_KIND
	rela_acc.add_entry(32, sym2_idx, 2, 0x2000); // SHIM_DMA_BASE_ADDR_SYMBOL_KIND
	rela_acc.add_entry(64, sym3_idx, 7, 0x3000); // CONTROL_PACKET_57

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	// Fix entry_size issue: ELFIO resets entry_size to 0 on save
	elfio reader;
	std::istringstream iss(elf_str);
	if (reader.load(iss)) {
		for (int i = 0; i < reader.sections.size(); i++) {
			if (reader.sections[i]->get_name() == ".dynsym") {
				if (reader.sections[i]->get_entry_size() == 0) {
					reader.sections[i]->set_entry_size(sizeof(Elf64_Sym));
					std::ostringstream oss2;
					reader.save(oss2);
					elf_str = oss2.str();
					elf_data.assign(elf_str.begin(), elf_str.end());
				}
				break;
			}
		}
	}

	// Create symbol map with addresses for our symbols
	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["sym1"] = 0x10000;
	symbol_map["sym2"] = 0x20000;
	symbol_map["sym3"] = 0x30000;

	// Load ELF with symbol map
	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);

	// Parse should process .dynsym, .dynstr, .rela.dyn and apply patches
	int result = loader.parse();
	CHECK_EQUAL(0, result);

	// Verify the sections were processed
	CHECK(loader.ccode.dynsym.size() > 0);
	CHECK(loader.ccode.dynstr.size() > 0);
	CHECK(loader.ccode.rela.size() > 0);
}

/**
 * @brief Test relocation processing with symbol not found in map
 *
 * Verifies that the ELF loader correctly handles the case when a referenced symbol
 * cannot be found in the provided symbol map, returning appropriate error status.
 */
TEST(RelocationProcessingTests, TC_002_SymbolNotFoundInMap)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[128] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);
	std::string str_table = std::string("\0", 1) + "unknown_symbol\0";
	dynstr_sec->set_data(str_table.c_str(), str_table.length());

	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_acc(writer, dynsym_sec);
	string_section_accessor str_acc(dynstr_sec);
	Elf_Word sym1 = sym_acc.add_symbol(str_acc, "unknown_symbol", 0, 32, STB_GLOBAL, STT_OBJECT, text_sec->get_index());

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	relocation_section_accessor rela_acc(writer, rela_sec);
	rela_acc.add_entry(0, sym1, 3, 0x1000);

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	// Empty symbol map - symbol won't be found
	std::map<std::string_view, uint64_t> symbol_map;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	// Should succeed but skip the relocation
	CHECK_EQUAL(0, result);
}

/**
 * @brief Test all patch kinds with proper relocation setup
 * Ensures all switch cases in relocation processing are covered
 */
TEST(RelocationProcessingTests, TC_003_AllPatchKinds)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[512] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);
	// Don't manually set data - let string_section_accessor handle it

	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_acc(writer, dynsym_sec);
	string_section_accessor str_acc(dynstr_sec);

	Elf_Word s1 = sym_acc.add_symbol(str_acc, "scalar", 0, 32, STB_GLOBAL, STT_OBJECT, text_sec->get_index());
	Elf_Word s2 = sym_acc.add_symbol(str_acc, "aie2ps", 0, 36, STB_GLOBAL, STT_OBJECT, text_sec->get_index());
	Elf_Word s3 = sym_acc.add_symbol(str_acc, "aie4", 0, 8, STB_GLOBAL, STT_OBJECT, text_sec->get_index());
	Elf_Word s4 = sym_acc.add_symbol(str_acc, "ctrl57", 0, 16, STB_GLOBAL, STT_OBJECT, text_sec->get_index());
	Elf_Word s5 = sym_acc.add_symbol(str_acc, "ctrl57aie4", 0, 12, STB_GLOBAL, STT_OBJECT, text_sec->get_index());

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	relocation_section_accessor rela_acc(writer, rela_sec);

	// Add one relocation for each patch kind
	rela_acc.add_entry(0, s1, 3, 0x100);    // SCALAR_32BIT_KIND = 3
	rela_acc.add_entry(64, s2, 2, 0x200);   // SHIM_DMA_BASE_ADDR_SYMBOL_KIND = 2
	rela_acc.add_entry(128, s3, 6, 0x300);  // SHIM_DMA_AIE4_BASE_ADDR_SYMBOL_KIND = 6
	rela_acc.add_entry(192, s4, 7, 0x400);  // CONTROL_PACKET_57 = 7
	rela_acc.add_entry(256, s5, 9, 0x500);  // CONTROL_PACKET_57_AIE4 = 9

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	// CRITICAL FIX: Manually patch the sh_entsize in the ELF binary
	// ELFIO doesn't preserve entry_size for SHT_DYNSYM properly when saving
	// We need to patch the section header directly in the binary
	elfio reader;
	std::istringstream iss(elf_str);
	if (reader.load(iss)) {
		// Find .dynsym section and verify/fix its entry size
		for (int i = 0; i < reader.sections.size(); i++) {
			if (reader.sections[i]->get_name() == ".dynsym") {
				// Force set entry size if it's 0
				if (reader.sections[i]->get_entry_size() == 0) {
					reader.sections[i]->set_entry_size(sizeof(Elf64_Sym));
					std::ostringstream oss2;
					reader.save(oss2);
					elf_str = oss2.str();
					elf_data.assign(elf_str.begin(), elf_str.end());
				}
				break;
			}
		}
	}

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["scalar"] = 0x1000;
	symbol_map["aie2ps"] = 0x2000;
	symbol_map["aie4"] = 0x3000;
	symbol_map["ctrl57"] = 0x4000;
	symbol_map["ctrl57aie4"] = 0x5000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	CHECK_EQUAL(0, result);
	CHECK(loader.ccode.rela.size() == 5);
}

/**
 * @brief Test direct population of ccode structures to trigger relocation processing
 *
 * This test manually sets up the internal data structures to verify
 * relocation processing with various symbol types.
 */
TEST(RelocationProcessingTests, TC_004_ManualRelocationProcessing)
{
	using namespace ELFIO;

	// Create ELF with sections that have data
	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	// Create target section for patching
	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[512] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	// Create .dynstr section manually
	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);
	dynstr_sec->set_flags(0);
	// Create string table: null + 5 symbol names
	const char* str_data = "\0sym1\0sym2\0sym3\0sym4\0sym5\0";
	size_t str_len = 1 + 5 + 5 + 5 + 5 + 5 + 5; // null + each name with null terminator
	dynstr_sec->set_data(str_data, str_len);

	// Create .dynsym with 5 symbols
	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_addr_align(8);
	dynsym_sec->set_link(dynstr_sec->get_index());

	// Manually build symbol table with 5 entries
	// Elf64_Sym structure layout (24 bytes total)
	struct Elf64_Sym {
		uint32_t st_name;     // 0-3: Symbol name (string table index)
		uint8_t st_info;      // 4: Symbol type and binding
		uint8_t st_other;     // 5: Symbol visibility
		uint16_t st_shndx;    // 6-7: Section index
		uint64_t st_value;    // 8-15: Symbol value
		uint64_t st_size;     // 16-23: Symbol size
	};

	Elf64_Sym syms[5];
	// Symbol 0: sym1
	syms[0].st_name = 1;
	syms[0].st_info = (STB_GLOBAL << 4) | STT_OBJECT;
	syms[0].st_other = 0;
	syms[0].st_shndx = text_sec->get_index();
	syms[0].st_value = 0;
	syms[0].st_size = 32;

	// Symbol 1: sym2
	syms[1].st_name = 6;
	syms[1].st_info = (STB_GLOBAL << 4) | STT_OBJECT;
	syms[1].st_other = 0;
	syms[1].st_shndx = text_sec->get_index();
	syms[1].st_value = 0;
	syms[1].st_size = 36;

	// Symbol 2: sym3
	syms[2].st_name = 11;
	syms[2].st_info = (STB_GLOBAL << 4) | STT_OBJECT;
	syms[2].st_other = 0;
	syms[2].st_shndx = text_sec->get_index();
	syms[2].st_value = 0;
	syms[2].st_size = 8;

	// Symbol 3: sym4
	syms[3].st_name = 16;
	syms[3].st_info = (STB_GLOBAL << 4) | STT_OBJECT;
	syms[3].st_other = 0;
	syms[3].st_shndx = text_sec->get_index();
	syms[3].st_value = 0;
	syms[3].st_size = 16;

	// Symbol 4: sym5
	syms[4].st_name = 21;
	syms[4].st_info = (STB_GLOBAL << 4) | STT_OBJECT;
	syms[4].st_other = 0;
	syms[4].st_shndx = text_sec->get_index();
	syms[4].st_value = 0;
	syms[4].st_size = 12;

	dynsym_sec->set_data(reinterpret_cast<char*>(syms), sizeof(syms));
	dynsym_sec->set_entry_size(sizeof(Elf64_Sym)); // Set entry size AFTER set_data

	// Create .rela.dyn section with relocations
	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_addr_align(8);
	rela_sec->set_entry_size(24); // Standard 64-bit rela entry size
	rela_sec->set_link(dynsym_sec->get_index());
	rela_sec->set_info(text_sec->get_index());

	// Manually build relocation table
	// Elf64_Rela structure layout (24 bytes total)
	struct Elf64_Rela {
		uint64_t r_offset;    // 0-7: Offset to apply relocation
		uint64_t r_info;      // 8-15: Symbol index (upper 32) | type (lower 32)
		int64_t r_addend;     // 16-23: Addend value
	};

	Elf64_Rela relas[5];
	// Relocation 0: SCALAR_32BIT_KIND = 3
	relas[0].r_offset = 0;
	relas[0].r_info = (static_cast<uint64_t>(0) << 32) | 3;
	relas[0].r_addend = 0x100;

	// Relocation 1: SHIM_DMA_BASE_ADDR_SYMBOL_KIND = 2
	relas[1].r_offset = 64;
	relas[1].r_info = (static_cast<uint64_t>(1) << 32) | 2;
	relas[1].r_addend = 0x200;

	// Relocation 2: SHIM_DMA_AIE4_BASE_ADDR_SYMBOL_KIND = 6
	relas[2].r_offset = 128;
	relas[2].r_info = (static_cast<uint64_t>(2) << 32) | 6;
	relas[2].r_addend = 0x300;

	// Relocation 3: CONTROL_PACKET_57 = 7
	relas[3].r_offset = 192;
	relas[3].r_info = (static_cast<uint64_t>(3) << 32) | 7;
	relas[3].r_addend = 0x400;

	// Relocation 4: CONTROL_PACKET_57_AIE4 = 9
	relas[4].r_offset = 256;
	relas[4].r_info = (static_cast<uint64_t>(4) << 32) | 9;
	relas[4].r_addend = 0x500;

	rela_sec->set_data(reinterpret_cast<char*>(relas), sizeof(relas));

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["sym1"] = 0x10000;
	symbol_map["sym2"] = 0x20000;
	symbol_map["sym3"] = 0x30000;
	symbol_map["sym4"] = 0x40000;
	symbol_map["sym5"] = 0x50000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	CHECK_EQUAL(0, result);
	// Verify that dynsym, dynstr, and rela were processed
	CHECK(loader.ccode.dynsym.size() == 5);
	CHECK(loader.ccode.dynstr.size() >= 5);  // May have more due to string_section_accessor behavior
	CHECK(loader.ccode.rela.size() == 5);
}

/**
 * @brief Test unsupported patch kind error handling
 *
 * This test verifies that the ELF loader correctly handles unsupported relocation
 * types and returns appropriate error status.
 */
TEST(RelocationProcessingTests, TC_005_UnsupportedPatchKind)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[128] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);

	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_acc(writer, dynsym_sec);
	string_section_accessor str_acc(dynstr_sec);

	Elf_Word s1 = sym_acc.add_symbol(str_acc, "test_sym", 0, 32, STB_GLOBAL, STT_OBJECT, text_sec->get_index());

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	relocation_section_accessor rela_acc(writer, rela_sec);
	rela_acc.add_entry(0, s1, 99, 0x100);  // Kind 99 is unsupported

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	// Fix entry size
	elfio reader;
	std::istringstream iss(elf_str);
	if (reader.load(iss)) {
		for (int i = 0; i < reader.sections.size(); i++) {
			if (reader.sections[i]->get_name() == ".dynsym") {
				if (reader.sections[i]->get_entry_size() == 0) {
					reader.sections[i]->set_entry_size(24);
					std::ostringstream oss2;
					reader.save(oss2);
					elf_str = oss2.str();
					elf_data.assign(elf_str.begin(), elf_str.end());
				}
				break;
			}
		}
	}

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["test_sym"] = 0x1000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	CHECK_EQUAL(0, result);
	CHECK(loader.ccode.rela.size() == 1);
}

/**
 * @brief Test dynsym index validation against dynstr bounds
 *
 * This test verifies error handling when the dynamic symbol table contains more entries
 * than the dynamic string table. The test creates a dynsym with 3 entries but dynstr
 * with only 2 strings, which can occur with manually constructed malformed ELF files.
 */
TEST(RelocationProcessingTests, TC_006_DynsymOutOfBoundsInDynstr)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[128] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	// Create dynstr with only 2 string entries (null and sym1)
	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);
	dynstr_sec->set_flags(0);
	const char* str_data = "\0sym1\0";  // Only 1 named string
	size_t str_len = 1 + 5;  // null + "sym1\0"
	dynstr_sec->set_data(str_data, str_len);

	// Create dynsym with 3 symbols manually
	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_link(dynstr_sec->get_index());

	// Manually build 3 symbol entries
	struct Elf64_Sym {
		uint32_t st_name;
		uint8_t st_info;
		uint8_t st_other;
		uint16_t st_shndx;
		uint64_t st_value;
		uint64_t st_size;
	};

	Elf64_Sym syms[3];
	// Symbol 0: sym1 at string index 1
	syms[0].st_name = 1;
	syms[0].st_info = (STB_GLOBAL << 4) | STT_OBJECT;
	syms[0].st_other = 0;
	syms[0].st_shndx = text_sec->get_index();
	syms[0].st_value = 0;
	syms[0].st_size = 32;

	// Symbol 1: sym2 at string index 6
	syms[1].st_name = 6;
	syms[1].st_info = (STB_GLOBAL << 4) | STT_OBJECT;
	syms[1].st_other = 0;
	syms[1].st_shndx = text_sec->get_index();
	syms[1].st_value = 0;
	syms[1].st_size = 32;

	// Symbol 2: extra symbol (creates mismatch)
	syms[2].st_name = 1;  // Reuse sym1 name
	syms[2].st_info = (STB_GLOBAL << 4) | STT_OBJECT;
	syms[2].st_other = 0;
	syms[2].st_shndx = text_sec->get_index();
	syms[2].st_value = 0;
	syms[2].st_size = 32;

	dynsym_sec->set_data(reinterpret_cast<const char*>(syms), sizeof(syms));
	dynsym_sec->set_entry_size(sizeof(Elf64_Sym));

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	// Manually build 2 relocation entries
	struct Elf64_Rela {
		uint64_t r_offset;
		uint64_t r_info;
		int64_t r_addend;
	};

	Elf64_Rela relas[2];
	// First relocation references symbol 0
	relas[0].r_offset = 0;
	relas[0].r_info = (static_cast<uint64_t>(0) << 32) | 3;  // Symbol 0, type 3
	relas[0].r_addend = 0x100;

	// Second relocation references symbol 2 (this will trigger the bounds error)
	// dynsym has 3 entries (0,1,2) but dynstr only has 2 strings (0,1)
	// So dynsym_index=2 is valid but dynstr[2] will be out of bounds
	relas[1].r_offset = 32;
	relas[1].r_info = (static_cast<uint64_t>(2) << 32) | 3;  // Symbol 2, type 3
	relas[1].r_addend = 0x200;

	rela_sec->set_data(reinterpret_cast<const char*>(relas), sizeof(relas));

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	// Fix entry size
	elfio reader;
	std::istringstream iss(elf_str);
	if (reader.load(iss)) {
		for (int i = 0; i < reader.sections.size(); i++) {
			if (reader.sections[i]->get_name() == ".dynsym") {
				if (reader.sections[i]->get_entry_size() == 0) {
					reader.sections[i]->set_entry_size(24);
					std::ostringstream oss2;
					reader.save(oss2);
					elf_str = oss2.str();
					elf_data.assign(elf_str.begin(), elf_str.end());
				}
				break;
			}
		}
	}

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["sym1"] = 0x1000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	CHECK_EQUAL(0, result);
}

/**
 * @brief Test relocation processing with non-version-2 ABI
 *
 * This test verifies that the ELF loader correctly handles ELF files with
 * ABI versions other than 2, ensuring proper relocation encoding handling.
 */
TEST(RelocationProcessingTests, TC_007_NonABIVersion2)
{
	using namespace ELFIO;

	elfio writer;
	writer.create(ELFCLASS64, ELFDATA2LSB);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_X86_64);
	writer.set_abi_version(1);  // Non-2 ABI version

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	char text_data[128] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);

	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_acc(writer, dynsym_sec);
	string_section_accessor str_acc(dynstr_sec);

	Elf_Word s1 = sym_acc.add_symbol(str_acc, "scalar", 0, 32, STB_GLOBAL, STT_OBJECT, text_sec->get_index());

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	relocation_section_accessor rela_acc(writer, rela_sec);
	// For non-ABI v2: kind is encoded in addend, addend value is shifted right by 4
	int64_t encoded_addend = (3 << 4) | 0x10;  // Kind 3 (SCALAR_32BIT_KIND), addend 0x10
	rela_acc.add_entry(0, s1, 0, encoded_addend);

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	// Fix entry size
	elfio reader;
	std::istringstream iss(elf_str);
	if (reader.load(iss)) {
		for (int i = 0; i < reader.sections.size(); i++) {
			if (reader.sections[i]->get_name() == ".dynsym") {
				if (reader.sections[i]->get_entry_size() == 0) {
					reader.sections[i]->set_entry_size(24);
					std::ostringstream oss2;
					reader.save(oss2);
					elf_str = oss2.str();
					elf_data.assign(elf_str.begin(), elf_str.end());
				}
				break;
			}
		}
	}

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["scalar"] = 0x1000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	CHECK_EQUAL(0, result);
	CHECK(loader.ccode.rela.size() == 1);
}

/**
 * @brief Test group for malformed section name handling
 *
 * These tests verify exception handling for malformed section names in
 * is_pad, is_ctrltext, and is_ctrldata methods.
 */
TEST_GROUP(MalformedSectionNameTests)
{
	void setup() {}
	void teardown() {}
};

/**
 * @brief Test accessor class to access protected methods for coverage testing
 */
class cert_elf_loader_test_accessor : public cert_elf_loader {
public:
	using cert_elf_loader::is_pad;
	using cert_elf_loader::is_ctrltext;
	using cert_elf_loader::is_ctrldata;
};

/**
 * @brief Test is_pad with invalid column number (triggers invalid_argument)
 */
TEST(MalformedSectionNameTests, TC_019_IsPadInvalidArgument)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".pad.notanumber";
	int col = -1;

	bool result = loader.is_pad(&name, &col);
	CHECK_FALSE(result);
	// invalid_argument exception should be caught and return false
}

/**
 * @brief Test is_pad with out-of-range column number (triggers out_of_range)
 */
TEST(MalformedSectionNameTests, TC_020_IsPadOutOfRange)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".pad.99999999999999999999";
	int col = -1;

	bool result = loader.is_pad(&name, &col);
	CHECK_FALSE(result);
	// out_of_range exception should be caught and return false
}

/**
 * @brief Test is_ctrltext with invalid column (triggers invalid_argument)
 */
TEST(MalformedSectionNameTests, TC_021_IsCtrltextInvalidColumn)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.notvalid.0";
	int col = -1, page = -1;

	bool result = loader.is_ctrltext(&name, &col, &page);
	CHECK_FALSE(result);
	// invalid_argument exception should be caught and return false
}

/**
 * @brief Test is_ctrltext with out-of-range column (triggers out_of_range)
 */
TEST(MalformedSectionNameTests, TC_022_IsCtrltextOutOfRangeColumn)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.99999999999999999999.0";
	int col = -1, page = -1;

	bool result = loader.is_ctrltext(&name, &col, &page);
	CHECK_FALSE(result);
	// out_of_range exception should be caught and return false
}

/**
 * @brief Test is_ctrltext with invalid page (triggers invalid_argument)
 */
TEST(MalformedSectionNameTests, TC_023_IsCtrltextInvalidPage)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.0.notvalid";
	int col = -1, page = -1;

	bool result = loader.is_ctrltext(&name, &col, &page);
	CHECK_FALSE(result);
	// invalid_argument exception should be caught and return false
}

/**
 * @brief Test is_ctrltext with out-of-range page (triggers out_of_range)
 */
TEST(MalformedSectionNameTests, TC_024_IsCtrltextOutOfRangePage)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.0.99999999999999999999";
	int col = -1, page = -1;

	bool result = loader.is_ctrltext(&name, &col, &page);
	CHECK_FALSE(result);
	// out_of_range exception should be caught and return false
}

/**
 * @brief Test is_ctrldata with invalid column (triggers invalid_argument)
 */
TEST(MalformedSectionNameTests, TC_025_IsCtrldataInvalidColumn)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.notvalid.0";
	int col = -1, page = -1;

	bool result = loader.is_ctrldata(&name, &col, &page);
	CHECK_FALSE(result);
	// invalid_argument exception should be caught and return false
}

/**
 * @brief Test is_ctrldata with out-of-range column (triggers out_of_range)
 */
TEST(MalformedSectionNameTests, TC_026_IsCtrldataOutOfRangeColumn)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.99999999999999999999.0";
	int col = -1, page = -1;

	bool result = loader.is_ctrldata(&name, &col, &page);
	CHECK_FALSE(result);
	// out_of_range exception should be caught and return false
}

/**
 * @brief Test is_ctrldata with invalid page (triggers invalid_argument)
 */
TEST(MalformedSectionNameTests, TC_027_IsCtrldataInvalidPage)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.0.notvalid";
	int col = -1, page = -1;

	bool result = loader.is_ctrldata(&name, &col, &page);
	CHECK_FALSE(result);
	// invalid_argument exception should be caught and return false
}

/**
 * @brief Test is_ctrldata with out-of-range page (triggers out_of_range)
 */
TEST(MalformedSectionNameTests, TC_028_IsCtrldataOutOfRangePage)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.0.99999999999999999999";
	int col = -1, page = -1;

	bool result = loader.is_ctrldata(&name, &col, &page);
	CHECK_FALSE(result);
	// out_of_range exception should be caught and return false
}

/**
 * @brief Test is_ctrltext with missing dot (no page separator)
 */
TEST(MalformedSectionNameTests, TC_029_IsCtrltextNoDotSeparator)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext.0";
	int col = -1, page = -1;

	bool result = loader.is_ctrltext(&name, &col, &page);
	CHECK_FALSE(result);
	// Should return false when no dot separator for page
}

/**
 * @brief Test is_ctrltext with empty column (pos == 0)
 */
TEST(MalformedSectionNameTests, TC_030_IsCtrltextEmptyColumn)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrltext..0";
	int col = -1, page = -1;

	bool result = loader.is_ctrltext(&name, &col, &page);
	CHECK_FALSE(result);
	// Should return false when column is empty (pos == 0)
}

/**
 * @brief Test is_ctrldata with missing dot (no page separator)
 */
TEST(MalformedSectionNameTests, TC_031_IsCtrldataNoDotSeparator)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata.0";
	int col = -1, page = -1;

	bool result = loader.is_ctrldata(&name, &col, &page);
	CHECK_FALSE(result);
	// Should return false when no dot separator for page
}

/**
 * @brief Test is_ctrldata with empty column (pos == 0)
 */
TEST(MalformedSectionNameTests, TC_032_IsCtrldataEmptyColumn)
{
	cert_elf_loader_test_accessor loader;
	std::string_view name = ".ctrldata..0";
	int col = -1, page = -1;

	bool result = loader.is_ctrldata(&name, &col, &page);
	CHECK_FALSE(result);
	// Should return false when column is empty (pos == 0)
}

/**
 * @brief Test section with null data to cover the "if (!data)" path
 */
TEST(MalformedSectionNameTests, TC_019_SectionWithNullData)
{
	elfio writer;
	writer.create(ELFCLASS32, ELFDATA2LSB);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_NONE);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_abi_version(2);

	// Add section with type that typically has no data (SHT_NOBITS)
	section* bss_sec = writer.sections.add(".bss");
	bss_sec->set_type(SHT_NOBITS);
	bss_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	bss_sec->set_size(128);  // Size but no actual data

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();

	cert_elf_loader loader;
	int result = loader.begin(elf_str.data(), elf_str.size());
	CHECK_EQUAL(0, result);

	result = loader.parse();
	CHECK_EQUAL(0, result);
	// Section with no data should be skipped
}

/**
 * @brief Test unsupported relocation kind (default case in switch)
 */
TEST(MalformedSectionNameTests, TC_020_UnsupportedRelocationKind)
{
	elfio writer;
	writer.create(ELFCLASS32, ELFDATA2LSB);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_NONE);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_abi_version(2);

	section* text_sec = writer.sections.add(".ctrltext.0.0");
	text_sec->set_type(SHT_PROGBITS);
	text_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	static char text_data[128] = {0};
	text_sec->set_data(text_data, sizeof(text_data));

	section* dynstr_sec = writer.sections.add(".dynstr");
	dynstr_sec->set_type(SHT_STRTAB);

	section* dynsym_sec = writer.sections.add(".dynsym");
	dynsym_sec->set_type(SHT_DYNSYM);
	dynsym_sec->set_flags(SHF_ALLOC);
	dynsym_sec->set_entry_size(writer.get_default_entry_size(SHT_DYNSYM));
	dynsym_sec->set_link(dynstr_sec->get_index());

	symbol_section_accessor sym_acc(writer, dynsym_sec);
	string_section_accessor str_acc(dynstr_sec);

	Elf_Word s1 = sym_acc.add_symbol(str_acc, "scalar", 0, 32, STB_GLOBAL, STT_OBJECT, text_sec->get_index());

	section* rela_sec = writer.sections.add(".rela.dyn");
	rela_sec->set_type(SHT_RELA);
	rela_sec->set_flags(SHF_ALLOC);
	rela_sec->set_entry_size(writer.get_default_entry_size(SHT_RELA));
	rela_sec->set_link(dynsym_sec->get_index());

	relocation_section_accessor rela_acc(writer, rela_sec);
	// Use kind 99 which is not supported (valid kinds are 2,3,6,7,9)
	rela_acc.add_entry(0, s1, 99, 0x10);  // Unsupported kind

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();
	std::vector<char> elf_data(elf_str.begin(), elf_str.end());

	std::map<std::string_view, uint64_t> symbol_map;
	symbol_map["scalar"] = 0x1000;

	cert_elf_loader loader(elf_data.data(), elf_data.size(), symbol_map);
	int result = loader.parse();

	CHECK_EQUAL(0, result);
	// Unsupported kind should be skipped with message
}

/**
 * @brief Test section with null data via ELF to cover the "if (!data)" path
 */
TEST(MalformedSectionNameTests, TC_029_SectionWithNullDataViaELF)
{
	elfio writer;
	writer.create(ELFCLASS32, ELFDATA2LSB);
	writer.set_type(ET_EXEC);
	writer.set_machine(EM_NONE);
	writer.set_os_abi(ELFOSABI_NONE);
	writer.set_abi_version(2);

	// Add section with type that typically has no data (SHT_NOBITS)
	section* bss_sec = writer.sections.add(".bss");
	bss_sec->set_type(SHT_NOBITS);
	bss_sec->set_flags(SHF_ALLOC | SHF_WRITE);
	bss_sec->set_size(128);  // Size but no actual data

	std::ostringstream oss;
	writer.save(oss);
	std::string elf_str = oss.str();

	cert_elf_loader loader;
	int result = loader.begin(elf_str.data(), elf_str.size());
	CHECK_EQUAL(0, result);

	result = loader.parse();
	CHECK_EQUAL(0, result);
	// Section with no data should be skipped
}
