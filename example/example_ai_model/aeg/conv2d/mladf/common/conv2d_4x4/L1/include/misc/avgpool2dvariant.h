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

#ifndef __AVGPOOL2DVARIANT_H__
#define __AVGPOOL2DVARIANT_H__


#include "aie.hpp"
#include <adf.h>

#include "avgpool2dvariant_params.h"
#include "utils.h"

using namespace aie;

template <uint8_t batch_size, typename dtype>
__attribute__((noinline)) void avgpool2d(
    dtype * __restrict ifm_ptr,
    dtype * __restrict ofm_ptr,
  avgpool2dvariant_internal_params<dtype> &avgpool2dvariant_params)
{
  if constexpr(std::is_same<bfloat16, dtype>::value) {
    dtype __aie_dm_resource_a *__restrict input_ptr = (dtype __aie_dm_resource_a *__restrict)ifm_ptr;
    dtype __aie_dm_resource_b *__restrict output_ptr = (dtype __aie_dm_resource_b *__restrict)ofm_ptr;
    float *__restrict rewrite_ptr = (float*)output_ptr;
    float *__restrict ofm_load_ptr = (float*)output_ptr;
    float *__restrict ofm_init_ptr = (float*)output_ptr;
    dtype *__restrict ifm_load_ptr = input_ptr;
    SETUP_ITR3D_COUNTERS(iterator_global)
    SETUP_ITR2D_COUNTERS(iterator_local)
    vector<dtype, 32> div_vector = broadcast<dtype, 32>(1);
    vector<dtype, 32> input_buff_0;
    accum<accfloat, 32> acc_buff_0; 
    accum<accfloat, 16> acc_buff_1; 
    accum<accfloat, 8> acc_buff_2; 
  
    for (int i = 0; i < avgpool2dvariant_params.inner_loop_count; i++)
    {
      vector<float, 32> ov = broadcast<float, 32>(0);
      store_v(ofm_init_ptr, ov);
      ofm_init_ptr = byte_incr(ofm_init_ptr, 128);
    }

    for (int j = 0; j < avgpool2dvariant_params.outer_loop_count; j++)
    {
      ofm_load_ptr = (float*)output_ptr;
      rewrite_ptr = (float*)output_ptr;
      ifm_load_ptr = input_ptr;
      input_ptr = INCR_ITR2D_PARAM(input_ptr, iterator_local, avgpool2dvariant_params)
      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(2,) 
      for (int i = 0; i < avgpool2dvariant_params.inner_loop_count; i++){
          acc_buff_0.from_vector(aie::load_v<32>(ofm_load_ptr));
          ofm_load_ptr = byte_incr(ofm_load_ptr, 128); 
          input_buff_0 = aie::load_unaligned_v<32>(ifm_load_ptr);
          ifm_load_ptr = INCR_ITR3D_PARAM(ifm_load_ptr, iterator_global, avgpool2dvariant_params)
          acc_buff_0 = mac(acc_buff_0, input_buff_0, avgpool2dvariant_params.div_vector);
          store_v(rewrite_ptr, acc_buff_0.template to_vector<float>(0));
          rewrite_ptr = byte_incr(rewrite_ptr, 128); 
        }          
    }

    dtype* start_ptr = (dtype*)ifm_ptr;
    start_ptr = byte_incr(start_ptr, avgpool2dvariant_params.offset_Margin); 
    
    for (int j = 0; j < avgpool2dvariant_params.num_Margin; j++)
    { 
      ofm_load_ptr = (float*)output_ptr;
      rewrite_ptr = (float*)output_ptr;
      ifm_load_ptr = start_ptr;
      start_ptr = byte_incr(start_ptr, avgpool2dvariant_params.inc_Margin); 
      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(2,) 
      for (int i = 0; i < avgpool2dvariant_params.inner_loop_count; i++){
          acc_buff_0.from_vector(aie::load_v<32>(ofm_load_ptr));
          ofm_load_ptr = byte_incr(ofm_load_ptr, 128); 
          input_buff_0 = aie::load_unaligned_v<32>(ifm_load_ptr);
          ifm_load_ptr = INCR_ITR3D_PARAM(ifm_load_ptr, iterator_global, avgpool2dvariant_params)
          acc_buff_0 = mac(acc_buff_0, input_buff_0, avgpool2dvariant_params.div_vector_margin);
          store_v(rewrite_ptr, acc_buff_0.template to_vector<float>(0));
          rewrite_ptr = byte_incr(rewrite_ptr, 128); 
        }          
    }
    
    ofm_load_ptr = (float*)ofm_ptr;
    for (int i = 0; i < avgpool2dvariant_params.inner_loop_count; i++)
      {
        acc_buff_0.from_vector(aie::load_v<32>(ofm_load_ptr));
        ofm_load_ptr = byte_incr(ofm_load_ptr, 128); 
        acc_buff_1 = aie::add(acc_buff_0.template extract<16>(0), acc_buff_0.template extract<16>(1));
        acc_buff_2 = aie::add(acc_buff_1.template extract<8>(0), acc_buff_1.template extract<8>(1));        
        store_v(output_ptr, acc_buff_2.template to_vector<bfloat16>(0));
        output_ptr = byte_incr(output_ptr, 16); 
      }     
  }
  else {
    dtype __aie_dm_resource_a *__restrict input_ptr = (dtype __aie_dm_resource_a *__restrict)ifm_ptr;
    dtype __aie_dm_resource_b *__restrict output_ptr = (dtype __aie_dm_resource_b *__restrict)ofm_ptr;
    int16_t *__restrict rewrite_ptr = (int16_t*)output_ptr;
    int16_t *__restrict ofm_load_ptr = (int16_t*)output_ptr;
    dtype *__restrict ifm_load_ptr = input_ptr;
    SETUP_ITR3D_COUNTERS(iterator_global)
    SETUP_ITR2D_COUNTERS(iterator_local)
    vector<int16_t, 16> div_vector = broadcast<int16_t, 16>(avgpool2dvariant_params.div_factor);
    vector<dtype, 32> input_buff_0;
    vector<int16_t, 32> out_buff_0, out_buff_1;
    accum<acc32, 32> acc_buff_0, acc_buff_1, acc_buff_2; 
    accum<acc32, 16> acc_buff_3, acc_buff_4; 
    
    for (int j = 0; j < avgpool2dvariant_params.outer_loop_count; j++)
    {
      ofm_load_ptr = (int16_t*)output_ptr;
      rewrite_ptr = (int16_t*)output_ptr;
      ifm_load_ptr = input_ptr;
      input_ptr = INCR_ITR2D_PARAM(input_ptr, iterator_local, avgpool2dvariant_params)
      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(2,) 
      for (int i = 0; i < avgpool2dvariant_params.inner_loop_count; i++){
          acc_buff_0.from_vector(aie::load_v<32>(ofm_load_ptr));
          ofm_load_ptr = byte_incr(ofm_load_ptr, 64); 
          input_buff_0 = aie::load_unaligned_v<32>(ifm_load_ptr);
          ifm_load_ptr = INCR_ITR3D_PARAM(ifm_load_ptr, iterator_global, avgpool2dvariant_params)
          acc_buff_0 = mac(acc_buff_0, input_buff_0, avgpool2dvariant_params.div_vector);
          store_v(rewrite_ptr, acc_buff_0.template to_vector<int16_t>(0));
          rewrite_ptr = byte_incr(rewrite_ptr, 64);
        }          
    }

    dtype* start_ptr = (dtype*)ifm_ptr;
    start_ptr = byte_incr(start_ptr, avgpool2dvariant_params.offset_Margin); 
    
    for (int j = 0; j < avgpool2dvariant_params.num_Margin; j++)
    {
   
      ofm_load_ptr = (int16_t*)output_ptr;
      rewrite_ptr = (int16_t*)output_ptr;
      ifm_load_ptr = start_ptr;
      start_ptr = byte_incr(start_ptr, avgpool2dvariant_params.inc_Margin); 
      AIE_PREPARE_FOR_PIPELINING 
      AIE_LOOP_RANGE(2,) 
      for (int i = 0; i < avgpool2dvariant_params.inner_loop_count; i++){
          acc_buff_0.from_vector(aie::load_v<32>(ofm_load_ptr));
          ofm_load_ptr = byte_incr(ofm_load_ptr, 64); 
          input_buff_0 = aie::load_unaligned_v<32>(ifm_load_ptr);
          ifm_load_ptr = INCR_ITR3D_PARAM(ifm_load_ptr, iterator_global, avgpool2dvariant_params)
          acc_buff_0 = mac(acc_buff_0, input_buff_0, avgpool2dvariant_params.div_vector_margin);
          store_v(rewrite_ptr, acc_buff_0.template to_vector<int16_t>(0));
          rewrite_ptr = byte_incr(rewrite_ptr, 64); 
        }          
    }

    ofm_load_ptr = (int16_t*)ofm_ptr;
    for (int i = 0; i < (avgpool2dvariant_params.inner_loop_count + 1) / 2; i++)
      {
        out_buff_0 = aie::load_v<32>(ofm_load_ptr);
        ofm_load_ptr = byte_incr(ofm_load_ptr, 64); 

        out_buff_1 = aie::load_v<32>(ofm_load_ptr);
        ofm_load_ptr = byte_incr(ofm_load_ptr, 64); 
  
        std::tie(out_buff_0, out_buff_1) = interleave_zip(out_buff_0, out_buff_1, 8);

        acc_buff_0.from_vector(out_buff_0, avgpool2dvariant_params.ifm_shift_biased);  
        acc_buff_1.from_vector(out_buff_1, avgpool2dvariant_params.ifm_shift_biased);  
        acc_buff_2 = aie::add(acc_buff_0, acc_buff_1);
        acc_buff_3 = aie::add(acc_buff_2.template extract<16>(0), acc_buff_2.template extract<16>(1));
        acc_buff_4 = aie::mul(acc_buff_3.template to_vector<int16_t>(0), div_vector);
    
        store_v(output_ptr, acc_buff_4.template to_vector<int8>(avgpool2dvariant_params.ofm_shift_biased + avgpool2dvariant_params.shift_div_factor));
        output_ptr = byte_incr(output_ptr, 16); 
      }          
  }   
}

template <uint8_t batch_size, typename dtype, typename config_ifm, typename config_ofm>
NOINLINE void avgpool2d(
  adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
  adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
  avgpool2dvariant_internal_params<dtype> &avgpool2dvariant_params)
{
  return avgpool2d<batch_size, dtype>(ifm.data(), ofm.data(), avgpool2dvariant_params);
}

#endif //__AVGPOOL2DVARIANT_H__
