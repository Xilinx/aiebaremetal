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

#include "adf.h"
#include "aie.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"

#include "elementwise_unary.h"
#include "sin_params.h"
#include "sin_lut.h"

template <typename dtype> struct sin_config {};
// use "#if __AIE_ARCH__ == 20, 21, 22" to fine tune your config
#if __AIE_ARCH__ == 20
template <> struct sin_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
template <> struct sin_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 16;
};
#else
template <> struct sin_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 4;
  static constexpr uint16_t vec_size = 32;
};
template <> struct sin_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 32;
};
#endif
template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_sin : elementwise_unary_base<dtype, sin_params_t<dtype>> {
public:
  static constexpr bool peel = sin_config<dtype>::peel;
  static constexpr uint8_t unroll = sin_config<dtype>::unroll;
  static constexpr uint16_t minimum_num_iter = sin_config<dtype>::minimum_num_iter;
  static constexpr uint16_t vec_size = sin_config<dtype>::vec_size;
  static constexpr uint16_t byte_size = sizeof(dtype);
  using vec_t = aie::vector<dtype, vec_size>;
  using params_t = elementwise_unary_params_t<sin_params_t<dtype>>;

public:
  // setup auxiliary member variable
  elementwise_sin(const params_t &params) {}

  inline __attribute__((always_inline))
  void body(const vec_t &in_vec, vec_t &out_vec, params_t &params) {
    if constexpr(std::is_same<bfloat16, dtype>::value) {
      sin_internal_bfloat16(in_vec, out_vec);
    } else {
      sin_internal_int8(in_vec, out_vec, params);
    }
  }

  inline __attribute__((always_inline))
  static void setup(params_t &params, const void *rtps) {}

private:
  static constexpr struct {
      float poly_sin[4];
      float poly_cos[4];
      const float twobypi, piby2_1;
  } sin_data = {
      .twobypi = 0x1.45f306dc9c883p-1,
      .piby2_1 = 0x1.921fb54400000p0,
      /*
      * Polynomial coefficients
      */
      .poly_sin = {
          -0x1.5555555555555p-3,
          0x1.1111111110bb3p-7,
          -0x1.a01a019e83e5cp-13,
          0x1.71de3796cde01p-19
      },
      .poly_cos = {
          0x1.5555555555555p-5,   /* 0.0416667 */
          -0x1.6c16c16c16967p-10, /* -0.00138889 */
          0x1.A01A019F4EC91p-16,  /* 2.48016e-005 */
          -0x1.27E4FA17F667Bp-22 /* -2.75573e-007 */
      },
  };

  inline __attribute__((always_inline))
  void sin_internal_bfloat16(const vec_t &ux, vec_t &output) {
    accum<accfloat, vec_size> ux_acc, x_acc, r_acc, x3_acc, t_acc, output_acc;
    accum<accfloat, vec_size> poly_acs, acs0, acs2, out_acs;
    accum<accfloat, vec_size> poly_acc, acc0, acc2, out_acc;
    accum<accfloat, vec_size> zp_acc, z_acc, one_acc;

    vector<bfloat16, vec_size> x, r, r2, out_abs;
    vector<bfloat16, vec_size> s0, s1, s2, s3, c0, c1, c2, c3;
    vector<bfloat16, vec_size> twobypi, oneby2, negoneby2, piby2_1, one, zero_f, negone;
    vector<int16_t, vec_size> z, oneint16, twoint16;

    // preparing vector & accum
    s0 = broadcast<bfloat16, vec_size>(sin_data.poly_sin[0]);
    s1 = broadcast<bfloat16, vec_size>(sin_data.poly_sin[1]);
    s2 = broadcast<bfloat16, vec_size>(sin_data.poly_sin[2]);
    s3 = broadcast<bfloat16, vec_size>(sin_data.poly_sin[3]);
    c0 = broadcast<bfloat16, vec_size>(sin_data.poly_cos[0]);
    c1 = broadcast<bfloat16, vec_size>(sin_data.poly_cos[1]);
    c2 = broadcast<bfloat16, vec_size>(sin_data.poly_cos[2]);
    c3 = broadcast<bfloat16, vec_size>(sin_data.poly_cos[3]);
    twobypi = broadcast<bfloat16, vec_size>(sin_data.twobypi);
    piby2_1 = broadcast<bfloat16, vec_size>(sin_data.piby2_1);
    oneby2 = broadcast<bfloat16, vec_size>(0.5);
    negoneby2 = broadcast<bfloat16, vec_size>(-0.5);
    one = broadcast<bfloat16, vec_size>(1.0);
    negone = aie::broadcast<bfloat16, vec_size>(-1.0);
    // preparing vector for z
    oneint16 = broadcast<int16, vec_size>(1);
    twoint16 = broadcast<int16, vec_size>(2);
    // acc
    acs0.from_vector(s0);
    acs2.from_vector(s2);
    acc0.from_vector(c0);
    acc2.from_vector(c2);
    one_acc.from_vector(one);
#define POLY_EVAL_4_VECTOR(r, s0, s1, s2, s3, c0, c1, c2, c3, outs, outc, r2) \
    {                                                                       \
        accum<accfloat, vec_size> t1s, t2s, t1c, t2c, r2_acc;               \
        t1s = mac(s0, s1, r);                                               \
        t2s = mac(s2, s3, r);                                               \
        t1c = mac(c0, c1, r);                                               \
        t2c = mac(c2, c3, r);                                               \
        r2_acc = mul(r, r);                                                 \
        r2 = r2_acc.template to_vector<bfloat16>();                         \
        outs = mac(t1s, r2, t2s.template to_vector<bfloat16>());            \
        outc = mac(t1c, r2, t2c.template to_vector<bfloat16>());            \
    }
    // Sin function /* ux from -256.0 to 256.0 res achieve 0.07*/
        // step 1 : float32 mod
        /*
            zf = ux*(2.0/pi) +/- 0.5
            z = to_fixed(zf)
            x = ux - z * pi/2
            to_fixed(zp_acc.template to_vector<bfloat16>()) seems works a floor
            so we just need z = ux*(2/pi) + 0.5
        */
        // step 2 : get sin(x) and cos(x) in -pi/4 ~ pi/4
        //          /* res achieve 1-ulp or 0.001 in -pi/4 ~ pi/4 */
        // step 3 : select
        /*
            if z == 0 : return sin(x)
            if z == 1 : return cos(x)
            if z == 2 : return -sin(x)
            if z == 3 : return -cos(x)
        */
    //  step 1 :
    zp_acc.from_vector(oneby2);
    ux_acc.from_vector(ux);
    zp_acc = mac(zp_acc, ux, twobypi);
    z = to_fixed<int16_t>(zp_acc.template to_vector<bfloat16>(), 0);
    x_acc = msc(ux_acc, piby2_1, to_float<bfloat16>(z));

    mask<vec_size> is_odd = eq(bit_and(z, oneint16), oneint16); // select sin or cos
    mask<vec_size> is_neg = eq(bit_and(z, twoint16), twoint16); // select positive or negetive

    // // step 2 : |x|<pi/4
    // sin x
    x = x_acc.template to_vector<bfloat16>();
    r_acc = mul(x, x); /* r = x*x */
    r = r_acc.template to_vector<bfloat16>();

    // cos x
    t_acc = mac(one_acc, negoneby2, r);

    /* x + (x * (r * (C0 + r * (C1 + r * (C2 + r * C3))))) */
    POLY_EVAL_4_VECTOR(r, acs0, s1, acs2, s3, acc0, c1, acc2, c3, poly_acs, poly_acc, r2);

    x3_acc = mul(r, x);
    out_acs = mac(x_acc, x3_acc.template to_vector<bfloat16>(), poly_acs.template to_vector<bfloat16>());
    out_acc = mac(t_acc, r2, poly_acc.template to_vector<bfloat16>());/* r2 = x*x*x*x */

    // step 3 :
    out_abs = select(out_acs.template to_vector<bfloat16>(), out_acc.template to_vector<bfloat16>(), is_odd);
    output_acc = aie::mul(negone, out_abs);
    output = select(out_abs, output_acc.template to_vector<bfloat16>(), is_neg);
  }

  inline __attribute__((always_inline))
  void sin_internal_int8(const vec_t &in_vec, vec_t &out_vec, params_t &params) {
    unsigned step = 0;
    unsigned bias = 0; //((input >> step_bits) + bias)
    aie::lut<4, dtype, dtype> lut_f(256, sin_round_ab, sin_round_cd);
    aie::parallel_lookup<int16, aie::lut<4, dtype, dtype>, aie::lut_oor_policy::truncate>
           lookup_int_sin(lut_f, step, bias);

    const uint32_t int8_scale = params.ofm_shift;
    vector<dtype, vec_size> in, scale6, out;
    vector<int16_t, vec_size> index = add(in_vec.unpack(), (int16_t)128);
    scale6 = lookup_int_sin.fetch(index);
    out_vec = downshift(scale6, (6-int8_scale));
  }
};
