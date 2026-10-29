// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*
(c) Copyright 2019 - 2021 Xilinx, Inc. All Rights reserved.
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

#ifndef __BD_CONFIG_PARAMETERS_H__
#define __BD_CONFIG_PARAMETERS_H__

#include <adf.h>

#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>

// conv
#define OCP 16
#define ODP 8
#define OWP 1
#define OHP 1
#define ICP 8
#define SHIFT_OCP 4
#define SHIFT_ODP 3
#define SHIFT_OWP 0
#define SHIFT_OHP 0
#define SHIFT_ICP 3
#define DIM_M 4
#define DIM_N 8
#define DIM_K 8

// qconv
#define M_gran 4
#define N_gran 8
#define M_tile 4
#define N_tile 8

// dwc
#define DWC_OCP 8
#define DWC_OHP 4
#define DWC_SHIFT_OCP 3
#define DWC_SHIFT_OWP 3
#define DWC_SHIFT_OHP 2

#ifndef PRELU_PARAM
#define PRELU_PARAM 204
#endif
#ifndef PRELU_SHIFT
#define PRELU_SHIFT 10
#endif


enum KernelType {
  ConvKernel = 0,
  DwcKernel = 1,
  ElewKernel = 2,
  PoolKernel = 3,
  SpopKernel = 4,
  ReshapeKernel = 5,
  SoftmaxKernel = 6,
  TransposeKernel = 7,
  ReduceKernel = 9,
  ConcatKernel = 10,
  ArgKernel = 11,
  PaddingKernel = 12,
  GlobalPoolKernel = 13,
  QConvKernel = 14,
  QPoolKernel = 15,
  InstanceNormalizationKernel = 16,
  QElewKernel = 17,
  QDwcKernel = 18,
  StrideSliceKernel = 19,
};

enum ReluType {
  NoRelu = 0,
  Relu = 1,
  Relu6 = 2,
  Leaky_Prelu = 4,
  Hsigmoid = 5,
  Hswish = 6,
};

struct Acc32_v {
  v32acc32 a;
  v32acc32 b;
  v32acc32 c;
  v32acc32 d;
};

static int8_t chess_storage(% chess_alignof(v32int8)) relu6_buf[32] = {
  96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96,
  96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96, 96
};
struct super_kernel_params {
  int32_t exec_type_1;
  int32_t exec_type_2;
  int32_t act_type_1;
  int32_t act_type_2;
  int32_t tile_dwc_ohg;
  int32_t tile_dwc_owg;
  int32_t tile_dwc_ocg;
  int32_t tile_owg;
  int32_t tile_odg;
  int32_t tile_ohg;
  int32_t tile_icg;
  int32_t tile_ocg;
  int32_t ker_h;
  int32_t ker_w;
  int32_t ker_d;
  int32_t ker_dw_w_ew_type;
  int32_t ker_dw_h_ecnt;
  int32_t str_h;
  int32_t str_w;
  int32_t str_d;
  int32_t str_dw_w;
  int32_t str_dw_h;
  int8_t  shift_wr;
  int32_t ifm_offset;
  int32_t ifm_wgt2_offset;
  int32_t wgt_offset;
  int32_t inner_ofm_offset;
  int32_t psum_offset_a;
  int32_t psum_offset_b;
  int32_t ofm_offset;
  int32_t iter_cnt;
  int32_t ddr_loop;
  int32_t ddr_mini_loop;
  int32_t loop_num;
  int32_t iter_wd;
  int32_t iter_oc;
  int32_t mt_diter;
  int32_t mt_witer;
  int32_t mt_citer;
  int32_t shift_bias_1;
  int32_t shift_out_1;
  int32_t shift_bias_2;
  int32_t shift_out_2;
  int32_t bias_offset_1;
  int32_t bias_offset_2;
  int32_t mode;
  int32_t mt_fusion_mode;
  int32_t prelu_in;
  int32_t shift_hsigmoid;
  int32_t prelu_shift;
  int32_t hsigmoid_in;
  int32_t shift_hswish;
  int32_t is_transpose;
  int32_t keep_count;
  int32_t keep_wgt;
  int32_t iter_cnt_in;
  int32_t iter_cnt_out;
  //used for padding cal
  int32_t conv_valid_tile_owg;
  int32_t conv_valid_tile_ohg;
  int32_t pad_top;
  int32_t pad_bottom;
  int32_t pad_left;
  int32_t pad_right;
  int32_t full_ofm_w;
  int32_t full_ofm_h;
  int32_t ddr2mt_ow_cnt;
  int32_t ddr2mt_oh_cnt;
  int32_t curr_loop_cnt;
  int32_t block_pad_top;
  int32_t block_pad_bottom;
  int32_t block_pad_left;
  int32_t block_pad_right;
  int32_t conv_ofm_c;
  int32_t conv_ofm_d;
  int32_t conv_ofm_w;
  int32_t conv_ofm_h;
  int32_t conv_ifm_c;
  int32_t step_align;
  // parallelism & loop
  int32_t conv_align_ker_d; // C8 -> ID(IW) -> IC/8-> (IW) -> IH, align ID to 4 to
  int32_t conv_ifm_d;
  int32_t conv_ifm_w;
  int32_t conv_ifm_h;
  int32_t dwc_ofm_c;
  int32_t dwc_ofm_w;
  int32_t dwc_ofm_h;
  int32_t dwc_valid_ofm_h;
  int32_t dwc_ifm_c;
  int32_t dwc_ifm_w;
  int32_t dwc_ifm_h;
  int32_t ker_w_align;
  
  int32_t incAI1;
  int32_t incAI2;
  int32_t numAL1;
  int32_t incAL1;
  int32_t numAL2;
  int32_t incAL2;
  int32_t numAL3;
  int32_t incAL3;
  int32_t incAL4;
  int32_t shfl;
  int32_t shfl_o;
  int32_t shft;
  int32_t numAO1;
  int32_t incAO1;
  int32_t numAO2;
  int32_t incAO2;
  int32_t incAO3;
  int32_t numALO1;
  int32_t incALO1;
  int32_t numALO2;
  int32_t incALO2;
  int32_t incALO3;
  int32_t numB;
  int32_t incB1;
  int32_t incB2;
  int32_t numCL1;
  int32_t incCL1;
  int32_t incCL2;
  int32_t numCO1;
  int32_t incCO1;
  // int32_t numCO2;
  int32_t incCO2;
  int32_t incCO3;
  int32_t incS0;
  int32_t incS1;
  int32_t incS2;
  int32_t numCS1;
  int32_t incCS1;
  int32_t numCS2;
  int32_t incCS2;
  int32_t incCS3;
  int32_t incCF0;
  int32_t loop_cnt_kw;
  int32_t inner_loop;
  int32_t outer_loop;
  int32_t io_mode;
  int32_t curr_iter_cnt;
  bool psum_in;
  bool psum_out;
  bool casc_in;
  int32_t dwc_incAI1;
  int32_t dwc_incAI2;
  int32_t dwc_incAO1;
  int32_t dwc_incAO2;
  int32_t dwc_numAL1;
  int32_t dwc_incAL1;
  int32_t dwc_numAL2;
  int32_t dwc_incAL2;
  int32_t dwc_incAL3;
  int32_t dwc_incBI;
  int32_t dwc_numB;
  int32_t dwc_incB1;
  int32_t dwc_incB2;
  int32_t dwc_numC;
  int32_t dwc_incC1;
  int32_t dwc_incC2;
  int32_t dwc_numALO1;
  int32_t dwc_incALO1;
  int32_t dwc_numALO2;
  int32_t dwc_incALO2;
  int32_t dwc_incALO3;
  int32_t dwc_inner_loop;
  int32_t dwc_outer_loop;
  int32_t enable_col_num;
};
typedef super_kernel_params conv3d_params_t;

struct qdq_conv_params {
  // used for qdq conv
  uint8 M_g;
  uint8 N_g;
  uint8 Y_g;
  uint8 str_w;
  uint8 reserved;
  int8 shift_Qb;
  int8 shift_Qout;
  uint8 wrap0;
  uint8 wrap1;
  uint8 wrap2;
  uint8 wrap3;
  uint16 step0;
  uint16 step1;
  uint16 step2;
  uint16 step3;
  uint16 step4;
  uint16 step_Mb;
  int8_t q_term_num;
};
typedef struct qdq_conv_params QDQParams;


struct __attribute__((__packed__)) BufferParam { // 22 bytes
  uint16_t ifm_offset;
  int16_t ifm_size;
  union {
    uint16_t data2_offset;
    uint16_t full_ifm_w;
    uint16_t coeff_offset;
  };
  union {

    int16_t data2_size;
    int16_t coeff_size;
  };
  union {
    uint16_t wgt_offset;
    uint16_t full_ifm_h;
  };
  int16_t wgt_size;
  uint16_t ofm_offset;
  int16_t ofm_size;
  union {
    uint16_t T_step1; // Setup for multi dimensional addressing. This might need
    uint16_t full_ofm_h;
  };
  // some tuning depending on needs.
  uint16_t T_step2 : 13;
  uint16_t T_mode : 1;
  uint16_t T_ifm_enable : 1;
  uint16_t T_ofm_enable : 1;
  union {
    uint8_t T_wrap0;
    uint8_t align_pad;
  };
  uint8_t T_wrap1;
};

struct __attribute__((__packed__)) ConvParam { // 11 bytes
  uint16_t psum_a_offset;
  uint16_t psum_b_offset;
  uint8_t step_ptr_ci; // Optimization to reduce parameter calculation cost ( ==
                       // step_ci / 32 )
  uint8_t step_ptr_co;
  uint16_t ker_w : 4;
  uint16_t ker_h : 4;
  uint16_t str_w : 4;
  uint16_t str_h : 4;
  int8_t shift_bias;
  int8_t shift_out;
  uint8_t act_type;
};

struct ReluParam { // 2 bytes
  uint8_t hsigmoid_in : 4;
  uint8_t shift_hswish : 4;
  uint8_t shift_hsigmoid : 8;
};

struct PadParam { // 4 bytes
  uint8_t bottom;
  uint8_t right;
  uint8_t top : 4;
  uint8_t left : 2;
  uint8_t spacial_split_w_ld : 4; // 1,2,4,8
  uint8_t spacial_split_h_ld : 4; // 1,2,4,8,16
};

struct DwcParam { // 6 bytes
  uint16_t ker_w : 4;
  uint16_t ker_h : 4;
  uint16_t str_w : 4;
  uint16_t str_h : 4;
  int8_t shift_bias;
  int8_t shift_cut;
  uint8_t act_type;
  uint8_t kernel_words;
};

struct ElewParam { // 6 bytes
  uint8_t type;
  uint8_t ecnt;
  int8_t shift_r0;
  union {
    int8_t shift_r1;
    int8_t clamp_min;
  };
  int8_t shift_wr;
  union {
    uint8_t relu;
    int8_t clamp_max;
  };
};

struct PoolParam { // 24 bytes (copy of current parameters)
  int8_t type;
  int8_t shf_wr;
  int16_t ofm_c;
  int16_t ifm_w;
  int16_t ifm_h;
  int16_t ofm_h;
  int16_t coeff;
  uint8_t kernel_w;
  uint8_t kernel_h;
  uint8_t stride_w;
  uint8_t stride_h;
  uint8_t pad_t;
  uint8_t pad_b;
  uint8_t pad_l;
  uint8_t pad_r;
  uint8_t col_div_c : 4;
  uint8_t row_div_c : 4;
  uint8_t col_div_w : 4;
  uint8_t row_div_w : 4;
  uint8_t col_div_h : 4;
  uint8_t row_div_h : 4;
  uint8_t ColNum : 4;
  uint8_t RowNum : 4;
};
typedef struct PoolParam pool_pmtr_t;

struct ReshapeParam { // 10 bytes (copy of current parameters)
  int16_t i_valid_bytes;
  int16_t i_total_bytes;
  int16_t o_valid_bytes;
  int16_t o_total_bytes;
  int16_t i_loop;
};
typedef struct ReshapeParam reshape_pmtr_t;

struct PaddingParam { //10 bytes
  int8_t pad_mode;
  int8_t pad_val;
  int16_t pad_valid;
  int8_t pad_left;
  int8_t pad_right;
  int8_t pad_top;
  int8_t pad_bot;
  int8_t pad_front;
  int8_t pad_back;
};
typedef struct PaddingParam padding_pmtr_t;

struct TransposeParam { // 20 bytes (copy of current parameters)
  int16_t ifm_h_valid_bytes;
  int16_t ifm_h_total_bytes;
  int16_t ifm_w_valid_bytes;
  int16_t ifm_w_total_bytes;
  int16_t ifm_c_valid_bytes;
  int16_t ifm_c_total_bytes;
  int16_t ofm_h_total_bytes;
  int16_t ofm_w_total_bytes;
  int16_t ofm_c_total_bytes;
  int16_t transpose_order;
  uint16_t transpose_dim1_size;
  uint16_t transpose_dim2_size;
};
typedef struct TransposeParam transpose_pmtr_t;

//typedef struct ReduceParam reduce_mean_pmtr_t;
struct SoftmaxParam { // 20 bytes (copy of current parameters)
  int32_t shift_hsigmoid_num_remain;
  int32_t hsigmoid_in_fix_point;
  int32_t act_type_tile_cnt;
  int32_t tile_len_loop_cnt;
  uint8_t ofm_type_mode;
  uint8_t ofm_fix_point;
  uint16_t lut_length;
};

struct concat_internal_pmtr {
  int16_t i_total_bytes3;
  int16_t i_total_bytes2;
  int16_t i_total_bytes1;
  int16_t i_total_bytes0;
  int16_t i_ifm_h;
  int16_t i_ifm_w;
  int16_t i_ifm_num;
  int16_t i_ifm_c;
};
typedef struct concat_internal_pmtr concat_pmtr_t;

#if (GLOBAL_POOL_CASE | QDQ_GLOBAL_POOL_CASE)
struct GlobalPoolParam { // 24 bytes (copy of current parameters)
  int16_t psum_loop;
  int16_t i_ifm_h;
  int16_t i_ifm_w;
  int16_t i_ifm_c;
  uint16_t i_ifm_coff;
  int8_t i_ifm_shift;
  uint8_t i_dim_offset : 4;
  uint8_t psum_offset : 4;
  uint32_t ifm_hw_total;
  uint32_t ifm_hw_mask;
  uint16_t ofm_ping_offset;
  uint16_t ofm_pong_offset;
};
typedef struct GlobalPoolParam global_pool_pmtr_t;
#endif

#if (INSTANCE_NORMALIZATION_CASE)
struct InstanceNormalizationParam { // 18 bytes (copy of current parameters)
  int16_t i_ifm_h;
  int16_t i_ifm_w;
  int16_t i_ifm_c;
  int8_t input_fix_point;
  int8_t scale_fix_point;
  int8_t bias_fix_point;
  int8_t output_fix_point;
  uint16_t scale_offset;
  uint16_t psum_size;
  uint8_t act_type;
  float epsilon;
};
typedef struct InstanceNormalizationParam instance_normalization_pmtr_t;
#endif

struct arg_internal_pmtr property(keep_in_registers) {// 8 bytes
  int16_t type;
  int16_t ofm_w;
  int16_t ofm_h;
  int16_t ofm_c;
};
typedef struct arg_internal_pmtr arg_pmtr_t;

enum KernelParamIterMode { // Used to control reuse of buffers like in some DWC
                           // cases
  KP_IterMode_Psum = 0,
  KP_IterMode_Ifm = 1,
  KP_IterMode_Wgt = 2
};

struct alignas(v8int32) KernelParam { // 64 bytes
  // Word 0
  uint8_t kernel_type;
  uint8_t core_sel : 5;
  uint8_t iter_mode : 2;
  uint8_t channel_mode : 1;
  uint16_t ofm_w; // TODO is 8 bit sufficient here? W: maybe, H: yes, due to
                  // kernel granularities
  // Word 1
  uint8_t ddr2mt_w;
  uint8_t ddr2mt_h;
  uint8_t ddr2mt_c;
  uint8_t ofm_h; // These 2 ofm parameters are used to compute the output size
                 // of the dwc kernel.
  // Word 2
  uint8_t mt2aie_w;
  uint8_t mt2aie_h;
  uint8_t mt2aie_c;
  uint8_t iter_inner;
  // Word 3
  
  uint8_t tile_owg; // Tile parameters with respect to the problem size of the
                // conv kernel. However, instead of the granularity, owg,
                // ocg and icg are given as tiles of 8.
  uint8_t tile_ohg;
  uint8_t tile_ocg;
  uint8_t tile_icg;

  // Word 4-9
  union {
    struct { // 24 bytes
      ConvParam conv;
      uint8_t kernel_type_2;
      uint16_t local_offset;
      union {
        DwcParam dwc;
        ElewParam elew;
      };
      union{
        PadParam pad;
        float elu_alpha;
        struct{
          bfloat16 epsilon;
          uint16 dirty_channels;
        };
      };
    };
    PoolParam pool;
    padding_pmtr_t padding;
    reshape_pmtr_t reshape;
    transpose_pmtr_t transpose;
    arg_pmtr_t argminmax;
    SoftmaxParam softmax;
    //reduce_mean_pmtr_t reduce_param;
    concat_pmtr_t concat;
#if (GLOBAL_POOL_CASE | QDQ_GLOBAL_POOL_CASE)
    global_pool_pmtr_t global_pool_param;
#endif

#if (INSTANCE_NORMALIZATION_CASE)
    instance_normalization_pmtr_t inst_norm_param;
#endif
  };
  // Word 10-15
  BufferParam bd;
  union {
    struct { // 2 bytes
      uint8_t prelu_in;
      uint8_t prelu_shift;
    };
    ReluParam rp;
    uint16_t norm_channels;
    // uint16_t full_ofm_h;
  };
};

template <typename T = int8>
#ifdef CONV2D_A16W8_QdQ_MODE
inline __attribute__((always_inline)) void
setup_conv2d_params(int8_t *params_ptr, conv3d_params_t &param, QDQParams &qdq_param) {
#else
inline __attribute__((always_inline)) void
setup_conv2d_params(int8_t *params_ptr, conv3d_params_t &param) {
#endif  
  
  struct KernelParam kp;
  aie::vector<int32,8> params = aie::load_v<8>((int32 *)(params_ptr));
  aie::store_v( (int32  __aie_dm_resource_a *) &kp, params);

  param.iter_cnt         = kp.iter_inner;
  param.curr_iter_cnt    = 0;
  int32_t outputDepth = kp.tile_ocg * 16;
  param.psum_offset_a    = kp.conv.psum_a_offset;
  #ifdef CONV2D_A16W8_QdQ_MODE
  param.wgt_offset       = 64 + outputDepth*4 + 16*4 + 2*outputDepth*4 + outputDepth*8;
  #else
  param.wgt_offset       = 64 + outputDepth*4;
  #endif
  
  const int batch = 1;
  constexpr unsigned bit_width = (sizeof(T) == sizeof(int8)) ? 1 : 2;

  int align_kx = (batch >= 4 ? 1 : 4 / batch);
  int oxg = kp.tile_owg;
  int oyg = kp.tile_ohg;
  int kx = kp.conv.ker_w;
  int ky = kp.conv.ker_h;
  
  int sx = (kp.conv.str_w == 4) ? 2 : kp.conv.str_w;
  int sy = kp.conv.str_h;
#if STRIDE3_CASE
  if (kp.conv.str_w == 3) {

    int step_xn = 96;      // 32 to 96
    int step_so = 32;      //(sx == 2 && batch < 4 ? 1 : 2) * step_xn;
    int step_si = step_xn; //(sx == 2 && batch >= 4 ? 2 : 1) * step_xn;
    int step_co = kp.conv.step_ptr_co * 32;
    int step_ci = kp.conv.step_ptr_ci * 32;
    int step_yo = step_co * kp.tile_ocg * 2;
    int step_yi = step_ci * kp.tile_icg;
    int incAI_next, incAI_reset;
    bool sv_opt = oxg * 4 == kp.conv.step_ptr_co;

    param.exec_type_1 = kp.kernel_type;

    param.conv_ofm_w = step_co / 8;
    param.conv_ofm_h = oyg;
    param.conv_ofm_c = kp.tile_ocg * 16;

    param.act_type_1 = kp.conv.act_type;
    param.tile_odg = kp.tile_owg;
    param.tile_owg = kp.tile_ohg;
    param.tile_icg = kp.tile_icg;
    param.tile_ocg = kp.tile_ocg;
    param.ker_w = kp.conv.ker_h;
    param.ker_d = kp.conv.ker_w;
    param.str_w = kp.conv.str_h;
    param.str_d = kp.conv.str_w;
    param.shift_bias_1 = kp.conv.shift_bias;
    param.shift_out_1 = kp.conv.shift_out;
    param.shift_hsigmoid = kp.rp.shift_hsigmoid;
    param.hsigmoid_in = kp.rp.hsigmoid_in;
    param.shift_hswish = kp.rp.shift_hswish;
    param.prelu_in = kp.prelu_in;
    param.prelu_shift = kp.prelu_shift;

    // int spacial_w_ld = get_spacial_mapping_w_ld( kp.pad.spacial_mapping );
    int spacial_w_ld = kp.pad.spacial_split_w_ld;
    bool even_padding = (kp.kernel_type_2 == DwcKernel) && (kp.ddr2mt_w == 1) &&
                        (spacial_w_ld == 0);
    bool skip_right = even_padding && (kp.pad.right >= 8) && !sv_opt;
    param.incAI1 = step_si;
    param.shfl = (sx == 2 && batch < 4 ? (batch == 1 ? T64_8x2_lo : T128_4x2_lo)
                                       : T512_1x2_lo);
    param.shfl_o = (kp.conv.str_w == 4 ? T64_8x2_lo : T512_1x2_lo);
    param.shft = 0; //(sx == 2 && batch < 4 ? param.shfl : T256_2x2_hi);
    incAI_next = step_si;
    incAI_reset = -step_si;
    param.incS0 = 0; //(sx == 2 && batch < 4 ? 0 : step_xn);

    param.numAL1 = kx - 1;
    param.incAL1 = incAI_reset + 8 * batch;
    // step_align is calculated differently when using fifo in conv
#if __AIE_ARCH__ >= 21
    param.step_align = 28 - clb(param.incAL1 ^ (param.incAL1 - 1));
#else
    param.step_align = (param.incAL1 & 15 ? 2 : (param.incAL1 & 31 ? 3 : 0));
#endif
    param.numAL2 = ky - 1;
    param.incAL2 = incAI_reset - 8 * batch * param.numAL1 + step_yi;
    param.incAL3 = incAI_reset - 8 * batch * param.numAL1 -
                   step_yi * param.numAL2 + step_ci;

    param.numAO1 = param.tile_ocg - 1;
    param.incAO1 = -step_yi;
    param.numAO2 = oxg * batch - 1 - skip_right;
    param.incAO2 = param.incAO1 + step_si; // param.incAO1 + 2 * step_si;
    param.incAO3 =
        step_yi * (sy - 1) - step_si * param.numAO2; // step_yi * (sy - 1) - 2 *
                                                     // step_si * param.numAO2;

    param.numCS1 = param.numAO1;
    param.incCS1 = step_co - step_so + 32; // step_co - step_so + step_xn;
    param.numCS2 = param.numAO2;
    param.incCS2 = step_co - step_yo + 32; // step_co - step_yo + step_xn;
    param.incCS3 =
        step_co + 32 - oxg * batch * step_so +
        2 * step_si * skip_right; // step_co + step_xn - oxg * batch * step_so +
                                  // 2 * step_si * skip_right;
    param.numB = param.tile_ocg - 1;
    param.incB1 = 0;
    param.incB2 = -kx * ky * param.tile_icg * param.tile_ocg * 128;

    param.inner_loop = ky * kx * param.tile_icg;
    param.outer_loop = oyg * (oxg - skip_right) * param.tile_ocg;
    
    #ifdef CONV2D_A16W8_QdQ_MODE
    // used for qdq conv
    qdq_param.M_g = ODP * kp.tile_owg / M_gran;
    qdq_param.N_g = OCP * kp.tile_ocg / N_gran;
    qdq_param.Y_g = kp.tile_ohg;   
    #endif 
    

  } else {
#endif
    int step_xn = 32;
    int step_so = (sx == 2 && batch < 4 ? 1 : 2) * step_xn;
    int step_si = (sx == 2 && batch >= 4 ? 2 : 1) * step_xn;
    int step_co = kp.conv.step_ptr_co * 32;
    int step_ci = kp.conv.step_ptr_ci * 32 * bit_width;
    int step_yo = step_co * kp.tile_ocg * 2;
    int step_yi = step_ci * kp.tile_icg;
    int incAI_next, incAI_reset;
    bool sv_opt = oxg * 4 == kp.conv.step_ptr_co;

    param.exec_type_1 = kp.kernel_type;

    param.conv_ofm_w = step_co / 8;
    param.conv_ofm_h = oyg;
    param.conv_ofm_c = kp.tile_ocg * 16;

    param.act_type_1 = kp.conv.act_type;
    param.tile_odg = kp.tile_owg;
    param.tile_owg = kp.tile_ohg;
    param.tile_icg = kp.tile_icg;
    param.tile_ocg = kp.tile_ocg;
    param.ker_w = kp.conv.ker_h;
    param.ker_d = kp.conv.ker_w;
    param.str_w = kp.conv.str_h;
    param.str_d = kp.conv.str_w;
    param.shift_bias_1 = kp.conv.shift_bias;
    param.shift_out_1 = kp.conv.shift_out;
    param.shift_hsigmoid = kp.rp.shift_hsigmoid;
    param.hsigmoid_in = kp.rp.hsigmoid_in;
    param.shift_hswish = kp.rp.shift_hswish;
    param.prelu_in = PRELU_PARAM; //kp.prelu_in;
    param.prelu_shift = PRELU_SHIFT;// kp.prelu_shift;

    // int spacial_w_ld = get_spacial_mapping_w_ld( kp.pad.spacial_mapping );
    int spacial_w_ld = kp.pad.spacial_split_w_ld;
    bool even_padding = (kp.kernel_type_2 == DwcKernel) && (kp.ddr2mt_w == 1) &&
                        (spacial_w_ld == 0);
    bool skip_right = even_padding && (kp.pad.right >= 8) && !sv_opt;
    param.incAI1 = step_si;
    param.shfl = (sx == 2 && batch < 4 ? (batch == 1 ? 8 : 16) : 64 / bit_width);
    param.shfl_o = (kp.conv.str_w == 4 ? T64_8x2_lo : T512_1x2_lo);
    param.shft = (sx == 2 && batch < 4 ? param.shfl : T256_2x2_hi);
    incAI_next = step_si;
    incAI_reset = -step_si;
    param.incS0 = (sx == 2 && batch < 4 ? 0 : step_xn);

    param.numAL1 = kx - 1;
    param.incAL1 = incAI_reset + 8 * batch * bit_width;
    // step_align is calculated differently when using fifo in conv
#if __AIE_ARCH__ >= 21
    param.step_align = 28 - clb(param.incAL1 ^ (param.incAL1 - 1));
#else
  param.step_align = (param.incAL1 & 15 ? 2 : (param.incAL1 & 31 ? 3 : 0));
#endif

    param.numAL2 = ky - 1;
    param.incAL2 = incAI_reset - 8 * batch * param.numAL1 * bit_width + step_yi;
    param.incAL3 = incAI_reset - 8 * batch * param.numAL1 * bit_width -
                   step_yi * param.numAL2 + step_ci;

    param.numAO1 = param.tile_ocg - 1;
    param.incAO1 = -step_yi;
    param.numAO2 = oxg * batch - 1 - skip_right;
    param.incAO2 = param.incAO1 + 2 * step_si;
    param.incAO3 = step_yi * (sy - 1) - 2 * step_si * param.numAO2;

    param.numCS1 = param.numAO1;
    param.incCS1 = step_co - step_so + step_xn;
    param.numCS2 = param.numAO2;
    param.incCS2 = step_co - step_yo + step_xn;
    param.incCS3 =
        step_co + step_xn - oxg * batch * step_so + 2 * step_si * skip_right;

    param.numB = param.tile_ocg - 1;
    param.incB1 = 0;
    param.incB2 = -kx * ky * param.tile_icg * param.tile_ocg * 128;

    param.inner_loop = ky * kx * param.tile_icg;
    param.outer_loop = oyg * (oxg - skip_right) * param.tile_ocg;

    #ifdef CONV2D_A16W8_QdQ_MODE
    // used for qdq conv
    int str_div = (param.str_w == 1) ? 1 : 2;
    qdq_param.M_g = ODP / str_div * kp.tile_owg / M_gran /bit_width;
    qdq_param.N_g = OCP * kp.tile_ocg / N_gran;
    qdq_param.Y_g = kp.tile_ohg;
    qdq_param.str_w = param.str_d;
    #endif 
        
#if STRIDE3_CASE
  }
#endif

}
#endif
