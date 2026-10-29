// 67d7842dbbe25473c3c32b93c0da8047785f30d78e8a024de1b57352245f9689
// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef __XIOUTILS_H__
#define __XIOUTILS_H__

#include <stdint.h>

void setWrite32Ptr(void (*func)(uint64_t, unsigned int));
void setRead32Ptr(unsigned int (*func)(uint64_t)) ;
void setWriteCmdPtr(void (*func)(unsigned char, unsigned char, unsigned char, unsigned int, unsigned int, unsigned char*)) ;
void setNpiWrite32Ptr(void (*func)(uint64_t, unsigned int));
void setNpiRead32Ptr(unsigned int (*func)(uint64_t));
void setWriteGMPtr(void (*func)(uint64_t, const void*, uint64_t));
void setReadGMPtr(void (*func)(uint64_t,  void*, uint64_t));
void setPLIOStartPtr(void (*func)());

#ifdef __cplusplus
extern "C" {
#endif
void ess_Write32(uint64_t Addr, unsigned int Data);
unsigned int ess_Read32(uint64_t Addr);
void ess_WriteGM(uint64_t addr, const void* data, uint64_t size);
void ess_ReadGM(uint64_t addr, void* data, uint64_t size);
void ess_NpiWrite32(uint64_t Addr, unsigned int Data);
unsigned int ess_NpiRead32(uint64_t Addr);
void ess_WriteCmd(unsigned char Command, unsigned char ColId, unsigned char RowId, unsigned int CmdWd0, unsigned int CmdWd1, unsigned char *CmdStr);
#ifdef __cplusplus
} // extern "C"
#endif



/* Low level read/write APIs for PL address space in AIESIM */
void XPl_Write32(uint64_t ctrl_pl_offset, uint32_t ctrl_ip_stride, int pl_ip_index, uint32_t relative_address, uint32_t value);
void XPl_Write64(uint64_t ctrl_pl_offset, uint32_t ctrl_ip_stride, int pl_ip_index, uint32_t relative_address, uint64_t value);
void XPl_Read32(uint64_t ctrl_pl_offset, uint32_t ctrl_ip_stride, int pl_ip_index, uint32_t relative_address, uint32_t& value);
void XPl_Read64(uint64_t ctrl_pl_offset, uint32_t ctrl_ip_stride, int pl_ip_index, uint32_t relative_address, uint64_t& value);
void XPl_IOstart();
#endif //__XIOUTILS_H__
