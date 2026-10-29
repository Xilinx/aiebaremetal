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

#ifndef __GLOBALAVGPOOL2D_H__
#define __GLOBALAVGPOOL2D_H__

#include "aie.hpp"
#include <adf.h>

#include "globalavgpool2d_params.h"

#include "mllib_const.h"
#include "utils.h"
#include "utils.hpp"

using namespace aie;

template <uint8_t batch_size,
          casc_mode_t casc_mode,
          typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, uint8_t, bfloat16>)
void globalavgpool2d(dtype __aie_dm_resource_a* __restrict input_ptr,
                     dtype __aie_dm_resource_a* __restrict output_ptr,
                     typename glbavg_traits<dtype>::param_type &globalavgpool2d_params,
                     IN_CASC<typename glbavg_traits<dtype>::acc_type>* casc_acc_in,
                     OUT_CASC<typename glbavg_traits<dtype>::acc_type>* casc_acc_out) {
    typedef typename glbavg_traits<dtype>::acc_type m_acc_type;

    SETUP_ITR2D_COUNTERS(iterator_inner)

    vector<typename glbavg_traits<dtype>::div_type, 32> div_vector =
        broadcast((typename glbavg_traits<dtype>::div_type)globalavgpool2d_params.div_factor);
    for (int i = 0; i < globalavgpool2d_params.outer_loop_count; i++) {
        dtype __aie_dm_resource_a* input_ptr_inner = input_ptr;

        vector<dtype, 32> input_buff_0, input_buff_1, input_buff_2, input_buff_3;

        input_buff_0 = load_v<32>(input_ptr_inner);
        input_buff_1 =
            load_v<32>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, globalavgpool2d_params.step_Ci));
        input_buff_2 =
            load_v<32>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 2 * globalavgpool2d_params.step_Ci));
        input_buff_3 =
            load_v<32>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 3 * globalavgpool2d_params.step_Ci));

        // change: Added 2D iterator to enable height traversal
        input_ptr_inner = INCR_ITR2D_PARAM(input_ptr_inner, iterator_inner, globalavgpool2d_params);

        accum<m_acc_type, 32> result_acc_0 = mul(input_buff_0, div_vector);
        accum<m_acc_type, 32> result_acc_1 = mul(input_buff_1, div_vector);
        accum<m_acc_type, 32> result_acc_2 = mul(input_buff_2, div_vector);
        accum<m_acc_type, 32> result_acc_3 = mul(input_buff_3, div_vector);

        int idx_incr;
        int idx_max;
        if constexpr (batch_size == 1) {
            idx_incr = 8;
            idx_max = globalavgpool2d_params.ifm_x_eff * idx_incr;
        } else if constexpr (batch_size == 2) {
            idx_incr = 16;
            idx_max = globalavgpool2d_params.ifm_x_eff * idx_incr;
        }

        int idx = idx_incr;
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4, ) 
        for (int j = 1; j < globalavgpool2d_params.inner_loop_count; j++){
                // AIE2 native vector load is 256 bits, AIE2p / AIE2ps native vector load is 512 bits.
                // Vector load in current design are all vector<dtype, 32>.
                // vector<int8, 32> is 256 bits and vector<bfloat16, 32> is 512 bits.

                // Shift happened when we need un-aligned vector load.
                // We have HCWN_C8, and load pointer move along W, C or H dimension.
                // "idx" marked actual pointer in dtype (not bytes).

                // int8: vector<int8, 32> is 256 bits.
                // for AIE2/2P/2PS, all native aligned vector load will get 32 elements.
                // if batch_num == 1, N*C8 = 8, possible shuffle is 8, 16, 24.
                // if batch_num == 2, N*C8 = 16, possible shuffle is 16.
                // if batch_num == 4, N*C8 = 32, no shuffle is needed

                // bfloat16: vector<bfloat16, 32> is 512 bits.
                // for AIE2, native aligned vector load will get 16 elements.
                //    if batch_num == 1, N*C8 = 8, possible shuffle is 8.
                //    if batch_num == 2 or 4, M*C8 = 16 or 32, no shufflle is needed.
                // for AIE2p or AIE2ps, native aligned vector load will get 32 elements.
                //    if batch_num == 1, N*C8 = 8, possible shuffle is 8, 16, 24.
                //    if batch_num == 2, N*C8 = 16, possible shuffle is 16.
                //    if batch_num == 4, N*C8 = 32, no shuffle is needed
                int shift = 0;
                if constexpr (sizeof(dtype) == 1) {
                    if constexpr (batch_size == 1) {
                        shift = idx & (int)0x00000018;
                    }
                    if constexpr (batch_size == 2) {
                        shift = idx & (int)(int)0x00000010;
                    }
                }
                if constexpr (sizeof(dtype) == 2) {
#if __AIE_ARCH__ == 20
                    if constexpr (batch_size == 1) {
                        shift = idx & (int)0x00000008;
                    }
#else
                    if constexpr (batch_size == 1) {
                        shift = idx & (int)0x00000018;
                    }
                    if constexpr (batch_size == 2) {
                        shift = idx & (int)0x00000010;
                    }
#endif
                }

                idx = idx + idx_incr;
                idx = (idx == idx_max) ? 0 : idx;

                // change: aligned load changed to unaligned load

                input_buff_0 = load_v<32>(input_ptr_inner);
                input_buff_0 = shuffle_down(input_buff_0, shift);

                input_buff_1 =
                    load_v<32>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, globalavgpool2d_params.step_Ci));
                input_buff_1 = shuffle_down(input_buff_1, shift);

                input_buff_2 = load_v<32>(
                    (dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 2 * globalavgpool2d_params.step_Ci));
                input_buff_2 = shuffle_down(input_buff_2, shift);

                input_buff_3 = load_v<32>(
                    (dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 3 * globalavgpool2d_params.step_Ci));
                input_buff_3 = shuffle_down(input_buff_3, shift);

                result_acc_0 = mac(result_acc_0, input_buff_0, div_vector);
                result_acc_1 = mac(result_acc_1, input_buff_1, div_vector);
                result_acc_2 = mac(result_acc_2, input_buff_2, div_vector);
                result_acc_3 = mac(result_acc_3, input_buff_3, div_vector);

                input_ptr_inner = INCR_ITR2D_PARAM(input_ptr_inner, iterator_inner, globalavgpool2d_params);
            } // End of inner loop

        if constexpr (casc_mode == CASC_START) {
            writeincr(casc_acc_out, result_acc_0);
            writeincr(casc_acc_out, result_acc_1);
            writeincr(casc_acc_out, result_acc_2);
            writeincr(casc_acc_out, result_acc_3);
        } else if constexpr ((casc_mode == CASC_MID) || (casc_mode == CASC_END)) {
            accum<m_acc_type, 32> prev_acc_0;
            accum<m_acc_type, 32> prev_acc_1;
            accum<m_acc_type, 32> prev_acc_2;
            accum<m_acc_type, 32> prev_acc_3;

            prev_acc_0 = readincr_v<32>(casc_acc_in);
            prev_acc_1 = readincr_v<32>(casc_acc_in);
            prev_acc_2 = readincr_v<32>(casc_acc_in);
            prev_acc_3 = readincr_v<32>(casc_acc_in);

            result_acc_0 = add(result_acc_0, prev_acc_0);
            result_acc_1 = add(result_acc_1, prev_acc_1);
            result_acc_2 = add(result_acc_2, prev_acc_2);
            result_acc_3 = add(result_acc_3, prev_acc_3);

            if constexpr (casc_mode == CASC_MID) {
                writeincr(casc_acc_out, result_acc_0);
                writeincr(casc_acc_out, result_acc_1);
                writeincr(casc_acc_out, result_acc_2);
                writeincr(casc_acc_out, result_acc_3);
            } else if constexpr (casc_mode == CASC_END) {
                if constexpr (batch_size == 1) {
                    vector<dtype, 32> sum_0 = result_acc_0.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_1 = result_acc_1.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_2 = result_acc_2.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_3 = result_acc_3.template to_vector<dtype>(globalavgpool2d_params.div_shift);

                    vector<dtype, 32> output;
                    output = shuffle_down_fill(output, sum_0, 8);
                    output = shuffle_down_fill(output, sum_1, 8);
                    output = shuffle_down_fill(output, sum_2, 8);
                    output = shuffle_down_fill(output, sum_3, 8);

                    store_v(output_ptr, output);
                } else if constexpr (batch_size == 2) {
                    vector<dtype, 32> sum_0 = result_acc_0.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_1 = result_acc_1.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_2 = result_acc_2.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_3 = result_acc_3.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> output;

                    output = shuffle_down_fill(output, sum_0, 16);
                    output = shuffle_down_fill(output, sum_1, 16);
                    store_v(output_ptr, output);

                    output = shuffle_down_fill(output, sum_2, 16);
                    output = shuffle_down_fill(output, sum_3, 16);
                    store_v((dtype __aie_dm_resource_a*)byte_incr(output_ptr, 2 * globalavgpool2d_params.step_Co),
                            output);
                } else { // batch of 4
                    store_v(output_ptr, result_acc_0.template to_vector<dtype>(globalavgpool2d_params.div_shift));

                    store_v((dtype __aie_dm_resource_a*)byte_incr(output_ptr, globalavgpool2d_params.step_Co),
                            result_acc_1.template to_vector<dtype>(globalavgpool2d_params.div_shift));

                    store_v((dtype __aie_dm_resource_a*)byte_incr(output_ptr, 2 * globalavgpool2d_params.step_Co),
                            result_acc_2.template to_vector<dtype>(globalavgpool2d_params.div_shift));

                    store_v((dtype __aie_dm_resource_a*)byte_incr(output_ptr, 3 * globalavgpool2d_params.step_Co),
                            result_acc_3.template to_vector<dtype>(globalavgpool2d_params.div_shift));
                }
            }
        }

        input_ptr = (dtype __aie_dm_resource_a*)byte_incr(input_ptr, 4 * globalavgpool2d_params.step_Ci);
        output_ptr = (dtype __aie_dm_resource_a*)byte_incr(output_ptr, 4 * globalavgpool2d_params.step_Co);
    }
}

template <uint8_t batch_size,
          casc_mode_t casc_mode,
          typename dtype,
          typename dtype_port,
          typename config_ifm,
          typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, uint8_t, bfloat16>)
INLINE void globalavgpool2d(adf::input_buffer_conf<dtype_port, config_ifm>& __restrict ifm,
                     adf::output_buffer_conf<dtype_port, config_ofm>& __restrict ofm,
                     typename glbavg_traits<dtype>::param_type &globalavgpool2d_params,
                     IN_CASC<typename glbavg_traits<dtype>::acc_type>* casc_acc_in,
                     OUT_CASC<typename glbavg_traits<dtype>::acc_type>* casc_acc_out) {
    typedef typename glbavg_traits<dtype>::acc_type m_acc_type;

    dtype __aie_dm_resource_a* input_ptr = (dtype __aie_dm_resource_a*)ifm.data();
    dtype __aie_dm_resource_a* output_ptr = (dtype __aie_dm_resource_a*)ofm.data();
    return globalavgpool2d<batch_size, casc_mode, dtype>(input_ptr, output_ptr, globalavgpool2d_params, casc_acc_in, casc_acc_out);
}

// Global avg pool with cascade modes excited at run time instead of template parameters
// Needed to reduce PM where multiple cascade modes are needed at run time
// TODO: Supports only casc start and end to reduce PM,enable extension to casc mid and reduce PM further

template <uint8_t batch_size,
          typename dtype>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, uint8_t, bfloat16>)
void globalavgpool2d(dtype __aie_dm_resource_a* __restrict input_ptr,
                     dtype __aie_dm_resource_a* __restrict output_ptr,
                     typename glbavg_traits<dtype>::param_type &globalavgpool2d_params,
                     IN_CASC<typename glbavg_traits<dtype>::acc_type>* casc_acc_in,
                     OUT_CASC<typename glbavg_traits<dtype>::acc_type>* casc_acc_out) {
    typedef typename glbavg_traits<dtype>::acc_type m_acc_type;

    casc_mode_t casc_mode=globalavgpool2d_params.casc_mode;// cascade mode via RTP

    SETUP_ITR2D_COUNTERS(iterator_inner)

    vector<typename glbavg_traits<dtype>::div_type, 32> div_vector =
        broadcast((typename glbavg_traits<dtype>::div_type)globalavgpool2d_params.div_factor);
    for (int i = 0; i < globalavgpool2d_params.outer_loop_count; i++) {
        dtype __aie_dm_resource_a* input_ptr_inner = input_ptr;

        vector<dtype, 32> input_buff_0, input_buff_1, input_buff_2, input_buff_3;

        input_buff_0 = load_v<32>(input_ptr_inner);
        input_buff_1 =
            load_v<32>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, globalavgpool2d_params.step_Ci));
        input_buff_2 =
            load_v<32>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 2 * globalavgpool2d_params.step_Ci));
        input_buff_3 =
            load_v<32>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 3 * globalavgpool2d_params.step_Ci));

        // change: Added 2D iterator to enable height traversal
        input_ptr_inner = INCR_ITR2D_PARAM(input_ptr_inner, iterator_inner, globalavgpool2d_params);

        accum<m_acc_type, 32> result_acc_0 = mul(input_buff_0, div_vector);
        accum<m_acc_type, 32> result_acc_1 = mul(input_buff_1, div_vector);
        accum<m_acc_type, 32> result_acc_2 = mul(input_buff_2, div_vector);
        accum<m_acc_type, 32> result_acc_3 = mul(input_buff_3, div_vector);

        int idx_incr;
        int idx_max;
        if constexpr (batch_size == 1) {
            idx_incr = 8;
            idx_max = globalavgpool2d_params.ifm_x_eff * idx_incr;
        } else if constexpr (batch_size == 2) {
            idx_incr = 16;
            idx_max = globalavgpool2d_params.ifm_x_eff * idx_incr;
        }

        int idx = idx_incr;
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE(4, ) 
        for (int j = 1; j < globalavgpool2d_params.inner_loop_count; j++){
                // AIE2 native vector load is 256 bits, AIE2p / AIE2ps native vector load is 512 bits.
                // Vector load in current design are all vector<dtype, 32>.
                // vector<int8, 32> is 256 bits and vector<bfloat16, 32> is 512 bits.

                // Shift happened when we need un-aligned vector load.
                // We have HCWN_C8, and load pointer move along W, C or H dimension.
                // "idx" marked actual pointer in dtype (not bytes).

                // int8: vector<int8, 32> is 256 bits.
                // for AIE2/2P/2PS, all native aligned vector load will get 32 elements.
                // if batch_num == 1, N*C8 = 8, possible shuffle is 8, 16, 24.
                // if batch_num == 2, N*C8 = 16, possible shuffle is 16.
                // if batch_num == 4, N*C8 = 32, no shuffle is needed

                // bfloat16: vector<bfloat16, 32> is 512 bits.
                // for AIE2, native aligned vector load will get 16 elements.
                //    if batch_num == 1, N*C8 = 8, possible shuffle is 8.
                //    if batch_num == 2 or 4, M*C8 = 16 or 32, no shufflle is needed.
                // for AIE2p or AIE2ps, native aligned vector load will get 32 elements.
                //    if batch_num == 1, N*C8 = 8, possible shuffle is 8, 16, 24.
                //    if batch_num == 2, N*C8 = 16, possible shuffle is 16.
                //    if batch_num == 4, N*C8 = 32, no shuffle is needed
                int shift = 0;
                if constexpr (sizeof(dtype) == 1) {
                    if constexpr (batch_size == 1) {
                        shift = idx & (int)0x00000018;
                    }
                    if constexpr (batch_size == 2) {
                        shift = idx & (int)(int)0x00000010;
                    }
                }
                if constexpr (sizeof(dtype) == 2) {
#if __AIE_ARCH__ == 20
                    if constexpr (batch_size == 1) {
                        shift = idx & (int)0x00000008;
                    }
#else
                    if constexpr (batch_size == 1) {
                        shift = idx & (int)0x00000018;
                    }
                    if constexpr (batch_size == 2) {
                        shift = idx & (int)0x00000010;
                    }
#endif
                }

                idx = idx + idx_incr;
                idx = (idx == idx_max) ? 0 : idx;

                // change: aligned load changed to unaligned load

                input_buff_0 = load_v<32>(input_ptr_inner);
                input_buff_0 = shuffle_down(input_buff_0, shift);

                input_buff_1 =
                    load_v<32>((dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, globalavgpool2d_params.step_Ci));
                input_buff_1 = shuffle_down(input_buff_1, shift);

                input_buff_2 = load_v<32>(
                    (dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 2 * globalavgpool2d_params.step_Ci));
                input_buff_2 = shuffle_down(input_buff_2, shift);

                input_buff_3 = load_v<32>(
                    (dtype __aie_dm_resource_a*)byte_incr(input_ptr_inner, 3 * globalavgpool2d_params.step_Ci));
                input_buff_3 = shuffle_down(input_buff_3, shift);

                result_acc_0 = mac(result_acc_0, input_buff_0, div_vector);
                result_acc_1 = mac(result_acc_1, input_buff_1, div_vector);
                result_acc_2 = mac(result_acc_2, input_buff_2, div_vector);
                result_acc_3 = mac(result_acc_3, input_buff_3, div_vector);

                input_ptr_inner = INCR_ITR2D_PARAM(input_ptr_inner, iterator_inner, globalavgpool2d_params);
            } // End of inner loop

        if (casc_mode == CASC_START) {
            writeincr(casc_acc_out, result_acc_0);
            writeincr(casc_acc_out, result_acc_1);
            writeincr(casc_acc_out, result_acc_2);
            writeincr(casc_acc_out, result_acc_3);
        //} else if ((casc_mode == CASC_MID) || (casc_mode == CASC_END)) {
        } else if (casc_mode == CASC_END) {
            accum<m_acc_type, 32> prev_acc_0;
            accum<m_acc_type, 32> prev_acc_1;
            accum<m_acc_type, 32> prev_acc_2;
            accum<m_acc_type, 32> prev_acc_3;

            prev_acc_0 = readincr_v<32>(casc_acc_in);
            prev_acc_1 = readincr_v<32>(casc_acc_in);
            prev_acc_2 = readincr_v<32>(casc_acc_in);
            prev_acc_3 = readincr_v<32>(casc_acc_in);

            result_acc_0 = add(result_acc_0, prev_acc_0);
            result_acc_1 = add(result_acc_1, prev_acc_1);
            result_acc_2 = add(result_acc_2, prev_acc_2);
            result_acc_3 = add(result_acc_3, prev_acc_3);

          /*  if (casc_mode == CASC_MID) {
                writeincr(casc_acc_out, result_acc_0);
                writeincr(casc_acc_out, result_acc_1);
                writeincr(casc_acc_out, result_acc_2);
                writeincr(casc_acc_out, result_acc_3);
            } else*/ if (casc_mode == CASC_END) {
                if constexpr (batch_size == 1) {
                    vector<dtype, 32> sum_0 = result_acc_0.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_1 = result_acc_1.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_2 = result_acc_2.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_3 = result_acc_3.template to_vector<dtype>(globalavgpool2d_params.div_shift);

                    vector<dtype, 32> output;
                    output = shuffle_down_fill(output, sum_0, 8);
                    output = shuffle_down_fill(output, sum_1, 8);
                    output = shuffle_down_fill(output, sum_2, 8);
                    output = shuffle_down_fill(output, sum_3, 8);

                    store_v(output_ptr, output);
                } else if constexpr (batch_size == 2) {
                    vector<dtype, 32> sum_0 = result_acc_0.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_1 = result_acc_1.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_2 = result_acc_2.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> sum_3 = result_acc_3.template to_vector<dtype>(globalavgpool2d_params.div_shift);
                    vector<dtype, 32> output;

                    output = shuffle_down_fill(output, sum_0, 16);
                    output = shuffle_down_fill(output, sum_1, 16);
                    store_v(output_ptr, output);

                    output = shuffle_down_fill(output, sum_2, 16);
                    output = shuffle_down_fill(output, sum_3, 16);
                    store_v((dtype __aie_dm_resource_a*)byte_incr(output_ptr, 2 * globalavgpool2d_params.step_Co),
                            output);
                } else if constexpr (batch_size == 4) {
                    store_v(output_ptr, result_acc_0.template to_vector<dtype>(globalavgpool2d_params.div_shift));

                    store_v((dtype __aie_dm_resource_a*)byte_incr(output_ptr, globalavgpool2d_params.step_Co),
                            result_acc_1.template to_vector<dtype>(globalavgpool2d_params.div_shift));

                    store_v((dtype __aie_dm_resource_a*)byte_incr(output_ptr, 2 * globalavgpool2d_params.step_Co),
                            result_acc_2.template to_vector<dtype>(globalavgpool2d_params.div_shift));

                    store_v((dtype __aie_dm_resource_a*)byte_incr(output_ptr, 3 * globalavgpool2d_params.step_Co),
                            result_acc_3.template to_vector<dtype>(globalavgpool2d_params.div_shift));
                }
            }
        }

        input_ptr = (dtype __aie_dm_resource_a*)byte_incr(input_ptr, 4 * globalavgpool2d_params.step_Ci);
        output_ptr = (dtype __aie_dm_resource_a*)byte_incr(output_ptr, 4 * globalavgpool2d_params.step_Co);
    }
}

template <uint8_t batch_size,
          typename dtype,
          typename dtype_port,
          typename config_ifm,
          typename config_ofm>
    requires(mllib::utils::is_one_of_v<dtype, int8_t, uint8_t, bfloat16>)
INLINE void globalavgpool2d(adf::input_buffer_conf<dtype_port, config_ifm>& __restrict ifm,
                     adf::output_buffer_conf<dtype_port, config_ofm>& __restrict ofm,
                     typename glbavg_traits<dtype>::param_type &globalavgpool2d_params,
                     IN_CASC<typename glbavg_traits<dtype>::acc_type>* casc_acc_in,
                     OUT_CASC<typename glbavg_traits<dtype>::acc_type>* casc_acc_out) {
    typedef typename glbavg_traits<dtype>::acc_type m_acc_type;

    dtype __aie_dm_resource_a* input_ptr = (dtype __aie_dm_resource_a*)ifm.data();
    dtype __aie_dm_resource_a* output_ptr = (dtype __aie_dm_resource_a*)ofm.data();
    return globalavgpool2d<batch_size, dtype>(input_ptr, output_ptr, globalavgpool2d_params, casc_acc_in, casc_acc_out);
}

#endif //__GLOBALAVGPOOL2D_H__
