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
    

#ifndef __GEMM_PARAMS_H__
#define __GEMM_PARAMS_H__

#include "aie.hpp"
#include <adf.h>
#include "mllib_const.h"

#define GEMM_INT_DIM_M 8
#define GEMM_INT_DIM_K 8
#define GEMM_INT_DIM_N 8

#if __AIE_ARCH__ == 20
// Basic matrix mult dimensions for AIE-ML
#define GEMM_DIM_M 4
#define GEMM_DIM_K 8
#define GEMM_DIM_N 4
#define GEMM_BIAS_DIM_M 4
#define GEMM_BIAS_DIM_K 2
#define GEMM_BIAS_DIM_N 8
#else
// Basic matrix mult dimensions for AIE2ps
#define GEMM_DIM_M 4
#define GEMM_DIM_K 8
#define GEMM_DIM_N 8
#define GEMM_BIAS_DIM_M 8
#define GEMM_BIAS_DIM_K 2
#define GEMM_BIAS_DIM_N 8
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

#ifndef GEMM_LAYER_PARAM_BYTES
#define GEMM_LAYER_PARAM_BYTES       0
#endif

#define GEMM_TRANSPOSE_MAT_B_SUPPORT 1 //Set this to 1 if GEMM kernel has to support Mat-B transpose at Runtime


///////////////////Runtime parameters/////////////////////////////
#ifndef INNER_DIM
#define INNER_DIM   64
#endif

#ifndef MAT_A_ROWS
#define MAT_A_ROWS  64
#endif

#ifndef MAT_B_COLS
#define MAT_B_COLS  64
#endif

#ifndef GEMM_NUM_DEPTH_ITERS
#define GEMM_NUM_DEPTH_ITERS  1
#endif

#ifndef GEMM_BIAS_IN
#define GEMM_BIAS_IN  0
#endif

#ifndef GEMM_PSUM_BUFF_OFFSET
#define GEMM_PSUM_BUFF_OFFSET 8192  
#endif

#ifndef GEMM_TRANSPOSE_MAT_B
#define GEMM_TRANSPOSE_MAT_B 0
#endif

#ifndef GEMM_LP_SIZE
#define GEMM_LP_SIZE 8
#endif

#ifndef GEMM_ACT
#define GEMM_ACT 0
#endif

#ifndef GEMM_INT_LP_SIZE
#define GEMM_INT_LP_SIZE 11
#endif

#if defined(AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16) && (AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16 == 0)
  #undef AIE_API_EMULATE_BFLOAT16_MMUL_WITH_BFP16
#endif 

/////////////////////////////////////////////////////////////////


#if __AIE_ARCH__ == 20

struct alignas(v16uint16) gemm_internal_params {
    
    uint8_t mat_a_rows;
    uint8_t inner_dim;
    uint8_t mat_b_cols;
    uint8_t act;
    
    int outer_count;
    int inner_count;

    int incAI;
    int incB1;
    int incB2;
    int incC1;    
    int psum_incC1;    
    
    int psum_buff_offset; 
    
    uint8_t mat_b_transpose;
    uint8_t mat_b_Imode_0;
    
    uint8_t psum_in;
    
    uint8_t bias_in;
    int     bias_len;    
    uint8_t bias_zero_flag;    
    
    uint8_t io_mode;
    uint16_t curr_depth_iter;
    uint16_t num_ifm_depth_iter;
    
    int8_t bias_init_shift;
    uint8_t shift_out;
    uint8_t out_sign;
    uint8_t ifm_sign;
    
    DECL_ITR3D(iterator_mat_a)
    DECL_ITR3D(iterator_mat_b)
    DECL_ITR2D(iterator_mat_c)
    DECL_ITR2D(iterator_psum_in)
    DECL_ITR2D(iterator_psum_out)  
};

#else

struct alignas(v16uint16) gemm_internal_params {
    
    uint8_t mat_a_rows;
    uint8_t inner_dim;
    uint8_t mat_b_cols;
    uint8_t act;
    
    int outer_count;
    int inner_count;

    int incAI;
    int incB1;
    int incB2;
    int incC1;    
    int psum_incC1;    
    
    int psum_buff_offset; 
    
    uint8_t mat_b_transpose;
    uint8_t mat_b_Imode_0;
    uint8_t mat_b_Imode_1;
    uint8_t mat_b_Imode_2;
    
    uint8_t psum_in;
    
    uint8_t bias_in;
    int     bias_len;    
    uint8_t bias_zero_flag;    
    
    uint8_t io_mode;
    uint8_t curr_depth_iter;
    uint8_t num_ifm_depth_iter;
    
    int8_t bias_init_shift;
    uint8_t shift_out;
    uint8_t out_sign;
    uint8_t ifm_sign;
    
    DECL_ITR3D(iterator_mat_a)
    DECL_ITR3D(iterator_mat_b)
    DECL_ITR2D(iterator_mat_c)
    DECL_ITR2D(iterator_psum_in)
    DECL_ITR2D(iterator_psum_out)  
};

#endif


typedef struct gemm_internal_params gemm_params_t;


static void setup_gemm_iter_params(gemm_params_t &param) {

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


static NOINLINE void load_gemm_rtp(gemm_params_t &param){

    param.mat_a_rows         = MAT_A_ROWS;
    param.inner_dim          = INNER_DIM;
    param.mat_b_cols         = MAT_B_COLS;
    param.bias_in            = GEMM_BIAS_IN;    
    param.num_ifm_depth_iter = GEMM_NUM_DEPTH_ITERS;
    param.psum_buff_offset   = GEMM_PSUM_BUFF_OFFSET; //4*MAT_A_ROWS*4*(mat_c_bits/8);//Offset of 4*Rc4
    param.mat_b_transpose    = GEMM_TRANSPOSE_MAT_B;
    param.act                = GEMM_ACT;
}
// function below is with actual RTPs
static NOINLINE void load_gemm_rtp(gemm_params_t &param,const uint32_t (&lp)[GEMM_LP_SIZE]){
    param.mat_a_rows         = lp[0];
    param.inner_dim          = lp[1];
    param.mat_b_cols         = lp[2];
    param.num_ifm_depth_iter = lp[3];
    param.bias_in            = lp[4];
    param.mat_b_transpose    = lp[5];
    param.act                = lp[7];
    param.psum_buff_offset   = GEMM_PSUM_BUFF_OFFSET; //4*MAT_A_ROWS*4*(mat_c_bits/8);//Offset of 4*Rc4
}


#if __AIE_ARCH__ == 20

static NOINLINE void setup_gemm(gemm_params_t &param){   
    
   // load_gemm_rtp(param); 
    
    uint8_t mat_a_bits  = 16; //same for mat_b
    uint8_t mat_c_bits  = 16;
    uint8_t bias_bits   = 16;
    uint8_t psum_bits   = 32;
    
    uint8_t mat_c_row_gran = 16; //Output row granularity
    uint8_t mat_c_col_gran = 16; //Output col granularity    
    
    param.bias_len    = param.bias_in*(bias_bits/8)*param.mat_b_cols;
    param.inner_count = param.inner_dim / 8;
    param.outer_count = param.mat_a_rows * param.mat_b_cols / (mat_c_row_gran * mat_c_col_gran);               
    
    int mat_a_step_C     =  4*(mat_a_bits/8)*param.mat_a_rows; //c4 vectorization * 2 bytes * num of row
    int mat_a_step_total =  mat_a_step_C * (param.inner_dim/4); //step total mat A           
    param.incAI          =  -4*32 + mat_a_step_C;             
    int numA1            =  param.inner_dim / 8  - 1;
    int incA1            =  -3*32 + mat_a_step_C; //Move along column/inner dimension of mat A    
    int numA2            =  param.mat_a_rows / 16 - 1;
    int incA2            =  (-2*mat_a_step_C *numA1) + 32 - mat_a_step_C; //Move along the row dimension of mat A    
    int incA3            =  -mat_a_step_total + 32; //Revert to the beginning of mat A
    SETUP_ITR3D_PARAM(iterator_mat_a,incA1,numA1,incA2,numA2,incA3,param)
    
   
    #if GEMM_TRANSPOSE_MAT_B_SUPPORT == 1       
        param.mat_b_Imode_0  =  param.mat_b_transpose ? 39 : 22;           
        int mat_b_step_C     =  param.mat_b_transpose ? 4*(mat_a_bits/8)*param.mat_b_cols : 4*(mat_a_bits/8)*param.inner_dim; //c4 vectorization * 2 bytes * num of row        
        param.incB1          =  param.mat_b_transpose ? mat_b_step_C : 32;
        param.incB2          =  param.mat_b_transpose ? 32 - mat_b_step_C : -32 + mat_b_step_C;    
        int numB1            =  numA1;
        int incB1            =  param.mat_b_transpose ? - 3*32 + mat_b_step_C : 32 - 3*mat_b_step_C; //Move along row/inner dimension of mat B      
        int numB2            =  numA2;
        int incB2            =  param.mat_b_transpose ? -2*mat_b_step_C*numA1 - 3*32 - mat_b_step_C : -64*numA1 -32 - 3*mat_b_step_C;//Revert back to begining of row/inner dimension of matB
        int incB3            =  param.mat_b_transpose ? -2*mat_b_step_C*numA1 - mat_b_step_C  + 32 : 32; //Move along col dimension of mat B.         
    #else
        param.mat_b_Imode_0  =  22;           
        int mat_b_step_C     =  4*(mat_a_bits/8)*param.inner_dim; //c4 vectorization * 2 bytes * num of row    
        param.incB1          =  32;
        param.incB2          =  -32 + mat_b_step_C;    
        int numB1            =  numA1;
        int incB1            =  32 - 3*mat_b_step_C; //Move along row/inner dimension of mat B      
        int numB2            =  numA2;
        int incB2            =  -64*numA1 -32 - 3*mat_b_step_C;//Revert back to begining of row/inner dimension of matB
        int incB3            =  32; //Move along col dimension of mat B.         
    #endif    
    SETUP_ITR3D_PARAM(iterator_mat_b,incB1,numB1,incB2,numB2,incB3,param)


    int mat_c_step_C     = 4*(mat_c_bits/8)*param.mat_a_rows; //c4 vectorization * 2 bytes * num of row   
    param.incC1          = -3*32 + mat_c_step_C;            
    int numC1            = numA2;    
    int incC1            = 32 - (3 * mat_c_step_C); //Move along output row dimension      
    int incC2            = 32; //Move along the output col dimension   
    SETUP_ITR2D_PARAM(iterator_mat_c,incC1,numC1,incC2,param)    
    
         
    int psum_step_C      = 4*(psum_bits/8)*param.mat_a_rows; //c4 vectorization*4bytes*num of row
    param.psum_incC1     = -3*64 + psum_step_C - 32;
    int psum_numC1       = numA2;
    int psum_incC1       = 32 - (3 * psum_step_C); //Move along output row dimension      
    int psum_incC2       = 32;    
   
    SETUP_ITR2D_PARAM(iterator_psum_in, psum_incC1,psum_numC1,psum_incC2,param)
    SETUP_ITR2D_PARAM(iterator_psum_out,psum_incC1,psum_numC1,psum_incC2,param)            
}

#else

static NOINLINE void setup_gemm(gemm_params_t &param){   
    
   // load_gemm_rtp(param); 
    
    uint8_t mat_a_bits  = 16; //same for mat_b
    uint8_t mat_c_bits  = 16;
    uint8_t bias_bits   = 16;
    uint8_t psum_bits   = 32;
    
    uint8_t mat_c_row_gran = 16; //Output row granularity
    uint8_t mat_c_col_gran = 16; //Output col granularity    
    
    param.bias_len    = param.bias_in*(bias_bits/8)*param.mat_b_cols;
    param.inner_count = param.inner_dim / 8;
    param.outer_count = param.mat_a_rows * param.mat_b_cols / (mat_c_row_gran * mat_c_col_gran);               
    
    int mat_a_step_C     =  4*(mat_a_bits/8)*param.mat_a_rows; //c4 vectorization * 2 bytes * num of row
    int mat_a_step_total =  mat_a_step_C * (param.inner_dim/4); //step total mat A           
    param.incAI          =  -2*32 + mat_a_step_C;             
    int numA1            =  param.inner_dim / 8  - 1;
    int incA1            =  -2*32 + mat_a_step_C; //Move along column/inner dimension of mat A    
    int numA2            =  param.mat_a_rows / 16 - 1;
    int incA2            =  (-2*mat_a_step_C *numA1) + 64 - mat_a_step_C; //Move along the row dimension of mat A    
    int incA3            =  -mat_a_step_total + 64; //Revert to the beginning of mat A
    SETUP_ITR3D_PARAM(iterator_mat_a,incA1,numA1,incA2,numA2,incA3,param)
    
    param.mat_b_Imode_0  = param.mat_b_transpose ? 32 : 4;
    param.mat_b_Imode_1  = param.mat_b_transpose ? 8  : 1;
    param.mat_b_Imode_2  = param.mat_b_transpose ? 4  : 32;
               
    int mat_b_step_C     =  4*(mat_a_bits/8)*param.inner_dim;  //c4 vectorization * 2 bytes * num of row
    int mat_b_step_R     =  4*(mat_a_bits/8)*param.mat_b_cols; //r4 vectorization * 2 bytes * num of cols....transpose case.
    param.incB1          =  param.mat_b_transpose ? mat_b_step_R      : mat_b_step_C;
    param.incB2          =  param.mat_b_transpose ? 64 - mat_b_step_R : mat_b_step_C;        
    int numB1            =  numA1;
    int incB1            =  param.mat_b_transpose ? mat_b_step_R - 64  : 64 - 3*mat_b_step_C; //Move along row/inner dimension of mat B      
    int numB2            =  numA2;
    int incB2            =  param.mat_b_transpose ? -2*mat_b_step_R*numA1 -mat_b_step_R -64 : -64*numA1 - 3*mat_b_step_C;//Revert back to begining of row/inner dimension of matB
    int incB3            =  param.mat_b_transpose ? -2*mat_b_step_R*numA1 -mat_b_step_R +64 : 64; //Move along col dimension of mat B.            
    SETUP_ITR3D_PARAM(iterator_mat_b,incB1,numB1,incB2,numB2,incB3,param)


    int mat_c_step_C     = 4*(mat_c_bits/8)*param.mat_a_rows; //c4 vectorization * 2 bytes * num of row   
    param.incC1          = -3*32 + mat_c_step_C;            
    int numC1            = numA2;    
    int incC1            = 32 - (3 * mat_c_step_C); //Move along output row dimension      
    int incC2            = 32; //Move along the output col dimension   
    SETUP_ITR2D_PARAM(iterator_mat_c,incC1,numC1,incC2,param)    
    
         
    int psum_step_C      = 4*(psum_bits/8)*param.mat_a_rows; //c4 vectorization*4bytes*num of row
    param.psum_incC1     = 0;
    int psum_numC1       = numA2;
    int psum_incC1       = 0; //Move along output row dimension      
    int psum_incC2       = 0;    
   
    SETUP_ITR2D_PARAM(iterator_psum_in, psum_incC1,psum_numC1,psum_incC2,param)
    SETUP_ITR2D_PARAM(iterator_psum_out,psum_incC1,psum_numC1,psum_incC2,param)            
}

#endif

//Without RTP- params fixed at compile time
static NOINLINE void setup_gemm_params(gemm_params_t &param){
   load_gemm_rtp(param); 
   setup_gemm(param);   
}

//With RTP
static NOINLINE void setup_gemm_params(gemm_params_t &param,const uint32_t (&lp)[GEMM_LP_SIZE]){
   load_gemm_rtp(param,lp);
   setup_gemm(param);
}
    
//***************************GEMM INT8 version************************
static NOINLINE void load_gemm_rtp(gemm_params_t &param,const uint32_t (&lp)[GEMM_INT_LP_SIZE]){
    param.mat_a_rows         = lp[0];
    param.inner_dim          = lp[1];
    param.mat_b_cols         = lp[2];
    param.num_ifm_depth_iter = lp[3];
    param.bias_in            = lp[4];
    param.mat_b_transpose    = lp[5];
    param.act                = lp[7];
    param.shift_out          = lp[8];
    param.bias_init_shift    = lp[9];
    param.ifm_sign           = lp[10];
    param.psum_buff_offset   = GEMM_PSUM_BUFF_OFFSET;
}

static NOINLINE void setup_gemm_rtp(gemm_params_t &param){
    param.outer_count  = (param.mat_a_rows*param.mat_b_cols) >> 8; //rows * cols granularity 8*8
    param.inner_count  = param.inner_dim >> 3;
    param.out_sign = 1 - param.act;
    param.bias_len    = param.bias_in*param.mat_b_cols*2; //bias int16 type
    int mat_a_row_gran = 16;
    int mat_b_col_gran = 16;
    
    int mat_a_step_C     = 8*param.mat_a_rows; //c8 vectorization * num of rows Hw8
    int mat_a_step_total = mat_a_step_C * (param.inner_dim/8); //WHw8
    int numA1            = (param.inner_dim >> 3) - 1; //W/8
    int incA1            = - 64 + mat_a_step_C; //H*w8
    int numA2            = (param.mat_a_rows >> 4) - 1;  //H/16
    int incA2            = 64 - numA1*mat_a_step_C;
    int incA3            = 64 - mat_a_step_total; //WHw8
    SETUP_ITR3D_PARAM(iterator_mat_a, incA1, numA1, incA2, numA2, incA3, param)
    
    param.mat_b_Imode_0  = param.mat_b_transpose ? 8 : 1;
    int mat_b_step_C     = param.mat_b_transpose ? param.mat_b_cols*8 : param.inner_dim*8; //c8 vectorization * num of rows Hw8
    param.incB1          = param.mat_b_transpose ? 64 : mat_b_step_C;
    int numB1            = numA1;
    int incB1            = param.mat_b_transpose ? (-64 + mat_b_step_C) : (64 - mat_b_step_C); //mat_b_col_gran/c8
    int numB2            = numA2;
    int incB2            = param.mat_b_transpose ? (-64 -numB1*mat_b_step_C) : (- numB1*64 - mat_b_step_C);
    int incB3            = param.mat_b_transpose ? (64 -numB1*mat_b_step_C) : 64;    
    SETUP_ITR3D_PARAM(iterator_mat_b, incB1, numB1, incB2, numB2, incB3, param)
    
    int mat_c_step_C     = 8*param.mat_a_rows; //c8 vectorization * num of rows Hw8
    param.incC1          = - 64 + mat_c_step_C;
    int numC1            = numA2;
    int incC1            = 64 - mat_c_step_C;
    int incC2            = 64;
    SETUP_ITR2D_PARAM(iterator_mat_c, incC1, numC1, incC2, param)
    
    //bias iterator
    int psum_numC1       = numA2;
    int psum_incC1       = 0;     
    int psum_incC2       = 32;  
    SETUP_ITR2D_PARAM(iterator_psum_out, psum_incC1,psum_numC1,psum_incC2,param) 
}

//With RTP
static NOINLINE void setup_gemm_params(gemm_params_t &param,const uint32_t (&lp)[GEMM_INT_LP_SIZE]){
   load_gemm_rtp(param,lp);
   setup_gemm_rtp(param);
}

#endif  //__GEMM_PARAMS_H__


