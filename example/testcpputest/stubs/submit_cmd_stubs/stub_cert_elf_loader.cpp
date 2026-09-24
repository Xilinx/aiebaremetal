#include "stub_cert_elf_loader.h"
#include "cert_elf_loader.h"

/* Dummy globals referenced by cert_host.cc */
extern "C" {
unsigned char cert_elf[] = { 0x7f, 'E', 'L', 'F' };
unsigned int  cert_elf_len = sizeof(cert_elf);
}

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

cert_elf_loader::cert_elf_loader(char *ctrl_elf, unsigned int ctrl_elf_len,
                                 std::map<std::string_view, uint64_t> &symbol)
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
