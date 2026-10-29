// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef __AIEPATCH_H__
#define __AIEPATCH_H__
typedef struct{
  int temp;
  unsigned int size_in_bytes;
} op_base;
typedef struct{
  uint32_t id;
} record_timer_id_op_t;
struct PatchShimOp {
  op_base op;
  uint32_t action;
  uint64_t regAddr;
  uint64_t extBufId;
  uint64_t extBufLen;
  uint64_t argplus;
};
extern "C"
{
 #include <xaiengine.h>
}
uint64_t ComputePatchRegAddr(uint8_t shimColumn, uint8_t bdId) {
  // get tile info
  XAie_LocType tileLoc = XAie_TileLoc(shimColumn, 0); //XAie_TileLoc(Col, Row)
  XAie_DmaDesc dmaInst;
  int driverStatus = XAie_DmaDescInit(&DevInst, &dmaInst, tileLoc);
  if (driverStatus != AieRC::XAIE_OK)
    std::cout << "########## ComputePatchRegAddr: AIE Driver Error" << std::endl;

  u64 regAddr = dmaInst.DmaMod->BaseAddr 
    + bdId * dmaInst.DmaMod->IdxOffset 
    + XAie_GetTileAddr(&DevInst, tileLoc.Row, tileLoc.Col) 
    + dmaInst.DmaMod->BdProp->Buffer->ShimDmaBuff.AddrLow.Idx * 4U;


  return regAddr;
}

void AddDDRCustomOp(uint32_t action, uint64_t bufId, uint64_t offsetInBytes, uint8_t shimCol, uint8_t bdId, std::string bufName,int32_t bufLen, int32_t patch_op_code)
{
    PatchShimOp op;
    op.action = action;
    op.regAddr = ComputePatchRegAddr(shimCol, bdId);
    op.extBufId = bufId;
    op.extBufLen = bufLen;
    op.argplus = offsetInBytes;

    XAie_AddCustomTxnOp(&DevInst, patch_op_code, (void*)&op, sizeof(op));
}
#endif

