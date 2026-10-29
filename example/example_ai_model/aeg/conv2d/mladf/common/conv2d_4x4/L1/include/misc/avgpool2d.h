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

#ifndef __AVGPOOL2D_H__
#define __AVGPOOL2D_H__


#include "aie.hpp"
#include <adf.h>

#include "avgpool2d_params.h"
#include "avgpool2dvariant.h"
#include "utils.h"

using namespace aie;


#define USE_LOAD_64 0 //64 byte load is not giving good schedule!!!. So we are using 32 byte loads.

#if __AIE_ARCH__ == 21     //AIE2p
#ifndef AVGPOOL2D_3X3S1N1
#undef USE_LOAD_64
#define USE_LOAD_64 1
#endif // AVGPOOL2D_3X3S1N1
#endif


template <uint8_t batch_size, typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, uint8_t, bfloat16>)
NOINLINE void avgpool2d(
    dtype * __restrict ifm_ptr,
    dtype * __restrict ofm_ptr,
    avgpool2d_internal_params<dtype> &avgpool2d_params)
{

    if constexpr(std::is_same<bfloat16, dtype>::value) {
        #if USE_LOAD_64 == 1
                dtype __aie_dm_resource_a *__restrict input_ptr = (dtype __aie_dm_resource_a *__restrict)ifm_ptr;
        #else
                dtype __aie_dm_resource_a *__restrict input_ptr = (dtype __aie_dm_resource_a *__restrict)ifm_ptr;
        #endif
        dtype __aie_dm_resource_a *__restrict output_ptr = (dtype __aie_dm_resource_a *__restrict)ofm_ptr;

        SETUP_ITR3D_COUNTERS(iterator)

        vector<dtype, 32> div_vector = broadcast((dtype)avgpool2d_params.div_factor);
        vector<dtype, 32> select_2_or_3 = broadcast((dtype)((dtype)(avgpool2d_params.Kx_g - 2) * (dtype)avgpool2d_params.div_factor)); // div_factor for 3x3 or 0 for 2x2
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(3,) 
        for (int j = 0; j < avgpool2d_params.outer_loop_count; j++){
            vector<dtype, 32> input_buff_0, input_buff_1, input_buff_2, input_buff_3, input_buff_4, shift_buff_0, shift_buff_1, shift_buff_2, shift_buff_3, shift_buff_4, shift_buff_5;
            vector<dtype, 32> interleave_0, interleave_1, interleave_2, interleave_3;
            accum<accfloat, 32> acc_buff_0, acc_buff_1;
            vector<dtype, 32> output_buff_0, output_buff_1;
            vector<dtype, 64> output_buff;

            // The first line
            input_buff_0 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_1 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_2 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_3 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_4 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, avgpool2d_params.inc_Ky);

            interleave_0 = interleave(input_buff_0, input_buff_1, avgpool2d_params.shfl_0);
            interleave_1 = interleave(input_buff_0, input_buff_1, avgpool2d_params.shfl_1);
            interleave_2 = interleave(input_buff_2, input_buff_3, avgpool2d_params.shfl_0);
            interleave_3 = interleave(input_buff_2, input_buff_3, avgpool2d_params.shfl_1);

            shift_buff_1 = interleave_1;
            shift_buff_2 = shuffle_down_fill(interleave_0, interleave_1, avgpool2d_params.shft_1);
            shift_buff_3 = shuffle_down_fill(interleave_1, interleave_2, avgpool2d_params.shft_1);
            shift_buff_4 = shuffle_down_fill(interleave_0, interleave_1, avgpool2d_params.shft_2);
            shift_buff_5 = shuffle_down_fill(interleave_1, interleave_2, avgpool2d_params.shft_3);

            shift_buff_1 = shuffle_down_fill(shift_buff_1, interleave_2, avgpool2d_params.shft_4);
            shift_buff_3 = shuffle_down_fill(shift_buff_3, interleave_3, avgpool2d_params.shft_5);
            shift_buff_4 = shuffle_down_fill(shift_buff_4, interleave_2, avgpool2d_params.shft_6);
            shift_buff_5 = shuffle_down_fill(shift_buff_5, input_buff_4, avgpool2d_params.shft_7);

            acc_buff_0 = mul(interleave_0, div_vector);
            acc_buff_0 = mac(acc_buff_0, shift_buff_2, div_vector);
            acc_buff_0 = mac(acc_buff_0, shift_buff_4, select_2_or_3);

            acc_buff_1 = mul(shift_buff_1, div_vector);
            acc_buff_1 = mac(acc_buff_1, shift_buff_3, div_vector);
            acc_buff_1 = mac(acc_buff_1, shift_buff_5, select_2_or_3);

            // The second line
            input_buff_0 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_1 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_2 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_3 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_4 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, avgpool2d_params.inc_A_0);

            interleave_0 = interleave(input_buff_0, input_buff_1, avgpool2d_params.shfl_0);
            interleave_1 = interleave(input_buff_0, input_buff_1, avgpool2d_params.shfl_1);
            interleave_2 = interleave(input_buff_2, input_buff_3, avgpool2d_params.shfl_0);
            interleave_3 = interleave(input_buff_2, input_buff_3, avgpool2d_params.shfl_1);

            shift_buff_1 = interleave_1;
            shift_buff_2 = shuffle_down_fill(interleave_0, interleave_1, avgpool2d_params.shft_1);
            shift_buff_3 = shuffle_down_fill(interleave_1, interleave_2, avgpool2d_params.shft_1);
            shift_buff_4 = shuffle_down_fill(interleave_0, interleave_1, avgpool2d_params.shft_2);
            shift_buff_5 = shuffle_down_fill(interleave_1, interleave_2, avgpool2d_params.shft_3);

            shift_buff_1 = shuffle_down_fill(shift_buff_1, interleave_2, avgpool2d_params.shft_4);
            shift_buff_3 = shuffle_down_fill(shift_buff_3, interleave_3, avgpool2d_params.shft_5);
            shift_buff_4 = shuffle_down_fill(shift_buff_4, interleave_2, avgpool2d_params.shft_6);
            shift_buff_5 = shuffle_down_fill(shift_buff_5, input_buff_4, avgpool2d_params.shft_7);

            acc_buff_0 = mac(acc_buff_0, interleave_0, div_vector);
            acc_buff_0 = mac(acc_buff_0, shift_buff_2, div_vector);
            acc_buff_0 = mac(acc_buff_0, shift_buff_4, select_2_or_3);

            acc_buff_1 = mac(acc_buff_1, shift_buff_1, div_vector);
            acc_buff_1 = mac(acc_buff_1, shift_buff_3, div_vector);
            acc_buff_1 = mac(acc_buff_1, shift_buff_5, select_2_or_3);

            // The third line
            input_buff_0 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_1 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_2 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_3 = aie::load_v<32>(input_ptr);
            input_ptr = byte_incr(input_ptr, 64);
            input_buff_4 = aie::load_v<32>(input_ptr);

            interleave_0 = interleave(input_buff_0, input_buff_1, avgpool2d_params.shfl_0);
            interleave_1 = interleave(input_buff_0, input_buff_1, avgpool2d_params.shfl_1);
            interleave_2 = interleave(input_buff_2, input_buff_3, avgpool2d_params.shfl_0);
            interleave_3 = interleave(input_buff_2, input_buff_3, avgpool2d_params.shfl_1);

            shift_buff_1 = interleave_1;
            shift_buff_2 = shuffle_down_fill(interleave_0, interleave_1, avgpool2d_params.shft_1);
            shift_buff_3 = shuffle_down_fill(interleave_1, interleave_2, avgpool2d_params.shft_1);
            shift_buff_4 = shuffle_down_fill(interleave_0, interleave_1, avgpool2d_params.shft_2);
            shift_buff_5 = shuffle_down_fill(interleave_1, interleave_2, avgpool2d_params.shft_3);

            shift_buff_1 = shuffle_down_fill(shift_buff_1, interleave_2, avgpool2d_params.shft_4);
            shift_buff_3 = shuffle_down_fill(shift_buff_3, interleave_3, avgpool2d_params.shft_5);
            shift_buff_4 = shuffle_down_fill(shift_buff_4, interleave_2, avgpool2d_params.shft_6);
            shift_buff_5 = shuffle_down_fill(shift_buff_5, input_buff_4, avgpool2d_params.shft_7);

            acc_buff_0 = mac(acc_buff_0, interleave_0, select_2_or_3);
            acc_buff_0 = mac(acc_buff_0, shift_buff_2, select_2_or_3);
            acc_buff_0 = mac(acc_buff_0, shift_buff_4, select_2_or_3);

            acc_buff_1 = mac(acc_buff_1, shift_buff_1, select_2_or_3);
            acc_buff_1 = mac(acc_buff_1, shift_buff_3, select_2_or_3);
            acc_buff_1 = mac(acc_buff_1, shift_buff_5, select_2_or_3);

            output_buff_0 = acc_buff_0.template to_vector<dtype>(avgpool2d_params.div_shift); // bfloat16 do not do shift in to_vector<>()
            store_v(output_ptr, output_buff_0);
            output_ptr = byte_incr(output_ptr, 64);

            output_buff_1 = acc_buff_1.template to_vector<dtype>(avgpool2d_params.div_shift); // bfloat16 do not do shift in to_vector<>()
            store_v(output_ptr, output_buff_1);
            output_ptr = byte_incr(output_ptr, 64);

            input_ptr = INCR_ITR3D_PARAM(input_ptr, iterator, avgpool2d_params)
        }
    } else {
        #if USE_LOAD_64==1
            dtype __aie_dm_resource_a * input_ptr = (dtype __aie_dm_resource_a *) ifm_ptr;
        #else
            dtype __aie_dm_resource_a * input_ptr = (dtype __aie_dm_resource_a *) ifm_ptr;
        #endif

        dtype __aie_dm_resource_a * output_ptr = (dtype __aie_dm_resource_a *) ofm_ptr;

        SETUP_ITR3D_COUNTERS(iterator)
#ifdef AVGPOOL2D_3X3S1N1 // 3x3, stride=1 batch=1
        typedef typename avgpool2d_traits<dtype>::input_container  container_type;
        vector<uint16_t, 32> div_vectorL = broadcast((uint16_t)avgpool2d_params.div_factor);
        vector<uint16_t, 64> div_vector = concat(div_vectorL, div_vectorL);

        AIE_PREPARE_FOR_PIPELINING
        AIE_LOOP_RANGE(3,)
        for (int j=0; j<avgpool2d_params.outer_loop_count; j++)
        
        
        {
            vector<dtype,64> output_buff;
            vector<dtype,64> output_buff_0, output_buff_1;
            vector<container_type,64> input_buff_0, input_buff_2, input_buff_4;

            vector<container_type,64> interleave_0, interleave_1;
            accum<acc32,64> acc_buff_0, acc_buff_1;

            #if USE_LOAD_64==1
                 vector<container_type,32> input_buff_1, input_buff_3;

                input_buff_0 = unpack(load_v<64>(input_ptr));
                input_ptr = byte_incr(input_ptr,64);
                input_buff_1 = unpack(aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,64);
            #else
                vector<container_type,64> input_buff_1, input_buff_3;
                input_buff_0.insert(0, unpack(aie::load_v<32>(input_ptr)));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_0.insert(1, unpack(aie::load_v<32>(input_ptr)));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_1.insert(0, unpack(aie::load_v<32>(input_ptr)));
                input_ptr = byte_incr(input_ptr,64);
            #endif
            input_ptr = byte_incr(input_ptr, avgpool2d_params.inc_Ky);

            #if USE_LOAD_64==1
                input_buff_2 = unpack(load_v<64>(input_ptr));
                input_ptr = byte_incr(input_ptr,64);
                input_buff_3 = unpack(aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,64);
            #else
                input_buff_2.insert(0, unpack(aie::load_v<32>(input_ptr)));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_2.insert(1, unpack(aie::load_v<32>(input_ptr)));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_3.insert(0, unpack(aie::load_v<32>(input_ptr)));
                input_ptr = byte_incr(input_ptr,64);
            #endif
            input_ptr = byte_incr(input_ptr, avgpool2d_params.inc_A_0);

            input_buff_2 =  add(input_buff_2,input_buff_0);
            input_buff_3 =  add(input_buff_3,input_buff_1);

            #if USE_LOAD_64==1
                input_buff_0 = unpack(load_v<64>(input_ptr));
                input_ptr = byte_incr(input_ptr,64);
                input_buff_1 = unpack(aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,64);
            #else
                input_buff_0.insert(0, unpack(aie::load_v<32>(input_ptr)));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_0.insert(1, unpack(aie::load_v<32>(input_ptr)));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_1.insert(0, unpack(aie::load_v<32>(input_ptr)));
                input_ptr = byte_incr(input_ptr,64);
            #endif

            input_buff_2 =  add(input_buff_2,input_buff_0);
            input_buff_3 =  add(input_buff_3,input_buff_1);

            input_buff_4.insert(0, input_buff_3);

            interleave_0 = shuffle_down_fill(input_buff_2, input_buff_4, 8);
            interleave_1 = shuffle_down_fill(input_buff_2, input_buff_4, 16);

            interleave_0 = add(input_buff_2,interleave_0);
            interleave_0 = add(interleave_1,interleave_0);

            acc_buff_0 = mul( interleave_0,div_vector);

            output_buff_0 = acc_buff_0.to_vector<dtype>(avgpool2d_params.div_shift);
            store_v((dtype __aie_dm_resource_a *)output_ptr,output_buff_0);
            output_ptr = byte_incr(output_ptr,64);

            input_ptr = INCR_ITR3D_PARAM(input_ptr, iterator,avgpool2d_params)
        }
#else // Generic flow 
        vector<uint16_t, 32> div_vector = broadcast((uint16_t)avgpool2d_params.div_factor);
        vector<uint16_t, 32> select_2_or_3 = broadcast((uint16_t)((avgpool2d_params.Kx_g - 2) * (uint16_t)avgpool2d_params.div_factor)); // div_factor for 3x3 or 0 for 2x2
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(3,) 
        for (int j=0; j<avgpool2d_params.outer_loop_count; j++){
            vector<dtype,64> input_buff_0, input_buff_1, input_buff_2, shift_buff_0, shift_buff_1, output_buff;
            vector<dtype,64> interleave_0, interleave_1;
            accum<acc32,32> acc_buff_0, acc_buff_1;
            vector<dtype,32> output_buff_0, output_buff_1;

            #if USE_LOAD_64==1
#ifdef __X86SIM__
                input_buff_0 = load_floor_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
                input_buff_1 = load_floor_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
#else
                input_buff_0 = load_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
                input_buff_1 = load_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
#endif
            #else
                input_buff_0.insert(0, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_0.insert(1, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_1.insert(0, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_1.insert(1, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
            #endif
            input_buff_2.insert(0, aie::load_v<32>(input_ptr));
            input_ptr = byte_incr(input_ptr, avgpool2d_params.inc_Ky);

            interleave_0 = interleave(input_buff_0,input_buff_1,avgpool2d_params.shfl_0);
            interleave_1 = interleave(input_buff_0,input_buff_1,avgpool2d_params.shfl_1);

            shift_buff_0 = shuffle_down_fill(interleave_1, input_buff_1, avgpool2d_params.shft_0);
            shift_buff_1 = shuffle_down_fill(interleave_0, input_buff_2, avgpool2d_params.shft_1);
            shift_buff_1 = shuffle_down_fill(shift_buff_1, input_buff_1, avgpool2d_params.shft_2);

            acc_buff_0 = mul(interleave_0.template extract<32>(0),div_vector);
            acc_buff_0 = mac(acc_buff_0,shift_buff_0.template extract<32>(0),div_vector);
            acc_buff_0 = mac(acc_buff_0,shift_buff_1.template extract<32>(0),select_2_or_3);

            acc_buff_1 = mul(interleave_0.template extract<32>(1),div_vector);
            acc_buff_1 = mac(acc_buff_1,shift_buff_0.template extract<32>(1),div_vector);
            acc_buff_1 = mac(acc_buff_1,shift_buff_1.template extract<32>(1),select_2_or_3);

            #if USE_LOAD_64==1
#ifdef __X86SIM__
                input_buff_0 = load_floor_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
                input_buff_1 = load_floor_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
#else
                input_buff_0 = load_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
                input_buff_1 = load_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
#endif
            #else
                input_buff_0.insert(0, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_0.insert(1, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_1.insert(0, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_1.insert(1, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
            #endif
            input_buff_2.insert(0, aie::load_v<32>(input_ptr));
            input_ptr = byte_incr(input_ptr, avgpool2d_params.inc_A_0);

            interleave_0 = interleave(input_buff_0,input_buff_1,avgpool2d_params.shfl_0);
            interleave_1 = interleave(input_buff_0,input_buff_1,avgpool2d_params.shfl_1);

            shift_buff_0 = shuffle_down_fill(interleave_1, input_buff_1, avgpool2d_params.shft_0);
            shift_buff_1 = shuffle_down_fill(interleave_0, input_buff_2, avgpool2d_params.shft_1);
            shift_buff_1 = shuffle_down_fill(shift_buff_1, input_buff_1, avgpool2d_params.shft_2);

            acc_buff_0 = mac(acc_buff_0,interleave_0.template extract<32>(0),div_vector);
            acc_buff_0 = mac(acc_buff_0,shift_buff_0.template extract<32>(0),div_vector);
            acc_buff_0 = mac(acc_buff_0,shift_buff_1.template extract<32>(0),select_2_or_3);

            acc_buff_1 = mac(acc_buff_1,interleave_0.template extract<32>(1),div_vector);
            acc_buff_1 = mac(acc_buff_1,shift_buff_0.template extract<32>(1),div_vector);
            acc_buff_1 = mac(acc_buff_1,shift_buff_1.template extract<32>(1),select_2_or_3);


            #if USE_LOAD_64==1
#ifdef __X86SIM__
                input_buff_0 = load_floor_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
                input_buff_1 = load_floor_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
#else
                input_buff_0 = load_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
                input_buff_1 = load_v<64>(input_ptr);
                input_ptr = byte_incr(input_ptr,64);
#endif
            #else
                input_buff_0.insert(0, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_0.insert(1, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_1.insert(0, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
                input_buff_1.insert(1, aie::load_v<32>(input_ptr));
                input_ptr = byte_incr(input_ptr,32);
            #endif
            input_buff_2.insert(0, aie::load_v<32>(input_ptr));

            interleave_0 = interleave(input_buff_0,input_buff_1,avgpool2d_params.shfl_0);
            interleave_1 = interleave(input_buff_0,input_buff_1,avgpool2d_params.shfl_1);

            shift_buff_0 = shuffle_down_fill(interleave_1, input_buff_1, avgpool2d_params.shft_0);
            shift_buff_1 = shuffle_down_fill(interleave_0, input_buff_2, avgpool2d_params.shft_1);
            shift_buff_1 = shuffle_down_fill(shift_buff_1, input_buff_1, avgpool2d_params.shft_2);

            acc_buff_0 = mac(acc_buff_0,interleave_0.template extract<32>(0),select_2_or_3);
            acc_buff_0 = mac(acc_buff_0,shift_buff_0.template extract<32>(0),select_2_or_3);
            acc_buff_0 = mac(acc_buff_0,shift_buff_1.template extract<32>(0),select_2_or_3);

            acc_buff_1 = mac(acc_buff_1,interleave_0.template extract<32>(1),select_2_or_3);
            acc_buff_1 = mac(acc_buff_1,shift_buff_0.template extract<32>(1),select_2_or_3);
            acc_buff_1 = mac(acc_buff_1,shift_buff_1.template extract<32>(1),select_2_or_3);

            output_buff_0 = acc_buff_0.to_vector<dtype>(avgpool2d_params.div_shift);
            store_v((dtype __aie_dm_resource_a *)output_ptr,output_buff_0);
            output_ptr = byte_incr(output_ptr,32);

            output_buff_1 = acc_buff_1.to_vector<dtype>(avgpool2d_params.div_shift);
            store_v((dtype __aie_dm_resource_a *)output_ptr,output_buff_1);
            output_ptr = byte_incr(output_ptr,32);

            input_ptr = INCR_ITR3D_PARAM(input_ptr, iterator,avgpool2d_params)
        }
#endif //AVGPOOL2D_3X3S1
    }
}

template <uint8_t batch_size, typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, uint8_t, bfloat16>)
INLINE void avgpool2d(
    adf::input_buffer_conf<dtype, config_ifm> &__restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
    avgpool2d_internal_params<dtype> &avgpool2d_params)
{
    return avgpool2d<batch_size, dtype>(ifm.data(), ofm.data(), avgpool2d_params);
}

#endif //__AVGPOOL2D_H__

