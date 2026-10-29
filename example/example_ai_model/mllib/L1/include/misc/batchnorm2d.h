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

#ifndef __BATCH_NORM_2D_H__
#define __BATCH_NORM_2D_H__

#include <adf.h>
#include <aie.hpp>
#include <aie_adf.hpp>
#include <utils.hpp>
#include "batchnorm2d_params.h"
#include "utils.h"

using namespace aie;

// the type here simply act as a light-weight documentation
template<typename dtype, typename T_it>
inline void shuffle_cpy_4x(T_it &it_wts, T_it &it_ofm) {
    using mllib::batchnorm2d::c_stride;
    vector<dtype, 4 * c_stride> v = *it_wts++;                      // v   = [1, 2, 3, 4] each number represents a vector of length c_stride
    const auto &[v13, v24] = interleave_unzip(v, v, c_stride);      // v13 = [1, 3, 1, 3]; v24 = [2, 4, 2, 4]
    const auto &[v1, v3] = interleave_unzip(v13, v13, c_stride);    // v1  = [1, 1, 1, 1]; v3  = [3, 3, 3, 3]
    const auto &[v2, v4] = interleave_unzip(v24, v24, c_stride);    // v2  = [2, 2, 2, 2]; v4  = [4, 4, 4, 4]
    *it_ofm++ = v1;
    *it_ofm++ = v2;
    *it_ofm++ = v3;
    *it_ofm++ = v4;
}

template<typename dtype, typename T_it>
inline void shuffle_cpy_2x(T_it &it_wts, T_it &it_ofm) {
    using mllib::batchnorm2d::c_stride;
    vector<dtype, 2 * c_stride> v = *it_wts++;        // v   = [1, 2] each number represents a vector of length c_stride
    const auto &[v1, v2] = interleave_unzip(v, v, c_stride);// unzip_res = ([1, 1], [2, 2])
    *it_ofm++ = v1;
    *it_ofm++ = v2;
}

template<typename dtype, typename T_it>
inline void shuffle_cpy_8x(T_it &it_wts, T_it &it_ofm) {
    using mllib::batchnorm2d::c_stride;
    const vector<dtype, c_stride * 8> v = *it_wts++;
    const auto &[v11223344, v55667788] = interleave_zip(v, v, c_stride);
    const auto &[v11112222, v33334444] = interleave_zip(v11223344, v11223344, 2 * c_stride);
    const auto &[v55556666, v77778888] = interleave_zip(v55667788, v55667788, 2 * c_stride);
    const auto &[v11111111, v22222222] = interleave_zip(v11112222, v11112222, 4 * c_stride);
    const auto &[v33333333, v44444444] = interleave_zip(v33334444, v33334444, 4 * c_stride);
    const auto &[v55555555, v66666666] = interleave_zip(v55556666, v55556666, 4 * c_stride);
    const auto &[v77777777, v88888888] = interleave_zip(v77778888, v77778888, 4 * c_stride);
    *it_ofm++ = v11111111;
    *it_ofm++ = v22222222;
    *it_ofm++ = v33333333;
    *it_ofm++ = v44444444;
    *it_ofm++ = v55555555;
    *it_ofm++ = v66666666;
    *it_ofm++ = v77777777;
    *it_ofm++ = v88888888;
}

/*
restrictions:
1. inner loop loads at least load_sz elements a time, make sure W * N * c_stride is enough
*/
template<typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
NOINLINE void batchnorm2d(
    dtype * __restrict ifm_ptr,
    dtype * __restrict wts_ptr,
    dtype * __restrict ofm_ptr,
    batchnorm2d_params_t &param
) {
    using namespace mllib::batchnorm2d;
    using cb_t = typename std::conditional<std::is_same_v<dtype, int8>, int16, bfloat16>::type;
    using acc_t = typename std::conditional<std::is_same_v<dtype, bfloat16>, accfloat, acc32>::type;
    constexpr unsigned lsz = load_sz<dtype>();
    auto it_ofm_cw = begin_vector<lsz, aie_dm_resource::a>(ofm_ptr + param.ofm_cw_offset);
    auto it_ofm_cb = begin_vector<lsz, aie_dm_resource::a>((cb_t * __restrict)(ofm_ptr + param.ofm_cb_offset));
    auto it_wts_cw = begin_vector<lsz, aie_dm_resource::a>(wts_ptr);
    auto it_wts_cb = begin_vector<lsz, aie_dm_resource::a>((cb_t * __restrict)(wts_ptr + param.wts_cb_offset));
    // note even there are only 8 wts elements, we will copy wts_dup * 8 elements into buffer
    const int shuffle_loop_cnt = param.wts_cb_offset / lsz;
    AIE_NO_PREPARE_FOR_PIPELINING 
    for (int i = 0; i < shuffle_loop_cnt; ++i) {
        if constexpr (wts_dup<dtype>() == 8) {
            // int8 case requires a copy of 4 times
            shuffle_cpy_8x<dtype>(it_wts_cw, it_ofm_cw);
            shuffle_cpy_8x<cb_t>(it_wts_cb, it_ofm_cb);
        } else if constexpr (wts_dup<dtype>() == 4) {
            shuffle_cpy_4x<dtype>(it_wts_cw, it_ofm_cw);
            shuffle_cpy_4x<cb_t>(it_wts_cb, it_ofm_cb);
        } else {
            shuffle_cpy_2x<dtype>(it_wts_cw, it_ofm_cw);
            shuffle_cpy_2x<cb_t>(it_wts_cb, it_ofm_cb);
        }
    }
    // we use a circular iterator here, note only the first channel elements are considered
    auto cit_ofm_cw = begin_vector_circular<lsz, aie_dm_resource::a>(ofm_ptr + param.ofm_cw_offset, param.channel * wts_dup<dtype>());
    auto cit_ofm_cb = begin_vector_circular<lsz, aie_dm_resource::a>((cb_t * __restrict)(ofm_ptr + param.ofm_cb_offset), param.channel * wts_dup<dtype>());
    auto it_ofm = begin_vector<lsz, aie_dm_resource::b>(ofm_ptr);
    auto it_ifm = begin_vector<lsz, aie_dm_resource::a>(ifm_ptr);

    const int output_shift = param.output_shift;
    const int outer_loop_cnt = param.outer_loop_cnt;
    const int inner_loop_cnt = param.inner_loop_cnt;
    for (int i = 0; i < outer_loop_cnt; ++i)
    {
        auto cw = *cit_ofm_cw++;
        auto cb = *cit_ofm_cb++;
        accum<acc_t, lsz> acc_cb(cb);
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(14,) 
        for (int j = 0; j < inner_loop_cnt; ++j){
            accum<acc_t, lsz> acc = mac(acc_cb, *it_ifm++, cw);
            *it_ofm++ = acc.template to_vector<dtype>(output_shift);
        }
    }
}

template<typename dtype, typename config_ifm, typename config_wts, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
INLINE void batchnorm2d(
    adf::input_buffer_conf<dtype, config_ifm> & __restrict ifm,
    adf::input_buffer_conf<dtype, config_wts> & __restrict wts,
    adf::output_buffer_conf<dtype, config_ofm> & __restrict ofm,
    batchnorm2d_params_t &param
) {
    return batchnorm2d<dtype>(ifm.data(), wts.data(), ofm.data(), param);
}
#endif
