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



#ifndef __MAXPOOL2D_H__
#define __MAXPOOL2D_H__

#if __AIE_ARCH__ == 22
#define USE_LOAD_64 1 // Set to 1 to use load_v<64>
                      // Set to 0 to use load_v<32>
#else
#define USE_LOAD_64 0
#endif

#include "aie.hpp"
#include <adf.h>

#include "maxpool2d_params.h"
#include "utils.h"

//#define CONV2D_MAXPOOL_2x2S1_FUSION

using namespace aie;

template<uint8_t batch_size, typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, uint8_t, bfloat16>)
INLINE void maxpool2d
(
    dtype * __restrict ifm_ptr,
    dtype * __restrict ofm_ptr,
    maxpool2d_params_t  &maxpool2d_params
){

    if constexpr(std::is_same<bfloat16, dtype>::value) {
        // ---- bfloat16 code ----
        v32bfloat16 __aie_dm_resource_a * input_ptr = (v32bfloat16 __aie_dm_resource_a *) ifm_ptr;
        v64bfloat16 __aie_dm_resource_a * ouput_ptr = (v64bfloat16 __aie_dm_resource_a *) ofm_ptr;

        SETUP_ITR3D_COUNTERS(iterator)

        /*    #ifndef CONV2D_MAXPOOL_2x2S1_FUSION
            // NOTE: For cases that dont need Conv2D+MaxPool2x2/s1 fusion, disable invocation pad2d kernel call.
            uint8_t core_col = 0;
            #else
            // NOTE: For Conv2D+MaxPool2x2/s1 fusion case, init function in the wrapper must set core_col global variable
            // with the logical column id extracted from physical core_id. This also needs the absolute kernel location pinning constraints
            // to be enabled in order to have a deterministic logical to physical core id mapping.
            extern uint8_t core_col;
            #endif
            uint8_t col_pad_mask;
            uint16_t pad_mask;
        */

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(MAXPOOL2D_LOOP_RANGE,) 
        for (int j=0; j<maxpool2d_params.outer_loop_count; j++){
            vector<dtype,32> input_buff_0, input_buff_1, input_buff_2, input_buff_3, input_buff_4;
            vector<dtype,32> interleave_0, interleave_1, interleave_2, interleave_3, interleave_4;
            vector<dtype,32> shift_buff_0, shift_buff_1, shift_buff_2, shift_buff_3, shift_buff_4, shift_buff_5;
            vector<dtype,64> output_buff;
            vector<dtype,64> concat_buff_0, concat_buff_1, concat_buff_2;

            // The first line
            input_buff_0 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_1 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_2 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_3 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_4 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);

            input_ptr = byte_incr(input_ptr, maxpool2d_params.inc_Ky);

            interleave_0 = interleave(input_buff_0,input_buff_1,maxpool2d_params.shfl_0);
            interleave_1 = interleave(input_buff_0,input_buff_1,maxpool2d_params.shfl_1);
            interleave_2 = interleave(input_buff_2,input_buff_3,maxpool2d_params.shfl_0);
            interleave_3 = interleave(input_buff_2,input_buff_3,maxpool2d_params.shfl_1);

            shift_buff_1 = interleave_1;
            shift_buff_2 = shuffle_down_fill(interleave_0, interleave_1, maxpool2d_params.shft_1);
            shift_buff_3 = shuffle_down_fill(interleave_1, interleave_2, maxpool2d_params.shft_1);
            shift_buff_4 = shuffle_down_fill(interleave_0, interleave_1, maxpool2d_params.shft_2);
            shift_buff_5 = shuffle_down_fill(interleave_1, interleave_2, maxpool2d_params.shft_3);

            shift_buff_1 = shuffle_down_fill(shift_buff_1, interleave_2, maxpool2d_params.shft_4);
            shift_buff_3 = shuffle_down_fill(shift_buff_3, interleave_3, maxpool2d_params.shft_5);
            shift_buff_4 = shuffle_down_fill(shift_buff_4, interleave_2, maxpool2d_params.shft_6);
            shift_buff_5 = shuffle_down_fill(shift_buff_5, input_buff_4, maxpool2d_params.shft_7);

            concat_buff_0 = concat(interleave_0, shift_buff_1);
            concat_buff_1 = concat(shift_buff_2, shift_buff_3);
            concat_buff_2 = concat(shift_buff_4, shift_buff_5);

            output_buff = concat_buff_0;
            output_buff = max(output_buff,concat_buff_1);
            output_buff = max(output_buff,concat_buff_2);

            // The second line 
#ifdef __X86SIM__
            input_buff_0 = load_floor_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_1 = load_floor_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_2 = load_floor_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_3 = load_floor_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_4 = load_floor_v<32>((dtype __aie_dm_resource_a *)input_ptr);

#else
            input_buff_0 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_1 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_2 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_3 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_4 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
#endif

            input_ptr = byte_incr(input_ptr, maxpool2d_params.inc_A_0);

            interleave_0 = interleave(input_buff_0,input_buff_1,maxpool2d_params.shfl_0);
            interleave_1 = interleave(input_buff_0,input_buff_1,maxpool2d_params.shfl_1);
            interleave_2 = interleave(input_buff_2,input_buff_3,maxpool2d_params.shfl_0);
            interleave_3 = interleave(input_buff_2,input_buff_3,maxpool2d_params.shfl_1);

            shift_buff_1 = interleave_1;
            shift_buff_2 = shuffle_down_fill(interleave_0, interleave_1, maxpool2d_params.shft_1);
            shift_buff_3 = shuffle_down_fill(interleave_1, interleave_2, maxpool2d_params.shft_1);
            shift_buff_4 = shuffle_down_fill(interleave_0, interleave_1, maxpool2d_params.shft_2);
            shift_buff_5 = shuffle_down_fill(interleave_1, interleave_2, maxpool2d_params.shft_3);

            shift_buff_1 = shuffle_down_fill(shift_buff_1, interleave_2, maxpool2d_params.shft_4);
            shift_buff_3 = shuffle_down_fill(shift_buff_3, interleave_3, maxpool2d_params.shft_5);
            shift_buff_4 = shuffle_down_fill(shift_buff_4, interleave_2, maxpool2d_params.shft_6);
            shift_buff_5 = shuffle_down_fill(shift_buff_5, input_buff_4, maxpool2d_params.shft_7);

            concat_buff_0 = concat(interleave_0, shift_buff_1);
            concat_buff_1 = concat(shift_buff_2, shift_buff_3);
            concat_buff_2 = concat(shift_buff_4, shift_buff_5);


            output_buff = max(output_buff,concat_buff_0);
            output_buff = max(output_buff,concat_buff_1);
            output_buff = max(output_buff,concat_buff_2);

            // The third line

            input_buff_0 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_1 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_2 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_3 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);
            input_ptr++;
            input_buff_4 = load_v<32>((dtype __aie_dm_resource_a *)input_ptr);


            interleave_0 = interleave(input_buff_0,input_buff_1,maxpool2d_params.shfl_0);
            interleave_1 = interleave(input_buff_0,input_buff_1,maxpool2d_params.shfl_1);
            interleave_2 = interleave(input_buff_2,input_buff_3,maxpool2d_params.shfl_0);
            interleave_3 = interleave(input_buff_2,input_buff_3,maxpool2d_params.shfl_1);

            shift_buff_1 = interleave_1;
            shift_buff_2 = shuffle_down_fill(interleave_0, interleave_1, maxpool2d_params.shft_1);
            shift_buff_3 = shuffle_down_fill(interleave_1, interleave_2, maxpool2d_params.shft_1);
            shift_buff_4 = shuffle_down_fill(interleave_0, interleave_1, maxpool2d_params.shft_2);
            shift_buff_5 = shuffle_down_fill(interleave_1, interleave_2, maxpool2d_params.shft_3);

            shift_buff_1 = shuffle_down_fill(shift_buff_1, interleave_2, maxpool2d_params.shft_4);
            shift_buff_3 = shuffle_down_fill(shift_buff_3, interleave_3, maxpool2d_params.shft_5);
            shift_buff_4 = shuffle_down_fill(shift_buff_4, interleave_2, maxpool2d_params.shft_6);
            shift_buff_5 = shuffle_down_fill(shift_buff_5, input_buff_4, maxpool2d_params.shft_7);

            concat_buff_0 = concat(interleave_0, shift_buff_1);
            concat_buff_1 = concat(shift_buff_2, shift_buff_3);
            concat_buff_2 = concat(shift_buff_4, shift_buff_5);

            output_buff = max(output_buff,concat_buff_0);
            output_buff = max(output_buff,concat_buff_1);
            output_buff = max(output_buff,concat_buff_2);

            store_v((dtype __aie_dm_resource_a *)ouput_ptr,output_buff);
            ouput_ptr++;

            //TODO: Need to replace with High level Api
            input_ptr = INCR_ITR3D_PARAM(input_ptr, iterator,maxpool2d_params)
        }
    } else {
        #if USE_LOAD_64==1
            v64int8 __aie_dm_resource_a * input_ptr = (v64int8 __aie_dm_resource_a *) ifm_ptr;
        #else
            v32int8 __aie_dm_resource_a * input_ptr = (v32int8 __aie_dm_resource_a *) ifm_ptr;
        #endif

        v64int8 __aie_dm_resource_a * ouput_ptr = (v64int8 __aie_dm_resource_a *) ofm_ptr;


        SETUP_ITR3D_COUNTERS(iterator)

        /*    #ifndef CONV2D_MAXPOOL_2x2S1_FUSION
            // NOTE: For cases that dont need Conv2D+MaxPool2x2/s1 fusion, disable invocation pad2d kernel call.
            uint8_t core_col = 0;
            #else
            // NOTE: For Conv2D+MaxPool2x2/s1 fusion case, init function in the wrapper must set core_col global variable
            // with the logical column id extracted from physical core_id. This also needs the absolute kernel location pinning constraints
            // to be enabled in order to have a deterministic logical to physical core id mapping.
            extern uint8_t core_col;
            #endif
            uint8_t col_pad_mask;
            uint16_t pad_mask;
        */

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(MAXPOOL2D_LOOP_RANGE,) 
        for (int j=0; j<maxpool2d_params.outer_loop_count; j++){
            vector<dtype,64> input_buff_0, input_buff_1, input_buff_2, input_buff_3, input_buff_4, input_buff_5, shift_buff_0, shift_buff_1,shift_buff_2, output_buff;
            vector<dtype,64> interleave_0, interleave_1;
            #if USE_LOAD_64==1
                input_buff_0 = load_v<64>((dtype __aie_dm_resource_a *)input_ptr);
                input_ptr++;
                input_buff_1 = load_v<64>((dtype __aie_dm_resource_a *)input_ptr);
                input_ptr++;
            #else
                input_buff_0.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
                input_buff_0.insert(1, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
                input_buff_1.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
                input_buff_1.insert(1, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
            #endif

            input_buff_2.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));

            //TODO: Need to replace with High level Api
            input_ptr = byte_incr(input_ptr, maxpool2d_params.inc_Ky);

            #if USE_LOAD_64==1
                input_buff_3 = load_v<64>((dtype __aie_dm_resource_a *)input_ptr);
                input_ptr++;
                input_buff_4 = load_v<64>((dtype __aie_dm_resource_a *)input_ptr);
                input_ptr++;
            #else
                input_buff_3.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
                input_buff_3.insert(1, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
                input_buff_4.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
                input_buff_4.insert(1, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
            #endif

            input_buff_5.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));

            //TODO: Need to replace with High level Api
            input_ptr = byte_incr(input_ptr, maxpool2d_params.inc_A_0);

            input_buff_3 = max(input_buff_0,input_buff_3);
            input_buff_4 = max(input_buff_1,input_buff_4);
            input_buff_5 = max(input_buff_2,input_buff_5);

            #if USE_LOAD_64==1
                input_buff_0 = load_v<64>((dtype __aie_dm_resource_a *)input_ptr);
                input_ptr++;
                input_buff_1 = load_v<64>((dtype __aie_dm_resource_a *)input_ptr);
                input_ptr++;
            #else
                input_buff_0.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
                input_buff_0.insert(1, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
                input_buff_1.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
                input_buff_1.insert(1, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));
                input_ptr++;
            #endif
            input_buff_2.insert(0, aie::load_v<32>((dtype __aie_dm_resource_a *)input_ptr));

            input_buff_0 = max(input_buff_0,input_buff_3);
            input_buff_1 = max(input_buff_1,input_buff_4);
            input_buff_2 = max(input_buff_2,input_buff_5);

            interleave_0 = interleave(input_buff_0,input_buff_1,maxpool2d_params.shfl_0);
            interleave_1 = interleave(input_buff_0,input_buff_1,maxpool2d_params.shfl_1);

            shift_buff_0 = shuffle_down_fill(interleave_1, input_buff_1, maxpool2d_params.shft_0);
            shift_buff_1 = shuffle_down_fill(interleave_0, input_buff_2, maxpool2d_params.shft_1);
            shift_buff_1 = shuffle_down_fill(shift_buff_1, input_buff_1, maxpool2d_params.shft_2);
            input_buff_2 = shuffle_down_fill(input_buff_2, input_buff_2, 8);
            shift_buff_2 = shuffle_down_fill(interleave_1, input_buff_2, maxpool2d_params.shft_3);

            output_buff = interleave_0;
            output_buff = max(output_buff,shift_buff_0);
            output_buff = max(output_buff,shift_buff_1);
            output_buff = max(output_buff,shift_buff_2);

            store_v((dtype __aie_dm_resource_a *)ouput_ptr,output_buff);
            ouput_ptr++;

            //TODO: Need to replace with High level Api
            input_ptr = INCR_ITR3D_PARAM(input_ptr, iterator,maxpool2d_params)
        }
    }
}


template<uint8_t batch_size, typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, uint8_t, bfloat16>)
void maxpool2d
(
    adf::input_buffer_conf<dtype, config_ifm>  & __restrict ifm,
    adf::output_buffer_conf<dtype, config_ofm> & __restrict ofm,
    maxpool2d_params_t  &maxpool2d_params
){
    maxpool2d<batch_size, dtype>(ifm.data(), ofm.data(), maxpool2d_params);
}
#endif //__MAXPOOL2D_H__

