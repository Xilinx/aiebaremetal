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

#ifndef __GLOBALAVGPOOL2D_OPT_H__
#define __GLOBALAVGPOOL2D_OPT_H__

#include <adf.h>
#include "aie.hpp"

#include "utils.h"         // Contains some utility functions and definitions related to intrinsics usage
#include "mllib_const.h"   // Constants shared across graphs like activation type
//#include "adf/io_buffer/io_buffer_access.h" // temporary utility from tools to access ping pong buffers, to be removed

//#include "arch_kernel_helpers.h"


#include "globalavgpool2d_opt_params.h"
//#include "utils.hpp"

using namespace aie;

template <uint8_t batch_size, typename dtype>
    requires(mllib::utils::is_one_of_v<dtype,uint16_t, int16_t, uint8_t, bfloat16>) && (batch_size == 1)
void globalavgpool2d(dtype __aie_dm_resource_a * __restrict input_ptr,
                     dtype __aie_dm_resource_a* __restrict output_ptr,
                    typename glbavg_traits<dtype>::param_type &globalavgpool2d_params) {
    typedef typename glbavg_traits<dtype>::acc_type m_acc_type;

    SETUP_ITR2D_COUNTERS(iterator_inner)

   vector<int32, 8> div_vector =
       broadcast<int32, 8>((int32_t)globalavgpool2d_params.div_factor);

    uint64_t offset = globalavgpool2d_params.offset;
  

    uint32_t offsetL = offset & 0xffffffff; // taking lower half
    int32_t offsetH = (((int64_t)offset) >> 32); // taking upper half
    
    vector<int32, 16> vecL = broadcast<int32_t, 16>(offsetL);
    vector<int32, 16> vecH = broadcast<int32_t, 16>(offsetH);

    vector<int32, 16> vecOffset = shuffle(vecL,vecH, 16); // Interleaving

    accum<acc32, 16> acc32_offset;
    acc32_offset.from_vector(vecOffset);
    
    accum<acc64, 8> acc64_offset =  acc32_offset.cast_to<acc64>(); //type casting acc32 to acc64
    
    for (int i = 0; i < globalavgpool2d_params.outer_loop_count; i++) {
        dtype __aie_dm_resource_a* input_ptr_inner = input_ptr;

        vector<dtype, 8> input_buff_0, input_buff_1, input_buff_2, input_buff_3;

        input_buff_0 = load_v<8>(input_ptr_inner);
        input_buff_1 =
            load_v<8>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, globalavgpool2d_params.step_Ci));
        input_buff_2 =
            load_v<8>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 2 * globalavgpool2d_params.step_Ci));
        input_buff_3 =
            load_v<8>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 3 * globalavgpool2d_params.step_Ci));
        
        
        // change: Added 2D iterator to enable height traversal
        input_ptr_inner = INCR_ITR2D_PARAM(input_ptr_inner, iterator_inner, globalavgpool2d_params);

        accum<acc64, 8> result_acc_0 = acc64_offset;
        accum<acc64, 8> result_acc_1 = acc64_offset;
        accum<acc64, 8> result_acc_2 = acc64_offset;
        accum<acc64, 8> result_acc_3 = acc64_offset;

        result_acc_0 = mac( result_acc_0, input_buff_0, div_vector);
        result_acc_1 = mac( result_acc_1, input_buff_1, div_vector);
        result_acc_2 = mac( result_acc_2, input_buff_2, div_vector);
        result_acc_3 = mac( result_acc_3, input_buff_3, div_vector);

        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4, ) 
        for (int j = 1; j < globalavgpool2d_params.inner_loop_count; j++)
              {

                // change: aligned load changed to unaligned load

                input_buff_0 = load_v<8>(input_ptr_inner);

                input_buff_1 =
                    load_v<8>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, globalavgpool2d_params.step_Ci));

                input_buff_2 = load_v<8>(
                    (dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 2 * globalavgpool2d_params.step_Ci));

                input_buff_3 = load_v<8>(
                    (dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 3 * globalavgpool2d_params.step_Ci));

                result_acc_0 = mac(result_acc_0, input_buff_0, div_vector);
                result_acc_1 = mac(result_acc_1, input_buff_1, div_vector);
                result_acc_2 = mac(result_acc_2, input_buff_2, div_vector);
                result_acc_3 = mac(result_acc_3, input_buff_3, div_vector);
                
                input_ptr_inner = INCR_ITR2D_PARAM(input_ptr_inner, iterator_inner, globalavgpool2d_params);
            } // End of inner loop
              
            accum<acc64, 32> acc_out  = concat(concat(result_acc_0,result_acc_1),concat(result_acc_2, result_acc_3));
            vector<dtype, 32> output = acc_out.template to_vector<dtype>(globalavgpool2d_params.div_shift);

            store_v(output_ptr, output);

        input_ptr = (dtype __aie_dm_resource_a*)byte_incr(input_ptr, 4 * globalavgpool2d_params.step_Ci);
        output_ptr = (dtype __aie_dm_resource_a*)byte_incr(output_ptr, 4 * globalavgpool2d_params.step_Co);
    }
}

template <uint8_t batch_size,
          typename dtype,
          typename config_ifm,
          typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype,uint16_t, int16_t, uint8_t, bfloat16>) && (batch_size == 1)
INLINE void globalavgpool2d(adf::input_buffer_conf<dtype, config_ifm>& __restrict ifm,
                     adf::output_buffer_conf<dtype, config_ofm>& __restrict ofm,
                    typename glbavg_traits<dtype>::param_type &globalavgpool2d_params) {
    typedef typename glbavg_traits<dtype>::acc_type m_acc_type;
    dtype __aie_dm_resource_a* input_ptr = (dtype __aie_dm_resource_a*)ifm.data();
    dtype __aie_dm_resource_a* output_ptr = (dtype __aie_dm_resource_a*)ofm.data();
    return globalavgpool2d<batch_size, dtype>(input_ptr, output_ptr, globalavgpool2d_params);
}

#endif //__GLOBALAVGPOOL2D_H__
