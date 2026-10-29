// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*
(c) Copyright 2019 - 2021 Xilinx, Inc. All Rights reserved.
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

#ifndef __LAYER_PARAMS_H__
#define __LAYER_PARAMS_H__

#include "utils.h"

// layer params for th next layer start after first 24 bytes
#define LAYER_OFFSET           32
#define PAD2D_OFFSET           34
#define GLOBAL_AVG_POOL_OFFSET 54
#define GAP_DIV_FAC_OFFSET     58
// Code for op modes
#define CONV2D_LAYER1  1
#define CONV2D_7x7S2_LYR1 3
#define CONV2D_7x7S2_LYR1_EDGE 4

#ifndef MLLIB_LP_SIZE
#define MLLIB_LP_SIZE 8
#endif

struct alignas(v16uint16) layer_params { 
// Layer Parameters (As of now this is superset of conv and maxpool ops)
// TODO: Need to have unique fields whenever layer fusion is enabled (e.g. conv2d + maxpool2x2_s2)
    unsigned int ifm_xsize ; 
    unsigned int ifm_ysize ; 
    unsigned int nifms ;
    unsigned int nofms ;         
    int ksize ; 
    int stride_bits  ;
    int8_t shift_bias_init ;    
    int8_t shift_psum_in ;
    int8_t shift_psum_out ;
    int8_t shift_out ;       
    int psum_buff_offset ; 
    int num_iter ;
    int16_t lrelu_alpha;
    int8_t shift_lrelu_input;
    int8_t shift_lrelu_alpha;
    int8_t shift_lrelu_out;
    int8_t num_ifm_depth_iter ;
    int8_t num_chout_iter ;
    int8_t num_width_iter ;
    int8_t num_height_iter ;
    uint16_t ofm_len;
    uint8_t  ofm_width_pad;
/*
//TODO: Add conv_type and layer parameters for maxpool, leakyReLU
    // ML Operation type
    // Conv Operations
    // 0  : int8 | int8 (result)
    // 1  : int8 | int16 (partial sum)
    // 2  : int16 (partial) | int8 (result)
    // 3  : int16 | int16 (load and store partial sums)
    // 4  : 15 (onwards): Reserved (Cascade and other Modes, TBD for future release)
    // Other Operations
    // 16 : maxpool
    // 17 : 23 Reserved
    // 24 : avgpool
    // 25 : 31 Reserved

    int conv_type ;
    int shift_bias;
    int shift_partial_load ;

    int lrelu_alpha ;
    int shift_lrelu;
*/
};
    
// Read layer parameters 
// TODO: Add other new fields whenever they get populated in layer_params

static void get_layer_params (uint32_t * params_ptr, uint32_t * curr_ptr) {
    
    //8 32-bit words
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(8,) 
    for (unsigned k=0;k<8;k++){
      *curr_ptr = *(params_ptr++);
       curr_ptr++;
    }
}

#endif  //__LAYER_PARAMS_H__
