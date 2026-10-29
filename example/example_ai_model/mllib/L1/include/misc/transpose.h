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



#ifndef __TRANSPOSE_H__
#define __TRANSPOSE_H__

#include "aie.hpp"
#include <adf.h>

#include "utils.h"
#include "mllib_const.h"
#include "transpose_params.h"
#include <math.h>

using namespace aie;

#define TRANSPOSE_16x16_1B(iv1, iv2, iv3, iv4, tv1, tv2, tv3, tv4, ov1, ov2, ov3, ov4)\
{\
    ov1 = shuffle(iv1, iv1, T8_4x16);\
    ov2 = shuffle(iv2, iv2, T8_4x16);\
    ov3 = shuffle(iv3, iv3, T8_4x16);\
    ov4 = shuffle(iv4, iv4, T8_4x16);\
\
    tv1 = shuffle(ov1, ov2, T32_2x16_lo);\
    tv2 = shuffle(ov1, ov2, T32_2x16_hi);\
    tv3 = shuffle(ov3, ov4, T32_2x16_lo);\
    tv4 = shuffle(ov3, ov4, T32_2x16_hi);\
\
    ov1 = shuffle(tv1, tv3, T64_2x8_lo);\
    ov2 = shuffle(tv1, tv3, T64_2x8_hi);\
    ov3 = shuffle(tv2, tv4, T64_2x8_lo);\
    ov4 = shuffle(tv2, tv4, T64_2x8_hi);\
}

#define TRANSPOSE_16x8_2B(iv1, iv2, iv3, iv4, tv1, tv2, tv3, tv4, ov1, ov2, ov3, ov4) \
{\
    ov1 = shuffle(iv1, iv1, T16_4x8);\
    ov2 = shuffle(iv2, iv2, T16_4x8);\
    ov3 = shuffle(iv3, iv3, T16_4x8);\
    ov4 = shuffle(iv4, iv4, T16_4x8);\
\
    tv1 = shuffle(ov1, ov2, T64_2x8_lo);\
    tv2 = shuffle(ov1, ov2, T64_2x8_hi);\
    tv3 = shuffle(ov3, ov4, T64_2x8_lo);\
    tv4 = shuffle(ov3, ov4, T64_2x8_hi);\
\
    ov1 = shuffle(tv1, tv3, T128_2x4_lo);\
    ov2 = shuffle(tv1, tv3, T128_2x4_hi);\
    ov3 = shuffle(tv2, tv4, T128_2x4_lo);\
    ov4 = shuffle(tv2, tv4, T128_2x4_hi);\
}

#define LOAD_16x16_1B(v1, v2, v3, v4, in_ptr, stride_bytes)\
{\
    v1.insert(0, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v1.insert(1, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v1.insert(2, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v1.insert(3, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
\
    v2.insert(0, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v2.insert(1, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v2.insert(2, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v2.insert(3, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
\
    v3.insert(0, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v3.insert(1, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v3.insert(2, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v3.insert(3, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
\
    v4.insert(0, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v4.insert(1, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v4.insert(2, load_v<16>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v4.insert(3, load_v<16>(in_ptr));\
}

#define STORE_16x16_1B(v1, v2, v3, v4, out_ptr, stride_bytes)\
{\
    store_v(out_ptr, v1.template extract<16>(0)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v1.template extract<16>(1)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v1.template extract<16>(2)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v1.template extract<16>(3)); out_ptr = byte_incr(out_ptr, stride_bytes);\
\
    store_v(out_ptr, v2.template extract<16>(0)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v2.template extract<16>(1)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v2.template extract<16>(2)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v2.template extract<16>(3)); out_ptr = byte_incr(out_ptr, stride_bytes);\
\
    store_v(out_ptr, v3.template extract<16>(0)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v3.template extract<16>(1)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v3.template extract<16>(2)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v3.template extract<16>(3)); out_ptr = byte_incr(out_ptr, stride_bytes);\
\
    store_v(out_ptr, v4.template extract<16>(0)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v4.template extract<16>(1)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v4.template extract<16>(2)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v4.template extract<16>(3));\
}

#define LOAD_16x8_2B(v1, v2, v3, v4, in_ptr, stride_bytes)\
{\
    v1.insert(0, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v1.insert(1, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v1.insert(2, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v1.insert(3, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
\
    v2.insert(0, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v2.insert(1, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v2.insert(2, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v2.insert(3, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
\
    v3.insert(0, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v3.insert(1, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v3.insert(2, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v3.insert(3, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
\
    v4.insert(0, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v4.insert(1, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v4.insert(2, load_v<8>(in_ptr)); in_ptr = byte_incr(in_ptr, stride_bytes);\
    v4.insert(3, load_v<8>(in_ptr));\
}

#define STORE_8x16_2B(v1, v2, v3, v4, out_ptr, stride_bytes)\
{\
    store_v(out_ptr, v1.template extract<16>(0)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v1.template extract<16>(1)); out_ptr = byte_incr(out_ptr, stride_bytes);\
\
    store_v(out_ptr, v2.template extract<16>(0)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v2.template extract<16>(1)); out_ptr = byte_incr(out_ptr, stride_bytes);\
\
    store_v(out_ptr, v3.template extract<16>(0)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v3.template extract<16>(1)); out_ptr = byte_incr(out_ptr, stride_bytes);\
\
    store_v(out_ptr, v4.template extract<16>(0)); out_ptr = byte_incr(out_ptr, stride_bytes);\
    store_v(out_ptr, v4.template extract<16>(1));\
}

template<typename dtype>
NOINLINE void transpose_internal(dtype * __restrict in_, dtype * __restrict out_, transpose_3d_params_t &param) {
    dtype __aie_dm_resource_a * __restrict in_ptr = (dtype __aie_dm_resource_a * __restrict) in_;
    dtype __aie_dm_resource_b * __restrict out_ptr = (dtype __aie_dm_resource_b * __restrict) out_;
    SETUP_ITR3D_COUNTERS(iters_in)
    SETUP_ITR3D_COUNTERS(iters_out)
    if (param.permute_odr == ODR_102) {
        AIE_LOOP_UNROLL(8) 
        AIE_PREPARE_FOR_PIPELINING 
        for (int i=0; i<param.num_iters; i++){
            constexpr int32_t vec_sz = 16 / sizeof(dtype); 
            vector<dtype, vec_sz> vec;
            vec = load_v<vec_sz>(in_ptr); in_ptr  = INCR_ITR3D_PARAM(in_ptr,  iters_in,  param)
            store_v(out_ptr, vec);        out_ptr = INCR_ITR3D_PARAM(out_ptr, iters_out, param)
        }
    } else {
        for (int i=0; i<param.num_iters; i++) {
            if constexpr (sizeof(dtype) == 1) {
                vector<dtype, 64> iv1, iv2, iv3, iv4, tv1, tv2, tv3, tv4, ov1, ov2, ov3, ov4;
                LOAD_16x16_1B(iv1, iv2, iv3, iv4, in_ptr, param.in_stride);
                TRANSPOSE_16x16_1B(iv1, iv2, iv3, iv4, tv1, tv2, tv3, tv4, ov1, ov2, ov3, ov4);
                STORE_16x16_1B(ov1, ov2, ov3, ov4, out_ptr, param.out_stride);
                INCR_ITR3D_PARAM(in_ptr, iters_in, param)
                INCR_ITR3D_PARAM(out_ptr, iters_out, param)
            }
            else if constexpr (sizeof(dtype) == 2) {
                vector<dtype, 32> iv1, iv2, iv3, iv4, tv1, tv2, tv3, tv4, ov1, ov2, ov3, ov4;
                LOAD_16x8_2B(iv1, iv2, iv3, iv4, in_ptr, param.in_stride);
                TRANSPOSE_16x8_2B(iv1, iv2, iv3, iv4, tv1, tv2, tv3, tv4, ov1, ov2, ov3, ov4);
                STORE_8x16_2B(ov1, ov2, ov3, ov4, out_ptr, param.out_stride);
                INCR_ITR3D_PARAM(in_ptr, iters_in, param)
                INCR_ITR3D_PARAM(out_ptr, iters_out, param)
            }
        }
    } 
}

template<typename dtype>
NOINLINE void trim_padding(dtype * __restrict in_ptr, dtype * __restrict out_ptr, transpose_3d_params_t &param3d) {
    SETUP_ITR2D_COUNTERS(trim_iters_in)
    SETUP_ITR2D_COUNTERS(trim_iters_out)
    constexpr uint32_t vec_sz = 16;
    vector<dtype, vec_sz> vec1;
    vector<dtype, vec_sz> vec2;
    vector<dtype, vec_sz> vec3;
    vector<dtype, vec_sz> vec4;
    vector<dtype, vec_sz> vec5;
    vector<dtype, vec_sz> vec6;
    vector<dtype, vec_sz> vec7;
    vector<dtype, vec_sz> vec8;
    for (int i=0; i<param3d.trim_loops; i++) {
        vec1 = load_v<vec_sz>(in_ptr); INCR_ITR2D_PARAM(in_ptr, trim_iters_in, param3d)
        vec2 = load_v<vec_sz>(in_ptr); INCR_ITR2D_PARAM(in_ptr, trim_iters_in, param3d)
        vec3 = load_v<vec_sz>(in_ptr); INCR_ITR2D_PARAM(in_ptr, trim_iters_in, param3d)
        vec4 = load_v<vec_sz>(in_ptr); INCR_ITR2D_PARAM(in_ptr, trim_iters_in, param3d)
        vec5 = load_v<vec_sz>(in_ptr); INCR_ITR2D_PARAM(in_ptr, trim_iters_in, param3d)
        vec6 = load_v<vec_sz>(in_ptr); INCR_ITR2D_PARAM(in_ptr, trim_iters_in, param3d)
        vec7 = load_v<vec_sz>(in_ptr); INCR_ITR2D_PARAM(in_ptr, trim_iters_in, param3d)
        vec8 = load_v<vec_sz>(in_ptr); INCR_ITR2D_PARAM(in_ptr, trim_iters_in, param3d)
        store_v(out_ptr, vec1); INCR_ITR2D_PARAM(out_ptr, trim_iters_out, param3d)
        store_v(out_ptr, vec2); INCR_ITR2D_PARAM(out_ptr, trim_iters_out, param3d)
        store_v(out_ptr, vec3); INCR_ITR2D_PARAM(out_ptr, trim_iters_out, param3d)
        store_v(out_ptr, vec4); INCR_ITR2D_PARAM(out_ptr, trim_iters_out, param3d)
        store_v(out_ptr, vec5); INCR_ITR2D_PARAM(out_ptr, trim_iters_out, param3d)
        store_v(out_ptr, vec6); INCR_ITR2D_PARAM(out_ptr, trim_iters_out, param3d)
        store_v(out_ptr, vec7); INCR_ITR2D_PARAM(out_ptr, trim_iters_out, param3d)
        store_v(out_ptr, vec8); INCR_ITR2D_PARAM(out_ptr, trim_iters_out, param3d)
    }
    return;
}

template<typename dtype, typename config_ifm, typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
INLINE void transpose
(
    adf::input_buffer_conf<dtype, config_ifm>   & __restrict ifm,       // Input
    adf::output_buffer_conf<dtype, config_ofm>  & __restrict ofm,       // Output
    transpose_3d_params_t &param3d
){
    dtype __aie_dm_resource_a * __restrict in_ptr  = (dtype __aie_dm_resource_a * __restrict) ifm.data();
    dtype __aie_dm_resource_b * __restrict out_ptr = (dtype __aie_dm_resource_b * __restrict) ofm.data();

    transpose_internal<dtype>(in_ptr, out_ptr, param3d);

    trim_padding<dtype>(out_ptr, out_ptr, param3d);

    return;
}
#endif //__TRANSPOSE_H__
