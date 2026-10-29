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
    
    
#ifndef __SCALE_ADD_PARAMS_H__
#define __SCALE_ADD_PARAMS_H__

#include "aie.hpp"
#include <adf.h>

#include "utils.h"
#include "layer_params.h"
#include "mllib_const.h"

#ifndef MLLIB_LP_SIZE
#define MLLIB_LP_SIZE  8
#endif

using namespace aie;

struct alignas(v32uint16) scale_add_params {
    int rows;
    int cols;
    int scale;
    int add_value;
    int along_col;
    int shift_out;
    int act;
    int col_superset;
    int col_offset;
    int ofm_len;
    int incr0;
    int num_iters;
    int out_sign;
    int shuffle_mode0;
    int shuffle_mode1;
    int shuffle_mode2;
    int interleave_mode;
    DECL_ITR2D(iterator_mat)
    DECL_ITR2D(iterator_mat_out)
} ;

typedef struct scale_add_params scale_add_params_t ;

// Run Time Param loading
static NOINLINE void load_scale_add_rtp(scale_add_params_t &param,const uint32_t (&lp)[MLLIB_LP_SIZE]){

    param.rows         = lp[0];
    param.cols         = lp[1];
    param.scale        = lp[2];
    param.add_value    = lp[3];
    param.along_col    = lp[4];
    param.shift_out    = lp[5];
    param.act          = lp[6];
    param.col_superset = 0; // to be updated in wrapper
}

template <typename dtype>
NOINLINE void setup_scale_add(scale_add_params_t &param){
   // Vectorization Information (Required to setup iterator)
   param.ofm_len = param.rows * param.cols;
   param.col_offset = param.col_superset * param.ofm_len;
   
   if constexpr(std::is_same<bfloat16, dtype>::value){ 
      param.shift_out = 16;
      int col_gran_bits = param.along_col ? 2 : 4;
      int row_gran_bits = param.along_col ? 4 : 2;      
      param.num_iters = param.ofm_len >> (col_gran_bits+row_gran_bits);      
   
   #if __AIE_ARCH__ == 20
      param.incr0 = param.along_col ? 32 : param.rows*4*2; //Hw4
      param.shuffle_mode0 = param.along_col ? 22 : 39;
      param.shuffle_mode1 = param.along_col ? 26 : 12;
      param.shuffle_mode2 = param.along_col ? 27 : 13;
      param.interleave_mode = param.along_col ? 16 : 4;
   
      int incr0 = param.along_col ? 32 : param.incr0; //along_col ? next set of rows : next set of cols
      int wrap0 = param.along_col ? (param.rows >> row_gran_bits)-1 : (param.cols >> col_gran_bits)-1;
      int incr1 = param.along_col ? 32 : param.incr0 + 32 - param.incr0*4*(wrap0+1); //along_col ? jump to next col : jump to the start of next row
      
      SETUP_ITR2D_PARAM(iterator_mat,incr0,wrap0,incr1,param)
      SETUP_ITR2D_PARAM(iterator_mat_out,incr0,wrap0,incr1,param)

   #else
      param.incr0 = 64; //32 elems
   #endif
   
   }else{
      param.out_sign = 1 - param.act;
      param.incr0 = param.along_col ? 32 : param.rows*8; //Hw8
      int col_gran_bits = param.along_col ? 3 : 4;
      int row_gran_bits = param.along_col ? 3 : 2;
      param.num_iters = param.ofm_len >> (col_gran_bits+row_gran_bits);
      
      int incr0 = param.along_col ? 32 : param.incr0; //along_col ? next set of rows : next set of cols
      int wrap0 = param.along_col ? (param.rows >> row_gran_bits)-1 : (param.cols >> col_gran_bits)-1;
      int incr1 = param.along_col ? 32 : param.incr0 + 32 - param.incr0*2*(wrap0+1); //along_col ? jump to next col : jump to the start of next row
      
      SETUP_ITR2D_PARAM(iterator_mat,incr0,wrap0,incr1,param)
      SETUP_ITR2D_PARAM(iterator_mat_out,incr0,wrap0,incr1,param)
   }
}

//Params fixed at run time
template <typename dtype>
NOINLINE void setup_scale_add_params(scale_add_params_t &param, const uint32_t (&lp)[MLLIB_LP_SIZE]) {
   load_scale_add_rtp(param,lp);
   setup_scale_add<dtype>(param);
}

#endif // __SCALE_ADD_PARAMS_H__

