// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <vector>

#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cert_elf_loader.h>

cert_elf_loader::cert_elf_loader() {}

cert_elf_loader::cert_elf_loader(char *ctrl_elf, unsigned int ctrl_elf_len,
                                 std::map<std::string_view, uint64_t> &symbol) {
    this->ccode.symbol_map = symbol;
    this->begin(ctrl_elf, ctrl_elf_len);
}

void cert_elf_loader::close_elf() {}

cert_elf_loader::~cert_elf_loader() { this->close_elf(); }

int cert_elf_loader::_elf_begin() {
    printf("CLASS: %s\n", this->elf.get_class() == ELFCLASS32 ? "ELF32" : "ELF64");
    printf("Encoding: %s endian\n", this->elf.get_encoding() == ELFDATA2LSB ? "Little" : "Big");

    return 0;
}

int cert_elf_loader::parse() {
    size_t phdrnum, shdrnum, shdrstrndx;
    unsigned char abi_version;

    phdrnum = this->elf.segments.size();
    shdrnum = this->elf.sections.size();
    shdrstrndx = this->elf.get_section_name_str_index();
    abi_version = this->elf.get_abi_version();

    printf("EI_ABIVERSION	: %d\n", abi_version);
    printf("Elf Class	: %d\n", this->elf.get_class());
    printf("phdrnum		: %ld\n", phdrnum);
    printf("shdrnum		: %ld\n", shdrnum);
    printf("shdrstrndx	: %ld\n", shdrstrndx);

   // Save symbol_map before resetting ccode
	auto saved_symbol_map = std::move(this->ccode.symbol_map);
	this->ccode = {};
	this->ccode.symbol_map = std::move(saved_symbol_map);


    for (int i = 0; i < phdrnum; i++) {
        segment *phdr;

        phdr = this->elf.segments[i];
        printf(
            "PHDR[%d]: type: 0x%8x flag: 0x%8x va:0x%16lx pa: 0x%16lx filesz: 0x%16lx memsz: 0x%16lx align: 0x%16lx\n",
            i, phdr->get_type(), phdr->get_flags(), phdr->get_virtual_address(), phdr->get_physical_address(),
            phdr->get_file_size(), phdr->get_memory_size(), phdr->get_align());
    }

    for (int i = 0; i < shdrnum; i++) {
        section *shdr;
        std::string_view name;
        int col, page;
        const char *cname;
        const char *data;
        size_t size;

        shdr = this->elf.sections[i];
        name = shdr->get_name();
        cname = name.data();
        data = shdr->get_data();
        if (!data) {
            printf("Failed to get section data for index %d.\n", i);
            continue;
        }
        size = shdr->get_size();

        printf("\nParsing shdr: %s\n", cname);
        printf("Index:		%d\n", shdr->get_index());
        printf("sh_size:	%ld bytes\n", shdr->get_size());
        printf("sh_entsize:	%ld\n", shdr->get_entry_size());
        printf("num of entries:	%ld\n", shdr->get_size() / (shdr->get_entry_size() ? shdr->get_entry_size() : 1));

        if (this->is_ctrltext(&name, &col, &page)) {
            printf("ctrltext found: col: %d, page: %d\n", col, page);
            this->ccode.cols[col].pages[page].text_buf = static_cast<const void *>(data);
            this->ccode.cols[col].pages[page].text_size = size;
            printf("DATA:\n");
            hexdump(data, size);
        } else if (this->is_ctrldata(&name, &col, &page)) {
            printf("Ctrldata found: col: %d, page: %d\n", col, page);
            this->ccode.cols[col].pages[page].data_buf = data;
            this->ccode.cols[col].pages[page].data_size = size;
            //			hexdump(data, size);
        } else if (this->is_pad(&name, &col)) {
            printf("Pad foung: col: %d\n", col);
            this->ccode.cols[col].pad.emplace_back(data, size);
        } else if (!name.compare(".rela.dyn")) {
            ELFIO::relocation_section_accessor rela(this->elf, shdr);

            for (int i = 0; i < rela.get_entries_num(); i++) {
                struct rela_patch rp;

                rela.get_entry(i, rp.offset, rp.symbol, rp.type, rp.addend);
                printf("Rela-> offset: %ld, symbol: 0x%x, type: 0x%x addend: %ld\n", rp.offset, rp.symbol, rp.type,
                       rp.addend);

                this->ccode.rela.emplace_back(rp);
            }
        } else if (!name.compare(".dynsym")) {
            ELFIO::symbol_section_accessor symbols(this->elf, shdr);

            for (int i = 0; i < symbols.get_symbols_num(); i++) {
                struct dynsym_patch dynsym;

                symbols.get_symbol(i, dynsym.sym_name, dynsym.value, dynsym.size, dynsym.bind, dynsym.type,
                                   dynsym.section_index, dynsym.other);
                printf("dynsym: name: %s value: 0x%lx size: %ld, bind: 0x%x, type: 0x%x sec_ndx: %d other: 0x%x\n",
                       dynsym.sym_name.data(), dynsym.value, dynsym.size, dynsym.bind, dynsym.type,
                       dynsym.section_index, dynsym.other);
                this->ccode.dynsym.emplace_back(dynsym);
            }
        } else if (!name.compare(".dynstr")) {
            const char *p = static_cast<const char *>(data);

            for (; p < static_cast<const char *>(data) + size;) {
                this->ccode.dynstr.emplace_back(p);
                printf("[%ld]: %s\n", this->ccode.dynstr.size() - 1, this->ccode.dynstr.back().data());
                p = p + strlen(p) + 1;
            }
        }
    }

    /* patch RELA addresses */
    for (struct rela_patch rela : this->ccode.rela) {
        section *shdr;
        uint32_t dynsym_index;
        struct dynsym_patch *dynsym;
        std::string_view *dynstr;
        std::map<std::string_view, uint64_t>::iterator symmap;
        const void *data;
        size_t size;
        std::string_view name;
        const char *cname;
        uint8_t *ptr;
        Kind kind;
        uint64_t addend;

        dynsym_index = rela.symbol;
        printf("dymsym index: %u\n", dynsym_index);
        if (dynsym_index >= this->ccode.dynsym.size()) {
            printf("dynsym index outof bound: %ld\n", this->ccode.dynsym.size());
            continue;
        }
        if (dynsym_index >= this->ccode.dynstr.size()) {
            printf("dynsym index out of bound in dynstr: %ld\n", this->ccode.dynstr.size());
            continue;
        }
        dynsym = &this->ccode.dynsym.at(dynsym_index);
        dynstr = &this->ccode.dynstr.at(dynsym_index);
        printf("Need to patch for: %s\n", dynstr->data());

        symmap = this->ccode.symbol_map.find(dynstr->data());
        if (symmap == this->ccode.symbol_map.end()) {
            printf("symbol: \"%s\" not found in symbol table.\n", dynstr->data());
            continue;
        } else {
            printf("Found map, %s: 0x%lx\n", dynstr->data(), symmap->second);
        }

        shdr = this->elf.sections[dynsym->section_index];
        name = shdr->get_name();
        cname = name.data();
        printf("patch for section: %s\n", cname);
        data = shdr->get_data();
        size = shdr->get_size();

        ptr = static_cast<uint8_t *>(const_cast<void *>(data));
        ptr += rela.offset + 16;
        kind = abi_version == 2 ? static_cast<Kind>(rela.type) : static_cast<Kind>(rela.addend);
        addend = abi_version == 2 ? rela.addend : rela.addend >> 4;
        printf("Patch kind: %d\n", static_cast<int>(kind));
        switch (kind) {
        case Kind::SCALAR_32BIT_KIND:
            this->patch_scalar_32(reinterpret_cast<uint32_t *>(ptr), symmap->second + addend);
            printf("Data:\n");
            hexdump(data, size);
            break;
        case Kind::SHIM_DMA_BASE_ADDR_SYMBOL_KIND:
            this->aie2ps_patch_57(reinterpret_cast<uint32_t *>(ptr), symmap->second + addend);
            break;
        case Kind::SHIM_DMA_AIE4_BASE_ADDR_SYMBOL_KIND:
            this->aie4_patch_57(reinterpret_cast<uint32_t *>(ptr), symmap->second + addend);
            break;
        case Kind::CONTROL_PACKET_57:
            this->aie2ps_patch_ctrl_57(reinterpret_cast<uint32_t *>(ptr), symmap->second + addend);
            break;
        case Kind::CONTROL_PACKET_57_AIE4:
            this->aie4_patch_ctrl_57(reinterpret_cast<uint32_t *>(ptr), symmap->second + addend);
            break;
        default:
            printf("patch kind not supported\n");
            continue;
        }
    }

    for (auto &[col, col_code] : this->ccode.cols) {

        printf("first: %d\n", col);
        for (auto &[page, cpage] : col_code.pages) {

            printf(" pages: %d\n", page);
            printf("  text: %p size: %ld bytes\n", cpage.text_buf, cpage.text_size);
            printf("  data: %p size: %ld bytes\n", cpage.data_buf, cpage.data_size);
            printf("   total: %ld bytes\n", cpage.text_size + cpage.data_size);
            std::copy_n(reinterpret_cast<const char *>(cpage.text_buf), cpage.text_size,
                        std::back_inserter(col_code.code));
            std::copy_n(reinterpret_cast<const char *>(cpage.data_buf), cpage.data_size,
                        std::back_inserter(col_code.code));
        }
        for (struct ctrl_pad &pad : col_code.pad) {
            std::copy_n(reinterpret_cast<const char *>(pad.data_buf), pad.data_size, std::back_inserter(col_code.code));
        }
        printf("col: %d total control code size: %ld bytes\n", col, col_code.code.size());
    }

    for (auto &[col, col_code] : this->ccode.cols) {
        printf("col: %d, code size: %ld\n", col, col_code.code.size());
    }
    return 0;
}

void cert_elf_loader::patch_scalar_32(uint32_t *ptr, uint64_t patch) {
    uint64_t base_address = ptr[0];
    base_address += patch;
    ptr[0] = (uint32_t)(base_address & 0xFFFFFFFF);
}

void cert_elf_loader::aie4_patch_57(uint32_t *bd_data_ptr, uint64_t offset_val) {
    // in aie4, 57bit host address is defined in BD register 0,1
    uint64_t base_address = (((uint64_t)bd_data_ptr[0] & 0x1FFFFFF) << 32) | bd_data_ptr[1];

    base_address += offset_val;

    bd_data_ptr[1] = (uint32_t)(base_address & 0xFFFFFFFF);
    bd_data_ptr[0] = (bd_data_ptr[0] & 0xFE000000) | ((base_address >> 32) & 0x1FFFFFF);
}

void cert_elf_loader::aie2ps_patch_57(uint32_t *bd_data_ptr, uint64_t patch) {
    uint64_t base_address =
        (((uint64_t)bd_data_ptr[8] & 0x1FF) << 48) | (((uint64_t)bd_data_ptr[2] & 0xFFFF) << 32) | bd_data_ptr[1];
    base_address += patch;
    bd_data_ptr[1] = (uint32_t)(base_address & 0xFFFFFFFF);
    bd_data_ptr[2] = (bd_data_ptr[2] & 0xFFFF0000) | ((base_address >> 32) & 0xFFFF);
    bd_data_ptr[8] = (bd_data_ptr[8] & 0xFFFFFE00) | ((base_address >> 48) & 0x1FF);
}

void cert_elf_loader::aie2ps_patch_ctrl_57(uint32_t *bd_data_ptr, uint64_t patch) {
    // TODO need to change below logic to patch 57 bits
    uint64_t base_address;

    base_address = ((static_cast<uint64_t>(bd_data_ptr[3]) & 0xFFFF) << 32) | // NOLINT
                   ((static_cast<uint64_t>(bd_data_ptr[2])));

    base_address += patch;
    bd_data_ptr[2] = (uint32_t)(base_address & 0xFFFFFFFC);                // NOLINT
    bd_data_ptr[3] = (bd_data_ptr[3] & 0xFFFF0000) | (base_address >> 32); // NOLINT
}

void cert_elf_loader::aie4_patch_ctrl_57(uint32_t *bd_data_ptr, uint64_t patch) {
    // bd_data_ptr is a pointer to the header of the control code
    uint64_t base_address = (((uint64_t)bd_data_ptr[1] & 0x1FFFFFF) << 32) | bd_data_ptr[2];

    base_address += patch;
    bd_data_ptr[2] = (uint32_t)(base_address & 0xFFFFFFFF);
    bd_data_ptr[1] = (bd_data_ptr[0] & 0xFE000000) | ((base_address >> 32) & 0x1FFFFFF);
}

int cert_elf_loader::begin(const char *filename) {
    bool ret;

    ret = elf.load(filename);
    if (ret == false) {
        printf("Loading elf file: %s failed.\n", filename);
        return -ENODEV;
    }

    return this->_elf_begin();
}

int cert_elf_loader::begin(const void *elf_ptr, const size_t elf_size) {
    bool ret;

    this->file_sstream.write(static_cast<const char *>(elf_ptr), elf_size);

    ret = this->elf.load(this->file_sstream);
    if (ret == false) {
        printf("Loading elf failed.\n");
        return -ENODEV;
    }

    return this->_elf_begin();
}

bool cert_elf_loader::is_pad(std::string_view *name, int *col) {
    constexpr char str[] = ".pad.";
    size_t strlen = std::strlen(str);
    std::string scol;

    if (name->compare(0, strlen, str))
        return false;
    scol = name->substr(strlen);
    try {
        *col = std::stoi(scol);
    } catch (std::invalid_argument const &ex) {
        return false;
    } catch (std::out_of_range const &ex) {
        return false;
    }

    return true;
}

bool cert_elf_loader::is_ctrltext(std::string_view *name, int *col, int *page) {
    constexpr char str[] = ".ctrltext.";
    size_t strlen = std::strlen(str);
    int pos;
    std::string col_page;
    std::string scol;
    std::string spage;

    if (name->compare(0, strlen, str))
        return false;
    col_page = name->substr(strlen);
    pos = col_page.find_first_of(".");
    if (pos <= 0)
        return false;
    scol = col_page.substr(0, pos);
    spage = col_page.substr(pos + 1);
    try {
        *col = std::stoi(scol);
    } catch (std::invalid_argument const &ex) {
        return false;
    } catch (std::out_of_range const &ex) {
        return false;
    }
    try {
        *page = std::stoi(spage);
    } catch (std::invalid_argument const &ex) {
        return false;
    } catch (std::out_of_range const &ex) {
        return false;
    }

    return true;
}

bool cert_elf_loader::is_ctrldata(std::string_view *name, int *col, int *page) {
    constexpr char str[] = ".ctrldata.";
    size_t strlen = std::strlen(str);
    int pos;
    std::string col_page;
    std::string scol;
    std::string spage;

    if (name->compare(0, strlen, str))
        return false;
    col_page = name->substr(strlen);
    pos = col_page.find_first_of(".");
    if (pos <= 0)
        return false;
    scol = col_page.substr(0, pos);
    spage = col_page.substr(pos + 1);
    try {
        *col = std::stoi(scol);
    } catch (std::invalid_argument const &ex) {
        return false;
    } catch (std::out_of_range const &ex) {
        return false;
    }
    try {
        *page = std::stoi(spage);
    } catch (std::invalid_argument const &ex) {
        return false;
    } catch (std::out_of_range const &ex) {
        return false;
    }

    return true;
}
