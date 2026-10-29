// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "adf/adf_api/AIERuntimeControl.h"
extern "C"
{
  #include <xaiengine.h>
}
#ifdef __AIE_RUNTIME_CONTROL_LOG__
#define __LOG__(msg) std::cout << msg << std::endl;
#else
#define __LOG__(msg)
#endif
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
extern "C" {
void ess_Write32(uint64 Addr, uint32 Data);
uint32 ess_Read32(uint64 Addr);
};
#endif
#define DEBUG_STR_MAX_LEN 512
typedef struct{
   int temp;
   unsigned int size_in_bytes;
 } op_base;
struct PatchShimOp {
  op_base op;
  uint32_t action;
  uint64_t regAddr;
  uint64_t extBufId;
  uint64_t argplus;
};
//std::map<int64_t, int64_t> external_buffers_map = {{0,0}, {1,1}, {2,2}, {3,3}};
void XAie_SubmitSerializedTransaction(uint8_t *Ptr)
{
    u64 BaseAddr = 0x20000000000;
    XAie_TxnHeader *Hdr = (XAie_TxnHeader *)Ptr;
    std::cout << "Header version: " << static_cast<unsigned>(Hdr->Major) << "." << static_cast<unsigned>(Hdr->Minor) << std::endl;
    std::cout << "Device Generation: " << static_cast<unsigned>(Hdr->DevGen) << std::endl;
    std::cout << "Cols, Rows, NumMemRows : (" << static_cast<unsigned>(Hdr->NumCols) << "," <<  static_cast<unsigned>(Hdr->NumRows) << "," << static_cast<unsigned>(Hdr->NumMemTileRows) << ") " << std::endl;
    std::cout << "TransactionSize: " << Hdr->TxnSize << std::endl;
    std::cout << "NumOps: " << Hdr->NumOps << std::endl;

    Ptr += sizeof(*Hdr);
    for(uint32_t i = 0; i < Hdr->NumOps; i++) {
        std::cout << "Executing op " << std::dec << i << " out of " << Hdr->NumOps << " total ops, " << i*100.0/Hdr->NumOps << "% complete" << std::endl;
        XAie_OpHdr *OpHdr = (XAie_OpHdr *)Ptr;
        if (OpHdr->Op == XAIE_IO_WRITE) {
            XAie_Write32Hdr *WHdr = (XAie_Write32Hdr *)Ptr;
            __LOG__("W: 0x"<< std::hex << (WHdr->RegOff + BaseAddr) << ", 0x" << std::hex<< WHdr->Value);
#if defined(__AIESIM__)
            ess_Write32(BaseAddr + WHdr->RegOff, WHdr->Value);
            
            uint32_t var = 0;
            int count = 5;
            while(var != WHdr->Value && count-- >0) {
                var = ess_Read32(BaseAddr + WHdr->RegOff);
                printf("var2: 0x%x WHdr->Value: 0x%x WHdr->RegOff = 0x%x\n", var, WHdr->Value,WHdr->RegOff);
                sleep(1);
            }
#endif
            Ptr += WHdr->Size;
            continue;
        }
        else if (OpHdr->Op == XAIE_IO_MASKWRITE) {
            XAie_MaskWrite32Hdr *MWHdr = (XAie_MaskWrite32Hdr *)Ptr;
            __LOG__("MW: 0x"<< std::hex << (MWHdr->RegOff + BaseAddr) << ", 0x" << std::hex<< MWHdr->Mask << ", 0x" << std::hex<< MWHdr->Value);
#if defined(__AIESIM__)
            uint32_t regVal = ess_Read32(BaseAddr + MWHdr->RegOff);
#else
            uint32_t regVal = 0;
#endif
            regVal &= ~(MWHdr->Mask);
            regVal |= MWHdr->Value;
#if defined(__AIESIM__)
            ess_Write32(BaseAddr + MWHdr->RegOff, regVal);
            uint32_t var = ess_Read32(BaseAddr + MWHdr->RegOff);
            printf("mask write var2: 0x%x MWHdr->Value: 0x%x MWHdr->RegOff = 0x%x\n", var, regVal,MWHdr->RegOff);
#endif
            Ptr += MWHdr->Size;
            continue;
        }
        else if (OpHdr->Op == XAIE_IO_MASKPOLL ) {//|| OpHdr->Op == XAIE_IO_MASKPOLL_BUSY) {
            std::cout << "XAIE_IO_MASKPOLL" <<std::endl;
            XAie_MaskPoll32Hdr *MPHdr = (XAie_MaskPoll32Hdr *)Ptr;
            __LOG__( "MP: 0x" << std::hex << (MPHdr->RegOff + BaseAddr) << ", 0x" << std::hex << MPHdr->Mask << ", 0x" << std::hex << MPHdr->Value);
            const char* pollType = OpHdr->Op == XAIE_IO_MASKPOLL ? "MP" : "MPB";
            printf("%s: 0x%lx, 0x%x, 0x%x, 0x%x\n", pollType,
                    MPHdr->RegOff + BaseAddr,
                    MPHdr->Mask, MPHdr->Value, MPHdr->Size);

            while(1) {
#if defined(__AIESIM__)
                uint32_t regVal = ess_Read32(BaseAddr + MPHdr->RegOff);
#else
                uint32_t regVal = 0;
          
#endif
                if ((regVal & MPHdr->Mask) == MPHdr->Value)
                    break;
                //usleep(100);
                //std::cout << "Polling regval = " << regVal << " MPHdr->Value= " << MPHdr->Value << " MPHdr->RegOff = "<< MPHdr->RegOff   << std::endl;
            }

            Ptr += MPHdr->Size;
            //sc_core::wait(256, SC_NS);
            continue;
        }
        else if (OpHdr->Op == XAIE_IO_BLOCKWRITE) {
            XAie_BlockWrite32Hdr *BWHdr = (XAie_BlockWrite32Hdr *)Ptr;
            uint32_t *Tmp = (uint32_t *)(Ptr + sizeof(*BWHdr));
            int Sz = (BWHdr->Size - sizeof(*BWHdr)) / 4;

            for(int i = 0; i < Sz; i++) {
                __LOG__("W: 0x" << std::hex<< (BWHdr->RegOff + BaseAddr+(i*4) ) << ", 0x" << std::hex<< Tmp[i] << ", 0x" << std::hex<< BWHdr->Size);
#if defined(__AIESIM__)
                ess_Write32(BWHdr->RegOff+BaseAddr+i*4, Tmp[i]);
                uint32_t var = 0;
                int count = 5;
                while(var != Tmp[i] && count-- >0) {
                     var = ess_Read32(BWHdr->RegOff+BaseAddr+i*4);
                     printf("block Sz = %d write var2: 0x%x MWHdr->Value: 0x%x BWHdr->RegOff = 0x%x\n", Sz, var, Tmp[i],BWHdr->RegOff+BaseAddr+i*4);
                     sleep(1);
                }
#endif
            }

            Ptr += BWHdr->Size;
            continue;
        } 
        else if (OpHdr->Op == XAIE_IO_CUSTOM_OP_BEGIN+1) {
                XAie_CustomOpHdr *hdr = (XAie_CustomOpHdr *)Ptr;
                PatchShimOp *op = (PatchShimOp *)(Ptr + sizeof(*hdr)); 

                  u64 tensorAddr = adf::getExternalBufferAddress(op->extBufId) + op->argplus;                
              
                std::cout << "CustomOp PatchBD extBufId:" << op->extBufId << ", regAddr: 0x" << std::hex << (op->regAddr+BaseAddr) << " addrVal: 0x" << std::hex << tensorAddr << std::endl;
                __LOG__("CustomOp PatchBD extBufId:" << op->extBufId << ", regAddr: 0x" << std::hex << (op->regAddr+BaseAddr) << " addrVal: 0x" << std::hex << tensorAddr);
                if (tensorAddr != 0) {
                  // patch if arg provided
                  // 1. write lower 32 bits
                  ess_Write32(op->regAddr+BaseAddr, 
                    tensorAddr & 0xFFFFFFFFC); // unused 2-LSB
                  
                  // 2. read upper 32 bits 
                  u32 regval = ess_Read32(op->regAddr+BaseAddr+4u);
                  // 3. mask-write upper 32 bits
                  u32 uppermask = 0x0000FFFF; 
                  ess_Write32(op->regAddr+BaseAddr+4u, 
                      (regval & (~uppermask)) | (tensorAddr >> 32));
                }

                Ptr += hdr->Size;
                continue;
        }
        /*
        else if (OpHdr->Op == XAIE_IO_CUSTOM_OP_RECORD_TIMER) {
            // Previously this was XAIE_IO_CUSTOM_OP_BEGIN + 3
            XAie_CustomOpHdr *hdr = (XAie_CustomOpHdr *)Ptr;
            u32 timer_id = *(uint32_t *)((Ptr + sizeof(XAie_CustomOpHdr)));
            std::stringstream logStream;
            adf::recordTimer(timer_id, logStream);
            __LOG__("RecordTimer: " << logStream.str())
            Ptr += hdr->Size;
            continue;
        }
        */
        else if ((OpHdr->Op > XAIE_IO_CUSTOM_OP_BEGIN) && (OpHdr->Op < XAIE_IO_CUSTOM_OP_MAX))
        {
                XAie_CustomOpHdr *hdr = (XAie_CustomOpHdr *)Ptr;
                Ptr += hdr->Size;
                continue;
        }
        /*
        else if (OpHdr->Op == XAIE_IO_PREEMPT) {
            XAie_PreemptHdr *PMHdr = (XAie_PreemptHdr *)Ptr;
            Ptr += sizeof(XAie_PreemptHdr);
            __LOG__("PmPt: " << PMHdr->Op << ", " << PMHdr->Preempt_level << ",  " << PMHdr->Reserved);
            continue;
        }
        */
        /*
        else if (OpHdr->Op == XAIE_IO_LOADPDI) {
            //XAie_LoadPdiHdr *loadPdiHdr = (XAie_LoadPdiHdr *)Ptr;
            //__LOG__("LoadPdi: " << loadPdiHdr->PdiId)
            //Ptr += sizeof(XAie_LoadPdiHdr);
            std::cout << "XAIE_IO_LOADPDI" << std::endl;
            continue;
        }*/
        else {
            std::cout << "ERROR: Unrecognized operation OpHdr->Op is " << OpHdr->Op << std::endl;
        }

    }
}
uint8_t* ReadSerializedTransactionFromBin(std::string TxnBinFileName)
{
    std::cout << "Reading Transaction Binary from file: " << TxnBinFileName << std::endl;
    u64 BaseAddr = 0x20000000000;
    std::ifstream txnBin(TxnBinFileName, std::ios::binary);

    XAie_TxnHeader *Hdr = new XAie_TxnHeader();

    txnBin.read((char*)Hdr, sizeof(XAie_TxnHeader));

    std::cout << "Header version: " << static_cast<unsigned>(Hdr->Major) << "." << static_cast<unsigned>(Hdr->Minor) << std::endl;
    std::cout << "Device Generation: " << static_cast<unsigned>(Hdr->DevGen) << std::endl;
    std::cout << "Cols, Rows, NumMemRows : (" << static_cast<unsigned>(Hdr->NumCols) << "," <<  static_cast<unsigned>(Hdr->NumRows) << "," << static_cast<unsigned>(Hdr->NumMemTileRows) << ") " << std::endl;
    std::cout << "TransactionSize: " << Hdr->TxnSize << std::endl;
    std::cout << "NumOps: " << Hdr->NumOps << std::endl;

    uint8_t *Ptr = new uint8_t[Hdr->TxnSize];
    std::memcpy(Ptr, Hdr, sizeof(XAie_TxnHeader));

    uint8_t *txnPtr = Ptr + sizeof(*Hdr);
    txnBin.read((char*)txnPtr, Hdr->TxnSize - sizeof(XAie_TxnHeader));

    return Ptr;
}
void executeRuntimeControl_TXN()
{
   std::cout << "executeRuntimeControl" << std::endl;

#ifdef __AIESIM__ 
    adf::syncPSToGM();
#endif
#if defined(__AIESIM__)
    std::cout << "ReadSerializedTransactionFromBin" << std::endl;
    //uint8_t *Ptr =  ReadSerializedTransactionFromBin("Work/ps/ml_txn/ml_txn.bin");
    uint8_t *Ptr =  ReadSerializedTransactionFromBin("./ml_txn/ml_txn.bin.aig");
    XAie_SubmitSerializedTransaction(Ptr);
#else
    //executeRuntimeControlInternal();
#endif
#ifdef __AIESIM__ 
    adf::syncPSFromGM();
#endif


}

