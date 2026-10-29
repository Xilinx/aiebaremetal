// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "stub_cert_elf_loader.h"
#include "cert_elf_loader.h"

#ifdef CERT_ELF_SYMBOLS_FROM_OTHER_MOCK
extern "C" {
extern unsigned char cert_elf[];
extern unsigned int cert_elf_len;
}
#else
extern "C" {
unsigned char cert_elf[] = {0x7f, 'E', 'L', 'F'};
unsigned int cert_elf_len = sizeof(cert_elf);
}
#endif

static struct ccode g_layout;

void StubCertElfLoader_SetColsLayout(unsigned int count,
                                     const std::vector<size_t> &sizes)
{
    g_layout.cols.clear();
    for (unsigned int i = 0; i < count; ++i) {
        struct col_code col;
        size_t sz = (i < sizes.size()) ? sizes[i] : 0u;
        col.code.resize(sz, 0);
        g_layout.cols.emplace(i, col);
    }
}

cert_elf_loader::cert_elf_loader(const char *ctrl_elf, uint32_t ctrl_elf_len,
		        const std::map<std::string_view, uint64_t> &symbol)
{
    (void)ctrl_elf;
    (void)ctrl_elf_len;
    (void)symbol;
}

cert_elf_loader::~cert_elf_loader()
{
}

int cert_elf_loader::parse()
{
    ccode = g_layout;
    return 0;
}
