// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <charconv>
#include <cstring>
#include <fcntl.h>
#include <cstdio>
#include <string>
#include <system_error>
#include <unistd.h>
#include <vector>

#include "aeg_log.h"
#include <sys/stat.h>
#include <sys/types.h>

#include <cert_elf_loader.h>

namespace {
void append_bytes(std::vector<char> &dst, const char *src, std::size_t n) {
    if ((src == nullptr) || (n == 0U)) {
        return;
    }
    const std::size_t old_size = dst.size();
    dst.resize(old_size + n);
    std::size_t i = 0U;
    while (i < n) {
        dst[old_size + i] = src[i];
        ++i;
    }
}
} // namespace

namespace cert_elfloader {

cert_elf_loader::cert_elf_loader() {}

cert_elf_loader::cert_elf_loader(const char *ctrl_elf, uint32_t ctrl_elf_len,
                                 const std::map<std::string_view, uint64_t> &symbol) {
    this->ccode.symbol_map = symbol;
    static_cast<void>(this->begin(ctrl_elf, static_cast<std::size_t>(ctrl_elf_len)));
}

void cert_elf_loader::close_elf() const {}

cert_elf_loader::~cert_elf_loader() { this->close_elf(); }

int32_t cert_elf_loader::elf_begin() const {
    AEG_LOG("CLASS: %s\n",
            (static_cast<uint32_t>(this->elf.get_class()) == static_cast<uint32_t>(ELFIO::ELFCLASS32)) ? "ELF32" : "ELF64");
    AEG_LOG("Encoding: %s endian\n",
            (static_cast<uint32_t>(this->elf.get_encoding()) == static_cast<uint32_t>(ELFIO::ELFDATA2LSB)) ? "Little" : "Big");

    return 0;
}

int32_t cert_elf_loader::parse() {
    const size_t shdrstrndx = static_cast<size_t>(this->elf.get_section_name_str_index());
    const size_t phdrnum = static_cast<size_t>(this->elf.segments.size());
    const size_t shdrnum = static_cast<size_t>(this->elf.sections.size());
    const uint8_t abi_version = this->elf.get_abi_version();

    AEG_LOG("EI_ABIVERSION\t: %d\n", static_cast<int32_t>(abi_version));
    AEG_LOG("Elf Class\t: %d\n", static_cast<int32_t>(this->elf.get_class()));
    AEG_LOG("phdrnum\t\t: %zu\n", phdrnum);
    AEG_LOG("shdrnum\t\t: %zu\n", shdrnum);
    AEG_LOG("shdrstrndx : %zu\n", shdrstrndx);

    const auto saved_symbol_map = this->ccode.symbol_map;
    this->ccode = {};
    this->ccode.symbol_map = saved_symbol_map;

    for (size_t i = 0U; i < phdrnum; ++i) {
        ELFIO::segment *phdr;
        phdr = this->elf.segments[static_cast<uint32_t>(i)];
        AEG_LOG("PHDR[%zu]: type: 0x%8x flag: 0x%8x va:0x%16llx pa: 0x%16llx filesz: 0x%16llx memsz: 0x%16llx align: 0x%16llx\n",
            i, static_cast<uint32_t>(phdr->get_type()), static_cast<uint32_t>(phdr->get_flags()),
            static_cast<uint64_t>(phdr->get_virtual_address()), static_cast<uint64_t>(phdr->get_physical_address()),
            static_cast<uint64_t>(phdr->get_file_size()), static_cast<uint64_t>(phdr->get_memory_size()),
            static_cast<uint64_t>(phdr->get_align()));
    }

    for (size_t i = 0U; i < shdrnum; ++i) {
        int32_t col = 0;
        int32_t page = 0;
        ELFIO::section *shdr = this->elf.sections[static_cast<uint32_t>(i)];
        if (shdr == nullptr) {
            AEG_ERROR("Failed to get section for index %zu.\n", i);
            continue;
        }
        const std::string name_str = shdr->get_name();
        std::string_view name = name_str;
        const char *cname = name.data();
        static_cast<void>(cname);

        const char *data = shdr->get_data();
        if (data == nullptr) {
            AEG_ERROR("Failed to get section data for index %zu.\n", i);
            continue;
        }
        size_t size = shdr->get_size();

        AEG_LOG("\nParsing shdr: %s\n", cname);
        AEG_LOG("Index:\t\t%d\n", static_cast<int32_t>(shdr->get_index()));
        AEG_LOG("sh_size:\t%zu bytes\n", static_cast<size_t>(shdr->get_size()));
        AEG_LOG("sh_entsize:\t%zu\n", static_cast<size_t>(shdr->get_entry_size()));
        AEG_LOG("num of entries:\t%zu\n",
                static_cast<size_t>(shdr->get_size() / ((shdr->get_entry_size() != 0U) ? shdr->get_entry_size() : 1U)));

        if (this->is_ctrltext(&name, &col, &page)) {
            AEG_LOG("ctrltext found: col: %d, page: %d\n", col, page);
            this->ccode.cols[col].pages[page].text_buf = data;
            this->ccode.cols[col].pages[page].text_size = size;
            AEG_LOG("DATA:\n");
            hexdump(data, static_cast<ssize_t>(size));
        } else if (this->is_ctrldata(&name, &col, &page)) {
            AEG_LOG("Ctrldata found: col: %d, page: %d\n", col, page);
            this->ccode.cols[col].pages[page].data_buf = data;
            this->ccode.cols[col].pages[page].data_size = size;
        } else if (this->is_pad(&name, &col)) {
            AEG_LOG("Pad foung: col: %d\n", col);
            static_cast<void>(this->ccode.cols[col].pad.emplace_back(data, size));
        } else if (name.compare(".rela.dyn") == 0) {
            ELFIO::relocation_section_accessor rela(this->elf, shdr);

            const int32_t rela_entries = static_cast<int32_t>(rela.get_entries_num());
            for (int32_t rela_idx = 0; rela_idx < rela_entries; rela_idx++) {
                struct rela_patch rp{};

                if (!rela.get_entry(rela_idx, rp.offset, rp.symbol, rp.type, rp.addend)) {
                    AEG_LOG("Failed to read rela entry %d\n", rela_idx);
                }
                AEG_LOG("Rela-> offset: %llu, symbol: 0x%x, type: 0x%x addend: %lld\n",
                        static_cast<uint64_t>(rp.offset), static_cast<uint32_t>(rp.symbol),
                        static_cast<uint32_t>(rp.type), static_cast<int64_t>(rp.addend));

                static_cast<void>(this->ccode.rela.emplace_back(rp));
            }
        } else if (name.compare(".dynsym") == 0) {
            ELFIO::symbol_section_accessor symbols(this->elf, shdr);

            const int32_t sym_count = static_cast<int32_t>(symbols.get_symbols_num());
            for (int32_t sym_idx = 0; sym_idx < sym_count; sym_idx++) {
                struct dynsym_patch dynsym{};

                if (!symbols.get_symbol(sym_idx, dynsym.sym_name, dynsym.value, dynsym.size, dynsym.bind, dynsym.type,
                                        dynsym.section_index, dynsym.other)) {
                    AEG_LOG("Failed to read dynsym entry %d\n", sym_idx);
                }
                AEG_LOG("dynsym: name: %s value: 0x%llx size: %llu, bind: 0x%x, type: 0x%x sec_ndx: %d other: 0x%x\n",
                        dynsym.sym_name.data(), static_cast<uint64_t>(dynsym.value), static_cast<uint64_t>(dynsym.size),
                        static_cast<uint32_t>(dynsym.bind),
                        static_cast<uint32_t>(dynsym.type), static_cast<int32_t>(dynsym.section_index),
                        static_cast<uint32_t>(dynsym.other));
                static_cast<void>(this->ccode.dynsym.emplace_back(dynsym));
            }
        } else if (name.compare(".dynstr") == 0) {
            const char *p = static_cast<const char *>(data);

            const char *const data_end = static_cast<const char *>(data) + size;
            while (p < data_end) {
                static_cast<void>(this->ccode.dynstr.emplace_back(p));
                AEG_LOG("[%zu]: %s\n", this->ccode.dynstr.size() - 1U, this->ccode.dynstr.back().data());
                size_t slen = 0U;
                while (((p + slen) < data_end) && (p[slen] != '\0')) {
                    ++slen;
                }
                p = p + slen + 1U;
            }
        } else {
            /* no action for other sections */
        }
    }

    /* patch RELA addresses */
    for (struct rela_patch rela : this->ccode.rela) {
        uint32_t dynsym_index = 0U;
        std::map<std::string_view, uint64_t>::iterator symmap;

        dynsym_index = rela.symbol;
        AEG_LOG("dymsym index: %u\n", dynsym_index);
        if (static_cast<size_t>(dynsym_index) >= this->ccode.dynsym.size()) {
            AEG_ERROR("dynsym index outof bound: %zu\n", this->ccode.dynsym.size());
            continue;
        }
        if (static_cast<size_t>(dynsym_index) >= this->ccode.dynstr.size()) {
            AEG_ERROR("dynsym index out of bound in dynstr: %zu\n", this->ccode.dynstr.size());
            continue;
        }
        struct dynsym_patch *dynsym = &this->ccode.dynsym.at(dynsym_index);
        std::string_view *dynstr = &this->ccode.dynstr.at(dynsym_index);
        AEG_LOG("Need to patch for: %s\n", dynstr->data());

        symmap = this->ccode.symbol_map.find(dynstr->data());
        if (symmap == this->ccode.symbol_map.end()) {
            AEG_ERROR("symbol: \"%s\" not found in symbol table.\n", dynstr->data());
            continue;
        } else {
            AEG_LOG("Found map, %s: 0x%llx\n", dynstr->data(), static_cast<uint64_t>(symmap->second));
        }

        ELFIO::section *shdr = this->elf.sections[static_cast<uint32_t>(dynsym->section_index)];
        if (shdr == nullptr) {
            AEG_ERROR("Failed to get section for index %d.\n", static_cast<int32_t>(dynsym->section_index));
            continue;
        }
        const std::string name_str = shdr->get_name();
        std::string_view name = name_str;
        const char *cname = name.data();
        static_cast<void>(cname);
        AEG_LOG("patch for section: %s\n", cname);
        const void *data = shdr->get_data();
        size_t size = shdr->get_size();

        uint8_t *ptr =
            const_cast<uint8_t *>(static_cast<const uint8_t *>(data));
        ptr += rela.offset + 16U;
        Kind kind = (abi_version == 2U) ? static_cast<Kind>(rela.type) : static_cast<Kind>(rela.addend);
        const uint64_t addend_u64 = static_cast<uint64_t>(rela.addend);
        const uint64_t addend =
            (abi_version == 2U) ? addend_u64 : (addend_u64 >> 4U);
        AEG_LOG("Patch kind: %d\n", static_cast<int32_t>(kind));
        switch (kind) {
        case Kind::SCALAR_32BIT_KIND:
        {
            this->patch_scalar_32(reinterpret_cast<uint32_t *>(ptr), symmap->second + addend);
            AEG_LOG("Data:\n");
            hexdump(data, static_cast<ssize_t>(size));
            break;
        }
        case Kind::SHIM_DMA_BASE_ADDR_SYMBOL_KIND:
        {
            this->aie2ps_patch_57(reinterpret_cast<uint32_t *>(ptr), symmap->second + addend);
            break;
        }
        case Kind::SHIM_DMA_AIE4_BASE_ADDR_SYMBOL_KIND:
        {
            this->aie4_patch_57(reinterpret_cast<uint32_t *>(ptr), symmap->second + addend);
            break;
        }
        case Kind::CONTROL_PACKET_57:
        {
            this->aie2ps_patch_ctrl_57(reinterpret_cast<uint32_t *>(ptr), symmap->second + addend);
            break;
        }
        case Kind::CONTROL_PACKET_57_AIE4:
        {
            this->aie4_patch_ctrl_57(reinterpret_cast<uint32_t *>(ptr), symmap->second + addend);
            break;
        }
        default:
        {
            AEG_ERROR("patch kind not supported\n");
            break;
        }
        }
    }

    for (auto &[col, cc_data] : this->ccode.cols) {
        AEG_LOG("first: %d\n", col);
        for (auto &[page, cpage] : cc_data.pages) {
            AEG_LOG(" pages: %d\n", page);
            AEG_LOG("  text: %p size: %zu bytes\n", cpage.text_buf, cpage.text_size);
            AEG_LOG("  data: %p size: %zu bytes\n", cpage.data_buf, cpage.data_size);
            AEG_LOG("   total: %zu bytes\n", cpage.text_size + cpage.data_size);
            append_bytes(cc_data.code, cpage.text_buf, cpage.text_size);
            append_bytes(cc_data.code, cpage.data_buf, cpage.data_size);
        }
        for (struct ctrl_pad &pad : cc_data.pad) {
            append_bytes(cc_data.code, pad.data_buf, pad.data_size);
        }
        AEG_LOG("col: %d total control code size: %zu bytes\n", col, cc_data.code.size());
    }

    for (auto &[cc_col, cc_data] : this->ccode.cols) {
        static_cast<void>(cc_col);
        AEG_LOG("col: %d, code size: %zu\n", cc_col, cc_data.code.size());
    }
    return 0;
}

void cert_elf_loader::patch_scalar_32(uint32_t *ptr, uint64_t patch) const {
    uint64_t base_address = static_cast<uint64_t>(ptr[0]);
    base_address += patch;
    ptr[0] = static_cast<uint32_t>(base_address & 0xFFFFFFFFU);
}

void cert_elf_loader::aie4_patch_57(uint32_t *bd_data_ptr, uint64_t offset_val) const {
    // in aie4, 57bit host address is defined in BD register 0,1
    uint64_t base_address = ((static_cast<uint64_t>(bd_data_ptr[0]) & 0x1FFFFFFU) << 32U) | static_cast<uint64_t>(bd_data_ptr[1]);

    base_address += offset_val;
    bd_data_ptr[1] = static_cast<uint32_t>(base_address & 0xFFFFFFFFU);
    bd_data_ptr[0] = (bd_data_ptr[0] & 0xFE000000U) | static_cast<uint32_t>((base_address >> 32U) & 0x1FFFFFFU);
}

void cert_elf_loader::aie2ps_patch_57(uint32_t *bd_data_ptr, uint64_t patch) const {
    std::uint64_t base_address =
        ((static_cast<uint64_t>(bd_data_ptr[8]) & 0x1FFU) << 48U) | ((static_cast<uint64_t>(bd_data_ptr[2]) & 0xFFFFU) << 32U) | static_cast<uint64_t>(bd_data_ptr[1]);
    base_address += patch;
    bd_data_ptr[1] = static_cast<uint32_t>(base_address & 0xFFFFFFFFU);
    bd_data_ptr[2] = (bd_data_ptr[2] & 0xFFFF0000U) | static_cast<uint32_t>((base_address >> 32U) & 0xFFFFU);
    bd_data_ptr[8] = (bd_data_ptr[8] & 0xFFFFFE00U) | static_cast<uint32_t>((base_address >> 48U) & 0x1FFU);
}

void cert_elf_loader::aie2ps_patch_ctrl_57(uint32_t *bd_data_ptr, uint64_t patch) const {
    // TODO need to change below logic to patch 57 bits
    const uint32_t bd_word2 = bd_data_ptr[2];
    const uint32_t bd_word3 = bd_data_ptr[3];
    std::uint64_t base_high = bd_word3;
    base_high &= 0xFFFFULL;
    base_high <<= 32U;
    const uint64_t base_low = bd_word2;
    uint64_t base_address = base_high | base_low;

    base_address += patch;
    bd_data_ptr[2] = static_cast<uint32_t>(base_address & 0xFFFFFFFCU);
    bd_data_ptr[3] = (bd_data_ptr[3] & 0xFFFF0000U) | static_cast<uint32_t>(base_address >> 32U); // NOLINT
}

void cert_elf_loader::aie4_patch_ctrl_57(uint32_t *bd_data_ptr, uint64_t patch) const {

    uint64_t base_address = ((static_cast<uint64_t>(bd_data_ptr[1]) & 0x1FFFFFFU) << 32U) | static_cast<uint64_t>(bd_data_ptr[2]);
    base_address += patch;
    bd_data_ptr[2] = static_cast<uint32_t>(base_address & 0xFFFFFFFFU);
    bd_data_ptr[1] = (bd_data_ptr[0] & 0xFE000000U) | static_cast<uint32_t>((base_address >> 32U) & 0x1FFFFFFU);
}

int32_t cert_elf_loader::begin(const char *filename) {
    const bool ret = elf.load(filename);
    if (!ret) {
        AEG_ERROR("Loading elf file: %s failed.\n", filename);
        return -ENODEV;
    }

    return this->elf_begin();
}

int32_t cert_elf_loader::begin(const void *elf_ptr, const size_t elf_size) {
    static_cast<void>(
        this->file_sstream.write(static_cast<const char *>(elf_ptr), static_cast<std::streamsize>(elf_size)));

    const bool ret = this->elf.load(this->file_sstream);
    if (!ret) {
        AEG_ERROR("Loading elf failed.\n");
        return -ENODEV;
    }

    return this->elf_begin();
}

bool cert_elf_loader::is_pad(const std::string_view *name, int32_t *col) const {
    constexpr std::string_view prefix{".pad."};

    if (name->compare(0U, prefix.size(), prefix) != 0) {
        return false;
    }
    const std::string_view scol = name->substr(prefix.size());
    int32_t parsed = 0;
    const char *const first = scol.data();
    const char *const last = first + scol.size();
    const std::from_chars_result res = std::from_chars(first, last, parsed);
    if ((res.ec != std::errc{}) || (res.ptr != last)) {
        return false;
    }
    *col = parsed;

    return true;
}

bool cert_elf_loader::is_ctrltext(const std::string_view *name, int32_t *col, int32_t *page) const {
    constexpr std::string_view prefix{".ctrltext."};

    if (name->compare(0U, prefix.size(), prefix) != 0) {
        return false;
    }
    const std::string_view col_page = name->substr(prefix.size());
    const size_t pos = col_page.find_first_of(".");
    if ((pos == std::string_view::npos) || (pos == 0U)) {
        return false;
    }
    const std::string_view scol = col_page.substr(0U, pos);
    const std::string_view spage = col_page.substr(pos + 1U);

    int32_t parsed_col = 0;
    const std::from_chars_result col_res =
        std::from_chars(scol.data(), scol.data() + scol.size(), parsed_col);
    if ((col_res.ec != std::errc{}) || (col_res.ptr != (scol.data() + scol.size()))) {
        return false;
    }

    int32_t parsed_page = 0;
    const std::from_chars_result page_res =
        std::from_chars(spage.data(), spage.data() + spage.size(), parsed_page);
    if ((page_res.ec != std::errc{}) || (page_res.ptr != (spage.data() + spage.size()))) {
        return false;
    }

    *col = parsed_col;
    *page = parsed_page;
    return true;
}

bool cert_elf_loader::is_ctrldata(const std::string_view *name, int32_t *col, int32_t *page) const {
    constexpr std::string_view prefix{".ctrldata."};

    if (name->compare(0U, prefix.size(), prefix) != 0) {
        return false;
    }
    const std::string_view col_page = name->substr(prefix.size());
    const size_t pos = col_page.find_first_of(".");
    if ((pos == std::string_view::npos) || (pos == 0U)) {
        return false;
    }
    const std::string_view scol = col_page.substr(0U, pos);
    const std::string_view spage = col_page.substr(pos + 1U);

    int32_t parsed_col = 0;
    const std::from_chars_result col_res =
        std::from_chars(scol.data(), scol.data() + scol.size(), parsed_col);
    if ((col_res.ec != std::errc{}) || (col_res.ptr != (scol.data() + scol.size()))) {
        return false;
    }

    int32_t parsed_page = 0;
    const std::from_chars_result page_res =
        std::from_chars(spage.data(), spage.data() + spage.size(), parsed_page);
    if ((page_res.ec != std::errc{}) || (page_res.ptr != (spage.data() + spage.size()))) {
        return false;
    }

    *col = parsed_col;
    *page = parsed_page;
    return true;
}
} // namespace cert_elfloader