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
    

#ifndef __LAYER_NORM_PARAMS_H__
#define __LAYER_NORM_PARAMS_H__

#include "aie.hpp"
#include "layer_params.h"
#include <adf.h>


#ifndef PSUM_DM_BANK
#define PSUM_DM_BANK __aie_dm_resource_a
#endif

#ifndef MAT_DM_BANK
#define MAT_DM_BANK  __aie_dm_resource_b
#endif

#ifndef LN_LP_SIZE
#define LN_LP_SIZE  5
#endif

#ifndef LN_EPSILON
#define LN_EPSILON 0.000000000001 // 1e-12 (float32)
#endif


struct alignas(v16uint16) layer_norm_internal_params {    
    uint32_t mat_rows;
    uint32_t mat_cols;
    uint32_t row_offset;    
    uint32_t outer_count;
    uint32_t inner_count; 
    uint32_t mat_step_W;
    uint32_t gamma_offset;
    uint32_t beta_offset;
    uint32_t coeff_offset;
    uint32_t curr_depth_iter;
    uint32_t num_ifm_depth_iter;    
    DECL_ITR2D(iterator_mat)
    DECL_ITR2D(iterator_out)
    //part1 extra RTPs
    bfloat16 div_factor;
    uint32_t psum_in;
    uint32_t psum_out;  
    uint32_t io_mode;
    uint32_t state;

};

typedef struct layer_norm_internal_params layer_norm_params_t;
static void setup_layer_norm_iter_params(layer_norm_params_t &param) {
    param.gamma_offset = param.mat_cols*param.curr_depth_iter*2;
    //param.beta_offset  = (param.mat_cols * LN_NUM_DEPTH_ITERS*2) + param.mat_cols*param.curr_depth_iter*2;
    param.beta_offset  = (param.mat_cols * param.num_ifm_depth_iter*2) + param.mat_cols*param.curr_depth_iter*2;

    param.curr_depth_iter++;  
    
    //printf("param current_depth iter=%d\n", param.curr_depth_iter);
    // io_modes:
    // 0 : bfloat16 -> bfloat16
    // 1 : bfloat16 -> float32
    // 2 : float32  -> bfloat16
    // 3 : float32  -> float32 
    param.io_mode = (param.num_ifm_depth_iter>1)*((param.curr_depth_iter == 1) + 2*(param.curr_depth_iter == param.num_ifm_depth_iter) + 3*((param.curr_depth_iter > 1) && (param.curr_depth_iter < param.num_ifm_depth_iter)));         
    
    param.psum_in  = (param.io_mode==0 || param.io_mode==1) ? 0 : 1;
    param.psum_out = (param.io_mode==0 || param.io_mode==2) ? 0 : 1;    
      
    if(param.curr_depth_iter == param.num_ifm_depth_iter){
        param.curr_depth_iter=0; // Reset depth_iter
    }    
}

static INLINE void load_layer_norm_rtp(layer_norm_params_t &param, const uint32_t (&lp)[MLLIB_LP_SIZE]){
    param.mat_rows           = lp[0];
    param.mat_cols           = lp[1]; 
    param.num_ifm_depth_iter = lp[2];
    uint32_t div_factor      = lp[3];
    param.div_factor         = *((bfloat16 *)&div_factor);
    param.coeff_offset       = lp[4];   //size in bytes    
}


static INLINE void setup_layer_norm(layer_norm_params_t &param){   
    
    uint8_t mat_bits  = 16;
    
    param.curr_depth_iter = 0;
    
    param.mat_step_W = 4*(mat_bits/8)*param.mat_rows; //w4 vectorization * 2 bytes * num of row 
    
    param.inner_count = param.mat_cols >> (param.state ? 4 : 2);
    param.outer_count = param.mat_rows >> (param.state ? 2 : 4);
    int numA1         = param.inner_count - 1;    
    int incA1         = param.mat_step_W + (param.state ? 0 : -4*32);
    int incA2         = param.state ? (-3*param.mat_step_W -(4*numA1*incA1) + 32):(-param.mat_step_W * numA1);
      
    SETUP_ITR2D_PARAM(iterator_mat,incA1,numA1,incA2,param)
    SETUP_ITR2D_PARAM(iterator_out,incA1,numA1,incA2,param) 	    
}


static NOINLINE void setup_layer_norm_params(layer_norm_params_t &param, const uint32_t (&lp)[MLLIB_LP_SIZE]){   
   load_layer_norm_rtp(param, lp); 
   setup_layer_norm(param);   
}


#endif  //__LAYER_NORM_PARAMS_H__


