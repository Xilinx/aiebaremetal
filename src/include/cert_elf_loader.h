// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef CERT_ELF_LOADER_H
#define CERT_ELF_LOADER_H

#include <cstring>
#include <map>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#include "undefelf.h"
#include <elfio/elfio.hpp>
#include "aeg_log.h"
namespace cert_elfloader {


enum class Kind : int32_t {
    UC_DMA_REMOTE_PTR_SYMBOL_KIND = 1,
    SHIM_DMA_BASE_ADDR_SYMBOL_KIND = 2,
    SCALAR_32BIT_KIND = 3,
    CONTROL_PACKET_48 = 4,
    SHIM_DMA_AIE4_BASE_ADDR_SYMBOL_KIND = 6,
    CONTROL_PACKET_57 = 7,
    CONTROL_PACKET_57_AIE4 = 9,
    UNKNOWN_SYMBOL_KIND = 10
};

inline void hexdump(const void *data, const ssize_t size) {
    for (ssize_t i = 0; i < size; i++) {
        if ((i % 16) == 0) {
            AEG_LOG("\n%08x: ", static_cast<uint32_t>(i));
        }
        AEG_LOG("%02x ", static_cast<uint32_t>(static_cast<const uint8_t *>(data)[i]));
    }
    AEG_LOG("\n");
}

struct ctrl_page {
    const char *text_buf;
    size_t text_size;
    const char *data_buf;
    size_t data_size;
};

struct ctrl_pad {
    const char *data_buf;
    size_t data_size;
    ctrl_pad() : data_buf(nullptr), data_size(0U) {}
    ctrl_pad(const char *data_buf, size_t data_size) : data_buf(data_buf), data_size(data_size) {}
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
    unsigned type; // must match ELFIO get_entry(unsigned&) signature
    ELFIO::Elf_Sxword addend;
};

struct dynsym_patch {
    std::string sym_name;
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    uint8_t bind;
    uint8_t type;
    ELFIO::Elf_Half section_index;
    uint8_t other;
};

struct ccode {
    /* map of col to col pages */
    std::map<int, struct col_code> cols;
    std::vector<struct rela_patch> rela;
    std::vector<struct dynsym_patch> dynsym;
    std::vector<std::string_view> dynstr;
    std::map<std::string_view, uint64_t> symbol_map;

    ccode() = default;
    ccode(const ccode &) = default;
    ccode(ccode &&) noexcept = default;
    ccode &operator=(const ccode &) & = default;
    ccode &operator=(ccode &&) & noexcept = default;
};

class cert_elf_loader {
  public:
    struct ccode ccode;
    ELFIO::elfio elf;
    std::stringstream file_sstream;
    std::istream *file_stream = nullptr;

    cert_elf_loader();
    cert_elf_loader(const char *ctrl_elf, uint32_t ctrl_elf_len, const std::map<std::string_view, uint64_t> &symbol);

    cert_elf_loader(const cert_elf_loader&) = delete;
    cert_elf_loader& operator=(const cert_elf_loader&) = delete;
    ~cert_elf_loader();
    int32_t begin(const char *filename);
    int32_t begin(const void *elf_ptr, const size_t elf_size);
    void close_elf() const;
    int32_t parse();

   protected:
    bool is_ctrltext(const std::string_view *name, int32_t *col, int32_t *page) const;
    bool is_ctrldata(const std::string_view *name, int32_t *col, int32_t *page) const;
    bool is_pad(const std::string_view *name, int32_t *col) const;
        void patch_scalar_32(uint32_t *ptr, uint64_t patch) const;
        void aie4_patch_57(uint32_t *bd_data_ptr, uint64_t offset_val) const;
  private:
    int32_t elf_begin() const;
    void aie2ps_patch_57(uint32_t *bd_data_ptr, uint64_t patch) const;
    void aie2ps_patch_ctrl_57(uint32_t *bd_data_ptr, uint64_t patch) const;
    void aie4_patch_ctrl_57(uint32_t *bd_data_ptr, uint64_t patch) const;
};
} // namespace cert_elfloader
#endif // CERT_ELF_LOADER_H
