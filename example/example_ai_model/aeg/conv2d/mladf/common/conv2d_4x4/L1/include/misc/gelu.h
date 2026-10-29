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

#ifndef __GELU_H__
#define __GELU_H__

#include "aie.hpp"
#include "gelu_params.h"
#include "utils.h"
#include <adf.h>

using namespace aie;

// GELU const
const bfloat16 P1 = 1.702;
const bfloat16 Q0 = 0.5;
const bfloat16 Q1 = 0.25;
const bfloat16 Q2 = 0.03125;
const bfloat16 minus_four = -4.0;
const bfloat16 four = 4.0;

// Input is ifm.
// Output is in ofm.
// All computation is performed in Bfloat16
// Kernel parameters are passed by structure params

template <typename dtype>
    requires(std::is_same_v<dtype, bfloat16>)
NOINLINE void gelu
  (
     dtype IFM_DM_BANK * __restrict p_in,
     dtype OFM_DM_BANK * __restrict p_out,
     gelu_params_t  &params
  ) {

	  // preparing vector & accum
	  aie::vector<dtype, VEC_LEN> vec_in, mul1_vec, mul1_vec_abs, P1_vec, Q0_vec;
	  aie::accum<accfloat, VEC_LEN> P1_acc, mac0_acc, mul1_acc, mul2_acc;
	  aie::accum<accfloat, VEC_LEN> Q0_acc, mul0, mac0, msc0;

	  P1_vec = aie::broadcast<bfloat16, VEC_LEN>(P1);
	  P1_acc.from_vector(P1_vec);

	  Q0_vec = aie::broadcast<bfloat16, VEC_LEN>(Q0);
	  Q0_acc.from_vector(Q0_vec);


	  // GELU func
	  AIE_PREPARE_FOR_PIPELINING 
	  AIE_LOOP_RANGE(4, ) 
	  for (int i = 0; i < params.num_iter; i = i + 1){

	      // data input
	      vec_in = aie::load_v<VEC_LEN>(p_in);
              p_in = byte_incr(p_in, 64);

	      // part-1: in * 1.702
	      mul1_acc = aie::mul(P1_vec, vec_in);
	      mul1_vec = mul1_acc.template to_vector<dtype>();

	      // part-2: using sigmoid for approximation
	      mul1_vec = aie::min(mul1_vec, four);
	      mul1_vec = aie::max(mul1_vec, minus_four);
	      mul1_vec_abs = aie::abs(mul1_vec);
	      mul0 = aie::mul(Q2, mul1_vec_abs);
	      mac0 = aie::mac(Q0_acc, mul1_vec, Q1);
	      msc0 = aie::msc(mac0, mul0.template to_vector<dtype>(), mul1_vec);

	      //part-3: in * sigmoid_results
	      mul2_acc = aie::mul(msc0.template to_vector<dtype>(), vec_in);

	      // result output
	      aie::store_v((bfloat16 *)p_out, mul2_acc.to_vector<dtype>());
              //aie::store_v((bfloat16 *)p_out, mul1_vec);
	      //p_out = byte_incr(p_out, VEC_LEN * sizeof(dtype));
              p_out = byte_incr(p_out, 64);
     }
}

template <typename dtype, typename config_ifm,typename config_ofm>
    requires(std::is_same_v<dtype, bfloat16>)
INLINE void gelu
  (
     adf::input_buffer_conf<dtype, config_ifm> &__restrict in,
     adf::output_buffer_conf<dtype, config_ofm> &__restrict out,
     gelu_params_t  &params
  ) {
    dtype IFM_DM_BANK * __restrict p_in = (bfloat16 IFM_DM_BANK * __restrict)in.data(); //TODO: Restrict
    dtype OFM_DM_BANK * __restrict p_out = (bfloat16 OFM_DM_BANK * __restrict)out.data(); //TODO: Restrict
    return gelu<dtype>(p_in, p_out, params);
}

#endif //__GELU_H__
