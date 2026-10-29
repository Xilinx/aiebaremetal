// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
extern "C"
{
  #include <xaiengine.h>
}
#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <vector>

//#define __AIE_RUNTIME_CONTROL_LOG__
#ifdef __AIE_RUNTIME_CONTROL_LOG__
#define __LOG__(msg) std::cout << msg << std::endl;
#else
#define __LOG__(msg)
#endif

//#define PROFILE_RESNET18
//#define DETAILED_BLOCKWRITE_PROFILE
#ifdef PROFILE_RESNET18
#if ((defined(__AIESIM__) || defined(__ADF_FRONTEND__)))
#undef PROFILE_RESNET18
#endif
#endif

// Silences the per-transaction completion line below. That line is emitted once
// per inference from inside the timed control-transaction phase, and at 115200
// baud it costs ~2.4 ms -- the same order as a whole ResNet18 round -- so timed
// loops must turn it off or they measure the UART instead of the AIE.
static bool txn_silent = false;

#define MILLISECONDS 1000.0
#define MICROSECONDS 1000000.0
#define NANOSECONDS  1000000000.0
#define UNITS NANOSECONDS

#include "xil_printf.h"
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_types.h"
#include "xstatus.h"
#include "xtime_l.h"


#include "./mladfpatch/baremetal/ml_txn/mltxn.h"

#if defined(__ZEPHYR__)
// On Zephyr use the aperture base mapped at runtime by the library
// (abr_aie_base_va, set during gr.init()), not the physical literal.
extern "C" uint64_t abr_aie_base_va(void);
#define AIE_APERTURE_BASE (abr_aie_base_va())
#else
#define AIE_APERTURE_BASE (0x20000000000ULL)
#endif

typedef struct{
   int temp;
   unsigned int size_in_bytes;
 } op_base;

struct PatchShimOp {
  op_base op;
  uint32_t action;
  uint64_t regAddr;
  uint64_t extBufId;
  uint64_t extBufLen;
  uint64_t argplus;
};

std::unordered_map<int, u64> external_map;
void AIE_Write32(uint64_t Addr, uint32_t Data) {
	Xil_Out32(Addr, Data);
}
uint32_t AIE_Read32(uint64_t Addr) {
	return Xil_In32(Addr);
}

void aie_tile_check() {
	for (int c = 0;  c< 7; c++) {
		for (int r = 3; r < 7; r++) {
  		    u64 status_c_r_offset = AIE_APERTURE_BASE + (c << 25) + (r << 20) + 0x00032004; 
			int value = AIE_Read32(status_c_r_offset);
			printf("-row %d col %d core status is 0x%x\n", r, c, value);
		}
	}
}

//std::map<int64_t, int64_t> external_buffers_map = {{0,0}, {1,1}, {2,2}, {3,3}};
void XAie_SubmitSerializedTransaction(uint8_t *Ptr)
{
    #ifdef PROFILE_RESNET18
        struct PInfo {
            int64_t count = 0;
            int64_t min_time = INT64_MAX;
            int64_t max_time = 0;
            int64_t avg_time = 0;
            int64_t total_time = 0;
        };
        
        struct BlockWriteInfo {
            int64_t count = 0;
            int64_t total_mem = 0;
            double avg_time = 0;
            int64_t total_time = 0;
            double avg_speed = 0;
            std::vector<int> regOffs;
        };
        
        struct {
            int64_t total_ops = 0;
            PInfo write = {};
            PInfo mask_write = {};
            PInfo mask_poll = {};
            PInfo block_write = {};
            struct {
                std::unordered_map<int64_t, int> RegOffMap;
                std::unordered_map<int64_t, BlockWriteInfo> size_info;
            } block_write_extra;
            PInfo custom_ddr_patch = {};
            PInfo custom_ddr_flush = {};
            PInfo custom_ddr_write = {};
            PInfo custom_op = {};
            
        } profile_info;
        int64_t total_mem = 0;
        profile_info.total_ops = ((XAie_TxnHeader *)Ptr)->NumOps;
        XTime totalStart, totalEnd;
        XTime_GetTime(&totalStart);
    #endif
    
    u64 BaseAddr = AIE_APERTURE_BASE;
    XAie_TxnHeader *Hdr = (XAie_TxnHeader *)Ptr;
    
    // std::cout << "4Header version: " << static_cast<unsigned>(Hdr->Major) << "." << static_cast<unsigned>(Hdr->Minor) << std::endl;
    // std::cout << "Device Generation: " << static_cast<unsigned>(Hdr->DevGen) << std::endl;
    // std::cout << "Cols, Rows, NumMemRows : (" << static_cast<unsigned>(Hdr->NumCols) << "," <<  static_cast<unsigned>(Hdr->NumRows) << "," << static_cast<unsigned>(Hdr->NumMemTileRows) << ") " << std::endl;
    // std::cout << "TransactionSize: " << Hdr->TxnSize << std::endl;
    // std::cout << "NumOps: " << Hdr->NumOps << std::endl;
    
    Ptr += sizeof(*Hdr);
    for(uint32_t i = 0; i < Hdr->NumOps; i++) {

	//if (0&&(i%30 ==0 || i*100.0/Hdr->NumOps >90 )) {
		//aie_tile_check();
        //	std::cout << "Executing op " << std::dec << i << " out of " << Hdr->NumOps << " total ops, " << i*100.0/Hdr->NumOps << "% complete" << std::endl;
	//}

        XAie_OpHdr *OpHdr = (XAie_OpHdr *)Ptr;
        if (OpHdr->Op == XAIE_IO_WRITE) {
            #ifdef PROFILE_RESNET18
                profile_info.write.count++;
                XTime tStart, tEnd;
                XTime_GetTime(&tStart);
            #endif

            XAie_Write32Hdr *WHdr = (XAie_Write32Hdr *)Ptr;
            //printf("XAIE_IO_WRITE Regoff is 0x%x value is 0x%x\n", WHdr->RegOff + BaseAddr, WHdr->Value);
            __LOG__("W: 0x"<< std::hex << (WHdr->RegOff + BaseAddr) << ", 0x" << std::hex<< WHdr->Value);
//#if defined(__AIESIM__)
            AIE_Write32(BaseAddr + WHdr->RegOff, WHdr->Value);
//#endif
            Ptr += WHdr->Size;

            #ifdef PROFILE_RESNET18
                XTime_GetTime(&tEnd);
                int64_t time = 1.0 * (tEnd - tStart) / COUNTS_PER_SECOND * UNITS;
                profile_info.write.total_time += time;
                profile_info.write.min_time = std::min(profile_info.write.min_time, time);
                profile_info.write.max_time = std::max(profile_info.write.max_time, time);
            #endif
            continue;
        }
        else if (OpHdr->Op == XAIE_IO_MASKWRITE) {
            #ifdef PROFILE_RESNET18
                profile_info.mask_write.count++;
                XTime tStart, tEnd;
                XTime_GetTime(&tStart);
            #endif

            XAie_MaskWrite32Hdr *MWHdr = (XAie_MaskWrite32Hdr *)Ptr;
            __LOG__("MW: 0x"<< std::hex << (MWHdr->RegOff + BaseAddr) << ", 0x" << std::hex<< MWHdr->Mask << ", 0x" << std::hex<< MWHdr->Value);
//#if defined(__AIESIM__)
            uint32_t regVal = AIE_Read32(BaseAddr + MWHdr->RegOff);
//#else
//            uint32_t regVal = 0;
//#endif
            regVal &= ~(MWHdr->Mask);
            regVal |= MWHdr->Value;
//#if defined(__AIESIM__)
            AIE_Write32(BaseAddr + MWHdr->RegOff, regVal);
            uint32_t var = AIE_Read32(BaseAddr + MWHdr->RegOff);
            //printf("mask write var2: 0x%x MWHdr->Value: 0x%x MWHdr->RegOff = 0x%x\n", var, regVal,MWHdr->RegOff);
//#endif
            Ptr += MWHdr->Size;

            #ifdef PROFILE_RESNET18
                XTime_GetTime(&tEnd);
                int64_t time = 1.0 * (tEnd - tStart) / COUNTS_PER_SECOND * UNITS;
                profile_info.mask_write.total_time += time;
                profile_info.mask_write.min_time = std::min(profile_info.mask_write.min_time, time);
                profile_info.mask_write.max_time = std::max(profile_info.mask_write.max_time, time);
            #endif
            continue;
        }
        else if (OpHdr->Op == XAIE_IO_MASKPOLL ) {//|| OpHdr->Op == XAIE_IO_MASKPOLL_BUSY) {
            #ifdef PROFILE_RESNET18
                profile_info.mask_poll.count++;
                XTime tStart, tEnd;
                XTime_GetTime(&tStart);
            #endif

            //std::cout << "XAIE_IO_MASKPOLL2" <<std::endl;
            XAie_MaskPoll32Hdr *MPHdr = (XAie_MaskPoll32Hdr *)Ptr;
            __LOG__( "MP: 0x" << std::hex << (MPHdr->RegOff + BaseAddr) << ", 0x" << std::hex << MPHdr->Mask << ", 0x" << std::hex << MPHdr->Value);
            const char* pollType = OpHdr->Op == XAIE_IO_MASKPOLL ? "MP" : "MPB";
            //printf("%s: 0x%lx, 0x%x, 0x%x, 0x%x\n", pollType,
            //        MPHdr->RegOff + BaseAddr,
            //        MPHdr->Mask, MPHdr->Value, MPHdr->Size);

            while(1) {
//#if defined(__AIESIM__)
                uint32_t regVal = AIE_Read32(BaseAddr + MPHdr->RegOff);
//#else
//                uint32_t regVal = 0;
          
///#endif
                if ((regVal & MPHdr->Mask) == MPHdr->Value)
                    break;
                //usleep(100);
                //std::cout << "Polling regval = " << regVal << " MPHdr->Value= " << MPHdr->Value << " MPHdr->RegOff = "<< MPHdr->RegOff   << std::endl;
            }

            Ptr += MPHdr->Size;
            //sc_core::wait(256, SC_NS);

            #ifdef PROFILE_RESNET18
                XTime_GetTime(&tEnd);
                int64_t time = 1.0 * (tEnd - tStart) / COUNTS_PER_SECOND * UNITS;
                profile_info.mask_poll.total_time += time;
                profile_info.mask_poll.min_time = std::min(profile_info.mask_poll.min_time, time);
                profile_info.mask_poll.max_time = std::max(profile_info.mask_poll.max_time, time);
            #endif
            continue;
        }
        else if (OpHdr->Op == XAIE_IO_BLOCKWRITE) {
            #ifdef PROFILE_RESNET18
                profile_info.block_write.count++;
                XTime tStart, tEnd;
                XTime_GetTime(&tStart);
            #endif

            XAie_BlockWrite32Hdr *BWHdr = (XAie_BlockWrite32Hdr *)Ptr;
            uint32_t *Tmp = (uint32_t *)(Ptr + sizeof(*BWHdr));
            int Sz = (BWHdr->Size - sizeof(*BWHdr)) / 4;
            //printf("XAIE_IO_BLOCKWRITE address is 0x%x Sz %d\n", BWHdr->RegOff+BaseAddr, Sz);
            for(int i = 0; i < Sz; i++) {
                //__LOG__("W: 0x" << std::hex<< (BWHdr->RegOff + BaseAddr+(i*4) ) << ", 0x" << std::hex<< Tmp[i] << ", 0x" << std::hex<< BWHdr->Size);
                AIE_Write32(BWHdr->RegOff+BaseAddr+i*4, Tmp[i]);
#if defined(__AIESIM__)        
                //uint32_t var = 0;
                //int count = 5;
                //while(var != Tmp[i] && count-- >0) {
                //     var = AIE_Read32(BWHdr->RegOff+BaseAddr+i*4);
                //     //printf("block Sz = %d write var2: 0x%x MWHdr->Value: 0x%x BWHdr->RegOff = 0x%x\n", Sz, var, Tmp[i],BWHdr->RegOff+BaseAddr+i*4);
                //     //sleep(1);
                //}
#endif
            }

            Ptr += BWHdr->Size;

            #ifdef PROFILE_RESNET18
                XTime_GetTime(&tEnd);
                total_mem += BWHdr->Size - sizeof(BWHdr);
                int64_t time = 1.0 * (tEnd - tStart) / COUNTS_PER_SECOND * UNITS;
                profile_info.block_write.total_time += time;
                profile_info.block_write.min_time = std::min(profile_info.block_write.min_time, time);
                profile_info.block_write.max_time = std::max(profile_info.block_write.max_time, time);
                
                #ifdef DETAILED_BLOCKWRITE_PROFILE
                    profile_info.block_write_extra.RegOffMap[BWHdr->RegOff] += 1;
                    profile_info.block_write_extra.size_info[BWHdr->Size - sizeof(BWHdr)].count++;
                    profile_info.block_write_extra.size_info[BWHdr->Size - sizeof(BWHdr)].total_time += time;
                    profile_info.block_write_extra.size_info[BWHdr->Size - sizeof(BWHdr)].total_mem += BWHdr->Size - sizeof(BWHdr);
                    profile_info.block_write_extra.size_info[BWHdr->Size - sizeof(BWHdr)].regOffs.push_back(BWHdr->RegOff);
                #endif
            #endif
            continue;
        } 
        else if (OpHdr->Op == XAIE_IO_CUSTOM_OP_DDR_PATCH) {
            #ifdef PROFILE_RESNET18
                profile_info.custom_ddr_patch.count++;
                XTime tStart, tEnd;
                XTime_GetTime(&tStart);
            #endif

            XAie_CustomOpHdr *hdr = (XAie_CustomOpHdr *)Ptr;
            PatchShimOp *op = (PatchShimOp *)(Ptr + sizeof(*hdr)); 

            u64 tensorAddr = external_map[op->extBufId] + op->argplus;
            u64 tensorLen = op->extBufLen;
            Xil_DCacheFlushRange((intptr_t)tensorAddr,(intptr_t)tensorLen*4);

            //(adf::getExternalBufferAddress(op->extBufId)) + op->argplus;                
            
            //std::cout << "7CustomOp PatchBD extBufId:" << op->extBufId << ", regAddr: 0x" << std::hex << (op->regAddr+BaseAddr) << " addrVal: 0x" << std::hex << tensorAddr << std::endl;
            __LOG__("CustomOp PatchBD extBufId:" << op->extBufId << ", regAddr: 0x" << std::hex << (op->regAddr+BaseAddr) << " addrVal: 0x" << std::hex << tensorAddr);
            if (tensorAddr != 0) {
                // patch if arg provided
                // 1. write lower 32 bitssss
//#if defined(__AIESIM__)
                AIE_Write32(op->regAddr+BaseAddr, 
                tensorAddr & 0xFFFFFFFFC); // unused 2-LSB
                // 2. read upper 32 bits 
                u32 regval = AIE_Read32(op->regAddr+BaseAddr+4u);
                // 3. mask-write upper 32 bits
                u32 uppermask = 0x0000FFFF; 
                AIE_Write32(op->regAddr+BaseAddr+4u, 
                    (regval & (~uppermask)) | (tensorAddr >> 32));
///#endif
            }
            Ptr += hdr->Size;

            #ifdef PROFILE_RESNET18
                XTime_GetTime(&tEnd);
                int64_t time = 1.0 * (tEnd - tStart) / COUNTS_PER_SECOND * UNITS;
                profile_info.custom_ddr_patch.total_time += time;
                profile_info.custom_ddr_patch.min_time = std::min(profile_info.custom_ddr_patch.min_time, time);
                profile_info.custom_ddr_patch.max_time = std::max(profile_info.custom_ddr_patch.max_time, time);
            #endif
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
            #ifdef PROFILE_RESNET18
                profile_info.custom_op.count++;
                XTime tStart, tEnd;
                XTime_GetTime(&tStart);
            #endif

            XAie_CustomOpHdr *hdr = (XAie_CustomOpHdr *)Ptr;
            Ptr += hdr->Size;

            //std::cout << "warning: Unrecognized operation OpHdr->Op is " << OpHdr->Op << std::endl;

            #ifdef PROFILE_RESNET18
                XTime_GetTime(&tEnd);
                int64_t time = 1.0 * (tEnd - tStart) / COUNTS_PER_SECOND * UNITS;
                profile_info.custom_op.total_time += time;
                profile_info.custom_op.min_time = std::min(profile_info.custom_op.min_time, time);
                profile_info.custom_op.max_time = std::max(profile_info.custom_op.max_time, time);
            #endif
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
        }
        */
        else {
            std::cout << "ERROR: Unrecognized operation OpHdr->Op is " << OpHdr->Op << std::endl;
        }
    }

    #ifdef PROFILE_RESNET18
        XTime_GetTime(&totalEnd);
        int64_t total_time = 1.0 * (totalEnd - totalStart) / COUNTS_PER_SECOND * UNITS;
        printf("Total Time %lld \n", total_time);
        printf("Total Memory: %d\n", total_mem);
        
        #ifdef DETAILED_BLOCKWRITE_PROFILE
            printf("Block Write Extra Info:\n");
            printf("%-10s %-10s %-10s %-10s %-10s %-10s\n", "Size", "Count", "Total Mem", "Avg Time", "Total Time", "Avg Speed (MB/s)");
            for(auto &[size, info] : profile_info.block_write_extra.size_info) {
                info.avg_time = info.total_time / info.count;
                info.avg_speed = (info.total_mem / 1000000.0) / (info.total_time / UNITS);
                printf("%-10lld %-10lld %-10lld %-10.2f %-10lld %-10.2f\n", size, info.count, info.total_mem, info.avg_time, info.total_time, info.avg_speed);
            }
            
            printf("\n\nBlock Write Register Address Info:\n\n");
            printf("Size, Addr, Is Data Mem, Total Writes, Row Col 1, Count 1, Row Col 2, Count 2, Row Col 3, Count 3, Row Col 4, Count 4\n");
            std::unordered_map<std::string, std::unordered_map<std::string, int>> m;
            for(auto &[size, info] : profile_info.block_write_extra.size_info) {
                m.clear();
                for(auto &regOff : info.regOffs) {
                    int row = regOff>>20 & ((1<<5)-1);
                    int col = regOff>>25 & ((1<<5)-1);
                    int64_t addr = regOff & ((1<<20)-1);
                    bool in_range = 0 < addr && addr < 64000;
                    
                    std::string key = std::to_string(addr) + '_' + std::to_string(in_range);
                    std::string subkey = std::to_string(row) + '_' + std::to_string(col);
                    m[key][subkey]++;
                }

                for (auto &[key, v] : m) {
                    std::string k = key;
                    size_t pos = k.find('_');
                    std::string addr = k.substr(0, pos);
                    std::string isDataMem = std::stoi(k.substr(pos + 1)) ? "True" : "False";
                    int total = 0;
                    std::string v_str;
    
                    for (auto &[k1, v1] : v) {
                        total += v1;
                        v_str += k1 + ", " + std::to_string(v1) + ", ";
                    }
                    if (!v_str.empty()) {
                        v_str.pop_back();
                        v_str.pop_back();
                    }
    
                    printf("%lld, 0x%llx, %s, %d, %s\n", size, std::stoll(addr), isDataMem.c_str(), total, v_str.c_str());
            }
            }

        #endif
        
        profile_info.write.avg_time = profile_info.write.total_time / profile_info.write.count;
        profile_info.mask_write.avg_time = profile_info.mask_write.total_time / profile_info.mask_write.count;
        profile_info.mask_poll.avg_time = profile_info.mask_poll.total_time / profile_info.mask_poll.count;
        profile_info.block_write.avg_time = profile_info.block_write.total_time / profile_info.block_write.count;
        profile_info.custom_ddr_patch.avg_time = profile_info.custom_ddr_patch.total_time / profile_info.custom_ddr_patch.count;
        profile_info.custom_ddr_flush.avg_time = profile_info.custom_ddr_flush.total_time / profile_info.custom_ddr_flush.count;
        profile_info.custom_ddr_write.avg_time = profile_info.custom_ddr_write.total_time / profile_info.custom_ddr_write.count;
        profile_info.custom_op.avg_time = profile_info.custom_op.total_time / profile_info.custom_op.count;
        
        profile_info.write.min_time = (profile_info.write.min_time == INT64_MAX) ? 0 : profile_info.write.min_time;
        profile_info.mask_write.min_time = (profile_info.mask_write.min_time == INT64_MAX) ? 0 : profile_info.mask_write.min_time;
        profile_info.mask_poll.min_time = (profile_info.mask_poll.min_time == INT64_MAX) ? 0 : profile_info.mask_poll.min_time;
        profile_info.block_write.min_time = (profile_info.block_write.min_time == INT64_MAX) ? 0 : profile_info.block_write.min_time;
        profile_info.custom_ddr_patch.min_time = (profile_info.custom_ddr_patch.min_time == INT64_MAX) ? 0 : profile_info.custom_ddr_patch.min_time;
        profile_info.custom_ddr_flush.min_time = (profile_info.custom_ddr_flush.min_time == INT64_MAX) ? 0 : profile_info.custom_ddr_flush.min_time;
        profile_info.custom_ddr_write.min_time = (profile_info.custom_ddr_write.min_time == INT64_MAX) ? 0 : profile_info.custom_ddr_write.min_time;
        profile_info.custom_op.min_time = (profile_info.custom_op.min_time == INT64_MAX) ? 0 : profile_info.custom_op.min_time;

        printf("%-15s %-10s %-10s %-10s %-10s %-10s\n", "", "Count", "Min", "Max", "Avg", "Total");
        printf("%-15s %-10lld %-10lld %-10lld %-10lld %-10lld\n", "Write", profile_info.write.count, profile_info.write.min_time, profile_info.write.max_time, profile_info.write.avg_time, profile_info.write.total_time);
        printf("%-15s %-10lld %-10lld %-10lld %-10lld %-10lld\n", "Mask Write", profile_info.mask_write.count, profile_info.mask_write.min_time, profile_info.mask_write.max_time, profile_info.mask_write.avg_time, profile_info.mask_write.total_time);
        printf("%-15s %-10lld %-10lld %-10lld %-10lld %-10lld\n", "Mask Poll", profile_info.mask_poll.count, profile_info.mask_poll.min_time, profile_info.mask_poll.max_time, profile_info.mask_poll.avg_time, profile_info.mask_poll.total_time);
        printf("%-15s %-10lld %-10lld %-10lld %-10lld %-10lld\n", "Block Write", profile_info.block_write.count, profile_info.block_write.min_time, profile_info.block_write.max_time, profile_info.block_write.avg_time, profile_info.block_write.total_time);
        printf("%-15s %-10lld %-10lld %-10lld %-10lld %-10lld\n", "DDR Patch", profile_info.custom_ddr_patch.count, profile_info.custom_ddr_patch.min_time, profile_info.custom_ddr_patch.max_time, profile_info.custom_ddr_patch.avg_time, profile_info.custom_ddr_patch.total_time);
        printf("%-15s %-10lld %-10lld %-10lld %-10lld %-10lld\n", "Custom Op", profile_info.custom_op.count, profile_info.custom_op.min_time, profile_info.custom_op.max_time, profile_info.custom_op.avg_time, profile_info.custom_op.total_time);
    #endif
    
    if (!txn_silent)
        std::cout << "Done executing transaction" << std::endl;
}

uint8_t* ReadSerializedTransactionFromBin(std::string TxnBinFileName)
{
    std::cout << "Reading Transaction Binary from file: " << TxnBinFileName << std::endl;
    u64 BaseAddr = AIE_APERTURE_BASE;
    std::ifstream txnBin(TxnBinFileName, std::ios::binary);

    XAie_TxnHeader *Hdr = new XAie_TxnHeader();

    txnBin.read((char*)Hdr, sizeof(XAie_TxnHeader));

    std::cout << "Header version: " << static_cast<unsigned>(Hdr->Major) << "." << static_cast<unsigned>(Hdr->Minor) << std::endl;
    std::cout << "Device Generation: " << static_cast<unsigned>(Hdr->DevGen) << std::endl;
    std::cout << "Cols, Rows, NumMemRows : (" << static_cast<unsigned>(Hdr->NumCols) << "," <<  static_cast<unsigned>(Hdr->NumRows) << "," << static_cast<unsigned>(Hdr->NumMemTileRows) << ") " << std::endl;
    std::cout << "TransactionSize: " << Hdr->TxnSize << std::endl;
    std::cout << "NumOps: " << Hdr->NumOps << std::endl;

    uint8_t *Ptr = new uint8_t[Hdr->TxnSize];
    memcpy(Ptr, Hdr, sizeof(XAie_TxnHeader));

    uint8_t *txnPtr = Ptr + sizeof(*Hdr);
    txnBin.read((char*)txnPtr, Hdr->TxnSize - sizeof(XAie_TxnHeader));

    return Ptr;
}
void executeRuntimeControl_TXN()
{
   //std::cout << "executeRuntimeControl" << std::endl;

#if defined(__AIESIM__)
    std::cout << "simu 3 ReadSerializedTransactionFromBin" << std::endl;
    //uint8_t *Ptr =  ReadSerializedTransactionFromBin("Work/ps/ml_txn/ml_txn.bin");
        //uint8_t *Ptr =  ReadSerializedTransactionFromBin("./baremetal/ml_txn/ml_txn.bin");
		uint8_t *Ptr = (uint8_t *)__ml_txn_bin;
    XAie_SubmitSerializedTransaction(Ptr);
#else
    //std::cout << "hw ReadSerializedTransactionFromBin" << std::endl;
    uint8_t *Ptr = (uint8_t *)__ml_txn_bin;
    XAie_SubmitSerializedTransaction(Ptr);
    //executeRuntimeControlInternal();
#endif


}
