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
    
    
    
#ifndef __ADD2D_H__
#define __ADD2D_H__

#include "aie.hpp"
#include <adf.h>

#include "utils.h"
#include "mllib_const.h"
#include "add2d_params.h"

#define ADD2D_OFFSET 64

using namespace aie;

//Unified Add2D Kernel

     #define ADD_STORE_GENERIC(acc1,acc2,acc_res)\
     {\
      if constexpr(std::is_same<int16, dtype_ifm1>::value){\
      acc1.from_vector_sign( load_v<32>( (dtype_ifm1 __aie_dm_resource_a * __restrict) in_ptr1 ),  1, 0); in_ptr1+=params.incr1;\
      }\
      else{\
      acc1.from_vector_sign( load_v<32>( (dtype_ifm1 __aie_dm_resource_a * __restrict) in_ptr1 ),  params.in0_sign, params.shift_in); in_ptr1+=params.incr1;\
      }\
      acc2.from_vector_sign( load_v<32>( (dtype_ifm2 __aie_dm_resource_b * __restrict) in_ptr2 ),  params.in0_sign, params.shift_in);\
      in_ptr2 = INCR_ITR3D_PARAM(in_ptr2,itr_left,params) ;\
      acc_res = aie::add(aie::op_zero(acc2, params.zero_ifm2), acc1);\
      store_v(out_ptr, acc_res.to_vector_sign<dtype_ofm>(result_sign, params.shift_out));out_ptr+=incr;\
      \
     }


template<typename dtype_ifm1,typename dtype_ifm2,typename dtype_ofm>
NOINLINE void add2d
(
    dtype_ifm1 * __restrict ifm1_data,    // Input-1
    dtype_ifm2 * __restrict ifm2_data,    // Input-2
    dtype_ofm *  __restrict ofm_data, //  Output.
    add2d_internal_params_t params
){
    dtype_ifm1 __aie_dm_resource_a * __restrict in_ptr1 = (dtype_ifm1 __aie_dm_resource_a * __restrict) (ifm1_data+params.row_offset); // Input1 coming from IFM1 bank
    dtype_ifm2 __aie_dm_resource_b * __restrict in_ptr2 = (dtype_ifm2 __aie_dm_resource_b * __restrict) (ifm2_data+params.header_offset+params.col_offset+params.offset); // Input1 coming from IFM2 bank
    dtype_ofm __aie_dm_resource_a * __restrict out_ptr = (dtype_ofm __aie_dm_resource_a * __restrict) ofm_data; // Output

    SETUP_ITR3D_COUNTERS(itr_left)

    const int incr = 32 ; // Number of acc lanes
    const int result_sign = 1 - (int) params.act ;

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(8,) 
    AIE_LOOP_UNROLL(4) 
    for (int i=0; i<params.num_inputs; i++){
	if constexpr(std::is_same<bfloat16, dtype_ifm1>::value)
	{
		accum<accfloat,32> acc1, acc2, acc_res ;
		ADD_STORE_GENERIC(acc1,acc2,acc_res)
	}
	else
        {
                accum<acc32,32> acc1, acc2, acc_res ;
                ADD_STORE_GENERIC(acc1,acc2,acc_res)
        }
    }

         #if RELU_INT8_ENABLE == 1
         	relu_int8_post_process((int *)ofm_data, result_sign, params.ofm_len);
         #endif
}


template<typename dtype_ifm1,typename dtype_ifm2,typename dtype_ofm,typename config_ifm1,typename config_ifm2,typename config_ofm>
INLINE void add2d
(
    adf::input_buffer_conf<dtype_ifm1, config_ifm1>  & __restrict ifm1,    // Input-1
    adf::input_buffer_conf<dtype_ifm2, config_ifm2>  & __restrict ifm2,    // Input-2
    adf::output_buffer_conf<dtype_ofm, config_ofm> & __restrict ofm, //  Output.
    add2d_internal_params_t & params
){
    return add2d<dtype_ifm1, dtype_ifm2, dtype_ofm>(ifm1.data(), ifm2.data(), ofm.data(), params);
}

// Required for Resnet 50 (TODO: Cleanup Design)

//#if __AIE_ARCH__ == 20

//add2d - 16 bit - 8 bit

template<typename dtype_in,typename dtype_out>
NOINLINE void add2d
(     
    dtype_in * __restrict in_data,    // Input-0 this should be from IFM
    dtype_out * __restrict out_data, // Input-1 and Output. This should be OFM of conv2d output
    add2d_params_t params
){
    
    dtype_in __aie_dm_resource_a * __restrict in_ptr0 = (dtype_in __aie_dm_resource_a * __restrict) (in_data+params.offset); // Input coming from IFM bank
    int16_t __aie_dm_resource_a * __restrict in_ptr1 = (int16_t __aie_dm_resource_a * __restrict) out_data; // Output of conv coming from OFM bank 
    int8_t __aie_dm_resource_a * __restrict out_ptr = (int8_t __aie_dm_resource_a * __restrict) out_data; // OUtput is in same bank as above 

      /*
       for(unsigned k=0;k<(params.num_inputs<<5);k++){
           printf("\n core=%d, index=%d ifm2 in with offset=%x\n", get_coreid(),k,*(((uint8_t*)in_ptr0)+k));
       }
      */
    // add2d works on two inputs left and right.
    // The left side input can be 8 or 16 bit
    // TODO: iterator for left side 16-bit
    SETUP_ITR3D_COUNTERS(itr_left)
       
    const int incr = 32 ; // Number of acc lanes
    const int result_sign = 1 - (int) params.act ;

    // TODO: Code below showing regressed 6 cycle loop, need to root cause and improve this

     #define SHIFT_ADD_STORE_ELEMS(in_ptr0,in_ptr1,out_ptr,result_sign,shift_in,shift_out,in0_sign,zero_ifm2)\
     {\
         accum<acc32,32> acc0 ;\
         accum<acc32,32> acc1 ;\
         accum<acc32,32> acc_res ;\
         vector<int8,64> zero_vec = zeros<int8,64>();\
         acc0.from_vector_sign( load_v<32>( in_ptr0 ), in0_sign, shift_in );\
         in_ptr0 = INCR_ITR3D_PARAM(in_ptr0, itr_left,params) ;\
         acc1.from_vector_sign( load_v<32>( in_ptr1 ), 1, 0 ); in_ptr1+=incr ;\
         acc_res = aie::add(aie::op_zero(acc0, zero_ifm2), acc1);\
         store_v(out_ptr, acc_res.to_vector_sign<int8_t>(result_sign,shift_out));out_ptr+=incr; \
     }
         //acc_res=addmac_4x8_8x8_conf(zero_vec,zero_vec,acc0,acc1,zero_ifm2,0,0,0,0); \
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(8,) 
    AIE_LOOP_UNROLL(2) 
    for (int i=0; i<params.num_inputs; i++){                
         SHIFT_ADD_STORE_ELEMS(in_ptr0,in_ptr1,out_ptr,result_sign,params.shift_in,params.shift_out,params.in0_sign,params.zero_ifm2)
    }

// TODO code below shows 4 cycle loop but mem stalls
   /*
    accum<acc32,32> acc0 ;
    accum<acc32,32> acc1 ;
    accum<acc32,32> acc_res ;
    vector<int8,64> zero_vec = zeros<int8,64>();
    for (int i=0; i<num_inputs/2; i++)
    chess_prepare_for_pipelining
    chess_loop_range(8,)
    {               
         acc0.from_vector( load_v<32>( in_ptr1 ), 0        ); in_ptr1=byte_incr(in_ptr1, 64);
         acc1.from_vector_sign( load_v<32>( in_ptr0 ),in0_sign,shift_in ); in_ptr0=byte_incr(in_ptr0, 32);
         acc_res=addmac_4x8_8x8_conf(zero_vec,zero_vec,acc0,acc1,0,0,0,0,0);
         store_v(out, acc_res.to_vector_sign<int8_t>(result_sign,shift_out));out=byte_incr(out, 32);
         acc0.from_vector( load_v<32>( in_ptr1 ), 0        ); in_ptr1=byte_incr(in_ptr1, 64);
         acc1.from_vector_sign( load_v<32>( in_ptr0 ),in0_sign,shift_in ); in_ptr0=byte_incr(in_ptr0, 32);
         acc_res=addmac_4x8_8x8_conf(zero_vec,zero_vec,acc0,acc1,0,0,0,0,0);
         store_v(out, acc_res.to_vector_sign<int8_t>(result_sign,shift_out));out=byte_incr(out, 32);
    }
   */
         #if RELU_INT8_ENABLE == 1
         	relu_int8_post_process((int *)out_data, result_sign, params.ofm_len);
         #endif
}

template<typename dtype_in,typename dtype_out,typename config_in,typename config_out>
INLINE void add2d
(     
    adf::input_buffer_conf<dtype_in, config_in>  & __restrict in,    // Input-0 this should be from IFM
    adf::output_buffer_conf<dtype_out, config_out> & __restrict out, // Input-1 and Output. This should be OFM of conv2d output
    add2d_params_t & params
){
    return add2d(in.data(), out.data(), params);
}

template<typename dtype_in,typename dtype_in1,typename dtype_out, int shift_ifm1=0>
NOINLINE void add2d
(     
    dtype_in* __restrict in_data,    // Input-0 this should be from IFM
    dtype_out* __restrict out_data, // Input-1 and Output. This should be OFM of conv2d output
    add2d_params_t &params
){
    
    dtype_in __aie_dm_resource_a * __restrict in_ptr0 = (dtype_in __aie_dm_resource_a * __restrict) (in_data+params.offset); // Input coming from IFM bank
    dtype_in1 __aie_dm_resource_a * __restrict in_ptr1 = (dtype_in1 __aie_dm_resource_a * __restrict) out_data; // Output of conv coming from OFM bank 
    int8_t __aie_dm_resource_a * __restrict out_ptr = (int8_t __aie_dm_resource_a * __restrict) out_data; // OUtput is in same bank as above 

    SETUP_ITR3D_COUNTERS(itr_left)
       
    const int incr = 32 ; // Number of acc lanes
    const int result_sign = 1 - (int) params.act ;
    int shift_out_eltadd;
    if constexpr(shift_ifm1==0){
       shift_out_eltadd=params.shift_out;
    }else{
       shift_out_eltadd=shift_ifm1;
    }


     #define ADD2D_BODY(in_ptr0,in_ptr1,out_ptr,result_sign,shift_in,shift_in1,shift_out,in0_sign,zero_ifm2)\
     {\
         accum<acc32,32> acc0 ;\
         accum<acc32,32> acc1 ;\
         accum<acc32,32> acc_res ;\
         vector<int8,64> zero_vec = zeros<int8,64>();\
         acc0.from_vector_sign( load_v<32>( in_ptr0 ), in0_sign, shift_in );\
         in_ptr0 = INCR_ITR3D_PARAM(in_ptr0, itr_left,params) ;\
         acc1.from_vector_sign( load_v<32>( in_ptr1 ), 1, shift_in1 ); in_ptr1+=incr ;\
         acc_res = aie::add(aie::op_zero(acc0, zero_ifm2), acc1);\
         store_v(out_ptr, acc_res.to_vector_sign<int8_t>(result_sign,shift_out));out_ptr+=incr; \
     }

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(8,) 
    AIE_LOOP_UNROLL(2) 
    for (int i=0; i<params.num_inputs; i++){                
         ADD2D_BODY(in_ptr0,in_ptr1,out_ptr,result_sign,params.shift_in,shift_ifm1,shift_out_eltadd,params.in0_sign,params.zero_ifm2)
    }

         #if RELU_INT8_ENABLE == 1
         	relu_int8_post_process((int *)out_data, result_sign, params.ofm_len);
         #endif
}

template<typename dtype_in,typename dtype_in1,typename dtype_out,typename config_in,typename config_out, int shift_ifm1=0>
INLINE void add2d
(     
    adf::input_buffer_conf<dtype_in, config_in>  & __restrict in,    // Input-0 this should be from IFM
    adf::output_buffer_conf<dtype_out, config_out> & __restrict out, // Input-1 and Output. This should be OFM of conv2d output
    add2d_params_t &params
){
    return add2d<dtype_in, dtype_in1, dtype_out, shift_ifm1>(in.data(), out.data(), params);
}

//#endif

//******************************************bfloat elt add code**************************************************

#if __AIE_ARCH__ == 20

template<typename dtype_ifm1, typename dtype_ifm2, typename dtype_ofm>
NOINLINE void add2d
(
    dtype_ifm1 * __restrict ifm1_data,    // Input-1
    dtype_ifm2 * __restrict ifm2_data,    // Input-2
    dtype_ofm *  __restrict ofm_data, //  Output.
    add2d_bf16_params_t params
){
    dtype_ifm1 __aie_dm_resource_a * __restrict in_ptr1 = (dtype_ifm1 __aie_dm_resource_a * __restrict) (ifm1_data+params.row_offset); // Input1 coming from IFM1 bank
    dtype_ifm2 __aie_dm_resource_b * __restrict in_ptr2 = (dtype_ifm2 __aie_dm_resource_b * __restrict) (ifm2_data+params.col_offset); // Input1 coming from IFM2 bank
    dtype_ofm __aie_dm_resource_c * __restrict out_ptr = (dtype_ofm __aie_dm_resource_c * __restrict) ofm_data; // Output

    const int incr = 16 ; // Number of acc lanes

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(8,) 
    AIE_LOOP_UNROLL(4) 
    for (int i=0; i<params.num_inputs; i++){
         accum<accfloat,16> acc0 ;
         accum<accfloat,16> acc1 ;
         accum<accfloat,16> acc_res ;
         vector<bfloat16,32> zero_vec = zeros<bfloat16,32>();
         acc0.from_vector( load_v<16>( (dtype_ifm1 __aie_dm_resource_a * __restrict) in_ptr1 ),  0); in_ptr1+=incr;
         acc1.from_vector( load_v<16>( (dtype_ifm2 __aie_dm_resource_b * __restrict) in_ptr2 ),  0); in_ptr2+=incr;
         acc_res=addmac_4x8_8x4_conf(zero_vec,zero_vec,acc0,acc1,0,0,0,0);
         store_v(out_ptr, acc_res.to_vector<dtype_ofm>(0));out_ptr+=incr;
    }
}

template<typename dtype_ifm1,typename dtype_ifm2,typename dtype_ofm,typename config_ifm1,typename config_ifm2,typename config_ofm>
INLINE void add2d
(
    adf::input_buffer_conf<dtype_ifm1, config_ifm1>  & __restrict ifm1,    // Input-1
    adf::input_buffer_conf<dtype_ifm2, config_ifm2>  & __restrict ifm2,    // Input-2
    adf::output_buffer_conf<dtype_ofm, config_ofm> & __restrict ofm, //  Output.
    add2d_bf16_params_t & params
){
    return add2d<dtype_ifm1, dtype_ifm2, dtype_ofm>(ifm1.data(), ifm2.data(), ofm.data(), params);
}

#endif

//Add2d-Mul2d bf16 generic code
template<int mode, typename dtype_ifm1,typename dtype_ifm2,typename dtype_ofm>
NOINLINE void add2d
(
    dtype_ifm1 * __restrict ifm1_data,    // Input-1
    dtype_ifm2 * __restrict ifm2_data,    // Input-2
    dtype_ofm *  __restrict ofm_data, //  Output.
    add2d_internal_params_t &params
){
    dtype_ifm1 __aie_dm_resource_a * __restrict in_ptr1 = (dtype_ifm1 __aie_dm_resource_a * __restrict) (ifm1_data); // Input1
    dtype_ifm2 __aie_dm_resource_b * __restrict in_ptr2 = (dtype_ifm2 __aie_dm_resource_b * __restrict) (ifm2_data+params.offset); // Input2
    dtype_ofm __aie_dm_resource_a * __restrict out_ptr = (dtype_ofm __aie_dm_resource_a * __restrict) ofm_data; // IFM1 and OFM are same physical buffers, so they are in same bank

    #define ADD_MUL_GENERIC(ifm1, ifm2, acc_res)\
     {\
      ifm1 = load_v<32>(in_ptr1); in_ptr1+=32;\
      ifm2 = load_v<32>(in_ptr2); in_ptr2+=32;\
      if constexpr(mode == 0){\
      acc_res = mul(ifm1, ifm2);\
      }else{\
      acc_res = add(ifm1, ifm2);\
      }\
      store_v(out_ptr, acc_res.to_vector<dtype_ofm>(0));out_ptr+=32;\
     }

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE(8,)
    AIE_LOOP_UNROLL(2)
    for (int i=0; i<params.num_inputs; i++){
         accum<accfloat,32> acc_res;
         vector<dtype_ifm1, 32> ifm1, ifm2;
         ADD_MUL_GENERIC(ifm1, ifm2, acc_res)
    }
}

template<int mode=0, typename dtype_ifm1,typename dtype_ifm2,typename dtype_ofm,typename config_ifm1,typename config_ifm2,typename config_ofm>
INLINE void add2d
(
    adf::input_buffer_conf<dtype_ifm1, config_ifm1>  & __restrict ifm1,    // Input-1
    adf::input_buffer_conf<dtype_ifm2, config_ifm2>  & __restrict ifm2,    // Input-2
    adf::output_buffer_conf<dtype_ofm, config_ofm> & __restrict ofm, //  Output.
    add2d_internal_params_t & params
){
    //Mode=0: Mul2d, 1:Add2d
    return add2d<mode, dtype_ifm1, dtype_ifm2, dtype_ofm>(ifm1.data(), ifm2.data(), ofm.data(), params);
}

#endif //__ADD2D_H__  
