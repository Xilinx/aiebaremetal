#ifndef STUB_CERT_ELF_LOADER_H
#define STUB_CERT_ELF_LOADER_H

#include <stdint.h>
#include <cstddef>

#ifdef __cplusplus
#include <vector>
extern "C" {
#endif

void Stub_CertElfLoader_Reset(void);
void Stub_CertElfLoader_SetNumCols(uint32_t num_cols);
void Stub_CertElfLoader_SetColumns(uint32_t num_cols, const int *col_ids, const uint32_t *sizes);

#ifdef __cplusplus
}

// C++-only function
void StubCertElfLoader_SetColsLayout(unsigned int count,
                                     const std::vector<size_t> &sizes);
#endif

#endif /* STUB_CERT_ELF_LOADER_H */
