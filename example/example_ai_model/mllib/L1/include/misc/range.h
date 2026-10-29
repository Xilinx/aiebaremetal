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

#ifndef __RANGE_H__
#define __RANGE_H__

#include "aie.hpp"
#include <adf.h>
#include "aie_api/utils.hpp"
#include "range_params.h"

template <typename dtype, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
NOINLINE void range(adf::output_buffer_conf<dtype, config_ofm> &__restrict ofm,
                    range_params_t<dtype> &params)
{
    dtype __aie_dm_resource_a *p_out = (dtype __aie_dm_resource_a *)ofm.data();
    dtype __aie_dm_resource_a *p_base = (dtype __aie_dm_resource_a *)params.base;
    aie::vector<dtype, RANGE_VEC_LEN> base_vec = load_v<RANGE_VEC_LEN>(p_base);
    aie::vector<dtype, RANGE_VEC_LEN> delta_vec = aie::broadcast<dtype, RANGE_VEC_LEN>(params.delta);


    auto base_delta_accum = aie::mul(base_vec, delta_vec);
    auto out_accum = aie::add(base_delta_accum, params.start);

    // iter-1...n
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_UNROLL(RANGE_UNROLL) 
    AIE_LOOP_RANGE(2,) 
    for (int i = 0; i < params.num_iters; i++){
        if constexpr (std::is_same<int8, dtype>::value){
            auto t1 = aie::upshift(out_accum.template to_vector<int16_t>(0), params.shift_out);
            auto t2 = aie::downshift(t1, params.shift_in);
            auto t3 = aie::pack(t2);
            store_v(p_out, t3);
        }
        else {
            store_v(p_out, out_accum.template to_vector<dtype>(0));
        }
        p_out = byte_incr(p_out, RANGE_VEC_LEN * sizeof(dtype));
        out_accum = aie::add(out_accum, params.vec_len_delta);
    }

    // mask last 2 iter
    p_out = byte_incr(p_out, -(RANGE_VEC_LEN * sizeof(dtype)));
    aie::vector<dtype, RANGE_VEC_LEN> out_vec = load_v<RANGE_VEC_LEN>(p_out);
    aie::vector<dtype, RANGE_VEC_LEN> zero_vec = aie::zeros<dtype, RANGE_VEC_LEN>();
    aie::vector<dtype, RANGE_VEC_LEN> out = aie::select(zero_vec, out_vec, params.last_iter_mask_1);
    store_v(p_out, out);

    p_out = byte_incr(p_out, -(RANGE_VEC_LEN * sizeof(dtype)));
    out_vec = load_v<RANGE_VEC_LEN>(p_out);
    out = aie::select(zero_vec, out_vec, params.last_iter_mask_0);
    store_v(p_out, out);

}

#endif //__RANGE_H__
