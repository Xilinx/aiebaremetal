/**
* Copyright (C) 2025 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#ifndef __TXN_H__
#define __TXN_H__
#define DEBUG_STR_MAX_LEN 512
typedef struct{
   int temp;
   unsigned int size_in_bytes;
 } op_base;
 typedef struct{
   uint32_t id;
 } record_timer_id_op_t;

typedef struct {
    op_base b;
    char msg[DEBUG_STR_MAX_LEN];
} print_op_t;

typedef struct {
    uint32_t word;
    uint32_t config;
} tct_op_t;
struct PatchShimOp {
  op_base op;
  uint32_t action;
  uint64_t regAddr;
  uint64_t extBufId;
  uint64_t argplus;
};
typedef struct {
	    uint32_t id;
} record_timer_op_t;

void PrintSerializedTransaction(uint8_t *Ptr)
{
    std::cout << "Dump Transaction Binary " << std::endl;
    u64 BaseAddr = 0x20000000000;
    XAie_TxnHeader *Hdr = (XAie_TxnHeader *)Ptr;
    std::cout << "Header version: " << static_cast<unsigned>(Hdr->Major) << "." << static_cast<unsigned>(Hdr->Minor) << std::endl;
    std::cout << "Device Generation: " << static_cast<unsigned>(Hdr->DevGen) << std::endl;
    std::cout << "Cols, Rows, NumMemRows : (" << static_cast<unsigned>(Hdr->NumCols) << "," <<  static_cast<unsigned>(Hdr->NumRows) << "," << static_cast<unsigned>(Hdr->NumMemTileRows) << ") " << std::endl;
    std::cout << "TransactionSize: " << Hdr->TxnSize << std::endl;
    std::cout << "NumOps: " << Hdr->NumOps << std::endl;

    Ptr += sizeof(*Hdr);
    for(uint32_t i = 0; i < Hdr->NumOps; i++) {
        XAie_OpHdr *OpHdr = (XAie_OpHdr *)Ptr;
        std::cout << "Op: " << unsigned(OpHdr->Op) << "; OpCount: " << i << std::endl;
        if (OpHdr->Op == XAIE_IO_WRITE) {
            XAie_Write32Hdr *WHdr = (XAie_Write32Hdr *)Ptr;
            printf("W: 0x%llx, 0x%x, 0x%x\n",
                    WHdr->RegOff + BaseAddr,
                    WHdr->Value, WHdr->Size);

            Ptr += WHdr->Size;
            continue;
        }
        if (OpHdr->Op == XAIE_IO_MASKWRITE) {
            XAie_MaskWrite32Hdr *MWHdr = (XAie_MaskWrite32Hdr *)Ptr;
            printf("MW: 0x%llx, 0x%x, 0x%x, 0x%x\n",
                    MWHdr->RegOff + BaseAddr,
                    MWHdr->Mask, MWHdr->Value, MWHdr->Size);

            Ptr += MWHdr->Size;
            continue;
        }
        if (OpHdr->Op == XAIE_IO_MASKPOLL) {
            XAie_MaskPoll32Hdr *MPHdr = (XAie_MaskPoll32Hdr *)Ptr;
            printf("MP: 0x%llx, 0x%x, 0x%x, 0x%x\n",
                    MPHdr->RegOff + BaseAddr,
                    MPHdr->Mask, MPHdr->Value, MPHdr->Size);


            Ptr += MPHdr->Size;
            continue;
        }
        if (OpHdr->Op == XAIE_IO_BLOCKWRITE) {
            XAie_BlockWrite32Hdr *BWHdr = (XAie_BlockWrite32Hdr *)Ptr;
            uint32_t *Tmp = (uint32_t *)(Ptr + sizeof(*BWHdr));
            int Sz = (BWHdr->Size - sizeof(*BWHdr)) / 4;
            printf("## BLOCK WRITE\n");

            for(int i = 0; i < Sz; i++) {
                printf("W: 0x%llx, 0x%x 0x%x\n",
                        BWHdr->RegOff +
                        BaseAddr + i * 4,
                        Tmp[i], BWHdr->Size);

            }

            Ptr += BWHdr->Size;
            continue;
        }
        if (OpHdr->Op == XAIE_IO_CUSTOM_OP_BEGIN+1) {
            XAie_CustomOpHdr *hdr = (XAie_CustomOpHdr *)Ptr;
            PatchShimOp *op = (PatchShimOp *)(Ptr + sizeof(*hdr));
            printf("CustomOp PatchBD extBufId %llu\n", op->extBufId);
            //u64 *argv = (u64*)args;
            printf("CustomOp PatchBD regaddr %llx\n", op->regAddr+BaseAddr);
            //u64 tensorAddr = adf::getExternalBufferAddress(0)/4 + op->argplus;
            u64 tensorAddr = op->argplus;
            printf("CustomOp PatchBD addr val %llx\n", tensorAddr);

                Ptr += hdr->Size;
                continue;
            }
        if (OpHdr->Op == XAIE_IO_CUSTOM_OP_TCT) {
            XAie_CustomOpHdr *co_header = (XAie_CustomOpHdr *)Ptr;
            tct_op_t *iptr = (tct_op_t *)(Ptr + sizeof(*co_header));
            printf("CustomOp TCT: %d\n", iptr->word);
            u32 word = iptr->word;
            uint8_t start_col_idx = 0; //assume start_col_idx is 0 for simulation purpose
            u8 Col = ((word & 0x00FF0000) >> 16) + start_col_idx;
            u8 Row = ((word & 0x0000FF00) >> 8);
            u8 dir = ((word)& 0x000000FF);
            XAie_DmaDirection Dir = (dir == 0) ? DMA_S2MM : DMA_MM2S;
            u32 config = iptr->config;
            u8 ChNum = ((config & 0xFF000000) >> 24);
            u8 ColNum = ((config & 0x00FF0000) >> 16);
            u8 RowNum = ((config & 0x0000FF00) >> 8);
            printf("Read TCT Custom Op: {col, row, chl, dir} = {%d+%d, %d+%d, %d, %d}\n", Col,
                ColNum, Row, RowNum, ChNum, Dir);
            Ptr += co_header->Size;
            continue;
        }
        if ((OpHdr->Op > XAIE_IO_CUSTOM_OP_BEGIN) && (OpHdr->Op < XAIE_IO_CUSTOM_OP_MAX))
        {
            XAie_CustomOpHdr *hdr = (XAie_CustomOpHdr *)Ptr;
            Ptr += hdr->Size;
            continue;
        }
        else {
            std::cout << "ERROR: Unrecognized operation" << std::endl;
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
#endif

