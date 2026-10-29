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

#pragma once

#include "adf.h"
#include "aie.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"

#include "reduce_base.h"
#include "reduceprod_params.h"

// User configuration for design optimization
template <typename dtype> struct reduceprod_config {
};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct reduceprod_config<int8_t> {
    static constexpr uint8_t inner_unroll_num = 1;
    static constexpr uint16_t vec_size = 16;
    static constexpr bool if_double_partial_result_size = true;
};
template <> struct reduceprod_config<bfloat16> {
    static constexpr uint8_t inner_unroll_num = 1;
    static constexpr uint16_t vec_size = 16;
    static constexpr bool if_double_partial_result_size = false;
};
#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct reduceprod_config<int8_t> {
    static constexpr uint8_t inner_unroll_num = 1;
    // vec_size affects the granularity of W
    static constexpr uint16_t vec_size = 32;
    static constexpr bool if_double_partial_result_size = true;
};
template <> struct reduceprod_config<bfloat16> {
    static constexpr uint8_t inner_unroll_num = 1;
    static constexpr uint16_t vec_size = 32;
    static constexpr bool if_double_partial_result_size = false;
};
#endif  // AIE_ARCH

template <typename dtype>
    requires(std::is_same_v<dtype, bfloat16>)
class reduceprod_impl: public reduce_base<dtype, reduceprod_params_t<dtype>> {
  public:
    static constexpr uint8_t inner_unroll_num = reduceprod_config<dtype>::inner_unroll_num;
    static constexpr uint16_t minimum_num_iter = reduceprod_config<dtype>::minimum_num_iter;
    static constexpr uint16_t vec_size = reduceprod_config<dtype>::vec_size;
    static constexpr bool if_double_partial_result_size = reduceprod_config<dtype>::if_double_partial_result_size;
    using init_type_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, int16_t, int8_t>;
    static constexpr init_type_t zero_iter_init_value = 1;
    using dtype_double_size_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, bfloat16, int16_t>;
    using dtype_data_t = std::conditional_t<if_double_partial_result_size, dtype_double_size_t, dtype>;
    using vec_data_t = aie::vector<dtype_data_t, vec_size>;
    using acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>,
                                     aie::accum<accfloat, vec_size>,
                                     aie::accum<acc32, vec_size>>;
    using vec_t = aie::vector<dtype, vec_size>;
    using mask_t = aie::mask<vec_size>;
    using params_t = reduce_params_t<reduceprod_params_t<dtype>>;

    reduceprod_impl(const params_t &op_params) {
    } // setup auxiliary member variable if needed

    inline __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
        op_params.op_params.shift_out = (uint8_t)(((uint32_t*)rtps)[0]);
    }

    inline __attribute__((always_inline)) static void setup_per_iter(params_t &op_params) {
    }

    inline __attribute__((always_inline)) static void body_hc(vec_t &input_vec, vec_data_t &output_vec,
                                                              params_t &op_params) {
        if constexpr (std::is_same<bfloat16, dtype>::value) {
            output_vec = aie::mul(output_vec, input_vec);
        } else {
            output_vec = aie::mul(output_vec, aie::unpack(input_vec));
        }
    }

    inline __attribute__((always_inline)) static void body_w(
            vec_data_t &output_vec, dtype_data_t &output,
            params_t &op_params) {
        output = aie::reduce_mul(output_vec);
    }

    inline __attribute__((always_inline)) static void body_postprocess(const vec_data_t &in_vec, vec_t &out_vec, params_t &params) {
        if constexpr (std::is_same<dtype, int8>::value) {
            acc_t in_acc(in_vec);
            out_vec = in_acc.template to_vector<dtype>(params.op_params.shift_out);
        }
    }

   private:
    // declare auxiliary members here
};