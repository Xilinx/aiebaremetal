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

#ifndef __PAD2D_H__
#define __PAD2D_H__

#include <adf.h>
#include "aie.hpp"

#include "pad2d_params.h"
#include "utils.h"

// NOTE: This kernel is used to initialize the boundary padded regions for Maxpool.
// For kernel_size>stride scenarios in Maxpool typically use SAME_UPPER padding
// Typically PAD_LEFT and PAD_TOP modes typically are not actively used for Fused Conv2D+Maxpool Op case.
// Hence disabling these fully functional TOP and LEFT pad modes by default.
// If needed in any specific case they could be activated by defining the following macros.
//#define  PAD_LEFT_EN
//#define  PAD_TOP_EN

#ifndef PAD_TOP_EN
#define PAD_TOP_EN    0
#endif

#ifndef PAD_RIGHT_EN
#define PAD_RIGHT_EN  0
#endif

#ifndef PAD_LEFT_EN
#define PAD_LEFT_EN   0
#endif

#ifndef PAD_BOT_EN
#define PAD_BOT_EN    0
#endif

// for int8/uint8 implementation
alignas(v16uint16) uint8 pad_mask_1   [32] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
alignas(v16uint16) uint8 pad_mask_2   [32] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
alignas(v16uint16) uint8 pad_mask_3   [32] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0,0,0,0,0,0,0,0};

// for bfloat16 implementation
alignas(v16uint16) uint16 pad_mask_bf_1   [32] = {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
alignas(v16uint16) uint16 pad_mask_bf_2   [32] = {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
alignas(v16uint16) uint16 pad_mask_bf_3   [32] = {0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0,0,0,0,0,0,0,0};

using namespace aie;

template <uint8_t batch_size, typename dtype>
INLINE void pad2d (
        dtype* __restrict ifm_data,
        dtype* __restrict ofm_data,
        pad2d_bf16_params_t &pad2d_params
) {
    //------------- bfloat16 implementation ------------------------//
    auto data_ptr = begin_vector<32>(ifm_data);
    auto input_ptr = begin_vector<32>(ifm_data);
    auto output_ptr = begin_vector<32>(ifm_data);

    int sv_x            = pad2d_params.sv_x     ;
    int sv_y            = pad2d_params.sv_y     ;
    int sv_ifm          = pad2d_params.nifms    ;
    bfloat16 pad_val    = pad2d_params.pad_val  ;
    int pad_left        = pad2d_params.pad_left ;
    int pad_right       = pad2d_params.pad_right;
    int pad_top         = pad2d_params.pad_top  ;
    int pad_bot         = pad2d_params.pad_bot  ;
    int numC            = (sv_ifm >>3);
    int numW;
    if constexpr(batch_size==2) {
        numW       = (sv_x >>1);
    }
    else{
        numW       = (sv_x >>2);
    }
    //v32dtype in_data;
    vector<dtype, 64> pad_val_0;
    vector<dtype, 32> pad_val_full;
    if constexpr(std::is_same<dtype, dtype>::value){
      pad_val_full  = broadcast<dtype, 32>(pad_val);
    }
    else{
      pad_val_0     = broadcast<dtype, 64>(pad_val);
      pad_val_full  = pad_val_0.template extract<32>(0);
    }

    //     now only support 2 values 0 or minimal of bf16

    // Kernel code pads only on the boundaries and does not iterate over the interior of the tensor.
    // Pad top
    // Enable the pipelining only if we have non-zero padding check in wrapper to ensure valid loop bounds.
    #ifdef PAD_TOP_EN
    for (int hcw=0; hcw<pad_top*numC*numW; hcw++)
    //chess_prepare_for_pipelining
    //chess_loop_range(3,)
    {
        *output_ptr++ = pad_val_full;
    }
    #endif

    #ifdef PAD_BOT_EN
    // Pad bottom
    // Enable the pipelining only if we have non-zero padding check in wrapper to ensure valid loop bounds.
    output_ptr  = data_ptr  + (sv_y-pad_bot)*numC*numW;
    for (int hcw=0; hcw<pad_bot*numC*numW; hcw++)
    //chess_prepare_for_pipelining
    //chess_loop_range(3,)
    {
        *output_ptr++ = pad_val_full;
    }
    #endif

    // Pad Left
    vector<dtype, 32>  mask;
    vector<dtype, 32>  mask_data, mask_pad, data_32, pad_32;
    int pad_lc;
    int w_gran;
    #ifdef PAD_LEFT_EN
      if constexpr(batch_size==2) {
          mask        = load_v<32>((dtype *)pad_mask_bf_2 );
          pad_lc      = pad_left >>1;
          w_gran      = 0x00000001;
      }
      else {
#ifdef __X86SIM__
          mask    = load_floor_v<32>((dtype *)((pad_left%4==1) ? pad_mask_bf_1  : ((pad_left%4==2)? pad_mask_bf_2 : pad_mask_bf_3)));
#else
          mask    = load_v<32>((dtype *)((pad_left%4==1) ? pad_mask_bf_1  : ((pad_left%4==2)? pad_mask_bf_2 : pad_mask_bf_3)));
#endif
          pad_lc      = pad_left >>2;
          w_gran      = 0x00000003;
      }

    mask_pad  = mask;
    mask_data = bneg(mask_pad);

    output_ptr  = data_ptr +(pad_top)*numC*numW;
    for (int hc=0; hc<(sv_y-pad_top-pad_bot)*numC; hc++) {

        for (int w=0; w < pad_lc; w++) {
            *output_ptr++ = pad_val_full;
        }
        //if(pad_left%w_gran!=0){
        if((pad_left & w_gran)!=0){
            data_32 = *output_ptr;
            pad_32  = band(mask_pad,  pad_val_full);
            data_32 = band(mask_data, data_32);
            data_32 = bor(pad_32, data_32);
            *output_ptr = data_32;
        }
        output_ptr = output_ptr - (pad_lc) + numW;
    }
    #endif


    #ifdef PAD_RIGHT_EN
    // Pad Right
      if constexpr(batch_size==2) {
          mask        = load_v<32>((dtype *)pad_mask_bf_2)  ;
          pad_lc      = pad_right >>1;
          w_gran      = 0x00000001;
      }
      else {
#ifdef __X86SIM__
          mask        = load_floor_v<32>((dtype *)((pad_right%4==1) ? pad_mask_bf_3  : ((pad_right%4==2)? pad_mask_bf_2 : pad_mask_bf_1)));
#else
          mask        = load_v<32>((dtype *)((pad_right%4==1) ? pad_mask_bf_3  : ((pad_right%4==2)? pad_mask_bf_2 : pad_mask_bf_1)));
#endif
          pad_lc      = pad_right >>2;
          w_gran      = 0x00000003;
      }

    mask_data = mask;
    mask_pad = bneg(mask_data);
    output_ptr  = data_ptr + ((pad_top)*numW*numC + numW - pad_lc);
    for (int hc=0; hc<(sv_y-pad_top-pad_bot)*numC; hc++) {
        if((pad_right & w_gran)!=0){
            output_ptr--;
            data_32 = *output_ptr;
            pad_32  = band(mask_pad,  pad_val_full);
            data_32 = band(mask_data, data_32);
            data_32 = bor(pad_32, data_32);
            *output_ptr++ = data_32;
        }
        for (int w=0; w<(pad_lc); w++) {
            *output_ptr++ = pad_val_full;
        }
        output_ptr = output_ptr - (pad_lc) + numW;
    }
    #endif
}

template <uint8_t batch_size, typename dtype, typename config_ifm, typename config_ofm>
void pad2d (
        adf::input_buffer_conf<dtype, config_ifm>  & __restrict ifm,
        adf::output_buffer_conf<dtype, config_ofm>  & __restrict ofm,
        pad2d_bf16_params_t &pad2d_params
) {
    pad2d<batch_size, dtype>(ifm.data(), ofm.data(), pad2d_params);
}

template <uint8_t batch_size, typename dtype>
INLINE void pad2d (
        dtype* __restrict ifm_data,
        dtype* __restrict ofm_data,
        pad2d_params_t &pad2d_params
) {
    v32int8 DM_BANK(a) * data_ptr = (v32int8 DM_BANK(a) *) ifm_data;
    v32int8 * input_ptr   = (v32int8 *)data_ptr;
    v32int8 * output_ptr  = (v32int8 *)data_ptr;

    int sv_x       = pad2d_params.sv_x     ;
    int sv_y       = pad2d_params.sv_y     ;
    int sv_ifm     = pad2d_params.nifms    ;
    int pad_val    = pad2d_params.pad_val  ;
    int pad_left   = pad2d_params.pad_left ;
    int pad_right  = pad2d_params.pad_right;
    int pad_top    = pad2d_params.pad_top  ;
    int pad_bot    = pad2d_params.pad_bot  ;
    int numC       = (sv_ifm >>3);
    int numW;
    if constexpr(batch_size==2) {
        numW       = (sv_x >>1);
    }
    else{
        numW       = (sv_x >>2);
    }
    v32int8 in_data;
    v64int8 pad_val_0     = broadcast_to_v64int8((int8)pad_val);
    v32int8 pad_val_full  = extract_v32int8(pad_val_0,0);


    // Kernel code pads only on the boundaries and does not iterate over the interior of the tensor.
    // Pad top
    // Enable the pipelining only if we have non-zero padding check in wrapper to ensure valid loop bounds.
    #if PAD_TOP_EN == 1
    for (int hcw=0; hcw<pad_top*numC*numW; hcw++)
    //chess_prepare_for_pipelining
    //chess_loop_range(3,)
    {
        *output_ptr++ = pad_val_full;
    }
    #endif

    #if PAD_BOT_EN == 1
    // Pad bottom
    // Enable the pipelining only if we have non-zero padding check in wrapper to ensure valid loop bounds.
    output_ptr  = (v32int8 *)data_ptr  + (sv_y-pad_bot)*numC*numW;
    for (int hcw=0; hcw<pad_bot*numC*numW; hcw++)
    //chess_prepare_for_pipelining
    //chess_loop_range(3,)
    {
        *output_ptr++ = pad_val_full;
    }
    #endif

    // Pad Left
    v32int8 mask;
    v64int8 mask_64_data, mask_64_pad, data_64, pad_64;
    int pad_lc;
    int w_gran;
    #if PAD_LEFT_EN == 1
    if constexpr(batch_size==2) {
        mask        = *(v32int8*)pad_mask_2  ;
        pad_lc      = pad_left >>1;
        w_gran      = 0x00000001;
    }
    else {
        mask    = *(v32int8*)((pad_left%4==1) ? pad_mask_1  : ((pad_left%4==2)? pad_mask_2 : pad_mask_3));
        pad_lc      = pad_left >>2;
        w_gran      = 0x00000003;
    }
    mask_64_pad  = concat(mask, mask);
    mask_64_data = bneg(mask_64_pad);

    output_ptr  = (v32int8 *)data_ptr +(pad_top)*numC*numW ;
    for (int hc=0; hc<(sv_y-pad_top-pad_bot)*numC; hc++) {
        for (int w=0; w < pad_lc; w++) {
            *output_ptr++ = pad_val_full;
        }
        //if(pad_left%w_gran!=0){
        if((pad_left & w_gran)!=0){
            data_64 = concat(*output_ptr, *output_ptr);
            pad_64  = band(mask_64_pad,  pad_val_0);
            data_64 = band(mask_64_data, data_64);
            data_64 = bor(pad_64,data_64);
            *output_ptr = extract_v32int8(data_64,0);
        }
        output_ptr = output_ptr - (pad_lc) + numW;
    }
    #endif


    #if PAD_RIGHT_EN == 1
    // Pad Right
    if constexpr(batch_size==2) {
        mask        = *(v32int8*)pad_mask_2  ;
        pad_lc      = pad_right >>1;
        w_gran      = 0x00000001;
    }
    else {
        mask        = *(v32int8*)((pad_right%4==1) ? pad_mask_3  : ((pad_right%4==2)? pad_mask_2 : pad_mask_1));
        pad_lc      = pad_right >>2;
        w_gran      = 0x00000003;
    }
    mask_64_data = concat(mask, mask);
    mask_64_pad = bneg(mask_64_data);
    output_ptr  = (v32int8 *)data_ptr +(pad_top)*numW*numC + numW - pad_lc;
    for (int hc=0; hc<(sv_y-pad_top-pad_bot)*numC; hc++) {
        if((pad_right & w_gran)!=0){
            output_ptr--;
            data_64 = concat(*output_ptr, *output_ptr);
            pad_64  = band(mask_64_pad,  pad_val_0);
            data_64 = band(mask_64_data, data_64);
            data_64 = bor(pad_64,data_64);
            *output_ptr++ = extract_v32int8(data_64,0);
        }
        for (int w=0; w<(pad_lc); w++) {
            *output_ptr++ = pad_val_full;
        }
        output_ptr = output_ptr - (pad_lc) + numW;
    }
    #endif
}

template <uint8_t batch_size, typename dtype, typename config_ifm, typename config_ofm>
void pad2d (
        adf::input_buffer_conf<dtype, config_ifm>  & __restrict ifm,
        adf::output_buffer_conf<dtype, config_ofm>  & __restrict ofm,
        pad2d_params_t &pad2d_params
) {
    pad2d<batch_size, dtype>(ifm.data(), ofm.data(), pad2d_params);
}

#endif //__PAD2D_H__
