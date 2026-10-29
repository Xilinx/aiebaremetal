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

#ifndef __CONV2D_1x8x8_H__
#define __CONV2D_1x8x8_H__

#include <adf.h>
#include "aie.hpp"

#include "utils.h"         // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"   // Constants shared across graphs like activation type
#include "adf/io_buffer/io_buffer_access.h" // temporary utility from tools to access ping pong buffers, to be removed


#include "./conv2d_1x8x8/direct_conv_int16x8_generic_kernel.c"

using namespace aie;

template <uint8_t batch_size, act_t act_type, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm> 
void conv2d
(
        adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict ifm,
        adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict wts,
        adf::output_buffer_conf<dtype_ofm, config_ofm> &  ofm,
        KernelParams            &conv2d_params,
        bool final_tdm_iter =0
)
{
    int lp=*((int32_t *)wts.data());
    int tmp_w=lp & 0x000000ff;
    int tmp_h=(lp & 0x0000ff00)>>8;
    int tmp_d=(lp & 0x00ff0000)>>16;
    tmp_w=((tmp_w*2*8+63)>>6)<<6;
    int sv_offset= tmp_w*tmp_h*tmp_d;
    //int * tdm1  = (int32 *)(0x70000 + 11520);
    //int * tdm2  = (int32 *)(0x74000 + 11520); 
    int * tdm1  = (int32 *)(0x70000 + sv_offset);
    int * tdm2  = (int32 *)(0x74000 + sv_offset); 
   // printf("\nsv_offset=%i", sv_offset);

    int * input    = (int *)ifm.data();
    int * weights  = (int *)wts.data()+ 16;//Skip the initial 64 byte header.
    int * output   = (int *)ofm.data();

    //direct_conv_int16x8_generic(input, weights, tdm1, tdm2, output, conv2d_params);
    direct_conv_int16x8_generic(input, weights, tdm1, tdm2, output, conv2d_params,final_tdm_iter);

}


#endif //__CONV2D_1x8x8_H__

