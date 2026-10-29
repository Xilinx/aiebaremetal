// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*
(c) Copyright 2014 - 2021 Xilinx, Inc. All Rights reserved.
(c) Copyright 2022 - 2024 Advanced Micro Devices, Inc. All Rights reserved.
   
    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.
   
    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.
   
    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.
   
    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */

#ifndef __GRAPH_PARAMS_H__
#define __GRAPH_PARAMS_H__

// AIE Engine logical array shape NUM_ROWS X NUM_COLS array
#ifndef NUM_COLS
    #define NUM_COLS        3  
#endif
#ifndef NUM_ROWS
    #define NUM_ROWS        2  
#endif

#ifndef COL_OFFSET
    #define COL_OFFSET     0  
#endif

#ifndef NUM_BATCH
    #define NUM_BATCH        2  // Batch
#endif

#ifndef ACT
    #define ACT  LINEAR 
#endif

#ifndef DTYPE_IN
    #define DTYPE_IN  int8 
#endif

#ifndef DTYPE_WTS
    #define DTYPE_WTS  int8 
#endif

#ifndef DTYPE_OUT
    #define DTYPE_OUT  int8 
#endif


//Max tensor sizes
#ifndef IFM_HEIGHT
    #define IFM_HEIGHT      26 // IFM: 26x26x16xNUM_BATCH
#endif
#ifndef IFM_WIDTH
    #define IFM_WIDTH       26 
#endif
#ifndef IFM_DEPTH
    #define IFM_DEPTH       16
#endif
#ifndef OFM_DEPTH
    #define OFM_DEPTH       64 
#endif
#ifndef FILTER_K
    #define FILTER_K         3  // WTS: 3x3x16x32
#endif
#ifndef FILTER_K_x
#define FILTER_K_x FILTER_K
#endif
#ifndef FILTER_K_y
#define FILTER_K_y FILTER_K
#endif
// Sub Volume (SV) Sizes. One SV is processed at a time by one core
#ifndef IFM_SV_HEIGHT
    #define IFM_SV_HEIGHT   10 //14   // IFM Sub-Vol: 10x10x16xNUM_BATCH
#endif
#ifndef IFM_SV_WIDTH
    #define IFM_SV_WIDTH    10
#endif
#ifndef IFM_SV_DEPTH
    #define IFM_SV_DEPTH    16
#endif
#ifndef OFM_SV_DEPTH
    #define OFM_SV_DEPTH    32
#endif
#ifndef LAYER_PARAM_SIZE
    #define LAYER_PARAM_SIZE  64 // Bytes(4n)
#endif
#ifndef IFM_DEPTH_GRAN
    #define IFM_DEPTH_GRAN 8
#endif
#ifndef ZP_LEN_H
   #define ZP_LEN_H        ((FILTER_K_y-1)/2)
#endif
#ifndef ZP_LEN_W
   #define ZP_LEN_W        ((FILTER_K_x-1)/2)
#endif
#ifndef OFM_HEIGHT
    #define OFM_HEIGHT      (IFM_HEIGHT - FILTER_K_y + 1)
#endif
#ifndef OFM_SV_HEIGHT
   #define OFM_SV_HEIGHT   (IFM_SV_HEIGHT - FILTER_K_y + 1)
#endif
#ifndef OFM_WIDTH
   #define OFM_WIDTH       (IFM_WIDTH - FILTER_K_x + 1)
#endif
#ifndef OFM_SV_WIDTH
   #define OFM_SV_WIDTH    (IFM_SV_WIDTH - FILTER_K_x + 1)
#endif
#ifndef BUF_SIZE_ALLOC
   #define BUF_SIZE_ALLOC       (512*1024)       // 512kB
#endif 
#ifndef IFM_BUF_FAC
   #define IFM_BUF_FAC 1
#endif
#ifndef OFM_BUF_FAC
   #define OFM_BUF_FAC 1
#endif
#ifndef WTS_BUF_FAC
   #define WTS_BUF_FAC 1
#endif
#ifndef ZP_LEN_R
    #define ZP_LEN_R  0
#endif

// Relative address constraints
#ifndef IFM_PING
  #define IFM_PING 0x8000
#endif
#ifndef IFM_PONG
  #define IFM_PONG 0xC000
#endif
#ifndef WTS_PING
  #define WTS_PING 0xA000
#endif
#ifndef WTS_PONG
  #define WTS_PONG 0xE000
#endif
#ifndef OFM_PING
  #define OFM_PING 0x0
#endif
#ifndef OFM_PONG
  #define OFM_PONG 0x4000
#endif
#ifndef STK_OFFSET
  #define STK_OFFSET 0x3000
#endif

#ifndef IFM_BUF_MAX
   #define IFM_BUF_MAX       (BUF_SIZE_ALLOC*IFM_BUF_FAC)        // 512kB
#endif
#ifndef OFM_BUF_MAX
   #define OFM_BUF_MAX       (BUF_SIZE_ALLOC*OFM_BUF_FAC)        // 512kB
#endif
#ifndef WTS_BUF_MAX
   #define WTS_BUF_MAX       (BUF_SIZE_ALLOC*WTS_BUF_FAC)        // 512kB
#endif

// Softmax related paramaters
#ifndef ELEM_BYTES
   #define ELEM_BYTES 2
#endif
#ifndef NUM_SOFTMAX_CLASSES
   #define NUM_SOFTMAX_CLASSES 1024
#endif
#ifndef SOFTMAX_BATCH_SIZE
   #define SOFTMAX_BATCH_SIZE 1
#endif
#ifndef SOFTMAX_WINSIZE
   #define SOFTMAX_WINSIZE (NUM_SOFTMAX_CLASSES*SOFTMAX_BATCH_SIZE)
#endif
#ifndef NUM_SOFTMAX_ITERS
   #define NUM_SOFTMAX_ITERS 1
#endif

// Number of Zero Padding BDs. 9 BDs are defined for 9 regions
#define NUM_BD_ROWS 3
#define NUM_BD_COLS 3
#define NUM_ZP_BDS (NUM_BD_ROWS*NUM_BD_COLS)

//Derived dimensions
#ifndef CONV_OUT_SV_HEIGHT 
  #define CONV_OUT_SV_HEIGHT   (IFM_SV_HEIGHT - FILTER_K_y + 1)
#endif
#ifndef CONV_OUT_SV_WIDTH 
  #define CONV_OUT_SV_WIDTH    (IFM_SV_WIDTH - FILTER_K_x + 1)
#endif
#define WIDTH_FOLDING   NUM_COLS
#define WTS_FOLDING     NUM_ROWS
#define BIAS_SIZE       (OFM_SV_DEPTH*2*4)                    //16-bit
#define WT_CONST_DELTA  (LAYER_PARAM_SIZE+BIAS_SIZE)        //Delta in bytes
#define WIDTH_ITER      (OFM_WIDTH/OFM_SV_WIDTH)
#define HEIGHT_ITER     (OFM_HEIGHT/OFM_SV_HEIGHT)
#define DEPTH_ITER      (OFM_DEPTH/OFM_SV_DEPTH) 
#define NUM_ITER        (HEIGHT_ITER*WIDTH_ITER*DEPTH_ITER/WIDTH_FOLDING/WTS_FOLDING)
#define IFM_WIN_SIZE    (IFM_SV_HEIGHT*IFM_SV_WIDTH*IFM_SV_DEPTH*NUM_BATCH)
#define WT_WIN_SIZE     (FILTER_K_y*FILTER_K_x*IFM_SV_DEPTH*OFM_SV_DEPTH+WT_CONST_DELTA) 
#define OFM_WIN_SIZE    (OFM_SV_HEIGHT*OFM_SV_WIDTH*OFM_SV_DEPTH*NUM_BATCH)
#define IFM_TENSOR_VOL  (IFM_HEIGHT*IFM_WIDTH*IFM_DEPTH*NUM_BATCH)
#define OFM_TENSOR_VOL  (OFM_HEIGHT*OFM_WIDTH*OFM_DEPTH*NUM_BATCH)
#define WT_TENSOR_VOL   (FILTER_K_y*FILTER_K_x*IFM_DEPTH*OFM_DEPTH+WT_CONST_DELTA*WTS_FOLDING)
#define OFM_DEPTH_GRAN  IFM_DEPTH_GRAN
#define IFM_DEPTH_NUM   (IFM_SV_DEPTH/IFM_DEPTH_GRAN)
#define OFM_DEPTH_NUM   (OFM_SV_DEPTH/OFM_DEPTH_GRAN)
#define IFM_NZP_WIDTH   (IFM_WIDTH - 2*ZP_LEN_W - ZP_LEN_R )            // Non-ZP IFM width
#define IFM_NZP_HEIGHT  (IFM_HEIGHT - 2*ZP_LEN_H)           // Non-ZP IFM height
#define IFM_TENSOR_VOL_NZP  (IFM_NZP_HEIGHT*(IFM_NZP_WIDTH)*IFM_DEPTH*NUM_BATCH)
#ifndef OFM_WIN_SIZE_MAX 
#define OFM_WIN_SIZE_MAX    (CONV_OUT_SV_HEIGHT*CONV_OUT_SV_WIDTH*OFM_SV_DEPTH*NUM_BATCH)
#endif


#endif //__GRAPH_PARAMS_H__

