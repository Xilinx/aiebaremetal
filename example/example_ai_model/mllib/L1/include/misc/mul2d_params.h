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
    
    
#ifndef __MUL2D_PARAMS_H__
#define __MUL2D_PARAMS_H__

#include "aie.hpp"
#include <adf.h>

#include "utils.h"
#include "mllib_const.h"

#define LAYER_PARAM_BYTES 64

using namespace aie;
struct alignas(v16uint16) mul2d_params {
   int ifm_sv_width; //cols
   int ifm_sv_height; //rows
   int ifm_sv_depth;   
   int shift_out; // Output shift before writing acc
   int act; //0: Linear, 1: ReLU
   int num_iters;
   int row_offset;
   int col_offset;
   int row_superset;
   int col_superset;
   int header_offset;
   DECL_ITR3D(itr_left)
};

typedef struct mul2d_params mul2d_params_t;

static NOINLINE void setup_mul2d_params(int8_t * ptr, mul2d_params_t &param, uint8_t batch_size) {
   param.ifm_sv_width  = (uint8_t) *ptr++;
   param.ifm_sv_height = (uint8_t) *ptr++;  
   param.ifm_sv_depth  = (uint8_t) *ptr++;
   param.num_iters     = ((batch_size * (param.ifm_sv_width) * (param.ifm_sv_height) * (param.ifm_sv_depth) )>>6); // divide by 64 for the 64-lane accumulator
   ptr                 = ptr+3;
   param.shift_out     = (uint8_t) *ptr++;
   ptr                 = ptr+20;
   param.act           = (uint8_t) *ptr++;
   param.row_offset   = 0;
   param.col_offset   = 0;
   param.header_offset = LAYER_PARAM_BYTES;
   
   //Setup iterator
   const int32_t num_acc_lanes = 64;
   const int32_t num_acc_lane_bits = 6;
   const int32_t chan_gran_bits = 3; // Depth of 8 format (from HCWN_C8)
   
   uint16_t row_width = (param.ifm_sv_width*batch_size)<<chan_gran_bits; //width*batchsize*8 
   uint16_t incr0     = num_acc_lanes ; 
   uint16_t wrap0     = (row_width>>num_acc_lane_bits)-1; //-1 is neded for the intrinsic
   uint16_t incr1     = incr0; //incr0=32  
   uint16_t wrap1     = ((param.ifm_sv_depth)>>chan_gran_bits)-1;  
   uint16_t incr2     =  incr1;  
   
   SETUP_ITR3D_PARAM(itr_left,incr0,wrap0,incr1,wrap1,incr2,param)
}

//Run Time Param loading
static NOINLINE void load_rtp(mul2d_params_t &param,const uint32_t (&lp)[MLLIB_LP_SIZE]){
   param.ifm_sv_height = lp[0];
   param.ifm_sv_width  = lp[1];
   param.row_superset  = lp[2];
   param.col_superset  = lp[3];
}

static NOINLINE void setup_rtp(mul2d_params_t &param) {
   const int32_t num_acc_lane_bits = 6;
   const int32_t num_acc_lanes = 64;
   uint32_t num_elems = param.ifm_sv_height * param.ifm_sv_width; //rows*cols
   param.num_iters    = num_elems>>num_acc_lane_bits;
   param.row_offset   = param.row_superset * num_elems;
   param.col_offset   = param.col_superset * num_elems;
   param.header_offset = 0;
   param.shift_out = 0;
   param.act = 1;
   
   uint16_t incr0     = num_acc_lanes*2; 
   uint16_t wrap0     = num_elems - 1; //-1 is neded for the intrinsic
   uint16_t incr1     = 0;
   uint16_t wrap1     = 0;  
   uint16_t incr2     = 0;  
   
   SETUP_ITR3D_PARAM(itr_left,incr0,wrap0,incr1,wrap1,incr2,param)
}

//Params fixed at run time
static NOINLINE void setup_mul2d_params(mul2d_params_t &param, const uint32_t (&lp)[MLLIB_LP_SIZE]) {
   load_rtp(param,lp);
   setup_rtp(param);
}
#endif //__MUL2D_PARAMS_H__
