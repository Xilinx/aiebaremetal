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

#ifndef __ADD2D_CASC_H__
#define __ADD2D_CASC_H__

#include "aie.hpp"
#include <adf.h>

#include "utils.h"
#include "mllib_const.h"
#include "add2d_params.h"

using namespace aie;

// add2d with output on cascade
template<typename dtype_in,typename dtype_out,typename config_in,typename config_out>
NOINLINE void add2d
(     
    adf::input_buffer_conf<dtype_in, config_in>  & __restrict in,    // Input-0 this should be from IFM2
    adf::output_buffer_conf<dtype_out, config_out> & __restrict out, // Input-1, this should be OFM of conv2d output
    OUT_CASC<acc32> *casc_acc_out,                         // Output on cascade port
    add2d_params_t params
){
    
    dtype_in __aie_dm_resource_b * __restrict in_ptr0 = (dtype_in __aie_dm_resource_b * __restrict) (in.data()+params.offset); // Input coming from IFM bank
    int16_t __aie_dm_resource_a * __restrict in_ptr1 = (int16_t __aie_dm_resource_a * __restrict) out.data(); // Output of conv coming from OFM bank 
    int16_t __aie_dm_resource_a * __restrict out_ptr = (int16_t __aie_dm_resource_a * __restrict) out.data(); // Output is stored temporarily in same buffer as above 

    // Setup local counters for two iterators, i.e., IFM2 and partial output iterator
    SETUP_ITR3D_COUNTERS(itr_left)
    SETUP_ITR3D_COUNTERS(iterator_psum)
       
    const int result_sign = 1 - (int) params.act ;
    const int incr = 32 ; // Number of acc lanes

   // Note on shifts in cascade version of add2d
   // shift_psum: This shift is for loading 16-bit partial product to the 32-bit accumulator
   // After loading and add operation this accumulator will be cascaded out to conv2d_cascade_end kernel on adjacent core
   // shift_psum should have same value as "shift_psum" of the corresponding chained conv2d kernel
   // This is required because on the casc end node, conv2d kernel will add this to 32-bit acc after convolution 
   // shift_in: This shift will get the 8-bit IFM2 on the same scale as 32-bit accumulator
   // shift_in differs from its non-cascade version of add2d
   // cascade out = ifm2*2^shift_in + conv_out_16bit*2^shift_psum


     // Stores the add2d sum output assuming no growth (ASSUMPTION: sum data remains in 16-bit)
     #define SHIFT_ADD_STORE_ELEMS16b(in_ptr0,in_ptr1,out_ptr,shift_in,in0_sign,zero_ifm2)\
     {\
         accum<acc32,32> acc0 ;\
         accum<acc32,32> acc1 ;\
         accum<acc32,32> acc_res ;\
         vector<int8,64> zero_vec = zeros<int8,64>();\
         acc0.from_vector_sign( load_v<32>( in_ptr0 ), in0_sign, shift_in );\
         in_ptr0 = INCR_ITR3D_PARAM(in_ptr0, itr_left,params) ;\
         acc1.from_vector_sign( load_v<32>( in_ptr1 ), 1, 0 ); in_ptr1+=incr ;\
         acc_res=addmac_4x8_8x8_conf(zero_vec,zero_vec,acc0,acc1,zero_ifm2,0,0,0,0); \
         writeincr(casc_acc_out,acc_res);out_ptr+=incr; \
     }

    // Sum the SVs from conv2d and IFM2 and 'write the result back to memory'
    // Read with iterator before sending to cascade
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(8,) 
    AIE_LOOP_UNROLL(2) 
    for (int i=0; i<params.num_inputs; i++){                
         SHIFT_ADD_STORE_ELEMS16b(in_ptr0,in_ptr1,out_ptr,params.shift_in,params.in0_sign,params.zero_ifm2)
    }
/*
    // Some local variables related to cascade output iterator 
    int    chnd_T         = 64;
    int    inc_ST_0       = 2*chnd_T;
    int    inc_ST_1       = params.inc_ST_0;
    int    inc_ST_2       = params.inc_ST_1;
    int    inc_ST_3       = chnd_T;    


    accum<acc32,32> acc0,acc1,acc2,acc3,acc4,acc5,acc6,acc7;
    in_ptr1 = (int16_t __aie_dm_resource_a * __restrict) out.data(); // Reset input pointer

    // Read with iterator and then send on cascade, this sends data with correct order as expected by conv2d kernel
    for (int i=0; i<params.num_inputs/8; i++)
    chess_prepare_for_pipelining
    chess_loop_range(4,)
    {                
         acc0.from_vector_sign( load_v<32>( in_ptr1 ), 1, params.shift_psum ); in_ptr1 = byte_incr(in_ptr1, inc_ST_0-chnd_T);
         acc1.from_vector_sign( load_v<32>( in_ptr1 ), 1, params.shift_psum ); in_ptr1 = byte_incr(in_ptr1, inc_ST_1-chnd_T);
         acc2.from_vector_sign( load_v<32>( in_ptr1 ), 1, params.shift_psum ); in_ptr1 = byte_incr(in_ptr1, inc_ST_0-chnd_T);
         acc3.from_vector_sign( load_v<32>( in_ptr1 ), 1, params.shift_psum ); in_ptr1 = byte_incr(in_ptr1, inc_ST_2-chnd_T);
         acc4.from_vector_sign( load_v<32>( in_ptr1 ), 1, params.shift_psum ); in_ptr1 = byte_incr(in_ptr1, inc_ST_0-chnd_T);
         acc5.from_vector_sign( load_v<32>( in_ptr1 ), 1, params.shift_psum ); in_ptr1 = byte_incr(in_ptr1, inc_ST_1-chnd_T);
         acc6.from_vector_sign( load_v<32>( in_ptr1 ), 1, params.shift_psum ); in_ptr1 = byte_incr(in_ptr1, inc_ST_0-chnd_T);
         acc7.from_vector_sign( load_v<32>( in_ptr1 ), 1, params.shift_psum ); in_ptr1 = byte_incr(in_ptr1, inc_ST_3-chnd_T);
         in_ptr1 = INCR_ITR3D_PARAM(in_ptr1, iterator_psum,params)
         writeincr(casc_acc_out,acc0);
         writeincr(casc_acc_out,acc1);
         writeincr(casc_acc_out,acc2);
         writeincr(casc_acc_out,acc3);
         writeincr(casc_acc_out,acc4);
         writeincr(casc_acc_out,acc5);
         writeincr(casc_acc_out,acc6);
         writeincr(casc_acc_out,acc7);
    }
*/
}

// add2d with input on cascade and final output to OFM port
template<typename dtype_in,typename dtype_out,typename config_in,typename config_out>
NOINLINE void add2d
(     
    adf::input_buffer_conf<dtype_in, config_in>  & __restrict in,    // Input-0 this should be from IFM2
    adf::output_buffer_conf<dtype_out, config_out> & __restrict out, // Input-1, this should be OFM of conv2d output
    IN_CASC<acc32> *casc_acc_in,                         // Output on cascade port
    add2d_params_t params
){
    
    dtype_in __aie_dm_resource_b * __restrict in_ptr0 = (dtype_in __aie_dm_resource_b * __restrict) (in.data()+params.offset); // Input coming from IFM bank
    int16_t __aie_dm_resource_a * __restrict in_ptr1 = (int16_t __aie_dm_resource_a * __restrict) out.data(); // Output of conv coming from OFM bank 
    int8_t __aie_dm_resource_a * __restrict out_ptr = (int8_t __aie_dm_resource_a * __restrict) out.data(); // Output is stored temporarily in same buffer as above 

    // Setup local counters for two iterators, i.e., IFM2 and partial output iterator
    SETUP_ITR3D_COUNTERS(itr_left)
    //SETUP_ITR3D_COUNTERS(iterator_psum)
       
    const int incr = 32 ; // Number of acc lanes
    const int result_sign = 1 - (int) params.act ;

   // Note on shifts in cascade version of add2d
   // shift_psum: This shift is for loading 16-bit partial product to the 32-bit accumulator
   // After loading and add operation this accumulator will be cascaded out to conv2d_cascade_end kernel on adjacent core
   // shift_psum should have same value as "shift_psum" of the corresponding chained conv2d kernel
   // This is required because on the casc end node, conv2d kernel will add this to 32-bit acc after convolution 
   // shift_in: This shift will get the 8-bit IFM2 on the same scale as 32-bit accumulator
   // shift_in differs from its non-cascade version of add2d
   // cascade out = ifm2*2^shift_in + conv_out_16bit*2^shift_psum

     #define SHIFT_ADD_STORE_ELEMS8b(in_ptr0,casc_acc_in,in_ptr1,out_ptr,result_sign,shift_in,shift_out,in0_sign,zero_ifm2)\
     {\
         accum<acc32,32> acc0 ;\
         accum<acc32,32> acc_res,tmp_acc ;\
         vector<int16,32> ones_vec = aie::broadcast<int16, 32>(1);\
         vector<int16,32> ifm1_vec;\
         ifm1_vec = insert(ifm1_vec, 0, *(v16int16 __aie_dm_resource_a *)in_ptr1); in_ptr1 = byte_incr(in_ptr1, incr);\
         ifm1_vec = insert(ifm1_vec, 1, *(v16int16 __aie_dm_resource_a *)in_ptr1); in_ptr1 = byte_incr(in_ptr1, incr);\
         acc0.from_vector_sign( load_v<32>( in_ptr0 ), in0_sign, shift_in );\
         in_ptr0 = INCR_ITR3D_PARAM(in_ptr0, itr_left,params) ;\
	 tmp_acc=readincr_v<32>(casc_acc_in);\
         acc_res = addmac_elem_32_conf(ifm1_vec, 1, ones_vec, 0, acc0, tmp_acc, zero_ifm2, 0, 0, 0, 0);\
         store_v(out_ptr, acc_res.template to_vector_sign<int8_t>(result_sign,shift_out));out_ptr+=incr; \
     }
    
    // Sum the SVs from conv2d and IFM2 and 'write the result back to memory'
    // Read with iterator before sending to cascade
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(8,) 
    AIE_LOOP_UNROLL(2) 
    for (int i=0; i<params.num_inputs; i++){                
         SHIFT_ADD_STORE_ELEMS8b(in_ptr0,casc_acc_in,in_ptr1,out_ptr,result_sign,params.shift_in,params.shift_out,params.in0_sign,params.zero_ifm2)
    }

}

#endif //__ADD2D_CASC_H__  
    


