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
    
#ifndef __PAD2D_PARAMS_H__
#define __PAD2D_PARAMS_H__

#include "utils.h"

#define PAD_RIGHT_BOTTOM 0x0A
#define PAD_RIGHT 0x02
#define PAD_LEFT_TOP 0x05
#define PAD_LEFT 0x01
#define PAD_TOP 0x04
#define PAD_BOT 0x08

#define PAD_PAT0 PAD_RIGHT_BOTTOM
#define PAD_PAT1 PAD_RIGHT

//TODO: need to check if keep_in_registers is required.
struct alignas(v32uint16) pad2d_internal_params{

    uint8_t sv_x;
    uint8_t sv_y;
    uint8_t nifms;
    uint8_t col_pad_mask;
    int8_t en_pad;
    int8_t pad_val;
    uint8_t pad_top;
    uint8_t pad_bot;
    uint8_t pad_left;
    uint8_t pad_right;
    bool enable_bottom_padding ;
};


typedef struct pad2d_internal_params pad2d_params_t ;

static void update_pad2d_params(pad2d_params_t &params,uint8_t pad_mask_override){
        uint8_t col_pad_mask = params.col_pad_mask & pad_mask_override ;
        params.pad_left  = (col_pad_mask & 0x1)?params.pad_left:0;
        params.pad_right = (col_pad_mask & 0x2)?params.pad_right:0;
        params.pad_top   = (col_pad_mask & 0x4)?params.pad_top:0;
        params.pad_bot   = (col_pad_mask & 0x8)?params.pad_bot:0;
}

// int8 overload
static void setup_pad2d_params(int8_t * params_ptr, pad2d_params_t &param, uint8_t batch_size) 
{
    param.nifms                 = (uint8_t) *params_ptr++;//0
    params_ptr += 3;//1-3                                         
    param.sv_x                  = (uint8_t) *params_ptr++; //4
    param.sv_y                  = (uint8_t) *params_ptr++; //5
                                             params_ptr++; //6
    param.en_pad                = (int8_t)  *params_ptr++; //7
    param.pad_val               = (int8_t)  *params_ptr++; //8
    param.pad_top               = (uint8_t) *params_ptr++; //9
    param.pad_bot               = (uint8_t) *params_ptr++; //10
    param.pad_left              = (uint8_t) *params_ptr++; //11
    param.pad_right             = (uint8_t) *params_ptr++; //12                 
}

// bfloat16 overload
struct alignas(v32uint16) pad2d_bf16_internal_params{

    uint8_t sv_x;
    uint8_t sv_y;
    uint8_t nifms;
    uint8_t col_pad_mask;
    int8_t en_pad;
    bfloat16 pad_val;
    uint8_t pad_top;
    uint8_t pad_bot;
    uint8_t pad_left;
    uint8_t pad_right;
    bool enable_bottom_padding ;
};

typedef struct pad2d_bf16_internal_params pad2d_bf16_params_t ;

static void setup_pad2d_params(bfloat16* params_ptr, pad2d_bf16_params_t &param, uint8_t batch_size) 
{
    uint8_t* params_ptr_ = (uint8_t *)params_ptr;
    bfloat16* params_bf16;

    param.nifms                 = (uint8_t) *params_ptr_++;//0
    params_ptr_ += 3;//1-3                                         
    param.sv_x                  = (uint8_t) *params_ptr_++; //4
    param.sv_y                  = (uint8_t) *params_ptr_++; //5
                                             params_ptr_++; //6
    param.en_pad                = (int8_t)  *params_ptr_++; //7
                                             params_bf16 = (bfloat16*) params_ptr_;
    param.pad_val               = (bfloat16)*params_bf16; //8
                                             params_ptr_++; //8
                                             params_ptr_++; //9
    param.pad_top               = (uint8_t) *params_ptr_++; //10
    param.pad_bot               = (uint8_t) *params_ptr_++; //11
    param.pad_left              = (uint8_t) *params_ptr_++; //12
    param.pad_right             = (uint8_t) *params_ptr_++; //13

}

#endif  //__PAD2D_PARAMS_H__   
