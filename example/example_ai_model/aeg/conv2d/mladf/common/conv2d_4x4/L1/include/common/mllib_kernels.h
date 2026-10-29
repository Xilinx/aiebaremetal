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

#ifndef __MLLIB_KERNELS_H__
#define __MLLIB_KERNELS_H__

// Helper and common sources from L1
#include "../../include/common/mllib_x86sim_workarounds.h"
#include "../../include/common/layer_params.h"
#include "../../include/common/mllib_const.h"
#include "../../include/common/utils.h"

// Kernel sources from L1
// Portable kernels, kernels that support AIE2/AIE2P/AIE2PS
//#include "../../include/misc/pad_3d.h"
//#include "../../include/misc/pad_3d_params.h"
#include "../../include/conv/conv1d_dw_params.h"
#include "../../include/conv/conv1d_dw.h"
#ifndef CONV2D_STX_OPT
#include "../../include/conv/conv2d.h"
#include "../../include/conv/conv2d_params.h"
#else
#include "../../include/conv/conv2d_stx_opt.h"
#include "../../include/conv/conv2d_params_stx_opt.h"
#endif
#ifndef CONV2D_A16W16
  #ifndef CONV2D_1x8x8_KERNEL
    #include "../../include/conv/conv2d_opt.h"
  #endif
#endif
#include "../../include/conv/conv2d_dw_params.h"
#include "../../include/conv/conv2d_dw.h"
//#include "../../include/conv/conv2d_dw_opt_params.h"
//#include "../../include/conv/conv2d_dw_opt.h"
#include "../../include/conv/conv2d_dw_bf16_params.h"
#include "../../include/conv/conv2d_dw_bf16.h"
#include "../../include/conv/conv2d_dw_bf16_casc.h"
#include "../../include/conv/conv2d_transpose.h"
#include "../../include/conv/conv2d_transpose_params.h"
#include "../../include/conv/conv2d_transpose_bf16.h"
#include "../../include/conv/conv2d_transpose_bf16_params.h"
#include "../../include/conv/conv2d_bf16.h"
#include "../../include/conv/conv2d_bf16_params.h"
#include "../../include/misc/maxpool2d.h"
#include "../../include/misc/maxpool2d_params.h"
#include "../../include/misc/avgpool2d.h"
#include "../../include/misc/avgpool2d_params.h"
#include "../../include/misc/pad2d.h"
#include "../../include/misc/pad2d_params.h"
#include "../../include/misc/pixelshuffle.h"
#include "../../include/misc/pixelshuffle_params.h"
#include "../../include/misc/add2d.h"
#include "../../include/misc/add2d_params.h"
#include "../../include/misc/clip.h"
#include "../../include/misc/clip_params.h"
#include "../../include/misc/tile.h"
#include "../../include/misc/tile_params.h"
#include "../../include/misc/globalavgpool2d.h"
#include "../../include/misc/globalavgpool2d_params.h"
#include "../../include/misc/globalavgpool2d_opt.h"
#include "../../include/misc/globalavgpool2d_opt_params.h"
#include "../../include/misc/fullyconnect.h"
#include "../../include/misc/fullyconnect_params.h"
#include "../../include/misc/min.h"
#include "../../include/misc/max.h"
#include "../../include/gemm/gemm.h"
#include "../../include/gemm/gemm_params.h"
#include "../../include/misc/mul2d.h"
#include "../../include/misc/mul2d_params.h"
#include "../../include/misc/div_params.h"
#include "../../include/misc/div.h"
#include "../../include/misc/reducemin1d_params.h"
#include "../../include/misc/reducemin1d.h"
#include "../../include/misc/reducemin_params.h"
#include "../../include/misc/reducemin.h"
#include "../../include/misc/reducemax_params.h"
#include "../../include/misc/reducemax.h"
#include "../../include/misc/reduceprod_params.h"
#include "../../include/misc/reduceprod.h"
#include "../../include/misc/reducesum_params.h"
#include "../../include/misc/reducesum.h"
#include "../../include/misc/globalmaxpool2d_params.h"
#include "../../include/misc/globalmaxpool2d.h"
#include "../../include/misc/hardsigmoid.h"
#include "../../include/misc/hardsigmoid_params.h"
#include "../../include/misc/sigmoid.h"
#include "../../include/misc/sigmoid_params.h"
#include "../../include/misc/silu.h"
#include "../../include/misc/silu_params.h"
#include "../../include/misc/sin.h"
#include "../../include/misc/sin_params.h"
#include "../../include/misc/sign.h"
#include "../../include/misc/sign_params.h"
#include "../../include/misc/sqrt.h"
#include "../../include/misc/sqrt_params.h"
#include "../../include/misc/sub.h"
#include "../../include/misc/sub_params.h"
#include "../../include/misc/add.h"
#include "../../include/misc/add_params.h"
#include "../../include/misc/gelu.h"
#include "../../include/misc/gelu_params.h"
#include "../../include/misc/tanh.h"
#include "../../include/misc/tanh_params.h"
#include "../../include/misc/casc_read_write.h"
#include "../../include/misc/resize_bilinear.h"
#include "../../include/misc/resize_bilinear_params.h"
#include "../../include/misc/resize_nearest.h"
#include "../../include/misc/resize_nearest_params.h"
#include "../../include/misc/channelsfirstflatten.h"
#include "../../include/misc/channelsfirstflatten_params.h"
#include "../../include/misc/layer_norm.h"
#include "../../include/misc/layer_norm_params.h"
#include "../../include/misc/layer_norm_c8.h"
#include "../../include/misc/layer_norm_c8_casc.h"
#include "../../include/misc/layer_norm_c8_params.h"
#include "../../include/misc/abs.h"
#include "../../include/misc/abs_params.h"
#include "../../include/misc/log.h"
#include "../../include/misc/log_params.h"
#include "../../include/misc/requantize.h"
#include "../../include/misc/requantize_params.h"
#include "../../include/misc/pow.h"
#include "../../include/misc/pow_params.h"
#include "../../include/misc/interpolate_linear_1d.h"
#include "../../include/misc/interpolate_linear_1d_params.h"
#include "../../include/misc/squeeze.h"
#include "../../include/misc/squeeze_params.h"
#include "../../include/misc/transpose.h"
#include "../../include/misc/transpose_params.h"
#include "../../include/misc/thresholdedrelu.h"
#include "../../include/misc/degroupg8.h"
#include "../../include/misc/degroupg8_params.h"
#include "../../include/misc/groupg8.h"
#include "../../include/misc/groupg8_params.h"
#include "../../include/misc/degroupg4.h"
#include "../../include/misc/degroupg4_params.h"
#include "../../include/misc/groupg4.h"
#include "../../include/misc/groupg4_params.h"
#include "../../include/misc/neg.h"
#include "../../include/misc/neg_params.h"
#include "../../include/misc/argmax1d.h"
#include "../../include/misc/argmax1d_params.h"
#include "../../include/misc/argmin1d.h"
#include "../../include/misc/argmin1d_params.h"
#include "../../include/misc/pixelunshuffle_params.h"
#include "../../include/misc/pixelunshuffle.h"
#include "../../include/misc/gridsample2d_bilinear.h"
#include "../../include/misc/gridsample2d_bilinear_params.h"
#include "../../include/misc/leakyrelu.h"
#include "../../include/misc/leakyrelu_params.h"
#include "../../include/misc/rescale.h"
#include "../../include/misc/rescale_params.h"
#include "../../include/misc/bitwise_and.h"
#include "../../include/misc/bitwise_and_params.h"
#include "../../include/misc/bitwise_or.h"
#include "../../include/misc/bitwise_or_params.h"
#include "../../include/misc/bitwise_xor.h"
#include "../../include/misc/bitwise_xor_params.h"
#include "../../include/misc/erf.h"
#include "../../include/misc/erf_params.h"
#include "../../include/misc/floor.h"
#include "../../include/misc/floor_params.h"
#include "../../include/misc/slice.h"
#include "../../include/misc/slice_params.h"
#include "../../include/misc/hardswish.h"
#include "../../include/misc/hardswish_params.h"
#include "../../include/misc/rsqrt.h"
#include "../../include/misc/rsqrt_params.h"
#include "../../include/misc/mul.h"
#include "../../include/misc/mul_params.h"
#include "../../include/misc/mod.h"
#include "../../include/misc/mod_params.h"
#include "../../include/misc/matmulc8.h"
#include "../../include/misc/softmax_params.h"
#include "../../include/misc/softmax.h"
#include "../../include/misc/elu.h"
#include "../../include/misc/elu_params.h"
#include "../../include/misc/compare_ops.h"
#include "../../include/misc/compare_ops_params.h"
#include "../../include/misc/ceil.h"
#include "../../include/misc/ceil_params.h"
#include "../../include/misc/select_params.h"
#include "../../include/misc/select.h"
#include "../../include/misc/logical_not.h"
#include "../../include/misc/logical_not_params.h"
#include "../../include/misc/logical_xor.h"
#include "../../include/misc/logical_xor_params.h"
#include "../../include/misc/bitwise_not.h"
#include "../../include/misc/bitwise_not_params.h"
#include "../../include/misc/exponent.h"
#include "../../include/misc/range_params.h"
#include "../../include/misc/range.h"
#include "../../include/misc/batchnorm2d.h"
#include "../../include/misc/mish.h"
#include "../../include/misc/bit_shift.h"
#include "../../include/misc/bit_shift_params.h"
#include "../../include/misc/reciprocal.h"
#include "../../include/misc/reciprocal_params.h"
#include "../../include/misc/reducemean_templated.h"
#include "../../include/misc/reducemean_templated_params.h"
#include "../../include/misc/reduce_mean_c8.h"
#include "../../include/misc/reduce_mean_c8_params.h"
#include "../../include/misc/reduce_sum_c8.h"
#include "../../include/misc/reduce_sum_c8_params.h"
#include "../../include/misc/reduce_prod_c8.h"
#include "../../include/misc/reduce_prod_c8_params.h"
#include "../../include/misc/cast.h"
#include "../../include/misc/cast_params.h"
#include "../../include/misc/scale_add.h"
#include "../../include/misc/scale_add_params.h"
#include "../../include/misc/round.h"
#include "../../include/misc/round_params.h"
#include "../../include/misc/shrink.h"
#include "../../include/misc/shrink_params.h"
#include "../../include/misc/avgpool2dvariant.h"
#include "../../include/misc/avgpool2dvariant_params.h"
#include "../../include/misc/instance_norm.h"
#include "../../include/misc/instance_norm_params.h"

// For kernels that only support AIE2
#if __AIE_ARCH__ == 20
// TODO: remove implicit dependence on conv2d before conv2d_casc by moving MACROs to utils
#include "../../include/conv/conv2d_casc.h"
#include "../../include/misc/gemv.h"
#include "../../include/misc/gemv_params.h"
#include "../../include/misc/add2d_casc.h"
#include "../../include/misc/bilinear_interpolation.h"
#include "../../include/misc/bilinear_interpolation_params.h"
#include "../../include/gemm/gemm_4x4.h"
#include "../../include/gemm/gemm_4x4_params.h"
#include "../../include/misc/stft.h"
#include "../../include/conv/group_conv2d.h"
#include "../../include/conv/group_conv2d_params.h"
#include "../../include/conv/group_conv2d_bf16.h"
#include "../../include/conv/group_conv2d_bf16_params.h"
#endif

#if __AIE_ARCH__ == 21
#ifdef CONV2D_1x8x8_KERNEL
#include "../../include/conv/conv2d_1x8x8.h"
#include "../../include/conv/conv2d_1x8x8_params.h"
#endif
#endif

#endif //__MLLIB_KERNELS_H__
