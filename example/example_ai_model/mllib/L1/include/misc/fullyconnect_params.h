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
    

#ifndef __FULLYCONNECT_PARAMS_H__
#define __FULLYCONNECT_PARAMS_H__

#include "aie.hpp"
#include <adf.h>
#include "mllib_const.h"

#if __AIE_ARCH__ == 20
// for bfloat16, int8 high leval API is not effecient for FC
// Basic matrix mult dimensions for AIE-ML
#define FC_DIM_M 4
#define FC_DIM_K 8
#define FC_DIM_N 4
#else
// Basic matrix mult dimensions for AIE2ps
#define FC_DIM_M 4
#define FC_DIM_K 8
#define FC_DIM_N 8
#endif
// Buffer placement directives for IFM, WTs and OFM buffers.
#ifndef MAT_A_DM_BANK
#define MAT_A_DM_BANK __aie_dm_resource_a
#endif

#ifndef MAT_B_DM_BANK
#define MAT_B_DM_BANK __aie_dm_resource_a
#endif

#ifndef MAT_C_DM_BANK
#define MAT_C_DM_BANK __aie_dm_resource_b
#endif

#ifndef PSUM_DM_BANK
#define PSUM_DM_BANK  __aie_dm_resource_c
#endif

#ifndef FC_LAYER_PARAM_BYTES
#define FC_LAYER_PARAM_BYTES   64
#endif

//#define FC_TRANSPOSE_MAT_B_SUPPORT 0 //Set this to 1 if FC kernel has to support Mat-B transpose at Runtime


///////////////////Runtime parameters/////////////////////////////
#ifndef INNER_DIM
#define INNER_DIM   64
#endif

#ifndef MAT_A_ROWS
#define MAT_A_ROWS  1
#endif

#ifndef MAT_B_COLS
#define MAT_B_COLS  128
#endif

#ifndef FC_NUM_DEPTH_ITERS
#define FC_NUM_DEPTH_ITERS  1
#endif

#ifndef FC_BIAS_IN
#define FC_BIAS_IN  0
#endif

#ifndef FC_INT8_SHIFT_OUT
#define FC_INT8_SHIFT_OUT  0
#endif

#ifndef FC_LP_SIZE
#define FC_LP_SIZE 8
#endif

/////////////////////////////////////////////////////////////////

struct alignas(v16uint16) fullyconnect_internal_params {
    uint16_t mat_a_rows;
    uint16_t inner_dim;
    uint16_t mat_b_cols; 
    
    int outer_count;
    int inner_count;

    int incB1;
    int incB2;
    int incB_int8;

    int8_t int8_shift_out;    
    //uint8_t psum_in;//
    
    uint8_t bias_in;//bias_flag
    int     bias_len;    
    uint8_t bias_zero_flag;    
    
    uint8_t io_mode;
    uint8_t curr_depth_iter;
    uint8_t num_ifm_depth_iter;
    
    DECL_ITR3D(iterator_mat_b)
};


typedef struct fullyconnect_internal_params fullyconnect_params_t;

inline void setup_fullyconnect_iter_params(fullyconnect_params_t &param) {

    param.curr_depth_iter++;  

    // io_modes:
    // 0 : bfloat16 -> bfloat16, int8 -> int8
    param.io_mode = 0;         
        
    param.bias_zero_flag = (1-param.bias_in);
      
    param.curr_depth_iter = (param.curr_depth_iter == param.num_ifm_depth_iter) ? 0 : param.curr_depth_iter;   
}

//Compile Time Param Loading
static NOINLINE void load_fullyconnect_rtp(fullyconnect_params_t &param){

    param.mat_a_rows         = MAT_A_ROWS*4;
    param.inner_dim          = INNER_DIM;
    param.mat_b_cols         = MAT_B_COLS;
    param.num_ifm_depth_iter = FC_NUM_DEPTH_ITERS;
    param.bias_in            = FC_BIAS_IN;
    param.int8_shift_out     = FC_INT8_SHIFT_OUT;   
    //param.psum_buff_offset   = FC_PSUM_BUFF_OFFSET; //4*MAT_A_ROWS*4*(mat_c_bits/8);//Offset of 4*Rc4 
}

//Run Time Param Loading
static NOINLINE void load_fullyconnect_rtp(fullyconnect_params_t &param, uint32_t *params_ptr){

    param.mat_a_rows         = params_ptr[0]*4;
    param.inner_dim          = params_ptr[1];
    param.mat_b_cols         = params_ptr[2]; 
    param.num_ifm_depth_iter = params_ptr[3];
    param.bias_in            = params_ptr[4];
    param.int8_shift_out     = params_ptr[5];
    //param.psum_buff_offset   = FC_PSUM_BUFF_OFFSET; //4*MAT_A_ROWS*4*(mat_c_bits/8);//Offset of 4*Rc4     
}

#if __AIE_ARCH__ == 20

template <typename dtype>
NOINLINE void setup_fullyconnect(fullyconnect_params_t &param){   
    
    uint8_t dtype_bits; //same for mat_b bias
    uint8_t mat_c_row_gran = 4; //Output row granularity
    uint8_t mat_c_col_gran;// 16 //Output col granularity
    uint8_t inner_min_ld; 
    if constexpr(std::is_same<bfloat16, dtype>::value) {
        dtype_bits = 16;// bf16 16bits
        mat_c_col_gran = 16; // 4*4 for matc
        inner_min_ld = 8;
    }else{     
        dtype_bits = 8;
        mat_c_col_gran = 32; // 4*8 for matc
        inner_min_ld = 16; //to match min load v16int8 mat_a  
    }
    
    param.bias_len    = param.bias_in*(dtype_bits/8)*param.mat_b_cols;
    param.inner_count = param.inner_dim / inner_min_ld;
    param.outer_count = param.mat_a_rows * param.mat_b_cols / (mat_c_row_gran * mat_c_col_gran);               
   
    // for mat_b          
    int mat_b_step_C     =  4*(dtype_bits/8)*param.inner_dim; //c4 vectorization * 2 bytes * num of row    
    param.incB1          =  32;
    param.incB2          =  -1*32 + mat_b_step_C;
    int numB1            =  param.inner_dim / 8 - 1; // w8 sv
    int incB1            =  1*32 - 3*mat_b_step_C; //Move along row/inner dimension of mat B      
    int incB2            =  32; 
    // for int8
    param.incB_int8      =  mat_b_step_C;

    if constexpr(std::is_same<bfloat16, dtype>::value) {
        incB1            =  1*32 - 3*mat_b_step_C; 
    }else{
        incB1            =  1*32 - 7*mat_b_step_C; 
    }      
      
    SETUP_ITR2D_PARAM(iterator_mat_b,incB1,numB1,incB2,param)
 
}

#else
// for fullyconnect bfloat16 AIE2ps / AIEp kernel

template <typename dtype>
NOINLINE void setup_fullyconnect(fullyconnect_params_t &param){   
    uint8_t dtype_bits; //same for mat_b bias
    uint8_t mat_c_row_gran = 4; //Output row granularity
    uint8_t mat_c_col_gran;// 16 //Output col granularity
    uint8_t inner_min_ld; 
    if constexpr(std::is_same<bfloat16, dtype>::value) {
        dtype_bits = 16;// bf16 16bits
        mat_c_col_gran = 32; // 4*8 for matc
        inner_min_ld = 8;
    }else{     
        dtype_bits = 8;
        mat_c_col_gran = 16;//32; // 4*8 for matc, actrully 8*8
        inner_min_ld = 16; //to match min load v16int8 mat_a  
    } 
    
    param.bias_len    = param.bias_in*(dtype_bits/8)*param.mat_b_cols;
    param.inner_count = param.inner_dim / inner_min_ld;
    param.outer_count = param.mat_a_rows * param.mat_b_cols / (mat_c_row_gran * mat_c_col_gran);               
    
    // for mat_b           
    int mat_b_step_C     =  4*(dtype_bits/8)*param.inner_dim; //c4 vectorization * 2 bytes * num of row    
    param.incB1          =  mat_b_step_C;   
    int numB1            =  param.inner_dim / 8 - 1; // w8 sv
    int incB1            =  1*64 - 7*mat_b_step_C; //Move along row/inner dimension of mat B      
    int incB2            =  64;     
    // for int8
    param.incB_int8      =  mat_b_step_C;

    if constexpr(std::is_same<bfloat16, dtype>::value) {
        incB1            =  1*64 - 7*mat_b_step_C;
        incB2            =  64;  
    }else{
        incB1            =  1*32 - 3*mat_b_step_C;
        incB2            =  32;  
    }   

    SETUP_ITR2D_PARAM(iterator_mat_b,incB1,numB1,incB2,param)          
}

#endif

//Without RTP- params fixed at compile time
template <typename dtype>
NOINLINE void setup_fullyconnect_params(fullyconnect_params_t &param){
    load_fullyconnect_rtp(param);
    setup_fullyconnect<dtype>(param); 
}

//With RTP
template <typename dtype>
NOINLINE void setup_fullyconnect_params(fullyconnect_params_t &param, uint32_t *params_ptr) {
   load_fullyconnect_rtp(param, params_ptr);
   setup_fullyconnect<dtype>(param);
}

#endif  //__FULLYCONNECT_PARAMS_H__


