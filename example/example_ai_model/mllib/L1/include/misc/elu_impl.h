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



// elu_params.h, elu_config {}

#pragma once

#include "adf.h"
#include "aie.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"

#include "elementwise_unary.h"
#include "elu_params.h"

// #include "expm1_lut_full.h"
#include "expm1_lut_half.h"
#include "aie_adf.hpp"

#ifndef IFM_DM_BANK
#define IFM_DM_BANK __aie_dm_resource_a
#endif

#ifndef OFM_DM_BANK
#define OFM_DM_BANK __aie_dm_resource_a
#endif

// User configuration for design optimization
template <typename dtype> struct elu_config {};

#if __AIE_ARCH__ == 20 // AIE2
template <> struct elu_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 32;
};
template <> struct elu_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 8;
  static constexpr uint16_t vec_size = 32;
};

#elif __AIE_ARCH__ == 21 || __AIE_ARCH__ == 22 // AIE2p and AIE2ps
template <> struct elu_config<int8_t> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 2;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 32;
};
template <> struct elu_config<bfloat16> {
  static constexpr bool peel = false;
  static constexpr uint8_t unroll = 1;
  static constexpr uint16_t minimum_num_iter = 2;
  static constexpr uint16_t vec_size = 32;
};
#endif

template <typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, bfloat16>)
class elementwise_elu {};

// implementation of elu<bfloat16>
template <> class elementwise_elu<bfloat16> : elementwise_unary_base<bfloat16, elu_params_t<bfloat16>> {
private:
  float coeff_elu[4] = {1.0f, 1.0f, 0.486328125f, 0.21875f};
  static constexpr bfloat16 exponent_min = FLOAT_TO_BFLOAT16(float(-88.0));
  static constexpr bfloat16 exponent_max = FLOAT_TO_BFLOAT16(float(88.0));
  // segmentation-0: [-0.4296875, ]
  static constexpr bfloat16 segment_0 = FLOAT_TO_BFLOAT16(float(-0.4296875)); 
  static constexpr bfloat16 Q3_s0 = FLOAT_TO_BFLOAT16(float(0.166015625));
  static constexpr bfloat16 Q2_s0 = FLOAT_TO_BFLOAT16(float(0.500000000));
  static constexpr bfloat16 Q1_s01 = FLOAT_TO_BFLOAT16(float(1.000000000));
  static constexpr bfloat16 Q1_s00 = FLOAT_TO_BFLOAT16(float(0.000000000));
  // segmentation-1: [-0.69921875, -0.4296875]
  static constexpr bfloat16 segment_1 = FLOAT_TO_BFLOAT16(float(-0.69921875)); 
  static constexpr bfloat16 Q3_s1 = FLOAT_TO_BFLOAT16(float(0.1083984375));
  static constexpr bfloat16 Q2_s1 = FLOAT_TO_BFLOAT16(float(0.3242187500));
  static constexpr bfloat16 Q1_s11 = FLOAT_TO_BFLOAT16(float(0.6484375));
  static constexpr bfloat16 Q1_s10 = FLOAT_TO_BFLOAT16(float(0.00227355957031));

  // static constexpr bfloat16 seg_s0 = -4.470348358154297e-07;   // -4.172325134277344e-07, alpha_res) == lut_margin
  static constexpr bfloat16 seg_s1 = FLOAT_TO_BFLOAT16(float(-3.8743019104003906e-07));  // -3.5762786865234375e-07, alpha_res)
  static constexpr bfloat16 seg_s2 = FLOAT_TO_BFLOAT16(float(-3.2782554626464844e-07));  // -2.980232238769531e-07, alpha_res
  static constexpr bfloat16 seg_s3 = FLOAT_TO_BFLOAT16(float(-2.682209014892578e-07));   //-2.384185791015625e-07, alpha_res)
  static constexpr bfloat16 seg_s4 = FLOAT_TO_BFLOAT16(float(-2.086162567138672e-07));   //-1.7881393432617188e-07, alpha_res)
  static constexpr bfloat16 seg_s5 = FLOAT_TO_BFLOAT16(float(-1.4901161193847656e-07));  //  -1.1920928955078125e-07, alpha_res)
  static constexpr bfloat16 seg_s6 = FLOAT_TO_BFLOAT16(float(-8.940696716308594e-08));   //-5.960464477539063e-08, alpha_res)
  static constexpr bfloat16 seg_s7 = FLOAT_TO_BFLOAT16(float(-2.9802322387695312e-08));  // 0.0, alpha_res)

  static constexpr bfloat16 value_seg_s0 = FLOAT_TO_BFLOAT16(float(-4.172325134277344e-07));
  static constexpr bfloat16 value_seg_s1 = FLOAT_TO_BFLOAT16(float(-3.5762786865234375e-07));  // -3.5762786865234375e-07, alpha_res)
  static constexpr bfloat16 value_seg_s2 = FLOAT_TO_BFLOAT16(float(-2.980232238769531e-07));  // -2.980232238769531e-07, alpha_res
  static constexpr bfloat16 value_seg_s3 = FLOAT_TO_BFLOAT16(float(-2.384185791015625e-07));   //-2.384185791015625e-07, alpha_res)
  static constexpr bfloat16 value_seg_s4 = FLOAT_TO_BFLOAT16(float(-1.7881393432617188e-07));   //-1.7881393432617188e-07, alpha_res)
  static constexpr bfloat16 value_seg_s5 = FLOAT_TO_BFLOAT16(float(-1.1920928955078125e-07));  //  -1.1920928955078125e-07, alpha_res)
  static constexpr bfloat16 value_seg_s6 = FLOAT_TO_BFLOAT16(float(-5.960464477539063e-08));   //-5.960464477539063e-08, alpha_res)
  static constexpr bfloat16 value_seg_s7 = FLOAT_TO_BFLOAT16(float(0.0));  // 0.0, alpha_res)

public:
  static constexpr bool peel = elu_config<bfloat16>::peel;
  static constexpr uint8_t unroll = elu_config<bfloat16>::unroll;
  static constexpr uint16_t minimum_num_iter = elu_config<bfloat16>::minimum_num_iter;
  static constexpr uint16_t vec_size = elu_config<bfloat16>::vec_size;

  using acc_t = accfloat;
  using vec_t = aie::vector<bfloat16, vec_size>;
  using params_t = elementwise_unary_params_t<elu_params_t<bfloat16>>;

  aie::mask<vec_size> x_mask; // false when  f(x) = alpha*(exp_x-1), true f(x) = x
  bfloat16 alpha;

  elementwise_elu(const params_t &op_params) {
      alpha = op_params.op_params.alpha;
  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
      const uint16_t *rtp_uint16 = (const uint16_t *)(rtps);
      op_params.op_params.alpha = *(bfloat16*)&rtp_uint16[16];
  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {
  

    aie::vector<bfloat16, vec_size> clip_vec = aie::max(aie::min(in_vec, exponent_max), exponent_min);

    //-----------------------------------------------------------------------------------//
    // poly-elu for [-88, -0.69921875)
    //-----------------------------------------------------------------------------------//
    aie::accum<accfloat, vec_size> acc (clip_vec);

    /* Step 1 : scaling
    # scaling_factor = x * 1.4426950408889634
    # n = scaling_factor
    # m = scaling_factor
    # j = n - m = 0
    # r = x - n*0.6931471805599453 */
    aie::vector<bfloat16, vec_size> sf_n_m = aie::mul((bfloat16)1.4453125f, clip_vec).to_vector<bfloat16>();
    aie::vector<int32_t, vec_size>  floor  = aie::to_fixed<int32_t>(sf_n_m, 0);

    auto floor_tof = aie::to_float(floor,0);
    aie::vector<bfloat16, vec_size> temp2  = aie::mul(floor_tof,(float)0.69140625f).to_vector<bfloat16>(); 
    aie::vector<bfloat16, vec_size> r      = aie::sub(acc, temp2).to_vector<bfloat16>();

    /* Step 2 : approx poly(r)
    y = c0 + x(c1 + x(c2 + x(c3)))
    */
    aie::vector<bfloat16, vec_size> vacc = aie::broadcast<bfloat16, vec_size>(coeff_elu[2]);
    aie::accum<accfloat, vec_size> acc_poly(vacc);  
    aie::vector<bfloat16, vec_size> vcoeff_elu = aie::broadcast<bfloat16, vec_size>(coeff_elu[3]);
    acc_poly = aie::mac(acc_poly, r, vcoeff_elu);
    vacc = acc_poly.to_vector<bfloat16>();
    vcoeff_elu = aie::broadcast<bfloat16, vec_size>(coeff_elu[1]);
    acc_poly = aie::accum<accfloat, vec_size>(vcoeff_elu);
    acc_poly = aie::mac(acc_poly, r, vacc);
    aie::vector<float, vec_size> temp_vec = acc_poly.to_vector<float>();
    vacc = acc_poly.to_vector<bfloat16>();

    vcoeff_elu = aie::broadcast<bfloat16, vec_size>(coeff_elu[0]);
    acc_poly = aie::accum<accfloat, vec_size>(vcoeff_elu);
    acc_poly = aie::mac(acc_poly, r, vacc);
    aie::vector<bfloat16, vec_size> exp_x = acc_poly.to_vector<bfloat16>();

    /* Step 3: 2**m * exp_x 
    = b = a + c.view(np.int32) */
    aie::vector<int32_t, vec_size> a = aie::upshift(floor,23);//7+16
    aie::vector<int16_t, vec_size> c16 = *(aie::vector<int16, vec_size>*)(&exp_x);
    aie::accum<acc32, vec_size>  acc1(c16);
    aie::vector<int32_t, vec_size> c32 = acc1.to_vector<int32>();
    aie::vector<int32_t, vec_size> c = aie::upshift(c32,16);
    aie::vector<int32_t, vec_size> b = aie::add(a,c);
    aie::vector<int32_t, vec_size> p = aie::downshift(b,16);
    aie::accum<acc32, vec_size>  acc2(p);
    aie::vector<int16_t, vec_size> p_16 = acc2.to_vector<int16>();

    aie::vector<int16,vec_size> * tmp = &p_16;
    aie::vector<bfloat16,vec_size> * tmp2=(aie::vector<bfloat16,vec_size>*)tmp; 
    aie::vector<bfloat16,vec_size> exp_vec = *tmp2;
    aie::vector<bfloat16,vec_size>  expm1_result = aie::sub(exp_vec, (bfloat16)1.f);
    // aie::vector<bfloat16,vec_size>  expm1_result = clip_vec;

    //----------------------------------------------------------------------------------//
    // table looking for [-3.7848949432373047e-06， -4.470348358154297e-07), index (13551, 13949]
    //----------------------------------------------------------------------------------//
    
    // define table, margin, and masks
    aie::lut<4, uint16_t, uint16_t> lut_f(256, expm1_table_ab, expm1_table_cd);
    aie::parallel_lookup<int16_t, aie::lut<4, uint16_t, uint16_t>, aie::lut_oor_policy::truncate>
        lookup_fraction(lut_f);

    int16_t index_mask = 0x7fff;
    bfloat16 seg_lut_min = -3.7848949432373047e-06;
    bfloat16 lut_margin = -4.470348358154297e-07; //index = 13551
    aie::mask<vec_size> is_large_lut = aie::ge(in_vec, seg_lut_min);

    // get the start point index value
    aie::vector<bfloat16, vec_size> vlut_margin = aie::broadcast<bfloat16, vec_size>(lut_margin);
    aie::vector<int16_t, vec_size> vlut_margin_int = vlut_margin.template cast_to<int16_t>(); 
    aie::vector<int16_t, vec_size> start_index = aie::bit_and(index_mask, vlut_margin_int);

    // get the input vector's index value
    aie::vector<int16_t, vec_size> lut_x_int = clip_vec.template cast_to<int16_t>(); 
    aie::vector<int16_t, vec_size> this_index = aie::bit_and(index_mask, lut_x_int);

    // sub the two index to get the real index for lut, clip index to 0~511
    aie::vector<int16_t, vec_size> index_int16 = aie::sub(this_index, start_index);
    index_int16 = aie::sub(index_int16, (int16_t)1);
    aie::vector<int16_t, vec_size> index_int16_clip = aie::max(aie::min(index_int16, (int16_t)511), (int16_t)0);
    // print(index_int16_clip, true, "\n index_int16 = ");

    // Because lookup_table only support unint16 return rather not int8, But we only need int8 return.
    // For saving storage, we combine index n and index n + 1 together, returning ((int8 << 8) | int8).
    // And we split result into int8 later
    // So now we will use index_int16_clip, ranged in 0-511, to get the lut index for double int8, 
    // index_int16_clip // 2 and index_int16_clip // 2 + 1
    // aie::downshift has a carry feature, but we dont need it
    vector<int16_t, vec_size> index_int16_clip_for_downshift = aie::bit_and((int16_t)0xFFFE, index_int16_clip);
    vector<int16_t, vec_size> index_int16_downshift_remainder = aie::bit_and((int16_t)0x0001, index_int16_clip);
    vector<int16_t, vec_size> real_index_int8 = aie::downshift(index_int16_clip_for_downshift, 1);
    mask<vec_size> is_downshift_0 = aie::eq((int16_t)0, index_int16_downshift_remainder);

    aie::vector<uint16_t, vec_size> lut_result_combine = lookup_fraction.fetch(real_index_int8);
    aie::vector<uint8_t, vec_size> lut_result_combine_0 = aie::filter_even(lut_result_combine.cast_to<uint8_t>());
    aie::vector<uint8_t, vec_size> lut_result_combine_1 = aie::filter_odd(lut_result_combine.cast_to<uint8_t>());
    aie::vector<uint8_t, vec_size> lut_result_int8 = aie::select(lut_result_combine_0, lut_result_combine_1, is_downshift_0);

    // lut_bit_result stands for the bfloat16_result 's 0x01FE bit true values,
    // need to restore the 0xFE00 position vlaues by input's corresponding position values.
    aie::accum<acc32, vec_size> lut_result_acc;
    aie::vector<uint16_t, vec_size> lut_result_aligned;
    aie::vector<uint16_t, vec_size> lut_x_aligned;

    lut_result_acc.from_vector(lut_result_int8);
    aie::vector<uint16_t, vec_size> lut_result_int16 = lut_result_acc.to_vector<uint16_t>();
    lut_result_aligned = aie::upshift(lut_result_int16, 1);
    lut_result_aligned = aie::bit_and((uint16_t)0x01FE, lut_result_aligned);
    lut_x_aligned = aie::bit_and((uint16_t)0xFE00, lut_x_int.cast_to<uint16_t>());
    aie::vector<uint16_t, vec_size> lut_final_result_int16 = aie::bit_or(lut_x_aligned, lut_result_aligned);

    aie::vector<bfloat16, vec_size> lut_final_result_bf16 = lut_final_result_int16.cast_to<bfloat16>();

    //----------------------------------------------------------------------------------//
    // hardcode for [-4.470348358154297e-07, 0), index (0, 13552]
    //----------------------------------------------------------------------------------//
    aie::mask<vec_size> is_large_s0 = aie::ge(in_vec, lut_margin);
    aie::mask<vec_size> is_large_s1 = aie::ge(in_vec, seg_s1);
    aie::mask<vec_size> is_large_s2 = aie::ge(in_vec, seg_s2);
    aie::mask<vec_size> is_large_s3 = aie::ge(in_vec, seg_s3);
    aie::mask<vec_size> is_large_s4 = aie::ge(in_vec, seg_s4);
    aie::mask<vec_size> is_large_s5 = aie::ge(in_vec, seg_s5);
    aie::mask<vec_size> is_large_s6 = aie::ge(in_vec, seg_s6);
    aie::mask<vec_size> is_large_s7 = aie::ge(in_vec, seg_s7);

    // //-----------------------------------------------------------------------------------//
    // // taylor-elu for [-0.69921875, -3.7848949432373047e-06)
    // //-----------------------------------------------------------------------------------//
    
    aie::accum<accfloat, vec_size> acc_x_seg, acc_x2_seg, acc_x3_seg;
    aie::accum<accfloat, vec_size> acc_mac = aie::zeros<accfloat, vec_size>();
    aie::accum<accfloat, vec_size> acc_mac_s0, acc_mac_s1;

    // // pre-process: segmentation params
    aie::mask<vec_size> is_s0 = aie::ge(in_vec, segment_0); 
    aie::mask<vec_size> is_large_segment_1 = aie::ge(in_vec, segment_1); 

    aie::vector<bfloat16, vec_size> segment = aie::select(segment_0, (bfloat16)0.0, is_s0);
    aie::vector<bfloat16, vec_size> Q3 = aie::select(Q3_s1, Q3_s0, is_s0);
    aie::vector<bfloat16, vec_size> Q2 = aie::select(Q2_s1, Q2_s0, is_s0);
    aie::vector<bfloat16, vec_size> Q1_1 = aie::select(Q1_s11, Q1_s01, is_s0);
    aie::vector<bfloat16, vec_size> Q1_0 = aie::select(Q1_s10, Q1_s00, is_s0);
    // aie::vector<float, vec_size> Q0 = aie::select(Q0_s0, Q0_s1, is_s0);

    // pre-process: segmentation pow-x & segmention mul
    aie::vector<bfloat16, vec_size> vec_x_seg = aie::sub(in_vec, segment);
    acc_x2_seg = aie::mul_square(vec_x_seg);
    acc_x3_seg = aie::mul(acc_x2_seg.template to_vector<bfloat16>(), vec_x_seg);

    // talyor
    acc_mac = aie::mac(acc_mac, Q1_0, in_vec);
    acc_mac = aie::mac(acc_mac, Q1_1, in_vec);
    acc_mac = aie::mac(acc_mac, Q2, acc_x2_seg.template to_vector<bfloat16>());
    acc_mac = aie::mac(acc_mac, Q3, acc_x3_seg.template to_vector<bfloat16>());
    acc_mac_s1 = aie::add(acc_mac, (float)(-0.069684600496239));

    // post-process: select on fraction result
    aie::vector<bfloat16, vec_size> vec_mac_s0 = acc_mac.to_vector<bfloat16>();
    aie::vector<bfloat16, vec_size> vec_mac_s1 = acc_mac_s1.to_vector<bfloat16>();
    aie::vector<bfloat16, vec_size> taylor_result = aie::select(vec_mac_s1, vec_mac_s0, is_s0); 

    // //-----------------------------------------------------------------------------------//
    // // For int8 lut, there are 3 special cases, expm1_values = -1.9073486328125e-06
    // //-----------------------------------------------------------------------------------//

    bfloat16 special_value = -1.9073486328125e-06;
    uint16_t special_0_int = 46589;  // -1.8849968910217285e-06; 
    uint16_t special_1_int = 46590;  // -1.8924474716186523e-06
    uint16_t special_2_int = 46591;  // -1.8998980522155762e-06;
    aie::mask<vec_size> is_special_0 = aie::eq(lut_x_int.cast_to<uint16_t>(), special_0_int);
    aie::mask<vec_size> is_special_1 = aie::eq(lut_x_int.cast_to<uint16_t>(), special_1_int);
    aie::mask<vec_size> is_special_2 = aie::eq(lut_x_int.cast_to<uint16_t>(), special_2_int);

    // //-----------------------------------------------------------------------------------//
    //  select which seg to use
    // //-----------------------------------------------------------------------------------//

    expm1_result = aie::select(expm1_result, taylor_result,                is_large_segment_1);
    expm1_result = aie::select(expm1_result, lut_final_result_bf16,              is_large_lut);
    expm1_result = aie::select(expm1_result, value_seg_s0,                        is_large_s0);
    expm1_result = aie::select(expm1_result, value_seg_s1,                        is_large_s1);
    expm1_result = aie::select(expm1_result, value_seg_s2,                        is_large_s2);
    expm1_result = aie::select(expm1_result, value_seg_s3,                        is_large_s3);
    expm1_result = aie::select(expm1_result, value_seg_s4,                        is_large_s4);
    expm1_result = aie::select(expm1_result, value_seg_s5,                        is_large_s5);
    expm1_result = aie::select(expm1_result, value_seg_s6,                        is_large_s6);
    expm1_result = aie::select(expm1_result, value_seg_s7,                        is_large_s7);
    expm1_result = aie::select(expm1_result, special_value,                      is_special_0);
    expm1_result = aie::select(expm1_result, special_value,                      is_special_1);
    expm1_result = aie::select(expm1_result, special_value,                      is_special_2);
    
    // //-----------------------------------------------------------------------------------//
    // // make neg/pos part mask
    // //-----------------------------------------------------------------------------------//

    // alpha_result
    aie::accum<accfloat, vec_size> alpha_result = aie::mul(alpha, expm1_result);

    // final result
    x_mask = aie::le(in_vec, (bfloat16)0.0);
    aie::vector<bfloat16, vec_size> final_result = aie::select(in_vec, alpha_result.to_vector<bfloat16>(), x_mask);

    out_vec = final_result;
  }
};

// implementation of elu<int8_t>
template <> class elementwise_elu<int8_t> : elementwise_unary_base<int8_t, elu_params_t<int8_t>> {
private:
  float coeff_elu[4] = {1.0f, 1.0f, 0.486328125f, 0.21875f};
  static constexpr bfloat16 exponent_min = FLOAT_TO_BFLOAT16(float(-87.0));
  static constexpr bfloat16 exponent_max = FLOAT_TO_BFLOAT16(float(87.0));

public:
  static constexpr bool peel = elu_config<int8_t>::peel;
  static constexpr uint8_t unroll = elu_config<int8_t>::unroll;
  static constexpr uint16_t minimum_num_iter = elu_config<int8_t>::minimum_num_iter;
  static constexpr uint16_t vec_size = elu_config<int8_t>::vec_size;

  using acc_t = acc32;
  using vec_t = aie::vector<int8_t, vec_size>;
  using params_t = elementwise_unary_params_t<elu_params_t<int8_t>>;

  aie::mask<vec_size> x_mask; // false when  f(x) = alpha*(exp_x-1), true f(x) = x
  bfloat16 alpha;
  int input_shift;
  int output_shift;

  elementwise_elu(const params_t &op_params) {
      alpha = op_params.op_params.alpha;
  }

  // setup auxiliary member variable if needed
  __attribute__((always_inline)) static void setup(params_t &op_params, const void *rtps) {
      const uint16_t *rtp_uint16 = (const uint16_t *)(rtps);
      op_params.op_params.alpha = *(bfloat16*)&rtp_uint16[16];
  }

  inline __attribute__((always_inline)) void body(const vec_t &in_vec, vec_t &out_vec, params_t &op_params) {

    int shift_rd = op_params.ifm_shift;
    int shift_wt = op_params.ofm_shift;

    aie::vector<bfloat16, vec_size> x = int8_to_bfloat16(in_vec, shift_rd);

    aie::vector<bfloat16, vec_size> x_tmp = x;
    x = aie::max(x, exponent_min);
    x = aie::min(x, exponent_max);

    aie::accum<accfloat, vec_size> acc (x);

    /* Step 1 : scaling
    # scaling_factor = x * 1.4426950408889634
    # n = scaling_factor
    # m = scaling_factor
    # j = n - m = 0
    # r = x - n*0.6931471805599453 */
    aie::vector<bfloat16, vec_size> sf_n_m = aie::mul((bfloat16)1.4453125f, x).to_vector<bfloat16>();
    aie::vector<int32_t, vec_size> floor = aie::to_fixed<int32_t>(sf_n_m, 0);

    auto floor_tof = aie::to_float(floor,0);
    aie::vector<bfloat16, vec_size> temp2  = aie::mul(floor_tof,(float)0.69140625f).to_vector<bfloat16>(); 
    aie::vector<bfloat16, vec_size> r      = aie::sub(acc, temp2).to_vector<bfloat16>();

    /* Step 2 : approx poly(r)
    y = c0 + x(c1 + x(c2 + x(c3)))
    */
    aie::vector<bfloat16, vec_size> vacc = aie::broadcast<bfloat16, vec_size>(coeff_elu[2]);
    aie::accum<accfloat, vec_size> acc_poly(vacc);  
    aie::vector<bfloat16, vec_size> vcoeff_elu = aie::broadcast<bfloat16, vec_size>(coeff_elu[3]);
    acc_poly = aie::mac(acc_poly, r, vcoeff_elu);
    vacc = acc_poly.to_vector<bfloat16>();
    vcoeff_elu = aie::broadcast<bfloat16, vec_size>(coeff_elu[1]);
    acc_poly = aie::accum<accfloat, vec_size>(vcoeff_elu);
    acc_poly = aie::mac(acc_poly, r, vacc);
    vacc = acc_poly.to_vector<bfloat16>();
    vcoeff_elu = aie::broadcast<bfloat16, vec_size>(coeff_elu[0]);
    acc_poly = aie::accum<accfloat, vec_size>(vcoeff_elu);
    acc_poly = aie::mac(acc_poly, r, vacc);
    aie::vector<bfloat16, vec_size> exp_x = acc_poly.to_vector<bfloat16>();

    /* Step 3: 2**m * exp_x 
    = b = a + c.view(np.int32) */
    aie::vector<int32_t, vec_size> a = aie::upshift(floor,23);//7+16
    aie::vector<int16_t, vec_size> c16 = *(aie::vector<int16, vec_size>*)(&exp_x);
    aie::accum<acc32, vec_size>  acc1(c16);
    aie::vector<int32_t, vec_size> c32 = acc1.to_vector<int32>();
    aie::vector<int32_t, vec_size> c = aie::upshift(c32,16);
    aie::vector<int32_t, vec_size> b = aie::add(a,c);
    aie::vector<int32_t, vec_size> p = aie::downshift(b,16);
    aie::accum<acc32, vec_size>  acc2(p);
    aie::vector<int16_t, vec_size> p_16 = acc2.to_vector<int16>();

    aie::vector<int16,vec_size> * tmp = &p_16;
    aie::vector<bfloat16,vec_size> * tmp2=(aie::vector<bfloat16,vec_size>*)tmp; 
    aie::vector<bfloat16,vec_size> exp_vec = *tmp2;
    aie::vector<bfloat16,vec_size>  v_result = aie::sub(exp_vec, (bfloat16)1.f);
    auto alpha_result = aie::mul(alpha, v_result);
    aie::vector<bfloat16,vec_size> v_result_f16 = alpha_result.to_vector<bfloat16>();

    aie::mask<vec_size> cmp_msk;
    cmp_msk = aie::le(in_vec, aie::zeros<int8_t, vec_size>());    
    
    aie::vector<int8_t, vec_size> pos_x_out = bfloat16_to_int8(x_tmp, shift_wt);
    aie::vector<int8_t, vec_size> v_result_s8 = bfloat16_to_int8(v_result_f16, shift_wt);
    aie::vector<int8, vec_size> final_result = aie::select(pos_x_out, v_result_s8, cmp_msk);

    out_vec = final_result;
}

private:
  // helper function
  aie::vector<bfloat16, vec_size> __attribute__((always_inline))
    int8_to_bfloat16(const aie::vector<int8, vec_size>& x, const int shift_in) {
    aie::accum<accfloat, vec_size> acc_x = from_vector<accfloat>(aie::to_float(x,shift_in));
    return acc_x.template to_vector<bfloat16>();
    }

  aie::vector<int8_t, vec_size> __attribute__((always_inline))
    bfloat16_to_int8(const aie::vector<bfloat16, vec_size>& x, const int shift_out) {
    vector<int32_t, vec_size> exponent_int = aie::to_fixed<int32_t>(x, shift_out);
    aie::accum<acc32, vec_size> acc_x2(exponent_int);
    return acc_x2.template to_vector<int8_t>();
    }

};