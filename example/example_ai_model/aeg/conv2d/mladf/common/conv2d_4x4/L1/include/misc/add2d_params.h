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
    
    
    
#ifndef __ADD2D_PARAMS_H__
#define __ADD2D_PARAMS_H__

#include "aie.hpp"
#include <adf.h>

#include "utils.h"
#include "layer_params.h"
#include "mllib_const.h"
#include "../conv/conv2d_params.h" //TODO: find a way to fix relative path without needing to include for conv from Makefile

#ifndef COL_OFFSET
#define COL_OFFSET 0
#endif

#ifndef ROW_OFFSET
#define ROW_OFFSET 0
#endif

#ifndef ADD2D_LP_SIZE
#define ADD2D_LP_SIZE  15
#endif

#ifndef ADD2D_SS_SIZE
#define ADD2D_SS_SIZE 4
#endif


using namespace aie;

// ######### Add2d Params Unified Struct #################

struct alignas(v32uint16) add2d_internal_params {
    int  dim_x;
    int  dim_y;
    int  dim_d;
    int  shift_out;
    int  shift_psum;
    int  shift_in;
    int  ofm_len;
    int  act;
    int  in0_sign;
    int  header_offset;
    int  row_offset;
    int  col_offset;
    int  zero_ifm2;
    int  offset;
    int  add_mode;
    int  incr1;
 
    //derived params
    int  num_inputs;              // Length of tensors in multiples of 32 (H*W*N*C/32)
    int  batch_log2;

    //bfloat kernel
    int ifm1_offset;
    int ifm2_offset;
    DECL_ITR3D(itr_left)

    //Iterators for Casc Kernel
    DECL_ITR3D(iterator_psum)    // Iterator for partial output on cascade
    int32_t inc_ST_0;
    int32_t inc_ST_1;

    //RTP for Scalar Value when 1 Add2D input is Scalar
    int8_t scalar_val_int;
    uint8_t scalar_val_uint;
    bfloat16 scalar_val_bf16;
} ;

typedef struct add2d_internal_params add2d_internal_params_t ;

// ######## Add2d Struct for Casc Kernel (TODO: Merge with Unified Struct) ##############

struct alignas(v32uint16) add2d_params {
    int zero_ifm2;               // Force IFM2 to zero for cascade start
    int offset;                  // Offset inside IFM buffer from where input-0 will be picked.
    int num_inputs;              // Length of tensors in multiples of 32 (H*W*N*C/32)
    int shift_in;                // bit shift for the lower precision input while loading into acc
    int shift_psum;              // bit shift for the 16-bit input for loading to 32-bit accumulator (shd be same as psum_shift_in of conv2d)
    int shift_out;               // Output shift before writing acc
    int in0_sign;                // 0: treat input-0 as unsigned, 1: treat input-0 as signed
    act_t act;                   // 0: Linear, 1: ReLU
    DECL_ITR3D(itr_left)         // Iterator for IFM2 traversale (8-bit precision input)
    DECL_ITR3D(iterator_psum)    // Iterator for partial output on cascade
    int32_t inc_ST_0;
    int32_t inc_ST_1;
    int ofm_len;
    int row_offset;
    int col_offset;
    int header_offset;
} ;

typedef struct add2d_params add2d_params_t ;

// ######## Overloaded Kernel Setup Function for Standalone Add2D (wts header having rtp/data layout - HCWNC8)

// Run Time Param loading
static NOINLINE void load_add2d_rtp(int8_t * ptr, add2d_internal_params_t &param, uint8_t batch_size){

   param.dim_x       = (uint8_t) *ptr++;
   ptr+=2;
   param.dim_d       = (uint8_t) *ptr++;
   ptr+=2;
   param.shift_out   = (int8_t) *ptr++;
   ptr+=9;
   param.shift_in    = (int8_t) *ptr++;
   ptr+=3;
   param.ofm_len     = (uint16_t) *(uint16_t *)ptr;
   ptr+=7;
   param.act         = (int8_t) *ptr++;
   ptr+=2;
   param.in0_sign    = (uint8_t) *ptr++;
   
   param.batch_log2  = (batch_size==4)?2:((batch_size==2)?1:0); 
}

static NOINLINE void setup_add2d(add2d_internal_params_t &param, uint8_t batch_size){
   // Setup iterator
   const int32_t num_acc_lanes=32;
   const int32_t num_acc_lane_bits=5;
   const int32_t chan_gran_bits=3; // Depth of 8 format (from HCWN_C8)
   
   int row_width     = (param.dim_x<<param.batch_log2)<<chan_gran_bits;//width*batchsize*8
   int incr0         = num_acc_lanes ;
   int wrap0         = (row_width>>num_acc_lane_bits) -1 ; // -1 is neded for the intrinsic
   int incr1         = incr0 ;//row_width ; 
   int wrap1         = ((param.dim_d)>>chan_gran_bits)-1;
   int incr2         = incr0;  

   param.num_inputs         = (param.ofm_len)>>num_acc_lane_bits; // divide num of elems in OFM by 32 to account for the 32-lane concurrent addition using accumulator
   param.header_offset      = 64;
   param.row_offset         = 0;
   param.col_offset         = 0;
   param.offset             = 0;
   param.zero_ifm2          = 0;
    
   SETUP_ITR3D_PARAM(itr_left,incr0,wrap0,incr1,wrap1,incr2,param)

   param.incr1       = (param.add_mode==1)?0:32;
}

static NOINLINE void setup_add2d_params(int8_t * ptr, add2d_internal_params_t &param, uint8_t batch_size) {
  load_add2d_rtp(ptr, param, batch_size);
  setup_add2d(param, batch_size);

 }

// ######## Overloaded Kernel Setup Function for Standalone Add2D (rtp passed from AXI-MM/data layout - Linear)

// Run Time Param loading
static NOINLINE void load_add2d_rtp(add2d_internal_params_t &param,const uint32_t (&lp)[MLLIB_LP_SIZE]){

    param.dim_x             = lp[0];
    param.dim_y             = lp[1];
    param.ifm1_offset       = lp[2];
    param.ifm2_offset       = lp[3];
}

static NOINLINE void setup_add2d(add2d_internal_params_t &param){
   // Vectorization Information (Required to setup iterator)
   const int32_t num_acc_lanes=32;
   const int32_t num_acc_lane_bits=5;
   const int32_t inner_dim_gran_bits=2; // Column of 4 format (from CRC4)

   param.ofm_len    = param.dim_x * param.dim_y;
   param.row_offset = param.ifm1_offset * param.ofm_len;
   param.col_offset = param.ifm2_offset * param.ofm_len;
   param.num_inputs = (param.ofm_len)>>num_acc_lane_bits;

   int incr0         = num_acc_lanes*2 ;
   int wrap0         = param.num_inputs -1 ; // -1 is neded for the intrinsic
   int incr1         = 0;
   int wrap1         = 0;
   int incr2         = 0;

   param.act           = ELTWISE_ACT;//RELU ;
   param.shift_out     = 0;
   param.in0_sign      = 1;
   param.shift_in      = 0;
   param.header_offset = 0;
   param.offset        = 0;
   param.zero_ifm2     = 0;

   SETUP_ITR3D_PARAM(itr_left,incr0,wrap0,incr1,wrap1,incr2,param)

   param.incr1       = (param.add_mode==1)?0:32;

}

//Params fixed at run time
static NOINLINE void setup_add2d_params(add2d_internal_params_t &param, const uint32_t (&lp)[MLLIB_LP_SIZE]) {
   load_add2d_rtp(param,lp);
   setup_add2d(param);
}

// ######## Overloaded Kernel Setup Function for Fused Add2D (rtp passed from wts/data layout - HCWNC8/Sharing previous layer rtp)

static INLINE void setup_add2d_params(add2d_internal_params_t &param, conv2d_params_t &layer_params) {

   // Setup iterator
   const int32_t num_acc_lanes=32;
   const int32_t num_acc_lane_bits=5;
   const int32_t chan_gran_bits=3; // Depth of 8 format (from HCWN_C8)

   int row_width = (layer_params.oxp<<layer_params.batch_log2)<<chan_gran_bits;//width*batchsize*8
   int incr0 = num_acc_lanes ;
   int wrap0 = (row_width>>num_acc_lane_bits) -1 ; // -1 is neded for the intrinsic
   int incr1 =  incr0 ;//row_width ;
   int wrap1 = ((layer_params.num_ofm_ch)>>chan_gran_bits)-1;
   int num_ch = ((layer_params.num_ofm_ch)*ADD2D_SS_SIZE)-layer_params.num_ofm_ch; // increments are delta offset in add_3d_byte intrinsic
   int incr2 =  (layer_params.oxp<<layer_params.batch_log2)*num_ch+incr0; // "H" increment in HCWN_C8 formatted tensor scaled by 4 rows to look at super set data

   // Setup other elt-wise add params
   uint8_t tmp                  = (uint8_t) layer_params.row_offset; // Offset 29 (this is row_offset(lower bits) + zero_ifm2 (upper bits)
   uint8_t row_offset           = (uint8_t) (tmp & 0x0f); // This is lower 4 bits of offset 29 (row offset)
   param.zero_ifm2              = (int) ((tmp>>4) & 0x01);

   param.ofm_len = layer_params.ofm_len;
   param.offset = row_offset*((layer_params.num_ofm_ch)>>chan_gran_bits)*row_width;  // Pick up data from IFM2 with offset (offset applied to channels)
   param.num_inputs = (layer_params.ofm_len)>>num_acc_lane_bits; // divide num of elems in OFM by 32 to account for the 32-lane concurrent addition using accumulator
   param.shift_in = layer_params.shift_lrelu_out;// shift_in is required to bring left side (8-bit) to same scale as right side (16-bit)
                                                         // re-use shift_lrelu_out field to convey this shift, current layer is in 16-bit output mode
   param.shift_out     = layer_params.shift_out; //Final shift from 16-bit to 8-bit
   param.shift_psum    = layer_params.shift_psum_in; //Shift to get the partial 16-bit output from conv2d into 32-bit accumulator
   param.in0_sign      = layer_params.in0_sign; // Sign of left side input needed for sign extension of accumulator, right side is always signed
   param.act           = ELTWISE_ACT;//RELU ;//TODO: dynamic hardocded this for now. (act_t) (1-layer_params.out_sign);  // linear (signed)=0, relu (unsigned)=1
   param.row_offset    = 0;
   param.col_offset    = 0;
   param.header_offset = 0;

   SETUP_ITR3D_PARAM(itr_left,incr0,wrap0,incr1,wrap1,incr2,param)
   // 3d iterator setup for add2d partial sum output on cascade
   int chnd_T = 64;
   param.inc_ST_0 = layer_params.inc_ST_0;
   param.inc_ST_1 = layer_params.inc_ST_1;
   SETUP_ITR3D_PARAM(iterator_psum, 0, 0, layer_params.inc_Xo_T-chnd_T, layer_params.num_X, layer_params.inc_Co_T-chnd_T,param)

   param.incr1       = (param.add_mode==1)?0:32;
}





// ######## Overloaded Kernel Setup Function for Fused Add2D Casc (TODO: Merge with Fused Add2D Setup)(rtp passed from wts/data layout - HCWNC8/Sharing previous layer rtp)

//template<int num_bytes, int left_sign>
static INLINE void setup_add2d_params(add2d_params_t &param, conv2d_params_t layer_params) {

   // Setup iterator
   const int32_t num_acc_lanes=32;
   const int32_t num_acc_lane_bits=5;
   const int32_t chan_gran_bits=3; // Depth of 8 format (from HCWN_C8)
   int row_width = (layer_params.oxp<<layer_params.batch_log2)<<chan_gran_bits;//width*batchsize*8
   int incr0 = num_acc_lanes ;
   //int wrap0 --> width*batchsize*8/32,multiply by width in pixels by channel granularity of 8, divide by 32 for acc lanes
   int wrap0 = (row_width>>num_acc_lane_bits) -1 ; // -1 is neded for the intrinsic
   int incr1 =  incr0 ;//row_width ;
   //int wrap1 --> num of channels in SV /8 
   int wrap1 = ((layer_params.num_ofm_ch)>>chan_gran_bits)-1;
   // TODO: fixed to 4 rows !! total number of channels in shared space = number of rows in overlay*number of channels in SV
   //int num_ch = (layer_params.num_ofm_ch)<<2; 
   //ADD2D_SS_SIZE = number of elements in superset
   //              =  num rows of overlay in non cascade
   //              =  num rows of overlay /2  with cascade
   int num_ch = ((layer_params.num_ofm_ch)*ADD2D_SS_SIZE)-layer_params.num_ofm_ch; // increments are delta offset in add_3d_byte intrinsic
   //int incr2 =  (layer_params.oxp)*num_ch; // "H" increment in HCWN_C8 formatted tensor scaled by 4 rows to look at super set data
   //int incr2 =  (layer_params.oxp)*num_ch+incr0; // "H" increment in HCWN_C8 formatted tensor scaled by 4 rows to look at super set data
   int incr2 =  (layer_params.oxp<<layer_params.batch_log2)*num_ch+incr0; // "H" increment in HCWN_C8 formatted tensor scaled by 4 rows to look at super set data

   // Setup other elt-wise add params
   uint8_t tmp                  = (uint8_t) layer_params.row_offset; // Offset 29 (this is row_offset(lower bits) + zero_ifm2 (upper bits)
   uint8_t row_offset           = (uint8_t) (tmp & 0x0f); // This is lower 4 bits of offset 29 (row offset)
   param.zero_ifm2              = (int) ((tmp>>4) & 0x01); 

   //param.offset = (layer_params.row_offset)*((layer_params.num_ofm_ch)>>chan_gran_bits)*row_width;  // Pick up data from IFM2 with offset (offset applied to channels)
   param.ofm_len = layer_params.ofm_len; 
   param.offset = row_offset*((layer_params.num_ofm_ch)>>chan_gran_bits)*row_width;  // Pick up data from IFM2 with offset (offset applied to channels)
   param.num_inputs = (layer_params.ofm_len)>>num_acc_lane_bits; // divide num of elems in OFM by 32 to account for the 32-lane concurrent addition using accumulator
   param.shift_in = layer_params.shift_lrelu_out;// shift_in is required to bring left side (8-bit) to same scale as right side (16-bit)
                                                         // re-use shift_lrelu_out field to convey this shift, current layer is in 16-bit output mode
   param.shift_out = layer_params.shift_out; //Final shift from 16-bit to 8-bit
   param.shift_psum = layer_params.shift_psum_in; //Shift to get the partial 16-bit output from conv2d into 32-bit accumulator
   param.in0_sign  = layer_params.in0_sign; // Sign of left side input needed for sign extension of accumulator, right side is always signed
   param.act = ELTWISE_ACT;//RELU ;//TODO: dynamic hardocded this for now. (act_t) (1-layer_params.out_sign);  // linear (signed)=0, relu (unsigned)=1
   //param.act = (act_t) (1-layer_params.out_sign);  // linear (signed)=0, relu (unsigned)=1
   param.row_offset = 0;
   param.col_offset = 0;

   SETUP_ITR3D_PARAM(itr_left,incr0,wrap0,incr1,wrap1,incr2,param)
   // 3d iterator setup for add2d partial sum output on cascade
   int chnd_T = 64;
   param.inc_ST_0 = layer_params.inc_ST_0; 
   param.inc_ST_1 = layer_params.inc_ST_1; 
   SETUP_ITR3D_PARAM(iterator_psum, 0, 0, layer_params.inc_Xo_T-chnd_T, layer_params.num_X, layer_params.inc_Co_T-chnd_T,param)
}


// ##################################### Add2D Bf16 Kernel Setup ####################################

struct alignas(v8uint16) add2d_bf16_params {
    int rows_a;
    int cols_a;
    int row_superset;
    int col_superset;
    int row_offset;
    int col_offset;
    int num_inputs;              // Length of tensors in multiples of 16 (R*C/16)
    int ofm_len;
} ;

typedef struct add2d_bf16_params add2d_bf16_params_t ;

//Compile Time Param loading
/*
NOINLINE void load_add2d_rtp(add2d_bf16_params_t &param){
   param.rows_a = IFM_ROW;
   param.cols_a = IFM_COL;
   param.row_superset = ROW_OFFSET;
   param.col_superset = COL_OFFSET;
}
*/
// Run Time Param loading
static NOINLINE void load_add2d_rtp(add2d_bf16_params_t &param,const uint32_t (&lp)[MLLIB_LP_SIZE]){

    param.rows_a             = lp[0];
    param.cols_a             = lp[1];
    param.row_superset       = 0; //lp[2]; // to be updated in wrapper
    param.col_superset       = 0;//lp[3]; // to be updated in wrapper
}

static NOINLINE void setup_add2d(add2d_bf16_params_t &param){
   // Vectorization Information (Required to setup iterator)
   const int32_t num_acc_lanes=16;
   const int32_t num_acc_lane_bits=4;
   const int32_t inner_dim_gran_bits=2; // Column of 4 format (from CRC4)

   param.ofm_len    = param.rows_a * param.cols_a;
   param.row_offset = param.row_superset * param.ofm_len;
   param.col_offset = param.col_superset * param.ofm_len;
   param.num_inputs = (param.ofm_len)>>num_acc_lane_bits;

}

//Params fixed at compile time5
/*
NOINLINE void setup_add2d_params(add2d_bf16_params_t &param) {
   load_add2d_rtp(param);
   setup_add2d(param);
}*/

//Params fixed at run time
static NOINLINE void setup_add2d_params(add2d_bf16_params_t &param, const uint32_t (&lp)[MLLIB_LP_SIZE]) {
   load_add2d_rtp(param,lp);
   setup_add2d(param);
}


#endif // __ADD2D_PARAMS_H__

