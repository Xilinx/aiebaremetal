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

#ifndef __MAXPOOL2D_PARAMS_H__
#define __MAXPOOL2D_PARAMS_H__

#include "utils.h"

#define STEP_X_I 16 //Move 16 units in X dimension (8px for batch-of-2 and 4px for batch-of-4)

#define MAXPOOL2D_LOOP_RANGE 2

//TODO: need to check if keep_in_registers is required.
struct alignas(v32uint16) maxpool2d_internal_params{

    uint8_t Kx_g;
    uint8_t Ky_g;
    uint8_t Ci_g;
    uint16_t outer_loop_count;

    DECL_ITR3D(iterator)

    // Changing below to int16
    int16_t inc_Ky;
    int16_t inc_A_0;
    // changing below to int8
    int8_t shft_0;
    int8_t shft_1;
    int8_t shft_2;
    int8_t shft_3;
    int8_t shft_4;
    int8_t shft_5;
    int8_t shft_6;
    int8_t shft_7;
    int8_t shfl_0; //Check
    int8_t shfl_1; //Check

    uint16_t ofm_len ;

    uint8_t nifms;

};

typedef struct maxpool2d_internal_params maxpool2d_params_t ;

// int8 overload
static void setup_maxpool2d_params(int8_t * params_ptr, maxpool2d_params_t &param, uint8_t batch_size)
{
    uint8_t ifm_xsize           = (uint8_t) *params_ptr++;//0
    uint8_t ifm_ysize           = (uint8_t) *params_ptr++;//1
    param.nifms                 = (uint8_t) *params_ptr++;//2
                                             params_ptr++;//3  No need for OFMs so just increment pointer
    uint8_t ksize               = (uint8_t) *params_ptr++;//4
    uint8_t stride_bits         = (uint8_t) *params_ptr++;//5
    params_ptr+=11; //6-16
    uint8_t manual_ba_pad_val   = (uint8_t) *params_ptr++; //17
    params_ptr+=3; //  OFM length is at offset 20
    param.ofm_len               = *( (uint16_t *) params_ptr);

    //Variables required to setup the 3D iterator
    uint8_t num_X;
    uint8_t num_Co;
    int16_t inc_Xi;
    int16_t inc_Ci;
    int16_t inc_Yi;

    uint8_t stride = 1 << stride_bits;
    uint8_t X_g;
    uint8_t Co_g = param.nifms >> 3;

    param.Kx_g = ksize;
    param.Ky_g = ksize;

    uint8_t batch_log2 = (batch_size==4)?2:((batch_size==2)?1:0);

    int8_t ks_diff = ksize-stride;
    uint8_t boundary_align = (manual_ba_pad_val!=0)? manual_ba_pad_val : ((batch_size==4) ? 0:(ks_diff!=0 ? ((4>>batch_log2) -ks_diff):0));

    unsigned int ifm_xsize_padded =  ifm_xsize + boundary_align;

    uint16_t step_Ky = ifm_xsize_padded * param.nifms * batch_size;

    //Based on ifm width format, we have to deduce X_g
    uint8_t factor = ((8*stride)>>batch_log2);
    uint8_t factor_log2 = (factor==16)?4:( (factor==8)?3:( (factor==4)?2:1 ));

    X_g = (uint8_t)((ifm_xsize - ks_diff)>>factor_log2); //Number of iteration long x dimension

    uint16_t step_Xi = stride > 1 ? STEP_X_I : (STEP_X_I >>1);
    uint16_t step_Ci = (ifm_xsize_padded * batch_size) << 3;
    uint16_t step_Yi = (ifm_xsize_padded *batch_size) * param.nifms * stride;

    param.outer_loop_count = X_g  *  Co_g * ((uint8_t)((ifm_ysize-param.Kx_g)>>stride_bits) + 1);

    param.shfl_0 = (stride==2 ? (batch_size==1 ? T64_8x2_lo : (batch_size==2 ? T128_4x2_lo:T256_2x2_lo)) :T512_1x2_lo);
    param.shfl_1 = (stride==2 ? (batch_size==1 ? T64_8x2_hi : (batch_size==2 ? T128_4x2_hi:T256_2x2_hi)) :T512_1x2_lo);

    param.shft_0 = 8 * batch_size * (stride == 1);
    param.shft_1 = 8 * batch_size * (param.Kx_g>2) * (stride == 2);
    param.shft_2 = 2 * 8 * batch_size * (param.Kx_g>2) * (stride == 1);

    num_X  = X_g  - 1;
    num_Co = Co_g - 1;

    param.inc_Ky  = -128 + step_Ky;

    //TODO
    param.inc_A_0 = chess_copy(param.Ky_g>2 ? param.inc_Ky : -128);

    inc_Xi = -step_Ky *(param.Ky_g-1) + (stride>1 ? 0 : -64);
    inc_Ci = -step_Ky * (param.Ky_g-1) + (stride > 1 ? 0 : -64) - 8*step_Xi * X_g + step_Ci;
    inc_Yi = -step_Ky * (param.Ky_g-1) + (stride>1 ? 0 : -64) - 8*step_Xi * X_g - step_Ci * num_Co + step_Yi;

    SETUP_ITR3D_PARAM(iterator,inc_Xi,num_X,inc_Ci,num_Co,inc_Yi,param)
}

// overload for bfloat16
static void setup_maxpool2d_params(bfloat16* params_ptr, maxpool2d_params_t &param, uint8_t batch_size)
{
    uint8_t* params_ptr_ = (uint8_t *)params_ptr;

    uint8_t ifm_xsize0          = (uint8_t) *params_ptr_++;//0
    uint8_t ifm_ysize           = (uint8_t) *params_ptr_++;//1
    param.nifms                 = (uint8_t) *params_ptr_++;//2
                                             params_ptr_++;//3  No need for OFMs so just increment pointer
    uint8_t ksize               = (uint8_t) *params_ptr_++;//4
    uint8_t stride_bits         = (uint8_t) *params_ptr_++;//5
    params_ptr_+=2; //6-7
    uint8_t ifm_xsize           = (uint8_t) *params_ptr_++;//8 This should be ifm_width size which is padded so that we can get 8-aligned ofm_width
    params_ptr_+=8; //9-16
    uint8_t manual_ba_pad_val   = (uint8_t) *params_ptr_++; //17
    params_ptr_+=3; //  OFM length is at offset 20
    param.ofm_len               = *( (uint16_t *) params_ptr_);

    //Variables required to setup the 3D iterator
    uint8_t num_X;
    uint8_t num_Co;
    int16_t inc_Xi;
    int16_t inc_Ci;
    int16_t inc_Yi;

    uint8_t stride = 1 << stride_bits;
    uint8_t X_g;
    uint8_t Co_g = param.nifms >> 3;

    param.Kx_g = ksize;
    param.Ky_g = ksize;

    uint8_t batch_log2 = (batch_size==4)?2:((batch_size==2)?1:0);

    int8_t ks_diff = ksize-stride;
    uint8_t boundary_align = (manual_ba_pad_val!=0)? manual_ba_pad_val : ((batch_size==4) ? 0:(ks_diff!=0 ? ((4>>batch_log2) -ks_diff):0));

    unsigned int ifm_xsize_padded =  ifm_xsize + boundary_align;

    uint16_t step_Ky = ifm_xsize_padded * param.nifms * batch_size * 2;

    //Based on ifm width format, we have to deduce X_g
    uint8_t factor = ((8*stride)>>batch_log2);
    uint8_t factor_log2 = (factor==16)?4:( (factor==8)?3:( (factor==4)?2:1 ));

    X_g = (uint8_t)((ifm_xsize - ks_diff)>>factor_log2); //Number of iteration long x dimension

    uint16_t step_Xi = stride > 1 ? STEP_X_I : (STEP_X_I >>1);
    uint16_t step_Ci = (ifm_xsize_padded * batch_size) << 3;
    uint16_t step_Yi = (ifm_xsize_padded *batch_size) * param.nifms * stride;

    param.outer_loop_count = X_g  *  Co_g * ((uint8_t)((ifm_ysize-param.Kx_g)>>stride_bits) + 1);

    // batch 2 / 4 not supported
    param.shfl_0 = (stride==2 ? (batch_size==1 ? T128_4x2_lo : T256_2x2_lo) :T512_1x2_lo);
    param.shfl_1 = (stride==2 ? (batch_size==1 ? T128_4x2_hi : T256_2x2_hi) :T512_1x2_hi);

    param.shft_0 = 0;
    param.shft_1 = 8 * (stride == 1 ? 1:4);
    param.shft_2 = 8 * (stride == 1 ? 2 * (param.Kx_g > 2):0);
    param.shft_3 = 8 * (stride == 1 ? 2 * (param.Kx_g > 2):4);
    param.shft_4 = 8 * (stride == 1 ? 0:4);
    param.shft_5 = 8 * (stride == 1 ? 0:4);
    param.shft_6 = 8 * (stride == 1 ? 0:(param.Kx_g > 2?1:0));
    param.shft_7 = 8 * (stride == 1 ? 0:(param.Kx_g > 2?1:0));

    num_X  = X_g  - 1;
    num_Co = Co_g - 1;

    // here 256 = 32 * 2 *4 ( v32 bfloat16 4 times input_ptr++);
    param.inc_Ky  = -256 + step_Ky;

    //TODO
    param.inc_A_0 = chess_copy(param.Ky_g>2 ? param.inc_Ky : -256);

    // here -128 is when stride==2
    inc_Xi = - step_Ky * (param.Ky_g-1) + (stride >1 ? 0 : -128);
    inc_Ci = - step_Ky * (param.Ky_g-1) + (stride >1 ? 0 : -128) - 2*8*step_Xi * X_g + 2*step_Ci;
    inc_Yi = - step_Ky * (param.Ky_g-1) + (stride >1 ? 0 : -128) - 2*8*step_Xi * X_g - 2*step_Ci * num_Co + 2*step_Yi;

    SETUP_ITR3D_PARAM(iterator,inc_Xi,num_X,inc_Ci,num_Co,inc_Yi,param)

}

#endif  //__MAXPOOL2D_PARAMS_H__
