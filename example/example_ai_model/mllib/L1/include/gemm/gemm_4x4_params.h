/*
    Copyright (C) 2014 - 2022 Xilinx, Inc. All rights reserved.
    Copyright (C) 2025 Advanced Micro Devices, Inc.
   
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
    

#ifndef __GEMM_4x4_PARAMS_H__
#define __GEMM_4x4_PARAMS_H__

#include "aie.hpp"
#include <adf.h>
#include "mllib_const.h"

// Basic matrix mult dimensions for AIE-ML
#define GEMM_4x4_DIM_M 4
#define GEMM_4x4_DIM_K 8
#define GEMM_4x4_DIM_N 4

// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef GEMM_4x4_MAT_A_DM_BANK
#define GEMM_4x4_MAT_A_DM_BANK __aie_dm_resource_a
#endif

#ifndef GEMM_4x4_MAT_B_DM_BANK
#define GEMM_4x4_MAT_B_DM_BANK __aie_dm_resource_a
#endif

#ifndef GEMM_4x4_MAT_C_DM_BANK
#define GEMM_4x4_MAT_C_DM_BANK __aie_dm_resource_b
#endif

#ifndef GEMM_4x4_PSUM_DM_BANK
#define GEMM_4x4_PSUM_DM_BANK  __aie_dm_resource_c
#endif

#ifndef GEMM_4x4_LAYER_PARAM_BYTES
#define GEMM_4x4_LAYER_PARAM_BYTES   0
#endif

#ifndef GEMM_4x4_PSUM_BUFF_OFFSET
#define GEMM_4x4_PSUM_BUFF_OFFSET    8192  
#endif

struct alignas(v16uint16) gemm_4x4_internal_params {
    
    uint16_t mat_a_rows;
    uint16_t inner_dim;
    uint16_t mat_b_cols;
    
    int outer_count;
    int inner_count;

    int incA1;
    int incB1;  
    
    int psum_buff_offset; 
    
    uint8_t psum_in;
    
    uint8_t bias_in;
    int     bias_len;    
    uint8_t bias_zero_flag;    
    
    uint8_t io_mode;
    uint8_t curr_depth_iter;
    uint8_t num_ifm_depth_iter;
    
    DECL_ITR3D(iterator_mat_a)
    DECL_ITR3D(iterator_mat_b)
    DECL_ITR2D(iterator_mat_c)
    DECL_ITR2D(iterator_bias)  
};

typedef struct gemm_4x4_internal_params gemm_4x4_params_t;


static void setup_gemm_4x4_iter_params(gemm_4x4_params_t &param) {

    param.curr_depth_iter++;  

    // io_modes:
    // 0 : bfloat16 -> bfloat16
    // 1 : bfloat16 -> float32
    // 2 : float32  -> bfloat16
    // 3 : float32  -> float32 
    param.io_mode = (param.num_ifm_depth_iter>1)*((param.curr_depth_iter == 1) + 2*(param.curr_depth_iter == param.num_ifm_depth_iter) + 3*((param.curr_depth_iter > 1) && (param.curr_depth_iter < param.num_ifm_depth_iter)));         
        
    param.psum_in  = (param.io_mode==0 || param.io_mode==1) ? 0 :1;
    
    param.bias_zero_flag = param.psum_in || (1-param.bias_in);
      
    param.curr_depth_iter = (param.curr_depth_iter == param.num_ifm_depth_iter) ? 0 : param.curr_depth_iter;   
}


// function below is with actual RTPs
static NOINLINE void load_gemm_4x4_rtp(gemm_4x4_params_t &param,const uint32_t (&lp)[MLLIB_LP_SIZE]){

    param.mat_a_rows         = lp[0];
    param.inner_dim          = lp[1];
    param.mat_b_cols         = lp[2];
    param.num_ifm_depth_iter = lp[3];
    param.bias_in            = lp[4];
    param.psum_buff_offset   = GEMM_4x4_PSUM_BUFF_OFFSET;    
}


static NOINLINE void setup_gemm_4x4(gemm_4x4_params_t &param){   
    
    uint8_t mat_a_bits  = 16; //same for mat_b
    uint8_t mat_c_bits  = 16;
    uint8_t bias_bits   = 16;
    uint8_t psum_bits   = 32;
    
    uint8_t mat_c_row_gran = 4; //Output row granularity
    uint8_t mat_c_col_gran = 4; //Output col granularity    
    
    param.bias_len    = param.bias_in*(bias_bits/8)*param.mat_b_cols;
    int bias_temp     = param.bias_len & (int)31;    
    int bias_len_adj  = bias_temp > 0 ? ((int)32 - bias_temp) : 0;
    param.bias_len    = param.bias_len + bias_len_adj;
    
    param.inner_count = param.inner_dim / 8;
    param.outer_count = param.mat_a_rows * param.mat_b_cols / (mat_c_row_gran * mat_c_col_gran);               
    
    int mat_a_step_C     =  4*(mat_a_bits/8)*param.mat_a_rows; //c4 vectorization * 2 bytes * num of row
    param.incA1          =  mat_a_step_C;
    int numA1            =  param.inner_dim / 8  - 1;
    int incA1            =  mat_a_step_C; //Move along column/inner dimension of mat A 
    int numA2            =  param.mat_a_rows / 4 - 1;
    int incA2            =  (-2*mat_a_step_C*numA1) - mat_a_step_C + 32 ; //Move along the row dimension of mat A
    int incA3            =  (-2*mat_a_step_C*numA1) - mat_a_step_C - (32*numA2); //Revert to the beginning of mat A
    SETUP_ITR3D_PARAM(iterator_mat_a,incA1,numA1,incA2,numA2,incA3,param)            
    
    int mat_b_step_C     =  4*(mat_a_bits/8)*param.inner_dim; //c4 vectorization * 2 bytes * num of row    
    param.incB1          =  32;    
    int numB1            =  numA1;
    int incB1            =  32; //Move along row/inner dimension of mat B      
    int numB2            =  numA2;
    int incB2            =  -64*numA1 -32 ;//Revert back to begining of row/inner dimension of matB
    int incB3            =  -64*numA1 -32 + mat_b_step_C; //Move along col dimension of mat B.           
    SETUP_ITR3D_PARAM(iterator_mat_b,incB1,numB1,incB2,numB2,incB3,param)
              
    int numBias1          = numA2;
    int incBias1          = 0;
    int incBias2          = 4*2;
    SETUP_ITR2D_PARAM(iterator_bias,incBias1,numBias1,incBias2,param)                   
}
    
//With RTP
static NOINLINE void setup_gemm_4x4_params(gemm_4x4_params_t &param,const uint32_t (&lp)[MLLIB_LP_SIZE]){   
   load_gemm_4x4_rtp(param,lp); 
   setup_gemm_4x4(param);   
}


#endif  //__GEMM_4x4_PARAMS_H__


