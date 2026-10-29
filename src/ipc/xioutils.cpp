// 67d7842dbbe25473c3c32b93c0da8047785f30d78e8a024de1b57352245f9689
// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "xioutils.h"

static void (*Write32Ptr)(uint64_t, unsigned int) = nullptr;
static unsigned int (*Read32Ptr)(uint64_t) = nullptr;
static void (*WriteCmdPtr)(unsigned char, unsigned char, unsigned char, unsigned int, unsigned int, unsigned char*) = nullptr;
static void (*NpiWrite32Ptr)(uint64_t, unsigned int) = nullptr;
static unsigned int (*NpiRead32Ptr)(uint64_t) = nullptr;
static void (*WriteGMPtr)(uint64_t, const void*, uint64_t) = nullptr;
static void (*ReadGMPtr)(uint64_t,  void*, uint64_t) = nullptr;
static void (*StartPlio)() = nullptr;

void setWrite32Ptr(void (*func)(uint64_t, unsigned int))
{
  Write32Ptr = func;
}
void setRead32Ptr(unsigned int (*func)(uint64_t)) 
{
  Read32Ptr = func;
}
void setWriteCmdPtr(void (*func)(unsigned char, unsigned char, unsigned char, unsigned int, unsigned int, unsigned char*)) 
{
  WriteCmdPtr = func;
}
void setNpiWrite32Ptr(void (*func)(uint64_t, unsigned int)) 
{
  NpiWrite32Ptr = func;
}
void setNpiRead32Ptr(unsigned int (*func)(uint64_t)) 
{
  NpiRead32Ptr = func;
}
void setWriteGMPtr(void (*func)(uint64_t, const void*, uint64_t)) 
{
  WriteGMPtr = func;
}
void setReadGMPtr(void (*func)(uint64_t,  void*, uint64_t))
{
  ReadGMPtr = func;
}
void setPLIOStartPtr(void (*func)())
{
  StartPlio = func;
}

extern "C" {
void ess_Write32(uint64_t Addr, unsigned int Data) {
  if (Write32Ptr)
    Write32Ptr(Addr, Data);
}
unsigned int ess_Read32(uint64_t Addr) {
  return Read32Ptr ? Read32Ptr(Addr) : 0U;
}
void ess_WriteGM(uint64_t addr, const void* data, uint64_t size) {
  if (WriteGMPtr)
   WriteGMPtr(addr, data, size); 
}
void ess_ReadGM(uint64_t addr, void* data, uint64_t size) {
  if (ReadGMPtr)
    ReadGMPtr(addr, data, size);
}
void ess_NpiWrite32(uint64_t Addr, unsigned int Data) {
  if (NpiWrite32Ptr)
    NpiWrite32Ptr(Addr, Data);
}
unsigned int ess_NpiRead32(uint64_t Addr)
{
  return NpiRead32Ptr ? NpiRead32Ptr(Addr) : 0U;
}
void ess_WriteCmd(unsigned char Command, unsigned char ColId, unsigned char RowId, unsigned int CmdWd0, unsigned int CmdWd1, unsigned char *CmdStr)
{
  if (WriteCmdPtr)
    WriteCmdPtr(Command, ColId, RowId, CmdWd0, CmdWd1, CmdStr);
}

}



/* Low level read/write APIs for PL address space in AIESIM */
	 
void XPl_Write32(uint64_t ctrl_pl_offset, uint32_t ctrl_ip_stride, int pl_ip_index, uint32_t relative_address, uint32_t value)
{
  uint64_t absolute_address = ctrl_pl_offset + pl_ip_index * ctrl_ip_stride + relative_address;
  ess_Write32(absolute_address, value);
}

void XPl_Write64(uint64_t ctrl_pl_offset, uint32_t ctrl_ip_stride, int pl_ip_index, uint32_t relative_address, uint64_t value)
{
  uint64_t absolute_address = ctrl_pl_offset + pl_ip_index * ctrl_ip_stride + relative_address;
  const uint32_t lo = static_cast<uint32_t>(value & 0xFFFFFFFFULL);
  const uint32_t hi = static_cast<uint32_t>((value >> 32) & 0xFFFFFFFFULL);
  ess_Write32(absolute_address, lo);
  ess_Write32(absolute_address + 4, hi);
}

void XPl_Read32(uint64_t ctrl_pl_offset, uint32_t ctrl_ip_stride, int pl_ip_index, uint32_t relative_address, uint32_t& value)
{
  uint64_t absolute_address = ctrl_pl_offset + pl_ip_index * ctrl_ip_stride + relative_address;
  value = ess_Read32(absolute_address);
}

void XPl_Read64(uint64_t ctrl_pl_offset, uint32_t ctrl_ip_stride, int pl_ip_index, uint32_t relative_address, uint64_t& value)
{
  uint64_t absolute_address = ctrl_pl_offset + pl_ip_index * ctrl_ip_stride + relative_address;
  const uint64_t lo = static_cast<uint64_t>(ess_Read32(absolute_address));
  const uint64_t hi = static_cast<uint64_t>(ess_Read32(absolute_address + 4));
  value = (hi << 32) | lo;
}

/* Wrapper function for start plios */

void XPl_IOstart() 
{
  if (StartPlio)
    StartPlio();
}
