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

#include <adf.h>
#include <aie.hpp>
#include <aie_adf.hpp>
#include <utils.hpp>

#include "elementwise_unary.h"
#include "mish_lut.h"
#include "mish_base.h"
#include "utils.h"

struct mish_params_t {
};

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_mish : elementwise_unary_base<dtype, mish_params_t> {
};

// partial specialization of elementwise_unary class for mish operator as we are using multiple loops currently
template <typename dtype>
class elementwise_unary<dtype, elementwise_mish<dtype>, mish_params_t> {
    public:
        using ifm_ptr_t = dtype __aie_dm_resource_a *__restrict;
        using ofm_ptr_t = dtype __aie_dm_resource_b *__restrict;
        using ofmb16_ptr_t = bfloat16 __aie_dm_resource_b *__restrict;
        using params_t = elementwise_unary_params_t<mish_params_t>;
        static constexpr unsigned load_sz = mllib::mish::load_sz;
        static constexpr unsigned byte_incr_sz = load_sz * sizeof(dtype);
        static constexpr unsigned byte_incr_sz_b16 = load_sz * sizeof(bfloat16);
        using vec_t = aie::vector<dtype, load_sz>;
        using vecf_t = aie::vector<bfloat16, load_sz>;
        using vecu16_t = aie::vector<uint16, load_sz>;
        using mask_t = aie::mask<load_sz>;
        using mask_val_t = uint32;
        using mask_ptr_t = uint32 __aie_dm_resource_b * __restrict;

        __attribute__((noinline)) static const void setup(params_t &params,
                                                        const void *rtps)
        {
            // keep up with unified interface of setting up unary operator parameters
            uint32_t* ptr_params = (uint32_t*) &params;
            ptr_params[0] = ((const uint32_t*) rtps)[0];
            ptr_params[1] = ((const uint32_t*) rtps)[1];
        };

        __attribute__((noinline)) static const void run(dtype* __restrict ifm,
                            dtype* __restrict ofm,
                            params_t &params)
        {
            using namespace aie;
            const int ifm_shift = params.ifm_shift;
            const int ofm_shift = params.ofm_shift;
            const int loop_cnt = params.ifmsv_size / load_sz;
            const int ifmsv_size = params.ifmsv_size;

            ifm_ptr_t pin = (ifm_ptr_t)(ifm);
            ofm_ptr_t pout = (ofm_ptr_t)(ofm);
            bfloat16 seg_nz = -97.0;
            bfloat16 seg_taylor0 = -0.79296875;
            bfloat16 seg_taylor1 = 0.8984375;
            bfloat16 seg_x = 2.640625;
            // first and last segment, negative zero or x
            const vecf_t vnz = broadcast<bfloat16, load_sz>(-0.0);
            const uint16 neg_zero_u16 = 0x8000;     // uint16 representation of -0.0
            const uint16 seg_nz_u16 = 0xc2c2;       // uint16 representation of -97.0
            const uint16 seg_taylor0_u16 = 0xbf4b;  // uint16 representation of -0.79296875
            const uint16 seg_taylor1_u16 = 0x3f66;  // uint16 representation of 0.8984375
            const uint16 seg_x_u16 = 0x4029;        // uint16 representation of 2.640625
            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4,) 
            for (int i = 0; i < loop_cnt / 2; ++i){
                vecf_t v1 = read_input_vector(pin, ifm_shift);
                pin = byte_incr(pin, byte_incr_sz);
                vecf_t v2 = read_input_vector(pin, ifm_shift);
                pin = byte_incr(pin, byte_incr_sz);

                vecu16_t vu16_1 = v1.cast_to<uint16>();
                mask_t mask_gex_1 = ge(vu16_1, seg_x_u16) & lt(vu16_1, neg_zero_u16);
                vecu16_t vu16_2 = v2.cast_to<uint16>();
                mask_t mask_gex_2 = ge(vu16_2, seg_x_u16) & lt(vu16_2, neg_zero_u16);

                vecf_t vres1 = select(vnz, v1, mask_gex_1);
                vecf_t vres2 = select(vnz, v2, mask_gex_2);

                store_v(pout, convert_output_vector(vres1, ofm_shift));
                pout = byte_incr(pout, byte_incr_sz);
                store_v(pout, convert_output_vector(vres2, ofm_shift));
                pout = byte_incr(pout, byte_incr_sz);
            }
            // second segment, look up table0
            constexpr unsigned mish_lut0_sz = 1024, mish_lut1_sz = 256;
            lut<4, uint16> mish_lut0(mish_lut0_sz, mish_lut<0, mish_lut0_sz * 2>::data_ab, mish_lut<0, mish_lut0_sz * 2>::data_cd);
            parallel_lookup<uint16, lut<4, uint16>, aie::lut_oor_policy::truncate> mish_lookup0(mish_lut0);
            // bfloat16 -97.0, note the hex value goes DOWN
            const vecu16_t vstart1 = broadcast<uint16, load_sz>(0xc2c2);
            pin = (ifm_ptr_t)(ifm);
            pout = (ofm_ptr_t)(ofm);
            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4, ) 
            for (int i = 0; i < loop_cnt; ++i){
                vecf_t v = read_input_vector(pin, ifm_shift);
                pin = byte_incr(pin, byte_incr_sz);
                vecu16_t vu16 = v.cast_to<uint16>();
                mask_t mask_seg = le(vu16, seg_nz_u16) & gt(vu16, seg_taylor0_u16);
                vecu16_t vidx = sub(vstart1, vu16);
                vecu16_t vu16_res = mish_lookup0.fetch(vidx);
                vecf_t vb16_res = vu16_res.cast_to<bfloat16>();
                vec_t vout0 = load_v<load_sz>(pout);
                vec_t vout1 = convert_output_vector(vb16_res, ofm_shift);
                vec_t vres = select(vout0, vout1, mask_seg);
                store_v(pout, vres);
                pout = byte_incr(pout, byte_incr_sz);
            }
            // fourth segment, look up table1
            lut<4, uint16> mish_lut1(mish_lut1_sz, mish_lut<1, mish_lut1_sz * 2>::data_ab, mish_lut<1, mish_lut1_sz * 2>::data_cd);
            parallel_lookup<uint16, lut<4, uint16>, aie::lut_oor_policy::truncate> mish_lookup1(mish_lut1);
            const vecu16_t vstart2 = broadcast<uint16, load_sz>(0x3f66);
            pin = (ifm_ptr_t)(ifm);
            pout = (ofm_ptr_t)(ofm);
            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4, ) 
            for (int i = 0; i < loop_cnt; ++i){
                vecf_t v = read_input_vector(pin, ifm_shift);
                vecu16_t vu16 = v.cast_to<uint16>();
                pin = byte_incr(pin, byte_incr_sz);
                mask_t mask_seg = ge(vu16, seg_taylor1_u16) & lt(vu16, seg_x_u16);
                vecu16_t vidx = sub(vu16, vstart2);
                vecu16_t vu16_res = mish_lookup1.fetch(vidx);
                vecf_t vb16_res = vu16_res.cast_to<bfloat16>();
                vec_t vout0 = load_v<load_sz>(pout);
                vec_t vout1 = convert_output_vector(vb16_res, ofm_shift);
                vec_t vres = select(vout0, vout1, mask_seg);
                store_v(pout, vres);
                pout = byte_incr(pout, byte_incr_sz);
            }
            // third segment, special care need to be take when write out intermediate results for int8
            pin = (ifm_ptr_t)(ifm);
            pout = (ofm_ptr_t)(ofm);
            ofm_ptr_t pout1 = (ofm_ptr_t)(ofm);
            ofm_ptr_t pout2 = (ofm_ptr_t)(ofm + load_sz);
            mask_ptr_t pmask = (mask_ptr_t)(ofm + ifmsv_size);
            pmask = byte_incr(pmask, 8 * byte_incr_sz_b16);
            uint16 coeff0_u16[5] = {0, 0x3f19, 0x3ea3, 0xbc83, 0xbd3b};
            uint16 coeff1_u16[5] = {0, 0x3b19, 0x3ad7, 0xb713, 0xb95e};
            int write_out_shifts[5] = {11, 11, 8, 7, ofm_shift};
            vec_t vclast = convert_output_vector(broadcast<bfloat16, load_sz>(reinterpret_cast<bfloat16*>(coeff0_u16)[4]), write_out_shifts[0]);
            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4, ) 
            for (int i = 0; i < loop_cnt / 2; ++i){
                vecf_t v1 = read_input_vector(pin, ifm_shift);
                pin = byte_incr(pin, byte_incr_sz);
                vecf_t v2 = read_input_vector(pin, ifm_shift);
                pin = byte_incr(pin, byte_incr_sz);
                vecu16_t vu16_1 = v1.cast_to<uint16>();
                vecu16_t vu16_2 = v2.cast_to<uint16>();
                mask_t mask_seg1 = (ge(vu16_1, neg_zero_u16) & le(vu16_1, seg_taylor0_u16)) | lt(vu16_1, seg_taylor1_u16);
                mask_t mask_seg2 = (ge(vu16_2, neg_zero_u16) & le(vu16_2, seg_taylor0_u16)) | lt(vu16_2, seg_taylor1_u16);
                *pmask++ = get_mask_val(mask_seg1);
                *pmask++ = get_mask_val(mask_seg2);
                vec_t vout0_1 = load_v<load_sz>(pout1);
                vec_t vout0_2 = load_v<load_sz>(pout2);
                vec_t vres1 = select(vout0_1, vclast, mask_seg1);
                vec_t vres2 = select(vout0_2, vclast, mask_seg2);
                store_v(pout1, vres1);
                store_v(pout2, vres2);
                pout1 = byte_incr(pout1, byte_incr_sz * 2);
                pout2 = byte_incr(pout2, byte_incr_sz * 2);
            }
            // setting up vector for coeff0 and coeff1
            ofmb16_ptr_t pvc0 = (ofmb16_ptr_t)(ofm + ifmsv_size);
            ofmb16_ptr_t pvc1 = (ofmb16_ptr_t)(ofm + ifmsv_size);
            pvc1 = byte_incr(pvc1, 4 * byte_incr_sz_b16);
            for (int i = 4; i > 0; --i) {
                vecf_t vc0 = broadcast<bfloat16, load_sz>(reinterpret_cast<bfloat16*>(coeff0_u16)[i - 1]);
                vecf_t vc1 = broadcast<bfloat16, load_sz>(reinterpret_cast<bfloat16*>(coeff1_u16)[i]);
                store_v(pvc0, vc0);
                store_v(pvc1, vc1);
                pvc0 = byte_incr(pvc0, byte_incr_sz_b16);
                pvc1 = byte_incr(pvc1, byte_incr_sz_b16);
            }
            if constexpr (std::is_same<int8, dtype>::value) {
                compute_taylor_expansion_2(ifm, ofm, params, write_out_shifts);
            } else {
                #if __AIE_ARCH__ == 20
                compute_taylor_expansion_2(ifm, ofm, params, write_out_shifts);
                #else
                compute_taylor_expansion_4(ifm, ofm, params, write_out_shifts);
                #endif
            }
        }
    private:
        // helper function for unification of bfloat16 and int8 implementation
        inline static const vec_t convert_output_vector(const vecf_t& v, int ofm_shift) {
            using namespace aie;
            if constexpr (std::is_same<bfloat16, dtype>::value) {
                return v;
            } else {
                accum<acc32, load_sz> acc(to_fixed(v, 16));
                return acc.to_vector<int8>(16 - ofm_shift);
            }
        }

        inline static const vecf_t convert_to_float(const aie::vector<int8, load_sz>& v, int shift) {
            using namespace aie;
            accum<accfloat, load_sz> acc(to_float(v, shift));
            auto vf = acc.to_vector<bfloat16>();
            return vf;
        }

        inline static const vecf_t read_input_vector(ifm_ptr_t pin, int ifm_shift) {
            using namespace aie;
            if constexpr (std::is_same<bfloat16, dtype>::value) {
                return load_v<load_sz>(pin);
            } else {
                vector<int8, load_sz> v = load_v<load_sz>(pin);
                return convert_to_float(v, ifm_shift);
            }
        }

        inline static const mask_val_t get_mask_val(const mask_t& m) {
            return m.to_uint32();
        }

        inline static const mask_t get_mask(const mask_val_t& val) {
            return mask_t::from_uint32(val);
        }
        __attribute__((always_inline)) static const void compute_taylor_expansion_1(dtype* ifm, dtype* ofm, params_t &params, int* __restrict write_out_shifts)
        {
            using namespace aie;
            const int ifm_shift = params.ifm_shift;
            const int ifmsv_size = params.ifmsv_size;
            const int loop_cnt = ifmsv_size / load_sz;
            constexpr int       inc_pvc = 0,                            inc_pshift = 0;
            const int           off_pvc = byte_incr_sz_b16,             off_pshift = 4;
            addr_t cnt_pin = 0, cnt_pvc0 = 0, cnt_pvc1 = 0, cnt_pmask = 0, cnt_pshift = 0;
            // iteration parameter of pin will NOT be affected by unrolling
            const int inc_pin = byte_incr_sz;
            const int off_pin = -ifmsv_size * sizeof(dtype) + inc_pin;
            const int sz_pin = loop_cnt - 1;
            // iteration parameter of pmask, NOT to be affected by unrolling
            const int inc_pmask = sizeof(mask_val_t);
            const int off_pmask = -ifmsv_size / 8 * (32 / load_sz) + inc_pmask;
            const int sz_pmask = sz_pin;
            // iteration parameter of pout WILL BE affected by unrolling
            const int inc_pout = byte_incr_sz;
            const int off_pout = -ifmsv_size * sizeof(dtype) + inc_pout;
            const int sz_outer_loop = loop_cnt - 1;   // size of pointers used by add_byte_2d of variables that need not be duplicated when unroll the loop
            const int sz_pout = sz_outer_loop;
            addr_t cnt_pout = 0;
            // pointer initialization
            ifm_ptr_t pin = (ifm_ptr_t)(ifm);
            ofm_ptr_t pout = (ofm_ptr_t)(ofm);
            mask_ptr_t pmask = (mask_ptr_t)(ofm + ifmsv_size);
            pmask = byte_incr(pmask, 8 * byte_incr_sz_b16);
            ofmb16_ptr_t pvc0 = (ofmb16_ptr_t)(ofm + ifmsv_size);
            ofmb16_ptr_t pvc1 = (ofmb16_ptr_t)(ofm + ifmsv_size);
            pvc1 = byte_incr(pvc1, 4 * byte_incr_sz_b16);
            int *pshift = write_out_shifts + 1;

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4, ) 
            for (int i = 0; i < loop_cnt * 4; ++i){
                // these variables need NOT to be duplicated when unroll the loop
                vecf_t vc0 = load_v<load_sz>(pvc0);
                pvc0 = add_2d_byte(pvc0, off_pvc, sz_outer_loop, cnt_pvc0, inc_pvc);
                vecf_t vc1 = load_v<load_sz>(pvc1);
                pvc1 = add_2d_byte(pvc1, off_pvc, sz_outer_loop, cnt_pvc1, inc_pvc);
                int shift = *pshift;
                int shift_prev = *(pshift - 1);
                pshift = add_2d_byte(pshift, off_pshift, sz_outer_loop, cnt_pshift, inc_pshift);

                // these variables SHALL be duplicated when unroll
                vecf_t v = read_input_vector(pin, ifm_shift);
                pin = add_2d_byte(pin, off_pin, sz_pin, cnt_pin, inc_pin);
                vec_t vout0_native = load_v<load_sz>(pout);
                vecf_t vout0;
                if constexpr(std::is_same<bfloat16, dtype>::value) {
                    vout0 = vout0_native;
                } else {
                    vout0 = convert_to_float(vout0_native, shift_prev);
                }
                mask_t mask_seg = get_mask(*pmask);
                pmask = add_2d_byte(pmask, off_pmask, sz_pmask, cnt_pmask, inc_pmask);
                accum<accfloat, load_sz> acc = from_vector<accfloat>(vc0);
                acc = mac(acc, v, vout0);
                acc = mac(acc, v, vc1);
                vecf_t vout1 = acc.to_vector<bfloat16>(0);
                vec_t vout1_native = convert_output_vector(vout1, shift);
                vec_t vres = select(vout0_native, vout1_native, mask_seg);
                store_v(pout, vres);
                pout = add_2d_byte(pout, off_pout, sz_pout, cnt_pout, inc_pout);
            }
        }
        __attribute__((always_inline)) static const void compute_taylor_expansion_2(dtype* ifm, dtype* ofm, params_t &params, int* __restrict write_out_shifts)
        {
            using namespace aie;
            const int ifm_shift = params.ifm_shift;
            const int ifmsv_size = params.ifmsv_size;
            const int loop_cnt = ifmsv_size / load_sz;
            constexpr int       inc_pvc = 0,                            inc_pshift = 0;
            const int           off_pvc = byte_incr_sz_b16,             off_pshift = 4;
            addr_t cnt_pin = 0, cnt_pvc0 = 0, cnt_pvc1 = 0, cnt_pmask = 0, cnt_pshift = 0;
            // iteration parameter of pin will NOT be affected by unrolling
            const int inc_pin = byte_incr_sz;
            const int off_pin = -ifmsv_size * sizeof(dtype) + inc_pin;
            const int sz_pin = loop_cnt - 1;
            // iteration parameter of pmask, NOT to be affected by unrolling
            const int inc_pmask = sizeof(mask_val_t);
            const int off_pmask = -ifmsv_size / 8 * (32 / load_sz) + inc_pmask;
            const int sz_pmask = sz_pin;
            // iteration parameter of pout WILL BE affected by unrolling
            const int inc_pout = byte_incr_sz * 2;
            const int off_pout = -ifmsv_size * sizeof(dtype) + inc_pout;
            const int sz_outer_loop = loop_cnt / 2 - 1;   // size of pointers used by add_byte_2d of variables that need not be duplicated when unroll the loop
            const int sz_pout = sz_outer_loop;
            addr_t cnt_pout1 = 0, cnt_pout2 = 0;
            // pointer initialization
            ifm_ptr_t pin = (ifm_ptr_t)(ifm);
            ofm_ptr_t pout1 = (ofm_ptr_t)(ofm);
            ofm_ptr_t pout2 = (ofm_ptr_t)(ofm + load_sz);
            mask_ptr_t pmask = (mask_ptr_t)(ofm + ifmsv_size);
            pmask = byte_incr(pmask, 8 * byte_incr_sz_b16);
            ofmb16_ptr_t pvc0 = (ofmb16_ptr_t)(ofm + ifmsv_size);
            ofmb16_ptr_t pvc1 = (ofmb16_ptr_t)(ofm + ifmsv_size);
            pvc1 = byte_incr(pvc1, 4 * byte_incr_sz_b16);
            int *pshift = write_out_shifts + 1;

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4, ) 
            for (int i = 0; i < 2 * loop_cnt; ++i){
                // these variables need NOT to be duplicated when unroll the loop
                vecf_t vc0 = load_v<load_sz>(pvc0);
                pvc0 = add_2d_byte(pvc0, off_pvc, sz_outer_loop, cnt_pvc0, inc_pvc);
                vecf_t vc1 = load_v<load_sz>(pvc1);
                pvc1 = add_2d_byte(pvc1, off_pvc, sz_outer_loop, cnt_pvc1, inc_pvc);
                int shift = *pshift;
                int shift_prev = *(pshift - 1);
                pshift = add_2d_byte(pshift, off_pshift, sz_outer_loop, cnt_pshift, inc_pshift);

                // these variables SHALL be duplicated when unroll
                vecf_t v1 = read_input_vector(pin, ifm_shift);
                pin = add_2d_byte(pin, off_pin, sz_pin, cnt_pin, inc_pin);
                vecf_t v2 = read_input_vector(pin, ifm_shift);
                pin = add_2d_byte(pin, off_pin, sz_pin, cnt_pin, inc_pin);
                vec_t vout0_native_1 = load_v<load_sz>(pout1);
                vec_t vout0_native_2 = load_v<load_sz>(pout2);
                vecf_t vout0_1, vout0_2;
                if constexpr(std::is_same<bfloat16, dtype>::value) {
                    vout0_1 = vout0_native_1;
                    vout0_2 = vout0_native_2;
                } else {
                    vout0_1 = convert_to_float(vout0_native_1, shift_prev);
                    vout0_2 = convert_to_float(vout0_native_2, shift_prev);
                }
                mask_t mask_seg1 = get_mask(*pmask);
                pmask = add_2d_byte(pmask, off_pmask, sz_pmask, cnt_pmask, inc_pmask);
                mask_t mask_seg2 = get_mask(*pmask);
                pmask = add_2d_byte(pmask, off_pmask, sz_pmask, cnt_pmask, inc_pmask);
                accum<accfloat, load_sz> acc1 = from_vector<accfloat>(vc0);
                accum<accfloat, load_sz> acc2 = from_vector<accfloat>(vc0);
                acc1 = mac(acc1, v1, vout0_1);
                acc2 = mac(acc2, v2, vout0_2);
                acc1 = mac(acc1, v1, vc1);
                acc2 = mac(acc2, v2, vc1);
                vecf_t vout1_1 = acc1.to_vector<bfloat16>(0);
                vecf_t vout1_2 = acc2.to_vector<bfloat16>(0);
                vec_t vout1_native_1 = convert_output_vector(vout1_1, shift);
                vec_t vout1_native_2 = convert_output_vector(vout1_2, shift);
                vec_t vres1 = select(vout0_native_1, vout1_native_1, mask_seg1);
                vec_t vres2 = select(vout0_native_2, vout1_native_2, mask_seg2);
                store_v(pout1, vres1);
                store_v(pout2, vres2);
                pout1 = add_2d_byte(pout1, off_pout, sz_pout, cnt_pout1, inc_pout);
                pout2 = add_2d_byte(pout2, off_pout, sz_pout, cnt_pout2, inc_pout);
            }
        }
        __attribute__((always_inline)) static const void compute_taylor_expansion_4(dtype* ifm, dtype* ofm, params_t &params, int* __restrict write_out_shifts)
        {
            using namespace aie;
            const int ifm_shift = params.ifm_shift;
            const int ifmsv_size = params.ifmsv_size;
            const int loop_cnt = ifmsv_size / load_sz;
            constexpr int       inc_pvc = 0,                            inc_pshift = 0;
            const int           off_pvc = byte_incr_sz_b16,             off_pshift = 4;
            addr_t cnt_pin = 0, cnt_pvc0 = 0, cnt_pvc1 = 0, cnt_pmask = 0, cnt_pshift = 0;
            // iteration parameter of pin will NOT be affected by unrolling
            const int inc_pin = byte_incr_sz;
            const int off_pin = -ifmsv_size * sizeof(dtype) + inc_pin;
            const int sz_pin = loop_cnt - 1;
            // iteration parameter of pmask, NOT to be affected by unrolling
            const int inc_pmask = sizeof(mask_val_t);
            const int off_pmask = -ifmsv_size / 8 * (32 / load_sz) + inc_pmask;
            const int sz_pmask = sz_pin;
            // iteration parameter of pout WILL BE affected by unrolling
            const int inc_pout = byte_incr_sz * 4;
            const int off_pout = -ifmsv_size * sizeof(dtype) + inc_pout;
            const int sz_outer_loop = loop_cnt / 4 - 1;   // size of pointers used by add_byte_2d of variables that need not be duplicated when unroll the loop
            const int sz_pout = sz_outer_loop;
            addr_t cnt_pout1 = 0, cnt_pout2 = 0, cnt_pout3 = 0, cnt_pout4 = 0;
            // pointer initialization
            ifm_ptr_t pin = (ifm_ptr_t)(ifm);
            ofm_ptr_t pout1 = (ofm_ptr_t)(ofm);
            ofm_ptr_t pout2 = (ofm_ptr_t)(ofm + load_sz);
            ofm_ptr_t pout3 = (ofm_ptr_t)(ofm + 2 * load_sz);
            ofm_ptr_t pout4 = (ofm_ptr_t)(ofm + 3 * load_sz);
            mask_ptr_t pmask = (mask_ptr_t)(ofm + ifmsv_size);
            pmask = byte_incr(pmask, 8 * byte_incr_sz_b16);
            ofmb16_ptr_t pvc0 = (ofmb16_ptr_t)(ofm + ifmsv_size);
            ofmb16_ptr_t pvc1 = (ofmb16_ptr_t)(ofm + ifmsv_size);
            pvc1 = byte_incr(pvc1, 4 * byte_incr_sz_b16);
            int *pshift = write_out_shifts + 1;

            AIE_PREPARE_FOR_PIPELINING 
            AIE_LOOP_RANGE(4, ) 
            for (int i = 0; i < loop_cnt; ++i){
                // these variables need NOT to be duplicated when unroll the loop
                vecf_t vc0 = load_v<load_sz>(pvc0);
                pvc0 = add_2d_byte(pvc0, off_pvc, sz_outer_loop, cnt_pvc0, inc_pvc);
                vecf_t vc1 = load_v<load_sz>(pvc1);
                pvc1 = add_2d_byte(pvc1, off_pvc, sz_outer_loop, cnt_pvc1, inc_pvc);
                int shift = *pshift;
                int shift_prev = *(pshift - 1);
                pshift = add_2d_byte(pshift, off_pshift, sz_outer_loop, cnt_pshift, inc_pshift);

                // these variables SHALL be duplicated when unroll
                vecf_t v1 = read_input_vector(pin, ifm_shift);
                pin = add_2d_byte(pin, off_pin, sz_pin, cnt_pin, inc_pin);
                vecf_t v2 = read_input_vector(pin, ifm_shift);
                pin = add_2d_byte(pin, off_pin, sz_pin, cnt_pin, inc_pin);
                vecf_t v3 = read_input_vector(pin, ifm_shift);
                pin = add_2d_byte(pin, off_pin, sz_pin, cnt_pin, inc_pin);
                vecf_t v4 = read_input_vector(pin, ifm_shift);
                pin = add_2d_byte(pin, off_pin, sz_pin, cnt_pin, inc_pin);

                vec_t vout0_native_1 = load_v<load_sz>(pout1);
                vec_t vout0_native_2 = load_v<load_sz>(pout2);
                vec_t vout0_native_3 = load_v<load_sz>(pout3);
                vec_t vout0_native_4 = load_v<load_sz>(pout4);

                vecf_t vout0_1, vout0_2, vout0_3, vout0_4;

                if constexpr(std::is_same<bfloat16, dtype>::value) {
                    vout0_1 = vout0_native_1;
                    vout0_2 = vout0_native_2;
                    vout0_3 = vout0_native_3;
                    vout0_4 = vout0_native_4;
                } else {
                    vout0_1 = convert_to_float(vout0_native_1, shift_prev);
                    vout0_2 = convert_to_float(vout0_native_2, shift_prev);
                    vout0_3 = convert_to_float(vout0_native_3, shift_prev);
                    vout0_4 = convert_to_float(vout0_native_4, shift_prev);
                }

                mask_t mask_seg1 = get_mask(*pmask);
                pmask = add_2d_byte(pmask, off_pmask, sz_pmask, cnt_pmask, inc_pmask);
                mask_t mask_seg2 = get_mask(*pmask);
                pmask = add_2d_byte(pmask, off_pmask, sz_pmask, cnt_pmask, inc_pmask);
                mask_t mask_seg3 = get_mask(*pmask);
                pmask = add_2d_byte(pmask, off_pmask, sz_pmask, cnt_pmask, inc_pmask);
                mask_t mask_seg4 = get_mask(*pmask);
                pmask = add_2d_byte(pmask, off_pmask, sz_pmask, cnt_pmask, inc_pmask);

                accum<accfloat, load_sz> acc1 = from_vector<accfloat>(vc0);
                accum<accfloat, load_sz> acc2 = from_vector<accfloat>(vc0);
                accum<accfloat, load_sz> acc3 = from_vector<accfloat>(vc0);
                accum<accfloat, load_sz> acc4 = from_vector<accfloat>(vc0);

                acc1 = mac(acc1, v1, vout0_1);
                acc2 = mac(acc2, v2, vout0_2);
                acc3 = mac(acc3, v3, vout0_3);
                acc4 = mac(acc4, v4, vout0_4);

                acc1 = mac(acc1, v1, vc1);
                acc2 = mac(acc2, v2, vc1);
                acc3 = mac(acc3, v3, vc1);
                acc4 = mac(acc4, v4, vc1);

                vecf_t vout1_1 = acc1.to_vector<bfloat16>(0);
                vecf_t vout1_2 = acc2.to_vector<bfloat16>(0);
                vecf_t vout1_3 = acc3.to_vector<bfloat16>(0);
                vecf_t vout1_4 = acc4.to_vector<bfloat16>(0);

                vec_t vout1_native_1 = convert_output_vector(vout1_1, shift);
                vec_t vout1_native_2 = convert_output_vector(vout1_2, shift);
                vec_t vout1_native_3 = convert_output_vector(vout1_3, shift);
                vec_t vout1_native_4 = convert_output_vector(vout1_4, shift);

                vec_t vres1 = select(vout0_native_1, vout1_native_1, mask_seg1);
                vec_t vres2 = select(vout0_native_2, vout1_native_2, mask_seg2);
                vec_t vres3 = select(vout0_native_3, vout1_native_3, mask_seg3);
                vec_t vres4 = select(vout0_native_4, vout1_native_4, mask_seg4);

                store_v(pout1, vres1);
                store_v(pout2, vres2);
                store_v(pout3, vres3);
                store_v(pout4, vres4);

                pout1 = add_2d_byte(pout1, off_pout, sz_pout, cnt_pout1, inc_pout);
                pout2 = add_2d_byte(pout2, off_pout, sz_pout, cnt_pout2, inc_pout);
                pout3 = add_2d_byte(pout3, off_pout, sz_pout, cnt_pout3, inc_pout);
                pout4 = add_2d_byte(pout4, off_pout, sz_pout, cnt_pout4, inc_pout);

            }
        }
};
