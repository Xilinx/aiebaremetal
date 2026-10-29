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

#ifndef __GEMV_PARAMS_H__
#define __GEMV_PARAMS_H__

//#include "layer_params.h"
#include "utils.h"


// Basic matrix mult dimensions for AIE-ML
#define DIM_M 4
#define DIM_N 8
#define DIM_K 8

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef WT_DM_BANK
#define WT_DM_BANK __aie_dm_resource_a
#endif

#ifndef OFM_DM_BANK
#define OFM_DM_BANK
#endif


// Bias is 16-bit
#define BIAS_BYTES 2
#define OFM_SETS 2


#define LAYER_PARAM_BYTES 64 
#define LAYER_PARAM_WORDS (LAYER_PARAM_BYTES>>2) 
#define ITER_PARAM_BYTES 32 
#define ITER_PARAM_WORDS (ITER_PARAM_BYTES>>2) 
#define ITER_PARAM_OFFSET 19
#define BIAS_MULTIPLE   2 


#define CONV2D_LAYER1  1

//typedef enum {
//  FULL,
//  PARTIAL
//} out_mode_t ;


struct alignas(v16uint16) gemv_internal_params {
    int hdr_len ; // larger than 8 bit so retaining
    uint8_t num_ifm_depth_iter;
    uint8_t curr_depth_iter ;
    uint8_t io_mode;
    uint8_t ifm;
    uint16_t ofm_len; 
    uint16_t ifm_len; 
    uint8_t exec_type; //  layer-1, maxpool2d etc
    uint16_t inner_g; 
    // Retaining as is belowa already uint8s
    int8_t shift_psum_in;
    int8_t shift_psum_out;
    int8_t shift_out;
    int8_t psum_in;
    int8_t psum_out;
    int    psum_buff_offset; 

};

typedef struct gemv_internal_params gemv_params_t ;

//INLINE int min(int a, int b) { return a<b ? a : b; }
//INLINE int max(int a, int b) { return a>b ? a : b; }


static INLINE void setup_gemv_iter_params(gemv_params_t &param) {
        param.curr_depth_iter++;
        
        if(param.num_ifm_depth_iter == 1)
        {       
             param.psum_in               = 0; 
             param.psum_out              = 0; 
             param.exec_type             = 0;
 
        }
        else if(param.curr_depth_iter == 1)
        {               
             param.psum_in               = 0; 
             param.psum_out              = 1; 
             param.exec_type             = 1;


        }
        else if(param.curr_depth_iter < param.num_ifm_depth_iter)
        {
             param.psum_in               = 1; 
             param.psum_out              = 1; 
             param.exec_type             = 1;
 
        }
        else
        {
             param.psum_in               = 1; 
             param.psum_out              = 0; 
             param.exec_type             = 0;

        }

    //printf("\n param.exec_type  = %d\n", param.exec_type );
    //printf("\n psum_in  = %d, psum_out = %d\n", param.psum_in,param.psum_out ); 
    if(param.curr_depth_iter==param.num_ifm_depth_iter){
       param.curr_depth_iter=0; // Reset depth_iter
    }       
}


static INLINE void setup_gemv_params (int8_t * params_ptr, gemv_params_t &param, uint8_t batch_size) {

	params_ptr++;
	params_ptr++;
	uint8_t nifms               = (uint8_t) *params_ptr++;
	uint8_t nofms               = (uint8_t) *params_ptr++;
	params_ptr++;
	params_ptr++;
    	param.shift_out             = (int8_t)  *params_ptr++;
	params_ptr++;
	params_ptr++;
    	param.shift_psum_in         = (int8_t)  *params_ptr++;
    	param.shift_psum_out        = (int8_t)  *params_ptr++;
    	param.psum_buff_offset      = ((int8_t) *params_ptr++)*128;
     	params_ptr += 2;
	params_ptr++;
	params_ptr++;
	params_ptr++;               
	params_ptr++;  
    	param.num_ifm_depth_iter    = (uint8_t) *params_ptr++;   
    	params_ptr++; 
    	param.ofm_len               = * ( (uint16_t *) params_ptr);               
    	params_ptr+=2; 
	params_ptr++;  
	params_ptr++;   
    	param.ifm_len               = * ( (uint16_t *) params_ptr);              
    	params_ptr += 2;
    	params_ptr++; 
    	param.ifm                   = nifms;     
    	param.curr_depth_iter       = 0; 
    	param.hdr_len               = (LAYER_PARAM_WORDS+nofms*BIAS_MULTIPLE);
    	param.inner_g               = nifms >> 3;
}

#endif  //__CONV2D_PARAMS_H__
