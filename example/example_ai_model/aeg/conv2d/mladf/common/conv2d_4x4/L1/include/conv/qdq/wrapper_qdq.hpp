// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*  (c) Copyright 2019 - 2021 Xilinx, Inc. All rights reserved.

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

#ifndef __WRAPPER_QDQ_HPP__
#define __WRAPPER_QDQ_HPP__
#include <adf.h>
//#include "../../arch_kernels/conv2d_8x1y_512/biased_conv_int8x8_ld2_template.h"

template<typename T>
#if __AIE_ARCH__ <= 20
//typedef qdq_C_t qdq_C_tt;
//typedef qdq_I_t qdq_I_tt;
//typedef qdq_O_t qdq_O_tt;
using qdq_WA_type = typename std::conditional_t<std::is_same_v<T, bfloat16>, int16, T>;
#else
using qdq_WA_type = typename std::conditional_t<std::is_same_v<T, bfloat16>, int16, std::conditional_t<std::is_same_v<T, float>, int32, T>>;
//using qdq_C_tt = typename std::conditional_t<std::is_same_v<qdq_C_t,float>,int32,qdq_C_t>;
//using qdq_I_tt = typename std::conditional_t<std::is_same_v<qdq_I_t,float>,int32,qdq_I_t>;
//using qdq_O_tt = typename std::conditional_t<std::is_same_v<qdq_O_t,float>,int32,qdq_O_t>;
#endif
using qdq_C_tt = qdq_WA_type<qdq_C_t>;
using qdq_I_tt = qdq_WA_type<qdq_I_t>;
using qdq_O_tt = qdq_WA_type<qdq_O_t>;

constexpr int Half_DM = 16 * 1024 / 4;

template<typename Ta, typename Ts, unsigned Mgran, unsigned Kgran, unsigned Mtile, unsigned Ktile, unsigned inner_lr_min=6, unsigned outer_lr_min=3>
__attribute__((noinline)) void sum_top( adf::input_buffer<Ta> &ifm, adf::output_buffer<Ts> &ofm, unsigned address_config=0 );

template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned vector_coeffs=2, unsigned fp_accuracy_mode=1, unsigned fp_split_threshold=1>
void wrapper_sym( adf::input_buffer<Ti> &ifm, adf::input_buffer<Tq> &coeffs_c, adf::output_buffer<Tr> &ofm );

template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned vector_coeffs=2, unsigned fp_accuracy_mode=1, unsigned fp_split_threshold=1, unsigned terms=3>
void wrapper_asym( adf::input_buffer<Ti> &ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<Tq> &coeffs_c, adf::output_buffer<Tr> &ofm );

template<typename Ts, typename Tq, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned fp_split_threshold=1, unsigned fp_accuracy_mode=1, typename T1=int32>
void wrapper_sym_conv( adf::input_buffer<T1> &ifm, adf::input_buffer<T1> &coeffs, adf::output_buffer<Tr> &ofm, qdq_conv_params &param);

template<typename Ts, typename Tq, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned fp_split_threshold=1, unsigned fp_accuracy_mode=1>
void wrapper_asym_conv( adf::input_buffer<int32> &ifm, adf::input_buffer<Ts> &ifm_sum, adf::input_buffer<qdq_C_tt> &coeffs, adf::output_buffer<Tr> &ofm );

template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned vector_coeffs=2, unsigned fp_accuracy_mode=1, unsigned fp_split_threshold=1>
void wrapper_sym_wa( adf::input_buffer<qdq_I_tt> &ifm, adf::input_buffer<qdq_C_tt> &coeffs_c, adf::output_buffer<qdq_O_tt> &ofm );

template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned vector_coeffs=2, unsigned fp_accuracy_mode=1, unsigned fp_split_threshold=1, unsigned terms=3>
void wrapper_asym_wa( adf::input_buffer<qdq_I_tt> &ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<qdq_C_tt> &coeffs_c, adf::output_buffer<qdq_O_tt> &ofm );


#endif //__WRAPPER_QDQ_HPP__
