// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*  (c) Copyright 2019 - 2021 Xilinx, Inc. All rights reserved.
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
    PART OF THIS FILE AT ALL TIMES.
*/

/*
 * A wrapper to show 3x3 conv2d(Converted from 1x1 conv2d) on 4x4 overlay.
*/

#include <adf.h>
#include "mllib_kernels.h"

// Parameters used to configure kernels at run time
conv2d_params_t    curr_layer;

static int curr_iter = 0 ;
static int depth_iter = 0 ;
static int num_depth_iter = 1; //default
static int num_iter = 1; //default

// Init function for rounding and saturation
void node_init() {
    tile::current().set_saturation(saturation_mode::truncate);
    tile::current().set_rounding(rounding_mode::conv_even);
}

template <uint8_t batch_size, act_t act_type, typename dtype_ifm, typename dtype_wts, typename dtype_ofm, typename config_ifm, typename config_wts, typename config_ofm> void conv2d_4x4_wrapper
(
        adf::input_buffer_conf<dtype_ifm, config_ifm>  & __restrict ifm,
        adf::input_buffer_conf<dtype_wts, config_wts>  & __restrict wts,
        adf::output_buffer_conf<dtype_ofm, config_ofm> & __restrict ofm
) {
		printf("conv2d_4x4_wrapper\n");
    if(curr_iter==0){
        // Conditionally acquire kernel parameters only at the start of first iteration
        // Setup increments and other kernel params
        // Do this only in the beginning (first iteration)
        setup_conv2d_params((int8 *)wts.data(),curr_layer,batch_size);

        num_depth_iter = curr_layer.num_ifm_depth_iter;
        int num_iter_out = curr_layer.num_width_iter*curr_layer.num_height_iter*curr_layer.num_chout_iter;
        num_iter = num_iter_out*curr_layer.num_ifm_depth_iter;
    }

    depth_iter++;

    // Acquire OFM at the beginning of depth_iter
    if(depth_iter==1) {
        ofm.acquire();
    }
    conv2d <batch_size, act_type,dtype_ifm,dtype_wts,dtype_ofm,config_ifm,config_wts,config_ofm>(
           ifm, wts, ofm, curr_layer );

    curr_iter++;

    // Release OFM at the end of depth_iter
    if(depth_iter==num_depth_iter){
       ofm.release();
       depth_iter=0; // Reset depth_iter
    }

    if(curr_iter==num_iter)
       curr_iter=0;  // Reset curr_iter
}
