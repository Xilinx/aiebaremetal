// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef AEG_IOUTILS_H
#define AEG_IOUTILS_H

#include <stdint.h>

void setWrite32Ptr   (void (*func)(uint64_t, uint32_t));
void setRead32Ptr    (uint32_t (*func)(uint64_t));
void setWriteCmdPtr  (void (*func)(uint8_t, uint8_t, uint8_t,
                                   uint32_t, uint32_t, uint8_t*));
void setNpiWrite32Ptr(void (*func)(uint64_t, uint32_t));
void setNpiRead32Ptr (uint32_t (*func)(uint64_t));
void setWriteGMPtr   (void (*func)(uint64_t, const void*, uint64_t));
void setReadGMPtr    (void (*func)(uint64_t, void*, uint64_t));
void setPLIOStartPtr (void (*func)());

extern "C" {
void         ess_Write32   (uint64_t Addr, uint32_t Data);
uint32_t ess_Read32    (uint64_t Addr);
void         ess_WriteGM   (uint64_t addr, const void* data, uint64_t size);
void         ess_ReadGM    (uint64_t addr, void* data, uint64_t size);
void         ess_NpiWrite32(uint64_t Addr, uint32_t Data);
uint32_t ess_NpiRead32 (uint64_t Addr);
void         ess_WriteCmd  (uint8_t Command,
                            uint8_t ColId, uint8_t RowId,
                            uint32_t CmdWd0, uint32_t CmdWd1,
                            uint8_t* CmdStr);
void         ess_debug     (const char* msg);

void XPl_Write32(uint64_t base, uint32_t stride, int idx, uint32_t offset, uint32_t value);
void XPl_Write64(uint64_t base, uint32_t stride, int idx, uint32_t offset, uint64_t value);
void XPl_Read32 (uint64_t base, uint32_t stride, int idx, uint32_t offset, uint32_t& value);
void XPl_Read64 (uint64_t base, uint32_t stride, int idx, uint32_t offset, uint64_t& value);
void XPl_IOstart();
} // extern "C"
#endif // AEG_IOUTILS_H