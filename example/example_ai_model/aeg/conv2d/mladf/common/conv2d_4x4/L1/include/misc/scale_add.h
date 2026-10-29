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
    
    
#ifndef __SCALE_ADD_H__
#define __SCALE_ADD_H__

#include "aie.hpp"
#include <adf.h>
#include <stdio.h>
#include "utils.h"
#include "mllib_const.h"
#include "scale_add_params.h"

using namespace aie;

template <typename dtype>
void __attribute__((noinline)) scale_add(
   dtype* __restrict ifm_data,
   dtype* __restrict ofm_data,
   scale_add_params_t param
) {

   dtype __aie_dm_resource_a * __restrict p_ifm = (dtype __aie_dm_resource_a * __restrict) (ifm_data + param.col_offset);
   dtype __aie_dm_resource_c * __restrict p_ofm = (dtype __aie_dm_resource_c * __restrict) ofm_data;
   
   SETUP_ITR2D_COUNTERS(iterator_mat)
   SETUP_ITR2D_COUNTERS(iterator_mat_out)
   
   if constexpr(std::is_same<bfloat16, dtype>::value){   
   
   #if __AIE_ARCH__ == 20
      #define SA_VEC_LEN 32
      #define SA_LOAD_GRAN 16
      #define LOOP_RANGE 2      
     
      #define SA_LOAD_DATA(p_ifm, iterator_mat, param, X0, X1)\
      {\
         X0.insert(0, load_v<SA_LOAD_GRAN>(p_ifm));p_ifm=byte_incr(p_ifm, param.incr0);\
         X0.insert(1, load_v<SA_LOAD_GRAN>(p_ifm));p_ifm=byte_incr(p_ifm, param.incr0);\
         X1.insert(0, load_v<SA_LOAD_GRAN>(p_ifm));p_ifm=byte_incr(p_ifm, param.incr0);\
         X1.insert(1, load_v<SA_LOAD_GRAN>(p_ifm));p_ifm=byte_incr(p_ifm, 0);\
         p_ifm = INCR_ITR2D_PARAM(p_ifm, iterator_mat, param);\
      }
      //along_col ? X0=h(0:7)w(0:3), X1=h(8:15)w(0:3) : X0=h(0:3)w(0:7), X1=h(0:3)w(8:15)
   
      #define SA_STORE_DATA(p_ofm, iterator_mat_out, param, C0_vec, C1_vec)\
      {\
         store_v(p_ofm, C0_vec.template extract<16>(0)); p_ofm=byte_incr(p_ofm, param.incr0);\
         store_v(p_ofm, C0_vec.template extract<16>(1)); p_ofm=byte_incr(p_ofm, param.incr0);\
         store_v(p_ofm, C1_vec.template extract<16>(0)); p_ofm=byte_incr(p_ofm, param.incr0);\
         store_v(p_ofm, C1_vec.template extract<16>(1)); p_ofm=byte_incr(p_ofm, 0);\
         p_ofm = INCR_ITR2D_PARAM(p_ofm, iterator_mat_out, param);\
      }
   
      vector<dtype,SA_LOAD_GRAN> scale_vector = broadcast<dtype,SA_LOAD_GRAN>(*((dtype *)&param.scale));
      accum<accfloat, SA_LOAD_GRAN> add_acc;
      add_acc.from_vector(broadcast<dtype,SA_LOAD_GRAN>(*((dtype *)&param.add_value)));

   #else
      #define SA_VEC_LEN 64
      #define SA_LOAD_GRAN 32
      #define LOOP_RANGE 5

      vector<dtype, SA_LOAD_GRAN>scale_bcst, add_bcst;
      scale_bcst = broadcast<dtype,SA_LOAD_GRAN>(*((dtype *)&param.scale));
      add_bcst = broadcast<dtype,SA_LOAD_GRAN>(*((dtype *)&param.add_value));
      accum<accfloat, SA_LOAD_GRAN> add_acc_temp = from_vector<accfloat>(add_bcst);

      vector<dtype,SA_VEC_LEN> scale_vector;
      scale_vector.insert(0, scale_bcst);
      scale_vector.insert(1, scale_bcst);
      accum<accfloat, SA_VEC_LEN> add_acc;
      add_acc.insert(0, add_acc_temp);
      add_acc.insert(1, add_acc_temp);

   #endif

      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(LOOP_RANGE,) 
      for(int i=0; i<param.num_iters; i++){
      #if __AIE_ARCH__ == 20

         vector<dtype,SA_VEC_LEN> X0, X1, Y0, Y1, C0_temp, C1_temp, C0_vec, C1_vec;
         vector<dtype,SA_LOAD_GRAN> Mi0, Mi1, Mi2, Mi3;
         accum<accfloat,SA_LOAD_GRAN> M0, M1, M2, M3;

         SA_LOAD_DATA(p_ifm, iterator_mat, param, X0, X1);
      
         X0 = interleave(X0, X1, param.shuffle_mode0);
         X1 = interleave(X1, X0, param.shuffle_mode0);
         //dumy operation if along_col

         Y0 = interleave(X0, X1, 24); //h(0:1)w(0:15)
         Y1 = interleave(X0, X1, 25); //h(2:3)w(0:15)

         M0 = mac(add_acc, Y0.template extract<16>(0), scale_vector); //h(0)w(0:15)
         M1 = mac(add_acc, Y0.template extract<16>(1), scale_vector); //h(1)w(0:15)
         M2 = mac(add_acc, Y1.template extract<16>(0), scale_vector); //h(2)w(0:15)
         M3 = mac(add_acc, Y1.template extract<16>(1), scale_vector); //h(3)w(0:15)
      
         std::tie(Mi0, Mi1) = interleave_zip(M0.template to_vector<dtype>(param.shift_out), M1.template to_vector<dtype>(param.shift_out), param.interleave_mode);
         std::tie(Mi2, Mi3) = interleave_zip(M2.template to_vector<dtype>(param.shift_out), M3.template to_vector<dtype>(param.shift_out), param.interleave_mode);
         //dumy operation if along_col

         C0_temp = concat(Mi0, Mi1); //along_col ? h(0)w(0:15)h(1)w(0:15) : h(0:1)w(0:3)h(0:1)w(4:7)h(0:1)w(8:11)h(0:1)w(12:15)
         C1_temp = concat(Mi2, Mi3); //along_col ? h(2)w(0:15)h(3)w(0:15) : h(2:3)w(0:3)h(2:3)w(4:7)h(2:3)w(8:11)h(2:3)w(12:15)
      
         C0_vec = interleave(C0_temp, C1_temp, param.shuffle_mode1);
         C1_vec = interleave(C0_temp, C1_temp, param.shuffle_mode2);
         //along_col ? C0_vec=h(0:7)w(0:3), C1_vec=h(8:15)w(0:3) : C0_vec=h(0:3)w(0:7), C1_vec=h(0:3)w(8:15)

         SA_STORE_DATA(p_ofm, iterator_mat_out, param, C0_vec, C1_vec);

      #else
         vector<dtype,SA_VEC_LEN> X0, X1, C0_vec, C1_vec;
         accum<accfloat,SA_VEC_LEN> M0, M1;

         X0.insert(0, load_v<SA_LOAD_GRAN>(p_ifm));p_ifm=byte_incr(p_ifm, param.incr0);
         X0.insert(1, load_v<SA_LOAD_GRAN>(p_ifm));p_ifm=byte_incr(p_ifm, param.incr0);
         X1.insert(0, load_v<SA_LOAD_GRAN>(p_ifm));p_ifm=byte_incr(p_ifm, param.incr0);
         X1.insert(1, load_v<SA_LOAD_GRAN>(p_ifm));p_ifm=byte_incr(p_ifm, param.incr0);

         M0 = mac(add_acc, X0, scale_vector);
         M1 = mac(add_acc, X1, scale_vector);

         store_v(p_ofm, M0.template to_vector<dtype>(param.shift_out).template extract<SA_LOAD_GRAN>(0)); p_ofm=byte_incr(p_ofm, param.incr0);
         store_v(p_ofm, M0.template to_vector<dtype>(param.shift_out).template extract<SA_LOAD_GRAN>(1)); p_ofm=byte_incr(p_ofm, param.incr0);
         store_v(p_ofm, M1.template to_vector<dtype>(param.shift_out).template extract<SA_LOAD_GRAN>(0)); p_ofm=byte_incr(p_ofm, param.incr0);
         store_v(p_ofm, M1.template to_vector<dtype>(param.shift_out).template extract<SA_LOAD_GRAN>(1)); p_ofm=byte_incr(p_ofm, param.incr0);

      #endif
      }
   }else{
   
   #if __AIE_ARCH__ == 20   
      #define SA_VEC_LEN 32
   #else
      #define SA_VEC_LEN 64
   #endif
      
      vector<dtype,SA_VEC_LEN> scale_vector = broadcast<dtype,SA_VEC_LEN>(*((dtype *)&param.scale));
      accum<acc32, SA_VEC_LEN> add_acc;
      add_acc.from_vector(broadcast<dtype,SA_VEC_LEN>(*((dtype *)&param.add_value)));
     
#ifdef __X86SIM__
      #define SA_LOAD_DATA(p_ifm, iterator_mat, param, X0, X1)\
      {\
         X0 = load_floor_v<SA_VEC_LEN>(p_ifm);p_ifm=byte_incr(p_ifm, param.incr0);\
         X1 = load_floor_v<SA_VEC_LEN>(p_ifm);p_ifm=byte_incr(p_ifm, 0);\
         p_ifm = INCR_ITR2D_PARAM(p_ifm, iterator_mat, param);\
      }
      
      #define SA_STORE_DATA(p_ofm, iterator_mat_out, param, C0, C1)\
      {\
         store_unaligned_v(p_ofm, C0.template to_vector_sign<dtype>(param.out_sign,param.shift_out));  p_ofm = byte_incr(p_ofm, param.incr0);\
         store_unaligned_v(p_ofm, C1.template to_vector_sign<dtype>(param.out_sign,param.shift_out));  p_ofm = byte_incr(p_ofm, 0);\
         p_ofm = INCR_ITR2D_PARAM(p_ofm, iterator_mat_out, param);\
      }
#else 
      #define SA_LOAD_DATA(p_ifm, iterator_mat, param, X0, X1)\
      {\
         X0 = load_v<SA_VEC_LEN>(p_ifm);p_ifm=byte_incr(p_ifm, param.incr0);\
         X1 = load_v<SA_VEC_LEN>(p_ifm);p_ifm=byte_incr(p_ifm, 0);\
         p_ifm = INCR_ITR2D_PARAM(p_ifm, iterator_mat, param);\
      }
      
      #define SA_STORE_DATA(p_ofm, iterator_mat_out, param, C0, C1)\
      {\
         store_v(p_ofm, C0.template to_vector_sign<dtype>(param.out_sign,param.shift_out));  p_ofm = byte_incr(p_ofm, param.incr0);\
         store_v(p_ofm, C1.template to_vector_sign<dtype>(param.out_sign,param.shift_out));  p_ofm = byte_incr(p_ofm, 0);\
         p_ofm = INCR_ITR2D_PARAM(p_ofm, iterator_mat_out, param);\
      }
#endif
 
      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(4,) 
      for(int i=0; i<param.num_iters; i++){
         vector<dtype,SA_VEC_LEN> X0, X1;
         accum<acc32,SA_VEC_LEN> C0, C1;
         
         SA_LOAD_DATA(p_ifm, iterator_mat, param, X0, X1);
         
         C0 = mac(add_acc, X0, scale_vector);
         C1 = mac(add_acc, X1, scale_vector);
         
         SA_STORE_DATA(p_ofm, iterator_mat_out, param, C0, C1);
      }   
   }
}

template <typename dtype, typename config_ifm, typename config_ofm>
INLINE void scale_add(
   adf::input_buffer_conf<dtype, config_ifm>  & __restrict ifm,
   adf::output_buffer_conf<dtype, config_ofm> & __restrict ofm,
   scale_add_params_t param
) {
   return scale_add<dtype>(ifm.data(), ofm.data(), param);
}

#endif //__SCALE_ADD_H__
