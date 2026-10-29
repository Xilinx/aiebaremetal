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

#pragma once

#include <array>
#include <type_traits>
#include <typeinfo>
#include <assert.h>

#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"
#include "utils.h"
#include "templatized_traits.h"

// Use (void) to silence unused warnings
#define assertm(exp, msg) assert(((void)msg, exp))

// #define __DEBUG_REDUCE_BASE_C8__

template <class concrete_param_t>
struct alignas(v32int16) reduce_c8_params_t {
    // ===================================================================
    // Following variables are external
    // ===================================================================
    uint32_t sv_height = 0;   // height of sv
    uint32_t sv_channel = 0;  // channel of sv
    uint32_t sv_width = 0;    // width of sv
    // reduce_axis = 0: forbidded
    // reduce_axis = 1: reduction dim is C
    // reduce_axis = 2: reduction dim is W
    // reduce_axis = 3: reduction dim is W & C
    // reduce_axis = 4: reduction dim is H
    // reduce_axis = 5: reduction dim is H & C
    // reduce_axis = 6: reduction dim is H & W
    // reduce_axis = 7: reduction dim is H & W & C
    // reduce_axis > 7: forbidded
    uint32_t reduce_axis = 7;
    uint32_t loop_num_height = 0;   // sv tiling loop numbers along height dim
    uint32_t loop_num_channel = 0;  // sv tiling loop numbers along channel dim
    uint32_t loop_num_width = 0;    // sv tiling loop numbers along width dim

    // ===================================================================
    // Following variables are internal
    // ===================================================================
    uint16_t num_iter = 0;
    uint16_t all_loop_num = 0;

    // loop_width indicates the iter times, ofm will sync and emit output every loop_width.
    // reduce_axis = 0: forbidded
    // reduce_axis = 1: loop_width = loop_num_channel
    // reduce_axis = 2: loop_width = loop_num_width
    // reduce_axis = 3: loop_width = loop_num_width * loop_num_channel
    // reduce_axis = 4: loop_width = loop_num_height
    // reduce_axis = 5: loop_width = loop_num_height * loop_num_channel
    // reduce_axis = 6: loop_width = loop_num_height * loop_num_width
    // reduce_axis = 7: loop_width = loop_num_height * loop_num_width * loop_num_channel
    // reduce_axis > 7: forbidded
    uint16_t loop_width = 0;
    uint16_t curr_depth_iter = 0;
    // number of independent lanes for the operator to be interact with
    // XXX: this notion is especially important for the 3-axis reduction,
    // as in-place update occurs under this circumstance, it will be splitted
    // by 4 (defalut) lanes for the operator to be interact with the IFM, thus
    // getting the "partial" intermediate reduced results, the final reduction
    // must be taken into consideration in body_postprocess implemented by the
    // user in the concrete operator implementation. The smaller of this variable
    // set, the higher risk the read-before-write may happens, causing the wrong
    // result generated by the kernel
    int32_t outer_loop = 0;
    // number of iterations under the granularity of the vec_size reflect on the corresponding reduction dimension(s)
    int32_t inner_loop = 0;

    // io_mode = 0                   means that sv is at the begining of volume
    // 0 < io_mode < loop_width - 1  means that sv is at the middle of volume
    // io_mode = loop_width - 1      means that sv is at the end of volume
    uint16_t io_mode = 0;

    // address interval between the adjacent unrolled processing lances
    int16_t i_ptr_inc_unroll = 0;
    // add_3d_ptr params
    int16_t i_ptr_inc1 = 0;
    uint16_t i_ptr_size1 = 0;
    int16_t i_ptr_inc2 = 0;
    uint16_t i_ptr_size2 = 0;
    int16_t i_ptr_offset = 0;
    // add_2d_ptr params
    uint16_t p_out_size = 0;
    int16_t p_out_offset = 0;

    concrete_param_t op_params;
};

template <typename dtype, class concrete_param_t>
class reduce_base_c8 {
  public:
    // this is default configuration and it's the most conservative way,
    // op implementers should tune these configurations for their benefit
    static constexpr uint8_t inner_unroll_num = 1;
    static constexpr uint8_t accum_3d_unroll_num = 4; // XXX: this variable is used to avoid in-place update conflicts, the smaller of this number, the higher risk the read-before-write occurs
    static constexpr uint16_t minimum_num_iter = 1;
    static constexpr uint16_t vec_size = reduce_c8_trait<dtype>::vec_size;
    static constexpr dtype zero_iter_init_value = 0;
    static constexpr bool if_double_partial_result_size = false;
    static constexpr bool if_highest_partial_result_size = false; // for those operators require highest precision on partial result, e.g. ReduceL2
    static constexpr bool if_postprocess = false;
    using dtype_double_size_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, bfloat16, int16_t>;
    using dtype_highest_size_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, float, int32_t>;
    using acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>,
                                     aie::accum<accfloat, vec_size>,
                                     aie::accum<acc32, vec_size>>;
    using dtype_data_t = std::conditional_t<if_highest_partial_result_size, dtype_highest_size_t, std::conditional_t<if_double_partial_result_size, dtype_double_size_t, dtype>>;
    using vec_data_t = aie::vector<dtype_data_t, vec_size>;
    using vec_t = aie::vector<dtype, vec_size>;
    using params_t = reduce_c8_params_t<concrete_param_t>;

    reduce_base_c8() = default;
    reduce_base_c8(const params_t &op_param){};
    reduce_base_c8(const reduce_base_c8 &) = delete;
    reduce_base_c8 &operator=(const reduce_base_c8 &) = delete;
    // XXX: our user can tune the performance by simply changing the [inner_unroll_num, acum_3d_unroll_num,
    // minimum_num_iter, vec_size] variables that we provided in our skeleton, as a general code template
    // is given here
    inline __attribute__((always_inline)) void body_main(vec_t &input_vec, vec_data_t &output_vec,
                                                       params_t &op_params) {}

    // XXX: as there is no general processing structure for all reduce operators, unlike the
    // main body function of the reduce-based operators where they can share the same code
    // template, it is not a good idea to give a constaining code template for the post processing
    // logic here. e.g. some OP may not have 3-axis reduction, it can save much PM by eliminating
    // the processing logic in this function. Hence, we leave this part to our users to let them
    // trade-off between performance and resources. However, if the operator does not require the
    // post-processing part, it can simply turn off this function in the code template by giving
    // a `false` value to if_postprocess defined in this skeleton
    inline __attribute__((always_inline)) void body_postprocess(void* p_out, params_t &op_params) {}

    inline __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {}

    inline __attribute__((always_inline)) static void setup_per_iter(params_t &op_params) {}
};

template <typename dtype, class concrete_operator, class concrete_param_t>
    requires(std::is_base_of_v<reduce_base_c8<dtype, concrete_param_t>, concrete_operator>)
class reduce_skeleton_c8 {
  public:
    using vec_t = aie::vector<dtype, concrete_operator::vec_size>;
    using dtype_double_size_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, bfloat16, int16_t>;
    using dtype_highest_size_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, float, int32_t>;
    using dtype_data_t = std::conditional_t<concrete_operator::if_highest_partial_result_size, dtype_highest_size_t, std::conditional_t<concrete_operator::if_double_partial_result_size, dtype_double_size_t, dtype>>;
    using vec_data_t = aie::vector<dtype_data_t, concrete_operator::vec_size>;
    using acc_t = std::conditional_t<std::is_same_v<dtype, bfloat16>,
            aie::accum<accfloat, concrete_operator::vec_size>,
            aie::accum<acc32, concrete_operator::vec_size>>;
    using ifm_ptr_t = dtype __aie_dm_resource_a *__restrict;
    using ofm_ptr_t = dtype __aie_dm_resource_b *__restrict;
    using ofm_data_ptr_t = dtype_data_t __aie_dm_resource_b *__restrict;
    static constexpr int k_vec_size = concrete_operator::vec_size;
    static constexpr int k_inner_unroll_num = concrete_operator::inner_unroll_num;
    static constexpr uint32_t k_byte_incr_size = vec_t::size() * sizeof(dtype);
    using params_t = reduce_c8_params_t<concrete_param_t>;

    __attribute__((noinline)) static const void setup(params_t &params,
                                                      const void *rtps_void) {
        const uint32_t *rtps = (const uint32_t*)rtps_void;
        params.sv_height = rtps[0];
        params.sv_channel = rtps[1];
        params.sv_width = rtps[2];
        params.reduce_axis = rtps[3];
        params.loop_num_height = rtps[4];
        params.loop_num_channel = rtps[5];
        params.loop_num_width = rtps[6];
#ifdef __DEBUG_REDUCE_BASE_C8__
        printf("sv_height: %d\n", params.sv_height);
        printf("sv_channel: %d\n", params.sv_channel);
        printf("sv_width: %d\n", params.sv_width);
        printf("reduce_axis = %d\n", params.reduce_axis);
        printf("loop_num_height: %d\n", params.loop_num_height);
        printf("loop_num_channel: %d\n", params.loop_num_channel);
        printf("loop_num_width: %d\n", params.loop_num_width);
#endif
        switch (params.reduce_axis) {
            case 1: { // reduce C
                    params.i_ptr_inc_unroll = params.sv_width * 8;
                    params.i_ptr_inc1 = k_vec_size - params.sv_width * (k_inner_unroll_num - 1) * 8;
                    params.i_ptr_size1 = (params.sv_width / (k_vec_size / 8)) - 1;
                    params.i_ptr_inc2 = k_vec_size - params.sv_width * k_inner_unroll_num * 8 + params.sv_width * params.sv_channel;
                    params.i_ptr_size2 = params.sv_height - 1;
                    params.i_ptr_offset = k_vec_size - params.sv_width * params.sv_channel * (params.sv_height - 1);
                    params.outer_loop = params.sv_height * params.sv_width / (k_vec_size / 8);
                    params.inner_loop = params.sv_channel / 8;
                    params.loop_width = params.loop_num_channel;
                }
                break;
            case 2: { // reduce W
                    params.i_ptr_inc_unroll = k_vec_size;
                    params.i_ptr_inc1 = -k_vec_size * (k_inner_unroll_num - 1) + params.sv_width * 8;
                    params.i_ptr_size1 = params.sv_channel / 8 - 1;
                    params.i_ptr_inc2 = 8 * (params.sv_width - (k_inner_unroll_num - 1) * (k_vec_size / 8));
                    params.i_ptr_size2 = params.sv_height - 1;
                    params.i_ptr_offset = k_vec_size + 8 * params.sv_width - params.sv_width * params.sv_channel * params.sv_height;
                    params.outer_loop = params.sv_height * params.sv_channel / 8;
                    params.inner_loop = params.sv_width / (k_vec_size / 8);
                    params.loop_width = params.loop_num_width;
                }
                break;
            case 3: { // reduce WC
                    params.i_ptr_inc_unroll = params.sv_width * 8;
                    params.i_ptr_inc1 = params.sv_width * (params.sv_channel - (k_inner_unroll_num - 1) * 8);
                    params.i_ptr_size1 = params.sv_height - 1;
                    params.i_ptr_inc2 = 8 * params.sv_width - params.sv_width * params.sv_channel * (params.sv_height - 1);
                    params.i_ptr_size2 = params.sv_channel / 8 / k_inner_unroll_num - 1;
                    params.i_ptr_offset = k_vec_size - params.sv_width * params.sv_channel * (params.sv_height - 1) - (params.sv_channel / 8 - 1) * 8 * params.sv_width;
                    params.outer_loop = params.sv_height;
                    params.inner_loop = (params.sv_channel * params.sv_width) / k_vec_size;
                    params.loop_width = params.loop_num_channel * params.loop_num_width;
                }
                break;
            case 4: { // reduce H
                    params.i_ptr_inc_unroll = params.sv_width * params.sv_channel;
                    params.i_ptr_inc1 = k_vec_size - params.sv_width * params.sv_channel * (k_inner_unroll_num - 1);
                    params.i_ptr_size1 = params.sv_width / (k_vec_size / 8) - 1;
                    params.i_ptr_inc2 = k_vec_size - params.sv_width * params.sv_channel * (k_inner_unroll_num - 1);
                    params.i_ptr_size2 = params.sv_channel / 8 - 1;
                    params.i_ptr_offset = k_vec_size;
                    params.outer_loop = params.sv_width * params.sv_channel / k_vec_size;
                    params.inner_loop = params.sv_height;
                    params.loop_width = params.loop_num_height;
                }
                break;
            case 5: { // reduce HC
                    params.i_ptr_inc_unroll = params.sv_width * 8;
                    params.i_ptr_inc1 = k_vec_size - params.sv_width * (k_inner_unroll_num - 1) * 8;
                    params.i_ptr_size1 = params.sv_width / (k_vec_size / 8) - 1;
                    params.i_ptr_inc2 = k_vec_size;
                    params.i_ptr_size2 = params.sv_channel / 8 / k_inner_unroll_num - 1;
                    params.i_ptr_offset = k_vec_size;
                    params.outer_loop = params.sv_width / (k_vec_size / 8);
                    params.inner_loop = params.sv_channel * params.sv_height / 8;
                    params.loop_width = params.loop_num_channel * params.loop_num_height;
                }
                break;
            case 6: { // reduce HW
                    params.i_ptr_inc_unroll = k_vec_size;
                    params.i_ptr_inc1 = -k_vec_size * (k_inner_unroll_num - 1) + params.sv_width * 8;
                    params.i_ptr_size1 = params.sv_channel / 8 - 1;
                    params.i_ptr_inc2 = k_vec_size - (params.sv_channel / 8 - 1) * 8 * params.sv_width;
                    params.i_ptr_size2 = params.sv_width / (k_vec_size / 8) / k_inner_unroll_num - 1;
                    params.i_ptr_offset = k_vec_size;
                    params.outer_loop = params.sv_channel / 8;
                    params.inner_loop = (params.sv_width * params.sv_height) / (k_vec_size / 8);
                    params.loop_width = params.loop_num_width * params.loop_num_height;
                }
                break;
            case 7: { // reduce HCW
                    params.i_ptr_inc_unroll = k_vec_size;
                    params.i_ptr_inc1 = k_vec_size;
                    params.i_ptr_size1 = (params.sv_channel * params.sv_width * params.sv_height) / k_vec_size - 1;
                    params.i_ptr_inc2 = 0;
                    params.i_ptr_size2 = 1;
                    params.i_ptr_offset = 0;
                    params.outer_loop = concrete_operator::accum_3d_unroll_num;
                    params.inner_loop = (params.sv_channel * params.sv_width * params.sv_height) / (k_vec_size * concrete_operator::accum_3d_unroll_num);
                    params.loop_width = params.loop_num_channel * params.loop_num_width * params.loop_num_height;
                }
                break;
            default: {
                assertm((params.reduce_axis > 0) && (params.reduce_axis < 8), "[ERROR]: reduce_axis must be in range [1, 7]");
                break;
            }
        }
        params.all_loop_num = params.loop_num_height * params.loop_num_channel * params.loop_num_width;
        params.curr_depth_iter = 0;
        params.io_mode = 0;
        params.p_out_offset = -k_vec_size * params.outer_loop + k_vec_size;
        params.p_out_size = params.outer_loop - 1;
        params.num_iter = params.sv_height * params.sv_channel * params.sv_width / k_vec_size;

        concrete_operator::setup(params, rtps + 7);
    };

    inline __attribute__((always_inline)) static const void setup_per_iter(params_t &params) {
        params.curr_depth_iter++;
        params.io_mode = params.curr_depth_iter % params.loop_width;
        concrete_operator::setup_per_iter(params);
#ifdef __DEBUG_REDUCE_BASE_C8__
        printf("io_mode: %d\n", params.io_mode);
#endif
    }

    __attribute__ ((noinline)) static const void run(dtype* __restrict ifm,
                                                     dtype* __restrict ofm,
                                                     params_t & __restrict params) {
        concrete_operator op_impl(params);
        ifm_ptr_t p_in = (ifm_ptr_t)ifm;
        ofm_ptr_t p_out = (ofm_ptr_t)ofm;
        // initialization at the very beginning of the reduction dimension
        if (params.io_mode == 0) {
            ofm_data_ptr_t p_out_init = (ofm_data_ptr_t)ofm;
            aie::vector<dtype_data_t, k_vec_size> init_output_vec;
            // bfloat16/float cannot be directly initialized as a constexpr, use int16_t/int32 as a workround
            init_output_vec = aie::broadcast<dtype_data_t, k_vec_size>(concrete_operator::zero_iter_init_value);
            for (int32_t i = 0; i < params.outer_loop; i++) {
                aie::store_v(p_out_init, init_output_vec);
                p_out_init = byte_incr(p_out_init, k_vec_size * sizeof(dtype_data_t));
            }
        }

        std::array<vec_t, k_inner_unroll_num> in_vec_group;
        vec_data_t reduce_vec;
        addr_t i_ptr_addr_count1 = 0;
        addr_t i_ptr_addr_count2 = 0;
        ofm_data_ptr_t o_ptr_load = (ofm_data_ptr_t)ofm;
        ofm_data_ptr_t o_ptr_store = (ofm_data_ptr_t)ofm;
        addr_t o_ptr_load_count = 0;
        addr_t o_ptr_store_count = 0;
        // core reduction logic, traversal through the entire sub-volume
        int i_iter = params.num_iter / k_inner_unroll_num;
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(concrete_operator::minimum_num_iter, ) 
        for (int i = 0; i < i_iter; ++i){
            // load previous reduced intermediate data
            reduce_vec = aie::load_v<k_vec_size>(o_ptr_load);
#ifdef __DEBUG_REDUCE_BASE_C8__
            aie::print(reduce_vec, true, "reduce_vec = ");
#endif
            o_ptr_load = add_2d_ptr(o_ptr_load, params.p_out_offset, params.p_out_size, o_ptr_load_count, k_vec_size);

            // core reduction logic
            unroll_body_main<k_inner_unroll_num>::run(
                op_impl, in_vec_group, reduce_vec, params, p_in, i_ptr_addr_count1, i_ptr_addr_count2);

            // store current reduced intermediate data
            aie::store_v(o_ptr_store, reduce_vec);
            o_ptr_store = add_2d_ptr(o_ptr_store, params.p_out_offset, params.p_out_size, o_ptr_store_count, k_vec_size);
        }

        // In the last kernel launch, in either one of the following conditons will trigger body_postprocess to execute:
        //     1. Use double partial_result_size for higher precision
        //     2. Generic post process logic is needed after reduction is done, like average, squre root, etc.
        if constexpr (concrete_operator::if_double_partial_result_size || concrete_operator::if_postprocess) {
            if (params.io_mode == params.loop_width - 1) {
                concrete_operator::body_postprocess((void*)p_out, params);
            }
        }

        // run setup_per_iter function for excuting the setup-params function of kernel that needs tort
        // be run after each iteration of the kernel
        setup_per_iter(params);
    }

   private:
    template <uint8_t INNER_NUM, uint8_t C = INNER_NUM>
    struct unroll_body_main {
        inline __attribute__((always_inline)) static void run(concrete_operator &op_impl,
                                                              std::array<vec_t, INNER_NUM> &in_vec_group,
                                                              vec_data_t &reduce_vec,
                                                              params_t & __restrict params,
                                                              ifm_ptr_t &p_in,
                                                              addr_t &i_ptr_addr_count1,
                                                              addr_t &i_ptr_addr_count2) {
            std::get<INNER_NUM - C>(in_vec_group).load(p_in);
            p_in = p_in + params.i_ptr_inc_unroll;
#ifdef __DEBUG_REDUCE_BASE_C8__
            aie::print(std::get<INNER_NUM-C>(in_vec_group), true, "in_vec = ");
#endif
            op_impl.body_main(std::get<INNER_NUM - C>(in_vec_group), reduce_vec, params);
            unroll_body_main<INNER_NUM, C - 1>::run(
                    op_impl, in_vec_group, reduce_vec, params, p_in,
                    i_ptr_addr_count1, i_ptr_addr_count2);
        }
    };

    template <uint8_t INNER_NUM>
    struct unroll_body_main<INNER_NUM , 1> {
        inline __attribute__((always_inline)) static void run(concrete_operator &op_impl,
                                                              std::array<vec_t, INNER_NUM> &in_vec_group,
                                                              vec_data_t &reduce_vec,
                                                              params_t & __restrict params,
                                                              ifm_ptr_t &p_in,
                                                              addr_t &i_ptr_addr_count1,
                                                              addr_t &i_ptr_addr_count2) {
            std::get<INNER_NUM - 1>(in_vec_group).load(p_in);
#ifdef __DEBUG_REDUCE_BASE_C8__
            aie::print(std::get<INNER_NUM-1>(in_vec_group), true, "in_vec = ");
#endif
            p_in = add_3d_ptr(p_in, params.i_ptr_offset, params.i_ptr_size1,
                              i_ptr_addr_count1, params.i_ptr_inc1, params.i_ptr_size2,
                              i_ptr_addr_count2, params.i_ptr_inc2);
            op_impl.body_main(std::get<INNER_NUM - 1>(in_vec_group), reduce_vec, params);
        }
    };
};
