// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#include "undefelf.h"
#include <elfio/elfio.hpp>
using namespace ELFIO;

enum class Kind {
    UC_DMA_REMOTE_PTR_SYMBOL_KIND = 1,
    SHIM_DMA_BASE_ADDR_SYMBOL_KIND = 2,
    SCALAR_32BIT_KIND = 3,
    CONTROL_PACKET_48 = 4,
    SHIM_DMA_AIE4_BASE_ADDR_SYMBOL_KIND = 6,
    CONTROL_PACKET_57 = 7,
    CONTROL_PACKET_57_AIE4 = 9,
    UNKNOWN_SYMBOL_KIND = 10
};

static inline void hexdump(const void *data, const ssize_t size) {
    int i;
    uint8_t *buf = (uint8_t *)data;

    for (i = 0; i < size; i++) {
        if (!(i % 16)) {
            printf("\n%08x: ", i);
        }
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

struct ctrl_page {
    const void *text_buf;
    size_t text_size;
    const void *data_buf;
    size_t data_size;
};

struct ctrl_pad {
    const void *data_buf;
    size_t data_size;
    ctrl_pad() : data_buf(0), data_size(0) {}
    ctrl_pad(const void *data_buf, size_t data_size) : data_buf(data_buf), data_size(data_size) {}
    ~ctrl_pad() {}
};

struct col_code {
    /* map of page number to ctrl_page */
    std::map<int, struct ctrl_page> pages;
    std::vector<struct ctrl_pad> pad;
    std::vector<char> code;
    std::vector<uint32_t> host_args;
};

struct patch {
    uint64_t address;
    bool patch_hostargs;
};

struct rela_patch {
    ELFIO::Elf64_Addr offset;
    ELFIO::Elf_Word symbol;
    unsigned int type;
    ELFIO::Elf_Sxword addend;
};

struct dynsym_patch {
    std::string sym_name;
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    unsigned char bind;
    unsigned char type;
    ELFIO::Elf_Half section_index;
    unsigned char other;
};

struct ccode {
    /* map of col to col pages */
    std::map<int, struct col_code> cols;
    std::vector<struct rela_patch> rela;
    std::vector<struct dynsym_patch> dynsym;
    std::vector<std::string_view> dynstr;
    std::map<std::string_view, uint64_t> symbol_map;
};

class cert_elf_loader {
  public:
    struct ccode ccode;

    cert_elf_loader();
    cert_elf_loader(char *ctrl_elf, unsigned int ctrl_elf_len, std::map<std::string_view, uint64_t> &symbol);

    ~cert_elf_loader();
    int begin(const char *filename);
    int begin(const void *elf_ptr, const size_t elf_size);
    void close_elf();
    int parse();

 protected:
	bool is_ctrltext(std::string_view *name, int *col, int *page);
	bool is_ctrldata(std::string_view *name, int *col, int *page);
	bool is_pad(std::string_view *name, int *col);
	void patch_scalar_32(uint32_t *ptr, uint64_t patch);
	void aie4_patch_57(uint32_t *bd_data_ptr, uint64_t offset_val);

  private:
    elfio elf;
    std::stringstream file_sstream;
    std::istream *file_stream;

    int _elf_begin();
    void aie2ps_patch_57(uint32_t *bd_data_ptr, uint64_t patch);
    void aie2ps_patch_ctrl_57(uint32_t *bd_data_ptr, uint64_t patch);
    void aie4_patch_ctrl_57(uint32_t *bd_data_ptr, uint64_t patch);
};
