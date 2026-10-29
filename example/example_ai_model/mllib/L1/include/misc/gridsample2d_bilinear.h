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

#ifndef __gridsample2d_H__
#define __gridsample2d_H__

#include "aie.hpp"
#include <adf.h>
#include <aie_api/utils.hpp>

#include "aie_api/utils.hpp" // needed for aie:print
#include "gridsample2d_bilinear_params.h"
#include "utils.h"

#define VEC_LEN_T 32

using namespace aie;




inline __attribute__((always_inline)) auto bounds_2d_vec(aie::vector<int16_t, VEC_LEN_T>& h,
                                                         aie::vector<int16_t, VEC_LEN_T>& w,
                                                         int16_t H,
                                                         int16_t W) {
    aie::mask<VEC_LEN_T> w_m1 = aie::lt(w, W);
    aie::mask<VEC_LEN_T> w_m2 = aie::ge(w, (int16_t)0);
    aie::mask<VEC_LEN_T> h_m1 = aie::lt(h, H);
    aie::mask<VEC_LEN_T> h_m2 = aie::ge(h, (int16_t)0);
    aie::mask<VEC_LEN_T> m = w_m1 & w_m2 & h_m1 & h_m2;

    return m;
}

template <uint8_t batch_size,
          typename dtype,
          int aligned_corner,
          int padding_mode>
requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
    __attribute__((noinline)) void gridsample2d_bilinear(dtype* __restrict ifm_data,
                                                         bfloat16* __restrict wts_data,
                                                         dtype* __restrict ofm_data,
                                                         gridsample2d_bilinear_params_t& params) {
    // interface definition
    dtype IFM_DM_BANK* __restrict p_in = (dtype IFM_DM_BANK* __restrict)ifm_data;
    dtype OFM_DM_BANK* __restrict p_out = (dtype OFM_DM_BANK* __restrict)ofm_data;
    bfloat16 IFM_DM_BANK* __restrict grid = (bfloat16 IFM_DM_BANK* __restrict)wts_data;

    // interface definition
    float OFM_DM_BANK* __restrict fp_out = (float OFM_DM_BANK* __restrict)ofm_data;

    // interface definition
    float OFM_DM_BANK* __restrict fp_release = (float OFM_DM_BANK* __restrict)ofm_data;

    using clean_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, int32_t, int8_t>;

    // pre-define const accum & vector
    int16_t C = params.ifm_depth;
    int16_t inp_H = params.ifm_height;
    int16_t inp_W = params.ifm_width;

    int16_t out_H = params.ofm_height;
    int16_t out_W = params.ofm_width;

    int16_t local_H = params.local_height;
    int16_t local_W = params.local_width;

    int16_t inp_sH = params.local_width * params.ifm_depth;
    int16_t inp_sW = 8;

    int16_t x_start_pos = params.x_start_pos;
    int16_t y_start_pos = params.y_start_pos;

    accum<accfloat, VEC_LEN_T> acc_neg_one_ix, acc_neg_one_iy;
    acc_neg_one_ix.from_vector(broadcast<float, VEC_LEN_T>((float)(-1.0)));
    acc_neg_one_iy.from_vector(broadcast<float, VEC_LEN_T>((float)(-1.0)));

    accum<accfloat, VEC_LEN_T> acc_half_ix, acc_half_iy;
    acc_half_ix.from_vector(broadcast<float, VEC_LEN_T>((float)(-0.5)));
    acc_half_iy.from_vector(broadcast<float, VEC_LEN_T>((float)(-0.5)));

    // vector<bfloat16, VEC_LEN_T> vec_half =
    //     aie::broadcast<bfloat16, VEC_LEN_T>((bfloat16)(0.5));

    // vector<bfloat16, VEC_LEN_T> vec_one =
    //     aie::broadcast<bfloat16, VEC_LEN_T>((bfloat16)(1.0));
    accum<accfloat, VEC_LEN_T> acc_one;
    acc_one.from_vector(broadcast<float, VEC_LEN_T>((float)(1.0)));

    vector<int16_t, VEC_LEN_T> vec_inp_sH = aie::broadcast<int16_t, VEC_LEN_T>((int16_t)(inp_sH));
    vector<int16_t, VEC_LEN_T> vec_inp_sW = aie::broadcast<int16_t, VEC_LEN_T>((int16_t)(inp_sW));
    accum<acc32, VEC_LEN_T> acc_inp_sH, acc_inp_sW;
    acc_inp_sH.from_vector(vec_inp_sH);
    acc_inp_sW.from_vector(vec_inp_sW);

    // outer loop: iter in dim-height, parallel processing in coeff-width
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2, ) 
    for (int i = 0; i < params.ofm_height; i++) 
    {
            // preparing vector & accum, loadint MAX_LEN = 32
            vector<int16_t, VEC_LEN_T> vec_ix_nw, vec_iy_nw;
            vector<int16_t, VEC_LEN_T> vec_ix_ne, vec_iy_ne;
            vector<int16_t, VEC_LEN_T> vec_ix_sw, vec_iy_sw;
            vector<int16_t, VEC_LEN_T> vec_ix_se, vec_iy_se;
            vector<int16_t, VEC_LEN_T> vec_ix_start_pos, vec_iy_start_pos;

            vector<int16_t, VEC_LEN_T> vec_idx_nw, vec_bounds_nw;
            vector<int16_t, VEC_LEN_T> vec_idx_ne, vec_bounds_ne;
            vector<int16_t, VEC_LEN_T> vec_idx_sw, vec_bounds_sw;
            vector<int16_t, VEC_LEN_T> vec_idx_se, vec_bounds_se;

            vector<bfloat16, VEC_LEN_T> vec_grid_1, vec_grid_2;
            vector<bfloat16, VEC_LEN_T> vec_grid_x, vec_grid_y;

            vector<float, VEC_LEN_T> vec_buff_x, vec_buff_y;
            vector<bfloat16, VEC_LEN_T> vec_ix, vec_iy;
            vector<bfloat16, VEC_LEN_T> vec_ix_sub, vec_iy_sub;
            vector<bfloat16, VEC_LEN_T> vec_ix_sub_neg, vec_iy_sub_neg;
            vector<float, VEC_LEN_T> vec_nw, vec_ne, vec_sw, vec_se;

            accum<accfloat, VEC_LEN_T> acc_buff_ix = aie::zeros<accfloat, VEC_LEN_T>();
            accum<accfloat, VEC_LEN_T> acc_buff_iy = aie::zeros<accfloat, VEC_LEN_T>();

            accum<accfloat, VEC_LEN_T> acc_nw, acc_ne, acc_sw, acc_se;
            accum<accfloat, VEC_LEN_T> acc_ix_nw, acc_iy_nw;

            accum<acc32, VEC_LEN_T> acc_ix_nw_mul, acc_iy_nw_mul;
            accum<acc32, VEC_LEN_T> acc_ix_nw_mac, acc_iy_nw_mac;
            accum<acc32, VEC_LEN_T> acc_idx_nw, acc_idx_ne, acc_idx_sw, acc_idx_se;

            mask<VEC_LEN_T> m_bounds_nw, m_bounds_ne, m_bounds_sw, m_bounds_se;

            // 1. loading grid location (x,y) (format in [-1.0, 1.0])
            vec_grid_1 = aie::load_v<VEC_LEN_T>(grid);
            grid = byte_incr(grid, out_W * sizeof(bfloat16));

            vec_grid_2 = aie::load_v<VEC_LEN_T>(grid);
            grid = byte_incr(grid, out_W * sizeof(bfloat16));

            std::tie(vec_grid_x, vec_grid_y) = aie::interleave_unzip(vec_grid_1, vec_grid_2, 1); // de interleaving to get x and y

            // ===================================================================
            // 2. calculate the coordinate within input H/W
            //    ix = ((x + 1) * inp_W - 1) / 2
            //    iy = ((y + 1) * inp_H - 1) / 2
            // ===================================================================

            // vec_buff_x = aie::add(vec_grid_x, bfloat16(1.0));
            // vec_buff_y = aie::add(vec_grid_y, bfloat16(1.0));

            accum<accfloat, VEC_LEN_T> acc_grid_x, acc_grid_y;
            acc_grid_x.from_vector_sign(vec_grid_x, true, 0);
            acc_grid_y.from_vector_sign(vec_grid_y, true, 0);
            acc_grid_x = aie::add(acc_grid_x, float(1.0));
            acc_grid_y = aie::add(acc_grid_y, float(1.0));

            vec_buff_x = acc_grid_x.template to_vector<float>();
            vec_buff_y = acc_grid_y.template to_vector<float>();

            //   acc_buff_ix = aie::mac(acc_neg_one_ix, vec_buff_x, (bfloat16)inp_W);
            //   acc_buff_iy = aie::mac(acc_neg_one_iy, vec_buff_y, (bfloat16)inp_H);

            //   acc_buff_ix =
            //       aie::mul(acc_buff_ix.template to_vector<bfloat16>(), (bfloat16)(0.5));
            //   acc_buff_iy =
            //       aie::mul(acc_buff_iy.template to_vector<bfloat16>(), (bfloat16)(0.5));

            if constexpr (aligned_corner == AlignedCornerMode::CORNER) {
                acc_buff_ix = aie::mac(acc_neg_one_ix, vec_buff_x, (float)inp_W);
                acc_buff_iy = aie::mac(acc_neg_one_iy, vec_buff_y, (float)inp_H);

                acc_buff_ix = aie::mul(acc_buff_ix.template to_vector<float>(), (float)(0.5));
                acc_buff_iy = aie::mul(acc_buff_iy.template to_vector<float>(), (float)(0.5));
            } else if constexpr (aligned_corner == AlignedCornerMode::CENTER) {
                auto vec_inpW = broadcast<float, VEC_LEN_T>((float)(inp_W - 1));
                auto vec_inpH = broadcast<float, VEC_LEN_T>((float)(inp_H - 1));

                acc_buff_ix = aie::mul(vec_inpW, vec_buff_x);
                acc_buff_iy = aie::mul(vec_inpH, vec_buff_y);

                acc_buff_ix = aie::mul(acc_buff_ix.template to_vector<float>(), (float)(0.5));
                acc_buff_iy = aie::mul(acc_buff_iy.template to_vector<float>(), (float)(0.5));
            }

            if constexpr (padding_mode == PaddingMode::BORDER) { 
                vector<float, VEC_LEN_T> vec_buff_ix, vec_buff_iy;
                vec_buff_ix = acc_buff_ix.template to_vector<float>();
                vec_buff_iy = acc_buff_iy.template to_vector<float>();

                vec_buff_ix = aie::clamp(vec_buff_ix, (float)0, (float)(inp_W - 1));
                vec_buff_iy = aie::clamp(vec_buff_iy, (float)0, (float)(inp_H - 1));

                acc_buff_ix.from_vector(vec_buff_ix, 0);
                acc_buff_iy.from_vector(vec_buff_iy, 0);
            }

            // 3. calculate four corner around the grid-points

            // nw: north-east (ground-conner, by applying floor)
            vec_ix_nw = aie::to_fixed<int16_t>(acc_buff_ix.template to_vector<bfloat16>(), 0);
            vec_iy_nw = aie::to_fixed<int16_t>(acc_buff_iy.template to_vector<bfloat16>(), 0);

            // ne: north-east
            vec_ix_ne = aie::add(vec_ix_nw, int16_t(1));
            vec_iy_ne = vec_iy_nw;

            // sw: south-west
            vec_ix_sw = vec_ix_nw;
            vec_iy_sw = aie::add(vec_iy_nw, int16_t(1));

            // se: south-east
            vec_ix_se = aie::add(vec_ix_nw, int16_t(1));
            vec_iy_se = aie::add(vec_iy_nw, int16_t(1));

            // ===================================================================
            // 4. get surface to each neighbour (simplified)
            // nw = (1 - (ix - ix_nw)) * (1 - (iy - iy_nw))
            // ne =      (ix - ix_nw)  * (1 - (iy - iy_nw))
            // sw = (1 - (ix - ix_nw)) *      (iy - iy_nw)
            // se =      (ix - ix_nw)  *      (iy - iy_nw)
            // ===================================================================

            // Get (ix, iy) in bfloat16
            vec_ix = acc_buff_ix.template to_vector<bfloat16>();
            vec_iy = acc_buff_iy.template to_vector<bfloat16>();

            // Get (ix_nw, iy_nw) in bfloat16
            acc_ix_nw.template from_vector(aie::to_float(vec_ix_nw));
            acc_iy_nw.template from_vector(aie::to_float(vec_iy_nw));

            // apply surface calculation
            vec_ix_sub = aie::sub(vec_ix, acc_ix_nw.to_vector<bfloat16>());
            vec_iy_sub = aie::sub(vec_iy, acc_iy_nw.to_vector<bfloat16>());
            vec_ix_sub_neg = aie::sub(acc_one.to_vector<bfloat16>(), vec_ix_sub);
            vec_iy_sub_neg = aie::sub(acc_one.to_vector<bfloat16>(), vec_iy_sub);

            acc_nw = aie::mul(vec_ix_sub_neg, vec_iy_sub_neg);
            acc_ne = aie::mul(vec_ix_sub, vec_iy_sub_neg);
            acc_sw = aie::mul(vec_ix_sub_neg, vec_iy_sub);
            acc_se = aie::mul(vec_ix_sub, vec_iy_sub); 

            // 5. Pre-calculate holo-idx outside inner loop (acc32, int16_t)
            vec_ix_start_pos = aie::broadcast<int16_t, VEC_LEN_T>(x_start_pos);
            vec_iy_start_pos = aie::broadcast<int16_t, VEC_LEN_T>(y_start_pos);

            vec_ix_nw = aie::sub(vec_ix_nw, vec_ix_start_pos);
            vec_ix_ne = aie::sub(vec_ix_ne, vec_ix_start_pos);
            vec_ix_sw = aie::sub(vec_ix_sw, vec_ix_start_pos);
            vec_ix_se = aie::sub(vec_ix_se, vec_ix_start_pos);

            vec_iy_nw = aie::sub(vec_iy_nw, vec_iy_start_pos);
            vec_iy_ne = aie::sub(vec_iy_ne, vec_iy_start_pos);
            vec_iy_sw = aie::sub(vec_iy_sw, vec_iy_start_pos);
            vec_iy_se = aie::sub(vec_iy_se, vec_iy_start_pos);

            acc_ix_nw_mul = aie::mul(vec_ix_nw, vec_inp_sW);
            acc_iy_nw_mul = aie::mul(vec_iy_nw, vec_inp_sH);
            acc_ix_nw_mac = aie::mac(acc_inp_sW, vec_ix_nw, vec_inp_sW);// plus 1
            acc_iy_nw_mac = aie::mac(acc_inp_sH, vec_iy_nw, vec_inp_sH);// plus 1

            acc_idx_nw = aie::add(acc_iy_nw_mul, acc_ix_nw_mul); // y => row  + x => pos in column
            acc_idx_ne = aie::mac(acc_ix_nw_mac, vec_iy_nw, vec_inp_sH);
            acc_idx_sw = aie::mac(acc_iy_nw_mac, vec_ix_nw, vec_inp_sW);
            acc_idx_se = aie::add(acc_iy_nw_mac, acc_ix_nw_mac);

            // 5. Calculate the surface bounds
            m_bounds_nw = bounds_2d_vec(vec_iy_nw, vec_ix_nw, local_H, local_W);
            m_bounds_ne = bounds_2d_vec(vec_iy_ne, vec_ix_ne, local_H, local_W);
            m_bounds_sw = bounds_2d_vec(vec_iy_sw, vec_ix_sw, local_H, local_W);
            m_bounds_se = bounds_2d_vec(vec_iy_se, vec_ix_se, local_H, local_W);

            // 6. the output of outer-loop, inner loop is the dim-width processing
            vec_idx_nw = acc_idx_nw.template to_vector<int16_t>();
            vec_idx_ne = acc_idx_ne.template to_vector<int16_t>();
            vec_idx_sw = acc_idx_sw.template to_vector<int16_t>();
            vec_idx_se = acc_idx_se.template to_vector<int16_t>();

            vec_bounds_nw = aie::select((int16_t)0, (int16_t)1, m_bounds_nw);
            vec_bounds_ne = aie::select((int16_t)0, (int16_t)1, m_bounds_ne);
            vec_bounds_sw = aie::select((int16_t)0, (int16_t)1, m_bounds_sw);
            vec_bounds_se = aie::select((int16_t)0, (int16_t)1, m_bounds_se);

            vec_nw = acc_nw.template to_vector<float>();
            vec_ne = acc_ne.template to_vector<float>();
            vec_sw = acc_sw.template to_vector<float>();
            vec_se = acc_se.template to_vector<float>();

            // inner loop: iter in dim-width, parallel processing in dim-channel
            // assume C == 8 & here use gran = 16 but only 8 is valued
            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(2, ) 
            for (int j = 0; j < params.ofm_width; j++) {
                    // prepare vec & accum
                    accum<accfloat, 16> acc_d00_mul, acc_d01_mul;
                    accum<accfloat, 16> acc_d02_mul, acc_d03_mul;
                    accum<accfloat, 16> acc_res_d00, acc_res_d01;
                    accum<accfloat, 16> acc_res_d02, acc_res_d03;

                    vector<bfloat16, 16> vec_buff_bf16;
                    vector<int16_t, 16> vec_buff_int16;
                    vector<int8_t, 16> vec_buff_bit8;

                    // maybe better to re-load from ofm_data as psum?
                    int16_t idx_off_nw = vec_idx_nw[j];
                    int16_t idx_off_ne = vec_idx_ne[j];
                    int16_t idx_off_sw = vec_idx_sw[j];
                    int16_t idx_off_se = vec_idx_se[j];

                    bool bounds_nw = vec_bounds_nw[j];
                    bool bounds_ne = vec_bounds_ne[j];
                    bool bounds_sw = vec_bounds_sw[j];
                    bool bounds_se = vec_bounds_se[j];

                    float nw = vec_nw[j];
                    float ne = vec_ne[j];
                    float sw = vec_sw[j];
                    float se = vec_se[j];

                    // there will be error when add 1 more chennel iteration
                    vector<dtype, 16> d00 = aie::load_unaligned_v<16>(p_in + idx_off_nw);
                    vector<dtype, 16> d01 = aie::load_unaligned_v<16>(p_in + idx_off_ne);
                    vector<dtype, 16> d02 = aie::load_unaligned_v<16>(p_in + idx_off_sw);
                    vector<dtype, 16> d03 = aie::load_unaligned_v<16>(p_in + idx_off_se);

                    if constexpr (std::is_same<bfloat16, dtype>::value) {
                        acc_res_d00.template from_vector(d00);
                        acc_res_d01.template from_vector(d01);
                        acc_res_d02.template from_vector(d02);
                        acc_res_d03.template from_vector(d03);
                    } else {
                        acc_res_d00.template from_vector(to_float(d00));
                        acc_res_d01.template from_vector(to_float(d01));
                        acc_res_d02.template from_vector(to_float(d02));
                        acc_res_d03.template from_vector(to_float(d03));
                    }

                    acc_d00_mul = aie::mul(acc_res_d00.template to_vector<float>(), nw);
                    acc_d01_mul = aie::mul(acc_res_d01.template to_vector<float>(), ne);
                    acc_d02_mul = aie::mul(acc_res_d02.template to_vector<float>(), sw);
                    acc_d03_mul = aie::mul(acc_res_d03.template to_vector<float>(), se);

                    aie::accum<accfloat, 16> acc_res = aie::zeros<accfloat, 16>();
                    acc_res = aie::add(acc_res, aie::op_zero(acc_d00_mul, 1 - bounds_nw));
                    acc_res = aie::add(acc_res, aie::op_zero(acc_d01_mul, 1 - bounds_ne));
                    acc_res = aie::add(acc_res, aie::op_zero(acc_d02_mul, 1 - bounds_sw));
                    acc_res = aie::add(acc_res, aie::op_zero(acc_d03_mul, 1 - bounds_se));

                    if constexpr (std::is_same<bfloat16, dtype>::value) {
                        aie::vector<float, 16> vec_ofm = aie::load_unaligned_v<16>(fp_out);
                        aie::vector<float, 8> vec_zero = aie::zeros<float, 8>();

                        accum<accfloat, 16> acc_ofm;
                        acc_ofm.from_vector_sign(vec_ofm, true, 0);
                        auto vec_res = acc_res.template to_vector<float>();
                        vec_res.insert(1, vec_zero);
                        acc_ofm = aie::add(acc_ofm, vec_res);
                        auto ofm_res = acc_ofm.template to_vector<float>();
                        aie::store_unaligned_v(fp_out, acc_ofm.template to_vector<float>());
                        fp_out = byte_incr(fp_out, 8 * sizeof(float));
                    } else {
                        vec_buff_bf16 = acc_res.template to_vector<bfloat16>();
                        vec_buff_bit8 = aie::to_fixed<int8_t>(vec_buff_bf16);
                        aie::vector<int8_t, 16> vec_ofm = aie::load_unaligned_v<16>(p_out);
                        aie::vector<int8_t, 16> vec_zero = aie::zeros<int8_t, 16>();
                        aie::vector<int8_t, 16> vec_high, vec_low;
                        std::tie(vec_high, vec_low) = aie::interleave_zip(vec_buff_bit8, vec_zero, 8);
                        auto vec_res = aie::add(vec_ofm, vec_high);
                        aie::store_unaligned_v(p_out, vec_res);
                        p_out = byte_incr(p_out, 8 * sizeof(dtype));
                    }
                    // p_out = byte_incr(p_out, 8 * sizeof(dtype));

                } // OUT_W
        }         // OUT_H
}

template <uint8_t batch_size,
          typename dtype,
          int aligned_corner,
          int padding_mode>
requires(mllib::utils::is_one_of_v<dtype, bfloat16>)
    __attribute__((noinline)) void gridsample2d_opt_bilinear(dtype* __restrict ifm_data,
                                                         bfloat16* __restrict wts_data,
                                                         dtype* __restrict ofm_data,
                                                         gridsample2d_bilinear_params_t& params) {
    // interface definition
    dtype IFM_DM_BANK* __restrict p_in = (dtype IFM_DM_BANK* __restrict)ifm_data;
    dtype OFM_DM_BANK* __restrict p_out = (dtype OFM_DM_BANK* __restrict)ofm_data;
    bfloat16 IFM_DM_BANK* __restrict grid = (bfloat16 IFM_DM_BANK* __restrict)wts_data;

    // interface definition
    bfloat16 OFM_DM_BANK* __restrict fp_out = (bfloat16 OFM_DM_BANK* __restrict)ofm_data;

    using clean_t = std::conditional_t<std::is_same_v<dtype, bfloat16>, int32_t, int8_t>;

    // pre-define const accum & vector
    int32_t C = params.ifm_depth;
    int32_t inp_H = params.ifm_height;
    int32_t inp_W = params.ifm_width;

    float inp_H_half = (float)inp_H * 0.5;
    float inp_W_half = (float)inp_W * 0.5;

    int32_t local_H = params.local_height;
    int32_t local_W = params.local_width;

    int32_t inp_sH = params.local_width * params.ifm_depth;
    int32_t inp_sW = 8;

    accum<accfloat, VEC_LEN_T> acc_neg_one_ix, acc_neg_one_iy;
    acc_neg_one_ix.from_vector(broadcast<float, VEC_LEN_T>((float)(-1.0)));
    acc_neg_one_iy.from_vector(broadcast<float, VEC_LEN_T>((float)(-1.0)));

    accum<accfloat, VEC_LEN_T> acc_half; 
    acc_half.from_vector(broadcast<float, VEC_LEN_T>((float)(-0.5)));
    accum<accfloat, VEC_LEN_T> acc_one;
    acc_one.from_vector(broadcast<float, VEC_LEN_T>((float)(1.0)));

    vector<int16_t, VEC_LEN_T> vec_inp_sH = aie::broadcast<int16_t, VEC_LEN_T>((int16_t)(inp_sH));
    vector<int16_t, VEC_LEN_T> vec_inp_sW = aie::broadcast<int16_t, VEC_LEN_T>((int16_t)(inp_sW));
    accum<acc32, VEC_LEN_T> acc_inp_sH, acc_inp_sW;
    acc_inp_sH.from_vector(vec_inp_sH);
    acc_inp_sW.from_vector(vec_inp_sW);

    // outer loop: iter in dim-height, parallel processing in coeff-width
    int32_t dim_xy_g = ((params.ofm_height *  params.ofm_width) + 31) >> 5;

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(2, ) 
    for (int i = 0; i < dim_xy_g; i++)
    {
        // preparing vector & accum, loadint MAX_LEN = 32
        vector<bfloat16, VEC_LEN_T> vec_grid_1, vec_grid_2;
        vector<bfloat16, VEC_LEN_T> vec_grid_x, vec_grid_y;

        accum<accfloat, VEC_LEN_T> acc_buff_ix, acc_buff_iy;
        accum<accfloat, VEC_LEN_T> acc_nw, acc_ne, acc_sw, acc_se;
        accum<accfloat, VEC_LEN_T> acc_ix_nw, acc_iy_nw,acc_xy;

        //loading grid location (x,y) (format in [-1.0, 1.0])
        vec_grid_1 = aie::load_v<VEC_LEN_T>(grid);
        grid = byte_incr(grid, 32 * sizeof(bfloat16));

        vec_grid_2 = aie::load_v<VEC_LEN_T>(grid);
        grid = byte_incr(grid, 32 * sizeof(bfloat16));

        std::tie(vec_grid_x, vec_grid_y) = aie::interleave_unzip(vec_grid_1, vec_grid_2, 1); // de interleaving to get x and y

        vec_grid_x =aie::min(aie::max(vec_grid_x, aie::broadcast<bfloat16, VEC_LEN_T>(-32766.0)), aie::broadcast<bfloat16, VEC_LEN_T>(32766.0));
        vec_grid_y =aie::min(aie::max(vec_grid_y, aie::broadcast<bfloat16, VEC_LEN_T>(-32766.0)), aie::broadcast<bfloat16, VEC_LEN_T>(32766.0));


        /*  calculate the coordinate within input H/W */
        // ix = ((x + 1) * inp_W - 1) / 2
        // iy = ((y + 1) * inp_H - 1) / 2

        accum<accfloat, VEC_LEN_T> acc_grid_x, acc_grid_y;
        acc_grid_x.from_vector_sign(vec_grid_x, true, 0);
        acc_grid_y.from_vector_sign(vec_grid_y, true, 0);
        acc_grid_x = aie::add(acc_grid_x, acc_one);
        acc_grid_y = aie::add(acc_grid_y, acc_one);

        vector<float, VEC_LEN_T> vec_buff_x = acc_grid_x.template to_vector<float>();
        vector<float, VEC_LEN_T> vec_buff_y = acc_grid_y.template to_vector<float>();

        if constexpr (aligned_corner == AlignedCornerMode::CORNER) {
            acc_buff_ix = aie::mac(acc_half, vec_buff_x, inp_W_half);
            acc_buff_iy = aie::mac(acc_half, vec_buff_y, inp_H_half);

        } else if constexpr (aligned_corner == AlignedCornerMode::CENTER) {
            auto vec_inpW = broadcast<float, VEC_LEN_T>((float)(inp_W - 1));
            auto vec_inpH = broadcast<float, VEC_LEN_T>((float)(inp_H - 1));

            acc_buff_ix = aie::mul(vec_inpW, vec_buff_x);
            acc_buff_iy = aie::mul(vec_inpH, vec_buff_y);

            acc_buff_ix = aie::mul(acc_buff_ix.template to_vector<float>(), (float)(0.5));
            acc_buff_iy = aie::mul(acc_buff_iy.template to_vector<float>(), (float)(0.5));
        }

        if constexpr (padding_mode == PaddingMode::BORDER) {
            vector<float, VEC_LEN_T> vec_buff_ix, vec_buff_iy;
            vec_buff_ix = acc_buff_ix.template to_vector<float>();
            vec_buff_iy = acc_buff_iy.template to_vector<float>();

            vec_buff_ix = aie::clamp(vec_buff_ix, (float)0, (float)(inp_W - 1));
            vec_buff_iy = aie::clamp(vec_buff_iy, (float)0, (float)(inp_H - 1));

            acc_buff_ix.from_vector(vec_buff_ix, 0);
            acc_buff_iy.from_vector(vec_buff_iy, 0);
        }

        // calculate four corner around the grid-points
        auto rnd1 = ::aie::get_rounding();  
        ::aie::set_rounding(::aie::rounding_mode::floor);
        vector<int16_t, VEC_LEN_T> vec_ix_nw = aie::to_fixed_floor<int16_t>(acc_buff_ix.template to_vector<bfloat16>(), 0);
        vector<int16_t, VEC_LEN_T> vec_iy_nw = aie::to_fixed_floor<int16_t>(acc_buff_iy.template to_vector<bfloat16>(), 0);
        ::aie::set_rounding(rnd1);
           
        // ne: north-east
        vector<int16_t, VEC_LEN_T> vec_ix_ne = aie::add(vec_ix_nw, int16_t(1));
        vector<int16_t, VEC_LEN_T> vec_iy_ne = vec_iy_nw;

        // sw: south-west
        vector<int16_t, VEC_LEN_T> vec_ix_sw = vec_ix_nw;
        vector<int16_t, VEC_LEN_T> vec_iy_sw = aie::add(vec_iy_nw, int16_t(1));

        // se: south-east
        vector<int16_t, VEC_LEN_T> vec_ix_se = aie::add(vec_ix_nw, int16_t(1));
        vector<int16_t, VEC_LEN_T> vec_iy_se = aie::add(vec_iy_nw, int16_t(1));

        // Pre-calculate holo-idx outside inner loop (acc32, int16_t)
        accum<acc32, VEC_LEN_T> acc_ix_nw_mul = aie::mul(vec_ix_nw, vec_inp_sW);
        accum<acc32, VEC_LEN_T> acc_iy_nw_mul = aie::mul(vec_iy_nw, vec_inp_sH);
        accum<acc32, VEC_LEN_T> acc_ix_nw_mac = aie::mac(acc_inp_sW, vec_ix_nw, vec_inp_sW);// plus 1
        accum<acc32, VEC_LEN_T> acc_iy_nw_mac = aie::mac(acc_inp_sH, vec_iy_nw, vec_inp_sH);// plus 1

        accum<acc32, VEC_LEN_T> acc_idx_nw = aie::add(acc_iy_nw_mul, acc_ix_nw_mul); // y => row  + x => pos in column
        accum<acc32, VEC_LEN_T> acc_idx_ne = aie::mac(acc_ix_nw_mac, vec_iy_nw, vec_inp_sH);
        accum<acc32, VEC_LEN_T> acc_idx_sw = aie::mac(acc_iy_nw_mac, vec_ix_nw, vec_inp_sW);
        accum<acc32, VEC_LEN_T> acc_idx_se = aie::add(acc_iy_nw_mac, acc_ix_nw_mac);

        // Calculate the surface bounds
        mask<VEC_LEN_T> m_bounds_nw = bounds_2d_vec(vec_iy_nw, vec_ix_nw, local_H, local_W);
        mask<VEC_LEN_T> m_bounds_ne = bounds_2d_vec(vec_iy_ne, vec_ix_ne, local_H, local_W);
        mask<VEC_LEN_T> m_bounds_sw = bounds_2d_vec(vec_iy_sw, vec_ix_sw, local_H, local_W);
        mask<VEC_LEN_T> m_bounds_se = bounds_2d_vec(vec_iy_se, vec_ix_se, local_H, local_W);

        // the output of outer-loop, inner loop is the dim-width processing
        vector<int16_t, VEC_LEN_T> vec_idx_nw = acc_idx_nw.template to_vector<int16_t>();
        vector<int16_t, VEC_LEN_T> vec_idx_ne = acc_idx_ne.template to_vector<int16_t>();
        vector<int16_t, VEC_LEN_T> vec_idx_sw = acc_idx_sw.template to_vector<int16_t>();
        vector<int16_t, VEC_LEN_T> vec_idx_se = acc_idx_se.template to_vector<int16_t>();

        vec_idx_nw = aie::select((int16_t)0, vec_idx_nw, m_bounds_nw);
        vec_idx_ne = aie::select((int16_t)0, vec_idx_ne, m_bounds_ne);
        vec_idx_sw = aie::select((int16_t)0, vec_idx_sw, m_bounds_sw);
        vec_idx_se = aie::select((int16_t)0, vec_idx_se, m_bounds_se);

        // Get (ix_nw, iy_nw) in bfloat16
        acc_ix_nw.template from_vector(aie::to_float(vec_ix_nw));
        acc_iy_nw.template from_vector(aie::to_float(vec_iy_nw));

        acc_ix_nw.template from_vector(aie::abs(aie::sub(acc_buff_ix,acc_ix_nw).to_vector<float>()));
        acc_iy_nw.template from_vector(aie::abs(aie::sub(acc_buff_iy,acc_iy_nw).to_vector<float>()));

        vector<float, VEC_LEN_T> vec_ix = acc_ix_nw.template to_vector<float>();
        vector<float, VEC_LEN_T> vec_iy = acc_iy_nw.template to_vector<float>();
        acc_xy = aie::mul(vec_ix, vec_iy);

        // 1 - x - y + xy
        acc_nw = aie::sub(acc_one, acc_ix_nw); 
        acc_nw = aie::sub(acc_nw,acc_iy_nw);
        acc_nw = aie::add(acc_nw, acc_xy);
            
        // y - xy
        acc_ne = aie::sub(acc_ix_nw, acc_xy);
        // x - xy
         acc_sw = aie::sub(acc_iy_nw, acc_xy);
        // xy
        acc_se = acc_xy;

        vector<bfloat16, VEC_LEN_T> vec_nw = acc_nw.template to_vector<bfloat16>();
        vector<bfloat16, VEC_LEN_T> vec_ne = acc_ne.template to_vector<bfloat16>();
        vector<bfloat16, VEC_LEN_T> vec_sw = acc_sw.template to_vector<bfloat16>();
        vector<bfloat16, VEC_LEN_T> vec_se = acc_se.template to_vector<bfloat16>();

        vec_nw = aie::select((bfloat16)0.0, vec_nw, m_bounds_nw);
        vec_ne = aie::select((bfloat16)0.0, vec_ne, m_bounds_ne);
        vec_sw = aie::select((bfloat16)0.0, vec_sw, m_bounds_sw);
        vec_se = aie::select((bfloat16)0.0, vec_se, m_bounds_se);

        AIE_PREPARE_FOR_PIPELINING 
        for (int j = 0; j < VEC_LEN_T; j++)
        {
            int32_t idx_off_nw = vec_idx_nw[j];
            int32_t idx_off_ne = vec_idx_ne[j];
            int32_t idx_off_sw = vec_idx_sw[j];
            int32_t idx_off_se = vec_idx_se[j];

            bfloat16 nw = vec_nw[j];
            bfloat16 ne = vec_ne[j];
            bfloat16 sw = vec_sw[j];
            bfloat16 se = vec_se[j];

            vector<dtype, 16> d00 = aie::load_unaligned_v<16>(p_in + idx_off_nw);
            vector<dtype, 16> d01 = aie::load_unaligned_v<16>(p_in + idx_off_ne);
            vector<dtype, 16> d02 = aie::load_unaligned_v<16>(p_in + idx_off_sw);
            vector<dtype, 16> d03 = aie::load_unaligned_v<16>(p_in + idx_off_se);

            aie::accum<accfloat, 16> acc_res = aie::mul(d00 , nw);
            acc_res = aie::mac(acc_res, d01 , ne);
            acc_res = aie::mac(acc_res, d02 , sw);
            acc_res = aie::mac(acc_res, d03 , se);

            auto vec_res = acc_res.template to_vector<bfloat16>();

            aie::store_unaligned_v(fp_out, vec_res);
            fp_out = byte_incr(fp_out, 8 * sizeof(bfloat16));
        } // for (int j = 0; j < VEC_LEN_T; j++)
    }// for (int i = 0; i < dim_xy_g; i++) 
}

template <uint8_t batch_size,
          typename dtype,
          typename config_ifm,
          typename config_wts,
          typename config_ofm,
          int aligned_corner,
          int padding_mode,
          int kernel_sel = gs_kernel_sel::IFM_FULL>
requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
    INLINE void gridsample2d_bilinear(adf::input_buffer_conf<dtype, config_ifm>& __restrict ifm,
                                                         adf::input_buffer_conf<bfloat16, config_wts>& __restrict wts,
                                                         adf::output_buffer_conf<dtype, config_ofm>& __restrict ofm,
                                                         gridsample2d_bilinear_params_t& params) {
    if constexpr (kernel_sel == gs_kernel_sel::IFM_FULL)
    {
       return gridsample2d_opt_bilinear<batch_size, dtype, aligned_corner, padding_mode>(
              ifm.data(), wts.data(), ofm.data(), params);
    }
    else{ //gs_kernel_sel::IFM_PARTIAL 
        return gridsample2d_bilinear<batch_size, dtype, aligned_corner, padding_mode>(
              ifm.data(), wts.data(), ofm.data(), params);
    }
}

#endif //__gridsample2d_H__
