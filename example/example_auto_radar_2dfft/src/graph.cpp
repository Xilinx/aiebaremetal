// © Copyright 2022 – 2023 Xilinx, Inc. All rights reserved.
// This file contains confidential and proprietary information of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property laws.
// DISCLAIMER
// This disclaimer is not a license and does not grant any rights to the materials distributed herewith.
// Except as otherwise provided in a valid license issued to you by Xilinx, and to the maximum extent
// permitted by applicable law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL
// FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR
// STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
// in contract or tort, including negligence, or under any other theory of liability) for any loss or damage of
// any kind or nature related to, arising under or in connection with these materials, including for any
// direct, or any indirect, special, incidental, or consequential loss or damage (including loss of data,
// profits, goodwill, or any type of loss or damage suffered as a result of any action brought by a third
// party) even if such damage or loss was reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-safe, or for use in any application requiring fail-
// safe performance, such as life-support or safety devices or systems, Class III medical devices, nuclear
// facilities, applications related to the deployment of airbags, or any other applications that could lead to
// death, personal injury, or severe property or environmental damage (individually and collectively,

//"Critical Applications"). Customer assumes the sole risk and liability of any use of Xilinx products in
// Critical Applications, subject only to applicable laws and regulations governing limitations on product
// liability.
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT ALL TIMES.

#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
FFT2D_graphall gradf;
#endif

#include "aiebaremetal.h"
#include <fstream>
#include <xaiengine.h>

#include "golden.data"
#include "hw_config.h"
#include "input.data"
#include <stdio.h>

#define CHECK 1
#define ITER_CNT 1
// AdfrGraph gr("gradf", "FFT2D_graphall", "graph.h");
AbrGraph gr("gradf");
// #ifdef __AIESIM__

////         The Base address of the AIE array in Versal address map is 0x20000000000
////         The offset of the Register space has 20bits (0 to 19th bit)
////         Column has 7bits (25th to 31st)
////         Row has  5bits (20th to 24th)
////         Ex 0x2000401d000 = Column = 2, Row = 2, register offset = 0x1d000

void shim_config1(XAie_DevInst DevInst) {
    printf("Inside shim_config1\n");
    // SHIMTILE S2MM     2BD FOR PING and PONG -> BD0 and BD1 loopback with LOCK Synchronisation
    // Xil_Out32(0x2000401d000, 0x00C00000);

    XAie_Write32(&DevInst, 0x401d000, 0x00C00000); //    Length = 0xC00000 = 12582912
    XAie_Write32(&DevInst, 0x401d004, 0x00000000); //    ADDRESS=0x00000000 (LOWER BASE Address)
    XAie_Write32(&DevInst, 0x401d008, 0x00000500); //    ADDRESS= 0x500 (Upper BASE Address: 16 MSB )   - PING Address
    XAie_Write32(&DevInst, 0x401d00c, 0x06000000); //    STEP_SIZE0 = 1; WRAP_SIZE0 = 96
    XAie_Write32(&DevInst, 0x401d010, 0x20005fff); //    STEP_SIZE1 = 24576; WRAP_SIZE1 = 512
    XAie_Write32(&DevInst, 0x401d014, 0x0000005f); //    STEP_SIZE2 = 96
    XAie_Write32(&DevInst, 0x401d018, 0x00000000);
    XAie_Write32(&DevInst, 0x401d01c,
                 0x0e041000); //    NEXT_BD(27th to 30th bits) = 1 ;USE_NEXT_BD(26th bit) ; // Valid_BD(25st bit) =
                              //    1;lock_acq_id = 0;lock_acq_val=0;lock_rel_id=0;lock_rel_val=1;

    XAie_Write32(&DevInst, 0x401d020, 0x00C00000);
    XAie_Write32(&DevInst, 0x401d024, 0x10000000); //    ADDRESS=0x10000000 (LOWER BASE Address)
    XAie_Write32(&DevInst, 0x401d028, 0x00000500); //    ADDRESS= 0x500 (Upper BASE Address: 16 MSB )   - PONG Address
    XAie_Write32(&DevInst, 0x401d02c, 0x06000000);
    XAie_Write32(&DevInst, 0x401d030, 0x20005fff);
    XAie_Write32(&DevInst, 0x401d034, 0x0000005f);
    XAie_Write32(&DevInst, 0x401d038, 0x00000000);
    XAie_Write32(&DevInst, 0x401d03c,
                 0x06043001); //    NEXT_BD(27th to 30th bits) = 0 ;USE_NEXT_BD(26th bit) ; // Valid_BD(25st bit) =
                              //    1;lock_acq_id = 1;lock_acq_val=0;lock_rel_id=1;lock_rel_val=1;
    // SHIMTILE MM2S     2BD FOR PING and PONG -> BD4 and BD5 loopback with LOCK Synchronisation
    XAie_Write32(&DevInst, 0x401d080, 0x00C00000); //    Length = 0xC00000 = 12582912
    XAie_Write32(&DevInst, 0x401d084, 0x00000000); //    ADDRESS=0x00000000 (LOWER BASE Address)
    XAie_Write32(&DevInst, 0x401d088, 0x00000500); //    ADDRESS= 0x500 (Upper BASE Address: 16 MSB )   - PING Address
    XAie_Write32(&DevInst, 0x401d08c, 0x00000000);
    XAie_Write32(&DevInst, 0x401d090, 0x00000000);
    XAie_Write32(&DevInst, 0x401d094, 0x00000000);
    XAie_Write32(&DevInst, 0x401d098, 0x00000000);
    XAie_Write32(&DevInst, 0x401d09c,
                 0x2FFC1020); //    NEXT_BD(27th to 30th bits) = 5 ;USE_NEXT_BD(26th bit) ; // Valid_BD(25st bit) =
                              //    1;lock_acq_id = 0;lock_acq_val=1;lock_rel_id=0;lock_rel_val=-1;

    XAie_Write32(&DevInst, 0x401d0a0, 0x00C00000);
    XAie_Write32(&DevInst, 0x401d0a4, 0x10000000); //    ADDRESS=0x10000000 (LOWER BASE Address)
    XAie_Write32(&DevInst, 0x401d0a8, 0x00000500); //    ADDRESS= 0x500 (Upper BASE Address: 16 MSB )   - PONG Address
    XAie_Write32(&DevInst, 0x401d0ac, 0x00000000);
    XAie_Write32(&DevInst, 0x401d0b0, 0x00000000);
    XAie_Write32(&DevInst, 0x401d0b4, 0x00000000);
    XAie_Write32(&DevInst, 0x401d0b8, 0x00000000);
    XAie_Write32(&DevInst, 0x401d0bc,
                 0x27FC3021); //    NEXT_BD(27th to 30th bits) = 4 ;USE_NEXT_BD(26th bit) ; // Valid_BD(25st bit) =
                              //    1;lock_acq_id = 1;lock_acq_val=1;lock_rel_id=1;lock_rel_val=-1;

    XAie_Write32(&DevInst, 0x401d204, 0x0); // S2MM Start Task Queue with BD 0
    XAie_Write32(&DevInst, 0x401d214, 0x4); // MM2S START Task Queue with BD 4
    printf("shim_config1 done\n");
}
void shim_config2(XAie_DevInst DevInst) {
    printf("Inside shim_config2\n");
    XAie_Write32(&DevInst, 0x601d000, 0x00C00000);
    XAie_Write32(&DevInst, 0x601d004, 0x20000000);
    XAie_Write32(&DevInst, 0x601d008, 0x00000500);
    XAie_Write32(&DevInst, 0x601d00c, 0x06000000);
    XAie_Write32(&DevInst, 0x601d010, 0x20005fff);
    XAie_Write32(&DevInst, 0x601d014, 0x0000005f);
    XAie_Write32(&DevInst, 0x601d018, 0x00000000);
    XAie_Write32(&DevInst, 0x601d01c, 0x0e041000);

    XAie_Write32(&DevInst, 0x601d020, 0x00C00000);
    XAie_Write32(&DevInst, 0x601d024, 0x30000000);
    XAie_Write32(&DevInst, 0x601d028, 0x00000500);
    XAie_Write32(&DevInst, 0x601d02c, 0x06000000);
    XAie_Write32(&DevInst, 0x601d030, 0x20005fff);
    XAie_Write32(&DevInst, 0x601d034, 0x0000005f);
    XAie_Write32(&DevInst, 0x601d038, 0x00000000);
    XAie_Write32(&DevInst, 0x601d03c, 0x06043001);

    XAie_Write32(&DevInst, 0x601d080, 0x00C00000);
    XAie_Write32(&DevInst, 0x601d084, 0x20000000);
    XAie_Write32(&DevInst, 0x601d088, 0x00000500);
    XAie_Write32(&DevInst, 0x601d08c, 0x00000000);
    XAie_Write32(&DevInst, 0x601d090, 0x00000000);
    XAie_Write32(&DevInst, 0x601d094, 0x00000000);
    XAie_Write32(&DevInst, 0x601d098, 0x00000000);
    XAie_Write32(&DevInst, 0x601d09c, 0x2FFC1020);

    XAie_Write32(&DevInst, 0x601d0a0, 0x00C00000);
    XAie_Write32(&DevInst, 0x601d0a4, 0x30000000);
    XAie_Write32(&DevInst, 0x601d0a8, 0x00000500);
    XAie_Write32(&DevInst, 0x601d0ac, 0x00000000);
    XAie_Write32(&DevInst, 0x601d0b0, 0x00000000);
    XAie_Write32(&DevInst, 0x601d0b4, 0x00000000);
    XAie_Write32(&DevInst, 0x601d0b8, 0x00000000);
    XAie_Write32(&DevInst, 0x601d0bc, 0x27FC3021);

    XAie_Write32(&DevInst, 0x601d204, 0x0);
    XAie_Write32(&DevInst, 0x601d214, 0x4);
    printf("Inside shim_config2 done\n");
}
void shim_config3(XAie_DevInst DevInst) {
    printf("Inside shim_config3\n");
    XAie_Write32(&DevInst, 0xC01d000, 0x00C00000);
    XAie_Write32(&DevInst, 0xC01d004, 0x40000000);
    XAie_Write32(&DevInst, 0xC01d008, 0x00000500);
    XAie_Write32(&DevInst, 0xC01d00c, 0x06000000);
    XAie_Write32(&DevInst, 0xC01d010, 0x20005fff);
    XAie_Write32(&DevInst, 0xC01d014, 0x0000005f);
    XAie_Write32(&DevInst, 0xC01d018, 0x00000000);
    XAie_Write32(&DevInst, 0xC01d01c, 0x0e041000);

    XAie_Write32(&DevInst, 0xC01d020, 0x00C00000);
    XAie_Write32(&DevInst, 0xC01d024, 0x50000000);
    XAie_Write32(&DevInst, 0xC01d028, 0x00000500);
    XAie_Write32(&DevInst, 0xC01d02c, 0x06000000);
    XAie_Write32(&DevInst, 0xC01d030, 0x20005fff);
    XAie_Write32(&DevInst, 0xC01d034, 0x0000005f);
    XAie_Write32(&DevInst, 0xC01d038, 0x00000000);
    XAie_Write32(&DevInst, 0xC01d03c, 0x06043001);

    XAie_Write32(&DevInst, 0xC01d080, 0x00C00000);
    XAie_Write32(&DevInst, 0xC01d084, 0x40000000);
    XAie_Write32(&DevInst, 0xC01d088, 0x00000500);
    XAie_Write32(&DevInst, 0xC01d08c, 0x00000000);
    XAie_Write32(&DevInst, 0xC01d090, 0x00000000);
    XAie_Write32(&DevInst, 0xC01d094, 0x00000000);
    XAie_Write32(&DevInst, 0xC01d098, 0x00000000);
    XAie_Write32(&DevInst, 0xC01d09c, 0x2FFC1020);

    XAie_Write32(&DevInst, 0xC01d0a0, 0x00C00000);
    XAie_Write32(&DevInst, 0xC01d0a4, 0x50000000);
    XAie_Write32(&DevInst, 0xC01d0a8, 0x00000500);
    XAie_Write32(&DevInst, 0xC01d0ac, 0x00000000);
    XAie_Write32(&DevInst, 0xC01d0b0, 0x00000000);
    XAie_Write32(&DevInst, 0xC01d0b4, 0x00000000);
    XAie_Write32(&DevInst, 0xC01d0b8, 0x00000000);
    XAie_Write32(&DevInst, 0xC01d0bc, 0x27FC3021);

    XAie_Write32(&DevInst, 0xC01d204, 0x0);
    XAie_Write32(&DevInst, 0xC01d214, 0x4);
    printf("Inside shim_config3 done\n");
}
void shim_config4(XAie_DevInst DevInst) {
    printf("Inside shim_config4\n");
    XAie_Write32(&DevInst, 0xe01d000, 0x00C00000);
    XAie_Write32(&DevInst, 0xe01d004, 0x60000000);
    XAie_Write32(&DevInst, 0xe01d008, 0x00000500);
    XAie_Write32(&DevInst, 0xe01d00c, 0x06000000);
    XAie_Write32(&DevInst, 0xe01d010, 0x20005fff);
    XAie_Write32(&DevInst, 0xe01d014, 0x0000005f);
    XAie_Write32(&DevInst, 0xe01d018, 0x00000000);
    XAie_Write32(&DevInst, 0xe01d01c, 0x0e041000);

    XAie_Write32(&DevInst, 0xe01d020, 0x00C00000);
    XAie_Write32(&DevInst, 0xe01d024, 0x70000000);
    XAie_Write32(&DevInst, 0xe01d028, 0x00000500);
    XAie_Write32(&DevInst, 0xe01d02c, 0x06000000);
    XAie_Write32(&DevInst, 0xe01d030, 0x20005fff);
    XAie_Write32(&DevInst, 0xe01d034, 0x0000005f);
    XAie_Write32(&DevInst, 0xe01d038, 0x00000000);
    XAie_Write32(&DevInst, 0xe01d03c, 0x06043001);

    XAie_Write32(&DevInst, 0xe01d080, 0x00C00000);
    XAie_Write32(&DevInst, 0xe01d084, 0x60000000);
    XAie_Write32(&DevInst, 0xe01d088, 0x00000500);
    XAie_Write32(&DevInst, 0xe01d08c, 0x00000000);
    XAie_Write32(&DevInst, 0xe01d090, 0x00000000);
    XAie_Write32(&DevInst, 0xe01d094, 0x00000000);
    XAie_Write32(&DevInst, 0xe01d098, 0x00000000);
    XAie_Write32(&DevInst, 0xe01d09c, 0x2FFC1020);

    XAie_Write32(&DevInst, 0xe01d0a0, 0x00C00000);
    XAie_Write32(&DevInst, 0xe01d0a4, 0x70000000);
    XAie_Write32(&DevInst, 0xe01d0a8, 0x00000500);
    XAie_Write32(&DevInst, 0xe01d0ac, 0x00000000);
    XAie_Write32(&DevInst, 0xe01d0b0, 0x00000000);
    XAie_Write32(&DevInst, 0xe01d0b4, 0x00000000);
    XAie_Write32(&DevInst, 0xe01d0b8, 0x00000000);
    XAie_Write32(&DevInst, 0xe01d0bc, 0x27FC3021);

    XAie_Write32(&DevInst, 0xe01d204, 0x0);
    XAie_Write32(&DevInst, 0xe01d214, 0x4);
    printf("Inside shim_config4 done\n");
}

void shim_perf_config(XAie_DevInst DevInst) {
    XAie_Write32(&DevInst, 0xa03ff00, 0x00000002);
    XAie_Write32(&DevInst, 0xa031000, 0x01014e4e);

    XAie_Write32(&DevInst, 0xc03ff00, 0x00000002);
    XAie_Write32(&DevInst, 0xc031000, 0x01014e4e);

    XAie_Write32(&DevInst, 0xe03ff00, 0x00000002);
    XAie_Write32(&DevInst, 0xe031000, 0x01014e4e);

    XAie_Write32(&DevInst, 0x1003ff00, 0x00000002);
    XAie_Write32(&DevInst, 0x10031000, 0x01014e4e);
}

int main(int argc, char **argv) {
    XAie_SetupConfig(ConfigPtr, HW_GEN, XAIE_BASE_ADDR, XAIE_COL_SHIFT, XAIE_ROW_SHIFT, XAIE_NUM_COLS, XAIE_NUM_ROWS,
                     XAIE_SHIM_ROW, XAIE_MEM_TILE_ROW_START, XAIE_MEM_TILE_NUM_ROWS, XAIE_AIE_TILE_ROW_START,
                     XAIE_AIE_TILE_NUM_ROWS);

    XAie_InstDeclare(DevInst, &ConfigPtr);

    AieRC RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);
    if (RC != XAIE_OK) {
        printf("Driver initialization failed.\n");
        return -1;
    }

    unsigned int itrc = ITER_CNT;
    // #if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
    // 	//std::cout << "gradf.a  =" << gradf.a << std::endl;
    // 	gradf.init();
    // 	///adf_run();
    // 	//std::cout << "adf_run" <<std::endl;
    // 	//return 0;
    // #else
    //  //FIXME, should handle the sync in runtime api
    // 	Xil_DCacheDisable();
    // 	Xil_ICacheDisable();
    // 	//get time
    //   XTime tStart, tEnd;
    //   //init_platform();
    // #endif
    gr.init();

    // writing the input file
    //  ofstream fout("data/input1.txt");
    //  for(int i = 0;i < 2048*64*2;)
    //  {
    //      for(int j = 0;j<4; j++)
    //      {
    //          fout<<m_fft_buffer_mapped[i++]<<" ";
    //      }
    //      fout<<std::endl;
    //  }
    //  fout.close();

    printf("Initialising  Graph ...\n");
#if CHECK
    shim_config1(DevInst);
    shim_config2(DevInst);
    shim_config3(DevInst);
    shim_config4(DevInst);

    // shim_perf_config(DevInst);
#endif

    uint32_t *buffer_in_mm2s_map_1;
    uint32_t *buffer_in_mm2s_map_2;
    uint32_t *buffer_in_mm2s_map_3;
    uint32_t *buffer_in_mm2s_map_4;

    int32_t *buffer_in_s2mm_map_1;
    int32_t *buffer_in_s2mm_map_2;
    int32_t *buffer_in_s2mm_map_3;
    int32_t *buffer_in_s2mm_map_4;

    uint32_t temp_buffer[2048 * 64 * 4];
    memcpy(temp_buffer, m_fft_buffer_mapped, 2048 * 64 * 4);

    buffer_in_mm2s_map_1 = (uint32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_mm2s_map_2 = (uint32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_mm2s_map_3 = (uint32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_mm2s_map_4 = (uint32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);

    buffer_in_s2mm_map_1 = (int32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_s2mm_map_2 = (int32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_s2mm_map_3 = (int32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_s2mm_map_4 = (int32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);

    // To reduce input data storage in DDR, we are only storing unique chirps for 64 Groups
    // i.e. input data buffer in DDR is [64][2048].
    // mm2s data mover takes these unique chirp sequence for each Group and generate chirps
    // for TX and RX channel iterations by rotating the chirp samples left by 4 samples.
    // For GMIO case the preprocessing step does the same thing and stores the data in heap.
    unsigned int k = 0;
    int ind = 0;
    int shft_cnt = 0;
    size_t total_samples = 2048 * 8 * 24;
    unsigned int size = total_samples / 4;

    uint32_t ptr[2048];
    for (int i = 0; i < 64; i++) {

        shft_cnt = 0;
        ind = 0;

        for (int p = 0; p < 2048; p++) {
            ptr[p] = temp_buffer[p + 2048 * i];
        }
        uint32_t index = 0;
        for (int q = 0; q < size; q++) {

            if ((q != 0) && (q % 512 == 0)) {
                ind = 0;
                shft_cnt++;
            }

            index = (ind + shft_cnt) % 512;

            buffer_in_mm2s_map_1[k++] = ptr[index * 4];
            buffer_in_mm2s_map_1[k++] = ptr[index * 4 + 1];
            buffer_in_mm2s_map_1[k++] = ptr[index * 4 + 2];
            buffer_in_mm2s_map_1[k++] = ptr[index * 4 + 3];

            ind++;
        }
    }

    memcpy(buffer_in_mm2s_map_2, buffer_in_mm2s_map_1, 2048 * 64 * 4 * 8 * 24);
    memcpy(buffer_in_mm2s_map_3, buffer_in_mm2s_map_1, 2048 * 64 * 4 * 8 * 24);
    memcpy(buffer_in_mm2s_map_4, buffer_in_mm2s_map_1, 2048 * 64 * 4 * 8 * 24);

    printf("K is :%d\n", k);
    printf("memcopy done\n");
    printf("Data transfer through GMIO is starting\n");

    gr.gm2aie_nb("gradf.gm_in[0]", reinterpret_cast<char *>(buffer_in_mm2s_map_1), 64 * 4 * 8 * 24 * 2048);
    gr.gm2aie_nb("gradf.gm_in[1]", reinterpret_cast<char *>(buffer_in_mm2s_map_2), 64 * 4 * 8 * 24 * 2048);
    gr.gm2aie_nb("gradf.gm_in[2]", reinterpret_cast<char *>(buffer_in_mm2s_map_3), 64 * 4 * 8 * 24 * 2048);
    gr.gm2aie_nb("gradf.gm_in[3]", reinterpret_cast<char *>(buffer_in_mm2s_map_4), 64 * 4 * 8 * 24 * 2048);

    printf("Data transfer through GMIO in progress\n");

    XAie_LocType core_loc = {3, 2};
    uint32_t core_data_memory = 0;
    printf("*************data memory of tile(2, 3) before run***************\n");
    // XAie_Write32(&DevInst  ,  XAie_GetTileAddr(&DevInst, core_loc.Row, core_loc.Col) +
    // 		0x6000, 55);
    // XAie_Read32(&DevInst  ,  XAie_GetTileAddr(&DevInst, core_loc.Row, core_loc.Col) +
    // 		0x6000, &core_data_memory);

    printf("data memory - %d\n", core_data_memory);

    uint32_t mem_tile_data = 0;
    XAie_LocType mem_loc = {2, 1};
    printf("*************memory tile data of tile(1, 2) before run***************\n");
    // XAie_Write32(&DevInst  ,  XAie_GetTileAddr(&DevInst, mem_loc.Row, mem_loc.Col) , 55);
    // XAie_Read32(&DevInst  ,  XAie_GetTileAddr(&DevInst, mem_loc.Row, mem_loc.Col) , &mem_tile_data);
    // printf("memory tile data - %d\n", mem_tile_data);

    gr.run(2048 * 64 * 8 * 24);
    sleep(100);

    printf("*************data memory of tile(2, 3) after run***************\n");
    // XAie_Read32(&DevInst  , XAie_GetTileAddr(&DevInst, core_loc.Row, core_loc.Col) +
    // 		0x6000, &core_data_memory);
    // printf("data memory - %d\n", core_data_memory);

    // printf("*************memory tile data of tile(1, 2) after run***************\n");
    // XAie_Read32(&DevInst  ,  XAie_GetTileAddr(&DevInst, mem_loc.Row, mem_loc.Col) , &mem_tile_data);
    // printf("memory tile data - %d\n", mem_tile_data);

    int errCnt = 0;
    printf("errCnt before running value %d...\n", errCnt);

    if (itrc == 1) {
        printf("Kernels are running for 1 iteration...\n");
        gr.aie2gm_nb("gradf.gm_out[0]", reinterpret_cast<char *>(buffer_in_s2mm_map_1), 2048 * 64 * 4 * 8 * 24);
        gr.aie2gm_nb("gradf.gm_out[1]", reinterpret_cast<char *>(buffer_in_s2mm_map_2), 2048 * 64 * 4 * 8 * 24);
        gr.aie2gm_nb("gradf.gm_out[2]", reinterpret_cast<char *>(buffer_in_s2mm_map_3), 2048 * 64 * 4 * 8 * 24);
        gr.aie2gm_nb("gradf.gm_out[3]", reinterpret_cast<char *>(buffer_in_s2mm_map_4), 2048 * 64 * 4 * 8 * 24);

        gr.gmio_wait("gradf.gm_out[0]");
        gr.gmio_wait("gradf.gm_out[1]");
        gr.gmio_wait("gradf.gm_out[2]");
        gr.gmio_wait("gradf.gm_out[3]");

        printf("Kernels execution completed ...\n");
        // Golden check ffor 1 iteration
        for (int j = 0; j < 2048 * 64 * 8 * 24; j++) {

            if (buffer_in_s2mm_map_1[j] != golden_data[j] || buffer_in_s2mm_map_2[j] != golden_data[j] ||
                buffer_in_s2mm_map_3[j] != golden_data[j] || buffer_in_s2mm_map_4[j] != golden_data[j]) {
                errCnt++;
            }
        }
        printf("errCnt after running value %d...\n", errCnt);
        if (errCnt == 0) {
            printf("\n ******** Golden Check Passed ******** \n");
        } else {
            printf("\n ******** Golden Check Failed with error count %d ******** \n", errCnt);
        }
    } else {

        printf("Kernels are running ...\n");
    }
    // printf("Output data is :%d\n",*buffer_in_s2mm_map_1 );
    for (int j = 0; j < 10; j++) {
        printf("Final output data :%d \n", buffer_in_s2mm_map_1[j]);
    }
    std::cout << "Closing Graph...\n";
    // gr.end();

    printf(" Graph end....\n");
    gr.free(buffer_in_mm2s_map_1);
    gr.free(buffer_in_mm2s_map_2);
    gr.free(buffer_in_mm2s_map_3);
    gr.free(buffer_in_mm2s_map_4);

    gr.free(buffer_in_s2mm_map_1);
    gr.free(buffer_in_s2mm_map_2);
    gr.free(buffer_in_s2mm_map_3);
    gr.free(buffer_in_s2mm_map_4);

    return 0;
}
// #endif
