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

#ifndef __STFT_HPP__
#define __STFT_HPP__

#include <adf.h>
#include <aie.hpp>
#include <aie_adf.hpp>
#include <utils.hpp>
#include "stft_params.h"
#include "utils.h"

using namespace aie;

namespace mllib{
    namespace stft{
        // internal constants
        constexpr unsigned rsv_bits = 5;
        constexpr unsigned constlog2(unsigned N) {
            unsigned l = 0;
            while (N > 1) {
                N = N >> 1;
                ++l;
            }
            return l;
        }
        template <unsigned N, unsigned NM, unsigned idx>
        void fft_single_stage(unsigned shift_tw, unsigned shift,
                cint32* __restrict tmp1, cint32* __restrict tmp2, cint16* __restrict tw_head) {
            fft_dit_r2_stage<N>(
                constlog2(NM / N) % 2 == 1 ? tmp1 : tmp2,
                tw_head + (1 << idx),
                NM, shift_tw, shift, false,
                constlog2(NM / N) % 2 == 1 ? tmp2 : tmp1);
        }
        template <unsigned NM, unsigned... N>
        void fft_unroll(unsigned shift_tw, unsigned shift,
            cint32* __restrict tmp1, cint32* __restrict tmp2,
            cint16* __restrict tw_head,
            std::integer_sequence<unsigned, N...> iter) {
            (fft_single_stage<((NM / 2) >> N), NM, N>(shift_tw, shift, tmp1, tmp2, tw_head), ...);
        }

        template <unsigned N>
        void fft_full(cint32* __restrict tmp1, // data initially stored in tmp1 of size N
            cint16* __restrict tw_head,
            unsigned shift_tw, unsigned shift,
            cint32* __restrict tmp2) {
                fft_unroll<N>(shift_tw, shift, tmp1, tmp2, tw_head, std::make_integer_sequence<unsigned, constlog2(N)>());
        }

        template <unsigned N>
        cint32* res_buf(cint32* tmp1, cint32* tmp2) {
            return constlog2(N) % 2 == 0 ? tmp1 : tmp2;
        }
    }
}

/*
currently supported dtype: cint16, use cint32 to store intermediate results
ifm layout:
    1D signal (param.signal_len * sizeof(cint16))
wts layout:
    param (STFT_WTS_OFFSET_BYTES) | window (param.win_len * sizeof(cint16)) | twiddle (fft_len * c16, layout: place-holder(4 byte), tw1, tw2, ...)
ofm layout:
    output (param.fft_out_len * param.frame_cnt * sizeof(cint16)) | buffer (fft_len * 2 * c32)
constraints:
load_len | win_pad_len
load_len | win_len
user make sure srs(signal * window, win_shift) lies in cint16 range
hop_len | (signal_len - fft_len)
*/
template <unsigned fft_len, typename dtype>
void stft
(
        dtype* __restrict ifm_data,
        dtype* __restrict wts_data,
        dtype* __restrict ofm_data,
        stft_params_t &param
) {
    // for each iteration
    // 1. copy the window to tmp1's center
    // 2. mul signal to window, perform win_shift while reserving bits
    // 3. perform stft
    // 4. copy the results back, shift out reserved bits and output shift
    // using DivFactorType = typename std::conditional<std::is_same<dtype, bfloat16>::value, bfloat16, uint16_t>::type;
    if constexpr(std::is_same<bfloat16, dtype>::value) {
        unsigned signal_start = 0;
        const unsigned load_len = 8;    // 256-bit load length
        const unsigned load_mask = ~(load_len * 2 - 1);
        const unsigned downshift_win = param.win_shift > mllib::stft::rsv_bits ? param.win_shift - mllib::stft::rsv_bits : 0;
        const unsigned upshift_win = mllib::stft::rsv_bits > param.win_shift ? mllib::stft::rsv_bits - param.win_shift : 0;
        cint32 *tmp1 = (cint32*)((cint16*)ofm_data + param.ofm_offset);
        cint32 *tmp2 = tmp1 + fft_len;
        cint32 *res = mllib::stft::res_buf<fft_len>(tmp1, tmp2);
        cint16 *tw_head = (cint16 *)wts_data + STFT_WTS_OFFSET_BYTES / sizeof(cint16) + param.win_len;
        const unsigned out_shift_final = param.out_shift + mllib::stft::rsv_bits;
        auto it_out = begin_vector<load_len * 2, aie_dm_resource::a>(ofm_data);
        for (int i = 0; i < param.frame_cnt; ++i) {
            auto it_win_padded = begin_vector<load_len>(tmp1);
            for (int j = 0; j < param.win_pad_len / load_len; ++j) {
                *it_win_padded++ = zeros<cint32, load_len>();
            }
            auto it_win = begin_vector<load_len * 2, aie_dm_resource::a>(wts_data + STFT_WTS_OFFSET_BYTES / sizeof(bfloat16));
            // in case hop_len is not multiple of load_len
            // start from a previous position
            auto it_in = begin_vector<load_len * 2, aie_dm_resource::a>(ifm_data + (signal_start & load_mask));
            const unsigned signal_shift_sz = ((signal_start - (signal_start & load_mask)) >> 1);
            auto v_signal0 = *it_in++;
            aie::vector<int16_t, load_len * 2> tmp_vec = aie::to_fixed<int16_t>(v_signal0);        
            aie::vector<cint16, load_len> cv_signal0 = tmp_vec.template cast_to<cint16>();
            signal_start += param.hop_len * 2;
            AIE_PREPARE_FOR_PIPELINING
            for (int j = 0; j < param.win_len / load_len; ++j) {
                auto v_signal1 = *it_in++;
                aie::vector<int16_t, load_len * 2> tmp_vec1 = aie::to_fixed<int16_t>(v_signal1);        
                aie::vector<cint16, load_len> cv_signal1 = tmp_vec1.template cast_to<cint16>();
                auto v_signal_shifted = shuffle_down_fill(cv_signal0, cv_signal1, signal_shift_sz);
                auto win = *it_win++;
                aie::vector<int16_t, load_len * 2> tmp_win = aie::to_fixed<int16_t>(win, param.win_shift);        
                aie::vector<cint16, load_len> cwin = tmp_win.template cast_to<cint16>();
                accum<cacc32, load_len> acc = mul(v_signal_shifted, cwin);
                auto v_win_padded = acc.to_vector<cint32>(downshift_win);
                v_win_padded = upshift(v_win_padded, upshift_win);
                *it_win_padded++ = v_win_padded;
                cv_signal0 = cv_signal1;
            }
            for (int j = 0; j < param.win_pad_len / load_len; ++j) {
                *it_win_padded++ = zeros<cint32, load_len>();
            }
            // perform fft using the double buffer
            mllib::stft::fft_full<fft_len>(tmp1, tw_head, param.tw_shift, param.tw_shift, tmp2);
            auto it_res = begin_vector<load_len>(res);
            // copy the results back to ofm, converting c32 to c16
            AIE_PREPARE_FOR_PIPELINING
            for (int j = 0; j < param.fft_out_len / load_len / 8; ++j) {
                auto v1 = downshift(*it_res++, out_shift_final);
                auto v2 = downshift(*it_res++, out_shift_final);
                auto v3 = downshift(*it_res++, out_shift_final);
                auto v4 = downshift(*it_res++, out_shift_final);
                auto v5 = downshift(*it_res++, out_shift_final);
                auto v6 = downshift(*it_res++, out_shift_final);
                auto v7 = downshift(*it_res++, out_shift_final);
                auto v8 = downshift(*it_res++, out_shift_final);
                *it_out++ = aie::to_float<bfloat16>(filter_even(v1.cast_to<int16_t>()));
                *it_out++ = aie::to_float<bfloat16>(filter_even(v2.cast_to<int16_t>()));
                *it_out++ = aie::to_float<bfloat16>(filter_even(v3.cast_to<int16_t>()));
                *it_out++ = aie::to_float<bfloat16>(filter_even(v4.cast_to<int16_t>()));
                *it_out++ = aie::to_float<bfloat16>(filter_even(v5.cast_to<int16_t>()));
                *it_out++ = aie::to_float<bfloat16>(filter_even(v6.cast_to<int16_t>()));
                *it_out++ = aie::to_float<bfloat16>(filter_even(v7.cast_to<int16_t>()));
                *it_out++ = aie::to_float<bfloat16>(filter_even(v8.cast_to<int16_t>()));

            }
        }
    }
    else {
        unsigned signal_start = 0;
        const unsigned load_len = 8;    // 256-bit load length
        const unsigned load_mask = ~(load_len - 1);
        const unsigned downshift_win = param.win_shift > mllib::stft::rsv_bits ? param.win_shift - mllib::stft::rsv_bits : 0;
        const unsigned upshift_win = mllib::stft::rsv_bits > param.win_shift ? mllib::stft::rsv_bits - param.win_shift : 0;
        cint32 *tmp1 = (cint32*)(ofm_data + param.ofm_offset);
        cint32 *tmp2 = tmp1 + fft_len;
        cint32 *res = mllib::stft::res_buf<fft_len>(tmp1, tmp2);
        cint16 *tw_head = wts_data + STFT_WTS_OFFSET_BYTES / sizeof(cint16) + param.win_len;
        const unsigned out_shift_final = param.out_shift + mllib::stft::rsv_bits;
        auto it_out = begin_vector<load_len, aie_dm_resource::a>(ofm_data);
        for (int i = 0; i < param.frame_cnt; ++i) {
            auto it_win_padded = begin_vector<load_len>(tmp1);
            for (int j = 0; j < param.win_pad_len / load_len; ++j) {
                *it_win_padded++ = zeros<cint32, load_len>();
            }
            auto it_win = begin_vector<load_len, aie_dm_resource::a>(wts_data + STFT_WTS_OFFSET_BYTES / sizeof(cint16));
            // in case hop_len is not multiple of load_len
            // start from a previous position
            auto it_in = begin_vector<load_len, aie_dm_resource::a>(ifm_data + (signal_start & load_mask));
            const unsigned signal_shift_sz = signal_start - (signal_start & load_mask);
            auto v_signal0 = *it_in++;
            signal_start += param.hop_len;
            AIE_PREPARE_FOR_PIPELINING
            for (int j = 0; j < param.win_len / load_len; ++j) {
                auto v_signal1 = *it_in++;
                auto v_signal_shifted = shuffle_down_fill(v_signal0, v_signal1, signal_shift_sz);
                accum<cacc32, load_len> acc = mul(v_signal_shifted, *it_win++);
                auto v_win_padded = acc.to_vector<cint32>(downshift_win);
                v_win_padded = upshift(v_win_padded, upshift_win);
                *it_win_padded++ = v_win_padded;
                v_signal0 = v_signal1;
            }
            for (int j = 0; j < param.win_pad_len / load_len; ++j) {
                *it_win_padded++ = zeros<cint32, load_len>();
            }
            // perform fft using the double buffer
            mllib::stft::fft_full<fft_len>(tmp1, tw_head, param.tw_shift, param.tw_shift, tmp2);
            auto it_res = begin_vector<load_len>(res);
            // copy the results back to ofm, converting c32 to c16
            AIE_PREPARE_FOR_PIPELINING
            for (int j = 0; j < param.fft_out_len / load_len / 8; ++j) {
                auto v1 = downshift(*it_res++, out_shift_final);
                auto v2 = downshift(*it_res++, out_shift_final);
                auto v3 = downshift(*it_res++, out_shift_final);
                auto v4 = downshift(*it_res++, out_shift_final);
                auto v5 = downshift(*it_res++, out_shift_final);
                auto v6 = downshift(*it_res++, out_shift_final);
                auto v7 = downshift(*it_res++, out_shift_final);
                auto v8 = downshift(*it_res++, out_shift_final);
                *it_out++ = filter_even(v1.cast_to<int16_t>()).cast_to<cint16>();
                *it_out++ = filter_even(v2.cast_to<int16_t>()).cast_to<cint16>();
                *it_out++ = filter_even(v3.cast_to<int16_t>()).cast_to<cint16>();
                *it_out++ = filter_even(v4.cast_to<int16_t>()).cast_to<cint16>();
                *it_out++ = filter_even(v5.cast_to<int16_t>()).cast_to<cint16>();
                *it_out++ = filter_even(v6.cast_to<int16_t>()).cast_to<cint16>();
                *it_out++ = filter_even(v7.cast_to<int16_t>()).cast_to<cint16>();
                *it_out++ = filter_even(v8.cast_to<int16_t>()).cast_to<cint16>();
            }
        }        
    }
}

template <unsigned fft_len, typename dtype, typename config_ifm, typename config_wts, typename config_ofm>
INLINE void stft
(
        adf::input_buffer_conf<dtype, config_ifm> & __restrict ifm,
        adf::input_buffer_conf<dtype, config_wts> & __restrict wts,
        adf::output_buffer_conf<dtype, config_ofm> & __restrict ofm,
        stft_params_t &param
) {
    return stft<fft_len, dtype>(ifm.data(), wts.data(), ofm.data(), param);
}

#endif
