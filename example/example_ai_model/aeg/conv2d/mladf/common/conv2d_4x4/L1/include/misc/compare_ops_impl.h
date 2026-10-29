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

#ifndef __COMPARE_OPS_IMPL_H__
#define __COMPARE_OPS_IMPL_H__

#include "aie.hpp"
#include "aie_api/utils.hpp"
#include "utils.h"

using namespace aie;

#ifndef IN_1_DM_BANK
#define IN_1_DM_BANK __aie_dm_resource_a
#endif

#ifndef IN_2_DM_BANK
#define IN_2_DM_BANK __aie_dm_resource_b
#endif

#ifndef OUT_DM_BANK
#define OUT_DM_BANK __aie_dm_resource_c
#endif

#ifndef COMPARE_OPS_LP_SIZE
#define COMPARE_OPS_LP_SIZE 6
#endif

enum compare_op : uint16_t
{
  CMP_LT = 1,
  CMP_LE = 2,
  CMP_GT = 4,
  CMP_GE = 8,
  CMP_EQ = 16
};

enum compare_kernel : uint16_t
{
  K_LT = 1,
  K_LE,
  K_LE_LT,
  K_GT,
  K_GT_LT,
  K_GT_LE,
  K_GT_LE_LT,
  K_GE,
  K_GE_LT,
  K_GE_LE,
  K_GE_LE_LT,
  K_GE_GT,
  K_GE_GT_LT,
  K_GE_GT_LE,
  K_GE_GT_LE_LT,
  K_EQ,
  K_EQ_LT,
  K_EQ_LE,
  K_EQ_LE_LT,
  K_EQ_GT,
  K_EQ_GT_LT,
  K_EQ_GT_LE,
  K_EQ_GT_LE_LT,
  K_EQ_GE,
  K_EQ_GE_LT,
  K_EQ_GE_LE,
  K_EQ_GE_LE_LT,
  K_EQ_GE_GT,
  K_EQ_GE_GT_LT,
  K_EQ_GE_GT_LE,
  K_EQ_GE_GT_LE_LT,
};


template<typename dtype>
struct CMP_VEC_SIZE_chooser {
  static constexpr uint16_t vec_size = 64;
};

template<>
struct CMP_VEC_SIZE_chooser<int32_t> {
  static constexpr uint16_t vec_size = 32;
};

template <bool mllib_bn_bool, int bitwidth>
struct SelectTypeByBnBW;

template <int bitwidth>
struct SelectTypeByBnBW<true, bitwidth> {
    using type = uint32_t;
};

template <>
struct SelectTypeByBnBW<true, 64> {
    using type = uint64_t;
};

template <int bitwidth>
struct SelectTypeByBnBW<false, bitwidth> {
    using type = uint8_t;
};


template <typename dtype, uint16_t k_impl>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, int32_t, bfloat16>)
struct compare_ops
{
  static constexpr uint16_t CMP_VEC_SIZE = CMP_VEC_SIZE_chooser<dtype>::vec_size;

  typedef struct alignas(v32uint16)
  {
    // set by rtp
    // uint32
    uint16_t ifmsv_size;
    uint16_t op;
    // uint32
    uint32_t num_kernel_iters; // used by superkernel to setup params only once, not used by kernel itself.
    uint32_t scalar_position; 
    uint16_t scalar;

    // used for kernel internal
    uint16_t num_iters;
    bool switch_operand;
    aie::mask<CMP_VEC_SIZE> mask_ge_le, mask_gt_lt, mask_eq;
    int32_t lhs_inc;// left-hand side operand's increment
    int32_t rhs_inc;// right-hand side operand's increment
    int32_t original_scalar_position;
  } params_t;

  __attribute__((noinline)) static void setup(params_t &params, const void *rtps)
  {
    const uint32_t* rtp_uint32 = (const uint32_t*) (rtps);
    uint32_t* params_ptr = (uint32_t*) &params;
    *params_ptr++ = *rtp_uint32++; // ifmsv_size, op
    *params_ptr++ = *rtp_uint32++; // num_kernel_iters
    *params_ptr++ = *rtp_uint32++; // scalar position

    const uint16_t*  rtp_uint16 = (uint16_t*) rtp_uint32;
    uint16_t* params_ptr_uint16 = (uint16_t*) params_ptr;
    *params_ptr_uint16++= *rtp_uint16++; // scalar

    params.original_scalar_position = params.scalar_position;
    params.num_iters = params.ifmsv_size / (CMP_VEC_SIZE);
    // signal for input switch
    if constexpr (((k_impl & CMP_LT) + (k_impl & CMP_GT)) == (CMP_LT + CMP_GT) || ((k_impl & CMP_LE) + (k_impl & CMP_GE)) == (CMP_LE + CMP_GE))
    {
      // when kernel support CMP_LT + CMP_GT or CMP_LE + CMP_GE at the same time, for CMP_GT and CMP_LE, operand need to be switched.
      params.switch_operand = (params.op == CMP_GE || params.op == CMP_LT) ? false : true;
    }

    // masks for output selection
    if constexpr (k_impl == K_LE_LT || k_impl == K_GT_LE || k_impl == K_GT_LE_LT || k_impl == K_GE_LT || k_impl == K_GE_LE_LT || k_impl == K_GE_GT ||
                  k_impl == K_GE_GT_LT || k_impl == K_GE_GT_LE || k_impl == K_GE_GT_LE_LT)
    {
      params.mask_ge_le = (params.op == CMP_GE || params.op == CMP_LE) ? aie::mask<CMP_VEC_SIZE>(true) : aie::mask<CMP_VEC_SIZE>(false);
      params.mask_gt_lt = (params.op == CMP_GT || params.op == CMP_LT) ? aie::mask<CMP_VEC_SIZE>(true) : aie::mask<CMP_VEC_SIZE>(false);
    }
    else if constexpr (k_impl > K_EQ)
    {
      if constexpr ((k_impl & CMP_GE) == CMP_GE || (k_impl & CMP_LE) == CMP_LE)
      {
        params.mask_ge_le = (params.op == CMP_GE || params.op == CMP_LE) ? aie::mask<CMP_VEC_SIZE>(true) : aie::mask<CMP_VEC_SIZE>(false);
      }
      if constexpr ((k_impl & CMP_GT) == CMP_GT || (k_impl & CMP_LT) == CMP_LT)
      {
        params.mask_gt_lt = (params.op == CMP_GT || params.op == CMP_LT) ? aie::mask<CMP_VEC_SIZE>(true) : aie::mask<CMP_VEC_SIZE>(false);
      }

      params.mask_eq = (params.op == CMP_EQ) ? aie::mask<CMP_VEC_SIZE>(true) : aie::mask<CMP_VEC_SIZE>(false);
    }
    // support broadcasting, 0. vector op vector. 1. scalar op vector. 2. vector op scalar.
    if (params.switch_operand){
      params.scalar_position = (params.scalar_position == 0) ? 1 : (params.scalar_position == 1) ? 0 : params.scalar_position;
    }

    params.lhs_inc = (params.scalar_position == 0) ? 0 : CMP_VEC_SIZE;
    params.rhs_inc = (params.scalar_position == 1) ? 0 : CMP_VEC_SIZE;

  }

  __attribute__((noinline)) static void run(dtype* __restrict ifm1,
                                            dtype* __restrict ifm2,
                                            int8_t* ofm,
                                            params_t & __restrict params)
  {

    dtype IN_1_DM_BANK *__restrict p_in_1;
    dtype IN_2_DM_BANK *__restrict p_in_2;
    if constexpr (((k_impl & CMP_LT) + (k_impl & CMP_GT)) == (CMP_LT + CMP_GT) || ((k_impl & CMP_LE) + (k_impl & CMP_GE)) == (CMP_LE + CMP_GE))
    {
      p_in_1 = params.switch_operand ? (dtype IN_1_DM_BANK *__restrict)ifm2 : (dtype IN_1_DM_BANK *__restrict)ifm1;
      p_in_2 = params.switch_operand ? (dtype IN_2_DM_BANK *__restrict)ifm1 : (dtype IN_2_DM_BANK *__restrict)ifm2;
    }
    else
    {
      p_in_1 = (dtype IN_1_DM_BANK *__restrict)ifm1;
      p_in_2 = (dtype IN_2_DM_BANK *__restrict)ifm2;
    }
#ifdef MLLIB_ONE_BIT_BOOL
    using pType = typename SelectTypeByBnBW<true,CMP_VEC_SIZE>::type;
    pType OUT_DM_BANK *__restrict p_out = (pType OUT_DM_BANK *__restrict) ofm;
#else
    using pType = typename SelectTypeByBnBW<false,CMP_VEC_SIZE>::type;
    pType OUT_DM_BANK *__restrict p_out = (pType OUT_DM_BANK *__restrict) ofm;
#endif

    // support broadcasting, write back to ifm.
    if (params.scalar_position == 0){
      auto sca_vec = aie::broadcast<dtype, CMP_VEC_SIZE>(*p_in_1);
      store_v(p_in_1, sca_vec);
    }else if (params.scalar_position == 1){
      auto sca_vec = aie::broadcast<dtype, CMP_VEC_SIZE>(*p_in_2);
      store_v(p_in_2, sca_vec);
    }

    aie::vector<dtype, CMP_VEC_SIZE> in_1, in_2;
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE(7, ) 
    for (int i = 0; i < params.num_iters; i++){
        // data input
        in_1 = load_v<CMP_VEC_SIZE>(p_in_1);
        p_in_1 = byte_incr(p_in_1, params.lhs_inc * sizeof(dtype));

        in_2 = load_v<CMP_VEC_SIZE>(p_in_2);
        p_in_2 = byte_incr(p_in_2, params.rhs_inc * sizeof(dtype));

        // compare
        aie::mask<CMP_VEC_SIZE> msk;
        if constexpr (k_impl == K_LT || k_impl == K_GT_LT)
        {
          msk = lt(in_1, in_2);
        }
        else if constexpr (k_impl == K_LE)
        {
          msk = ge(in_2, in_1);
        }
        else if constexpr (k_impl == K_LE_LT)
        {
          auto res1 = ge(in_2, in_1);
          auto res2 = lt(in_1, in_2);
          msk = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt);
        }
        else if constexpr (k_impl == K_GT)
        {
          msk = lt(in_2, in_1);
        }
        else if constexpr (k_impl == K_GT_LE)
        {
          auto res1 = ge(in_2, in_1);
          auto res2 = lt(in_2, in_1);
          msk = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt);
        }
        else if constexpr (k_impl == K_GE || k_impl == K_GE_LE)
        {
          msk = ge(in_1, in_2);
        }
        else if constexpr (k_impl == K_GE_GT)
        {
          auto res1 = ge(in_1, in_2);
          auto res2 = lt(in_2, in_1);
          msk = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt);
        }
        else if constexpr (k_impl == K_GE_LT || k_impl == K_GT_LE_LT || k_impl == K_GE_LE_LT || k_impl == K_GE_GT_LT || k_impl == K_GE_GT_LE || k_impl == K_GE_GT_LE_LT)
        {
          auto res1 = ge(in_1, in_2); // for GE/LE
          auto res2 = lt(in_1, in_2); // for GT/LT
          msk = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt);
        }
        // below with EQ
        else if constexpr (k_impl == K_EQ)
        {
          msk = eq(in_1, in_2);
        }
        else if constexpr (k_impl == K_EQ_LT || k_impl == K_EQ_GT_LT)
        {
          auto res1 = lt(in_1, in_2);
          auto res2 = eq(in_1, in_2);
          msk = (res1 & params.mask_gt_lt) | (res2 & params.mask_eq);
        }
        else if constexpr (k_impl == K_EQ_LE)
        {
          auto res1 = ge(in_2, in_1);
          auto res2 = eq(in_1, in_2);
          msk = (res1 & params.mask_ge_le) | (res2 & params.mask_eq);
        }
        else if constexpr (k_impl == K_EQ_LE_LT) // may optimize
        {
          aie::vector<dtype, CMP_VEC_SIZE> zeros = aie::zeros<dtype, CMP_VEC_SIZE>();
          aie::vector<dtype, CMP_VEC_SIZE> ones = aie::broadcast<dtype, CMP_VEC_SIZE>(1);
          auto res1 = ge(in_2, in_1); // for LE
          auto res2 = lt(in_1, in_2); // for LT
          auto res3 = eq(in_1, in_2); // for EQ
          if constexpr (std::is_same<bfloat16, dtype>::value)
          {
            // for bf16 vector ops are much more than scalar
            msk = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt) | (res3 & params.mask_eq);
          }
          else
          {
          auto res4 = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt);
          auto res3_vec = select(zeros, ones, res3);
          auto res4_vec = select(zeros, ones, res4);
          auto msk_vec = select(res4_vec, res3_vec, params.mask_eq);
          msk = ge(msk_vec, ones);
          }
        }
        else if constexpr (k_impl == K_EQ_GT)
        {
          auto res1 = lt(in_2, in_1);
          auto res2 = eq(in_1, in_2);
          msk = (res1 & params.mask_gt_lt) | (res2 & params.mask_eq);
        }
        else if constexpr (k_impl == K_EQ_GT_LE)
        {
          aie::vector<dtype, CMP_VEC_SIZE> zeros = aie::zeros<dtype, CMP_VEC_SIZE>();
          aie::vector<dtype, CMP_VEC_SIZE> ones = aie::broadcast<dtype, CMP_VEC_SIZE>(1);
          auto res1 = ge(in_2, in_1); // for LE
          auto res2 = lt(in_2, in_1); // for GT
          auto res3 = eq(in_1, in_2); // for EQ
          if constexpr (std::is_same<bfloat16, dtype>::value)
          {
            // for bf16 vector ops are much more than scalar
            msk = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt) | (res3 & params.mask_eq);
          }
          else
          {
          auto res4 = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt);
          auto res3_vec = select(zeros, ones, res3);
          auto res4_vec = select(zeros, ones, res4);
          auto msk_vec = select(res4_vec, res3_vec, params.mask_eq);
          msk = ge(msk_vec, ones);
          }
        }
        else if constexpr (k_impl == K_EQ_GE || k_impl == K_EQ_GE_LE)
        {
          auto res1 = ge(in_1, in_2);
          auto res2 = eq(in_1, in_2);
          msk = (res1 & params.mask_ge_le) | (res2 & params.mask_eq);
        }
        else if constexpr (k_impl == K_EQ_GE_GT) // may optimize
        {
          aie::vector<dtype, CMP_VEC_SIZE> zeros = aie::zeros<dtype, CMP_VEC_SIZE>();
          aie::vector<dtype, CMP_VEC_SIZE> ones = aie::broadcast<dtype, CMP_VEC_SIZE>(1);
          auto res1 = ge(in_1, in_2); // for GE
          auto res2 = lt(in_2, in_1); // for GT
          auto res3 = eq(in_1, in_2); // for EQ
          if constexpr (std::is_same<bfloat16, dtype>::value)
          {
            // for bf16 vector ops are much more than scalar
            msk = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt) | (res3 & params.mask_eq);
          }
          else
          {
          auto res4 = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt);
          auto res3_vec = select(zeros, ones, res3);
          auto res4_vec = select(zeros, ones, res4);
          auto msk_vec = select(res4_vec, res3_vec, params.mask_eq);
          msk = ge(msk_vec, ones);
          }
        }
        else if constexpr (k_impl == K_EQ_GE_LT || k_impl == K_EQ_GT_LE_LT || k_impl == K_EQ_GE_LE_LT || k_impl == K_EQ_GE_GT_LT || k_impl == K_EQ_GE_GT_LE || k_impl == K_EQ_GE_GT_LE_LT)
        {
          aie::vector<dtype, CMP_VEC_SIZE> zeros = aie::zeros<dtype, CMP_VEC_SIZE>();
          aie::vector<dtype, CMP_VEC_SIZE> ones = aie::broadcast<dtype, CMP_VEC_SIZE>(1);
          auto res1 = ge(in_1, in_2); // for GE/LE
          auto res2 = lt(in_1, in_2); // for GT/LT
          auto res3 = eq(in_1, in_2); // for EQ
          if constexpr (std::is_same<bfloat16, dtype>::value)
          {
            // for bf16 vector ops are much more than scalar
            msk = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt) | (res3 & params.mask_eq);
          }
          else
          {
          auto res4 = (res1 & params.mask_ge_le) | (res2 & params.mask_gt_lt);
          auto res3_vec = select(zeros, ones, res3);
          auto res4_vec = select(zeros, ones, res4);
          auto msk_vec = select(res4_vec, res3_vec, params.mask_eq);
          msk = ge(msk_vec, ones);
          }
        }

#ifdef MLLIB_ONE_BIT_BOOL
      pType out;
      if constexpr (CMP_VEC_SIZE == 64){
        out = msk.to_uint64();
      }
      else{
        out = msk.to_uint32();
      }
      // write output
      *p_out++ = out;
#else
    aie::vector<uint8_t, CMP_VEC_SIZE> zeros = aie::zeros<uint8_t, CMP_VEC_SIZE>();
    aie::vector<uint8_t, CMP_VEC_SIZE> ones = aie::broadcast<uint8_t, CMP_VEC_SIZE>(1);
    aie::vector<uint8_t, CMP_VEC_SIZE> out = select(zeros, ones, msk);
    store_v(p_out, out);
    p_out = byte_incr(p_out, CMP_VEC_SIZE);
#endif
      }
  }
};

#endif //__COMPARE_OPS_IMPL_H__
