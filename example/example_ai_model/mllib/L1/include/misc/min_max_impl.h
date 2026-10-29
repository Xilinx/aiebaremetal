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
#include <array>

#include "adf.h"
#include "aie.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"

#include "elementwise_binary.h"
// may extend to other simple elementwise binary op
enum class min_max_op {
    MIN = 0,
    MAX
};

namespace {
template <typename dtype>
struct min_max_config {};
template <>
struct min_max_config<int8_t> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 10;
    static constexpr uint16_t vec_size = 64;
};
#if __AIE_ARCH__ == 20
template <>
struct min_max_config<bfloat16> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 12;
    static constexpr uint16_t vec_size = 16;
};
#else
template <>
struct min_max_config<bfloat16> {
    static constexpr bool peel = false;
    static constexpr uint8_t unroll = 1;
    static constexpr uint16_t minimum_num_iter = 12;
    static constexpr uint16_t vec_size = 32;
};
#endif // __AIE_ARCH__
} // anonymous namespace

// dummy struct
struct min_max_params_t {};

template <typename dtype, min_max_op op>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class min_max_impl : public elementwise_binary_base<dtype, min_max_params_t> {
   public:
    static constexpr bool peel = min_max_config<dtype>::peel;
    static constexpr uint8_t unroll = min_max_config<dtype>::unroll;
    static constexpr uint16_t minimum_num_iter = min_max_config<dtype>::minimum_num_iter;
    static constexpr uint16_t vec_size = min_max_config<dtype>::vec_size;
    using vec_t = aie::vector<dtype, vec_size>;
    using params_t = elementwise_binary_params_t<min_max_params_t>;

   public:
    min_max_impl(const params_t& op_params) {} // setup auxiliary member variable if needed
    inline __attribute__((always_inline)) void body(const vec_t& in_vec1,
                                                    const vec_t& in_vec2,
                                                    vec_t& out_vec,
                                                    params_t& op_params) {
        fn_ptr fn = nullptr;
        if constexpr (op == min_max_op::MIN) {
            fn = aie::min;
        } else if (op == min_max_op::MAX) {
            fn = aie::max;
        }
        out_vec = fn(in_vec1, in_vec2);
    }
    inline __attribute__((always_inline)) static void setup(params_t& op_params, const void* rtps) {}

   private:
        using fn_ptr =  vec_t (*)(const vec_t&, const vec_t&);
};