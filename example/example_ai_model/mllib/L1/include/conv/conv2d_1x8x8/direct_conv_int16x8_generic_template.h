/*
    Copyright (C) 2019 - 2022 Xilinx, Inc. All rights reserved.
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

#ifndef __DIRECT_CONV_INT16X8_GENERIC_TEMPLATE_H__
#define __DIRECT_CONV_INT16X8_GENERIC_TEMPLATE_H__

#include <stdint.h>
#include "direct_conv_common.h"
#include "direct_conv_int16x8_generic_kernel.h"
#include "aie_api/aie.hpp"


template<typename Ta, typename Tv>
inline Ta from_vector_sign( Tv v, bool sign, int shift=0 ) {
    Ta a;
    a.from_vector_sign( v, sign, shift );
    return a;
}


template<enum KernelConfig kc_in, enum KernelConfig kc_out>
ALWAYS_INLINE void direct_conv_int16x8_generic_template
(
        int16 * input,
        int8  * weights,
        int32 * tdm1,
        int32 * tdm2,
        int16 * restrict output,
        //const KernelParams &params,
        KernelParams &params,
        const KernelControl ctrl,
        const Increments incrs
) {
    static_assert( kc_in == KC_ZERO || kc_in == KC_TDM32, 
                   "Unsupported input flavour for direct_conv int16x8_generic kernel" );
    static_assert( kc_out == KC_RESULT16 || kc_out == KC_TDM32, 
                   "Unsupported input flavour for direct_conv int16x8_generic kernel" );

    using mm_t = aie::mmul<4,8,8,int16,int8>;
    using acc_t = mm_t::accum_type;
    constexpr unsigned Va = 32;

    v32int16 * pA = ( v32int16 * ) input;
    int8  * pW = weights;
    int16 * pOut = output;

    int32 * pTdm1 = tdm1;
    int32 * pTdm2 = tdm2;
    int32 * restrict pOut1 = chess_copy( tdm1 );
    int32 * restrict pOut2 = chess_copy( tdm2 );

    fifo_state_t fA;
    fA.pos = 0;
    
    dims_3d_t dimsAI = incrs.dimsAI;
    dims_3d_t dimsAO = incrs.dimsAO;
    dims_3d_t dimsO = incrs.dimsO;
    dims_3d_t dimsW = incrs.dimsW;
    int op_mode = params.op_mode;
    
    event0();
    int incTi = incrs.incT_0;
    int incT_flag = 0;

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE(2,)
    for (unsigned j=0; j<params.outer_g; j++)
        
        
    {
        int zero_acc = ctrl.zero_init;
        //bool kc_in_casc = kc_in == KC_CASC || kc_in == KC_TDM16_CASC || kc_in == KC_TDM32_CASC || kc_in == KC_RESULT32_CASC;
        mm_t m[2];

        if (kc_in == KC_TDM32 || kc_in == KC_TDM32_CASC) {
            acc_t tdm1, tdm2;
            using acc_h_t = decltype( tdm1.extract<Va/2>( 0 ));
            tdm1.insert( 1, from_vector_sign<acc_h_t>( aie::load_v<Va/2>( pTdm1 + Va / 2 ), ctrl.sign_O, params.shift_tdm ));
            tdm1.insert( 0, from_vector_sign<acc_h_t>( aie::load_v<Va/2>( pTdm1 ),          ctrl.sign_O, params.shift_tdm ));
            tdm2.insert( 1, from_vector_sign<acc_h_t>( aie::load_v<Va/2>( pTdm2 + Va / 2 ), ctrl.sign_O, params.shift_tdm ));
            tdm2.insert( 0, from_vector_sign<acc_h_t>( aie::load_v<Va/2>( pTdm2 ),          ctrl.sign_O, params.shift_tdm ));
            m[0] = mm_t( aie::op_zero( tdm1, zero_acc ));
            m[1] = mm_t( aie::op_zero( tdm2, zero_acc ));
            pTdm1 = byte_incr( pTdm1, incTi );
            pTdm2 = byte_incr( pTdm2, incTi );
        }
        int incTo = chess_copy( incTi );
        incT_flag ^= 1;
        incTi = incT_flag ? incrs.incT_1 : incrs.incT_0;

        int bound = params.inner_g;
        {
            aie::vector<int16,32> s0 = fifo_ld_popx( pA, fA, incrs.step_align, 63);
            aie::vector<int16,32> s1 = fifo_ld_pop_3d_byte(pA, fA, dimsAI);

            aie::vector<int16,32> x0 = shuffle(s0, s1, incrs.shfl_0);
            aie::vector<int16,32> x1 = shuffle(x0, s1, incrs.shfl_1);

            aie::vector<int8, 64> y = aie::load_v<64>( pW );
            pW = add_3d_byte( pW, dimsW );

            //m[0].mac( aie::op_sign( x0, ctrl.sign_A ), aie::op_sign( y, ctrl.sign_W ));
            //m[1].mac( aie::op_sign( x1, ctrl.sign_A ), aie::op_sign( y, ctrl.sign_W ));
            m[0] = acc_t( mac_4x8_8x8_conf( x0, ctrl.sign_A, y, ctrl.sign_W, m[0].to_accum( ), zero_acc, 0, op_mode == OP_DWC_SUM, 0 ));
            m[1] = acc_t( mac_4x8_8x8_conf( x1, ctrl.sign_A, y, ctrl.sign_W, m[1].to_accum( ), zero_acc, 0, op_mode == OP_DWC_SUM, 0 ));
        }

        AIE_PREPARE_FOR_PIPELINING
        AIE_LOOP_RANGE(7,)
        for (int i=1; i<bound; i++)
            
            //chess_peel_pipelined_loop(2)
            //chess_pipeline_adjust_preamble(-3)
            
        {
            aie::vector<int16,32> s0 = fifo_ld_popx( pA, fA, incrs.step_align, 63);
            aie::vector<int16,32> s1 = fifo_ld_pop_3d_byte(pA, fA, dimsAI);

            aie::vector<int16,32> x0 = shuffle(s0, s1, incrs.shfl_0);
            aie::vector<int16,32> x1 = shuffle(x0, s1, incrs.shfl_1);

            aie::vector<int8, 64> y = aie::load_v<64>( pW );
            pW = add_3d_byte( pW, dimsW );

            //m[0].mac( aie::op_sign( x0, ctrl.sign_A ), aie::op_sign( y, ctrl.sign_W ));
            //m[1].mac( aie::op_sign( x1, ctrl.sign_A ), aie::op_sign( y, ctrl.sign_W ));
            m[0] = acc_t( mac_4x8_8x8_conf( x0, ctrl.sign_A, y, ctrl.sign_W, m[0].to_accum( ), 0, 0, op_mode == OP_DWC_SUM, 0 ));
            m[1] = acc_t( mac_4x8_8x8_conf( x1, ctrl.sign_A, y, ctrl.sign_W, m[1].to_accum( ), 0, 0, op_mode == OP_DWC_SUM, 0 ));
        }

        pA = add_3d_byte( pA, dimsAO );

        if (kc_out == KC_RESULT16) {
            aie::store_v( pOut, m[0].to_vector_sign<int16>( ctrl.sign_O, params.shift_res ));       pOut = byte_incr(pOut, incrs.incS_0);
            aie::store_v( pOut, m[1].to_vector_sign<int16>( ctrl.sign_O, params.shift_res ));
            pOut = add_3d_byte( pOut, dimsO );

        } else if (kc_out == KC_TDM32) {
            aie::store_v( pOut1 + Va / 2, m[0].to_accum( ).extract<Va/2>( 1 ).to_vector_sign<int32>( ctrl.sign_O, params.shift_tdm ));
            aie::store_v( pOut1,          m[0].to_accum( ).extract<Va/2>( 0 ).to_vector_sign<int32>( ctrl.sign_O, params.shift_tdm ));
            aie::store_v( pOut2 + Va / 2, m[1].to_accum( ).extract<Va/2>( 1 ).to_vector_sign<int32>( ctrl.sign_O, params.shift_tdm ));
            aie::store_v( pOut2,          m[1].to_accum( ).extract<Va/2>( 0 ).to_vector_sign<int32>( ctrl.sign_O, params.shift_tdm ));
            pOut1 = byte_incr( pOut1, incTo );
            pOut2 = byte_incr( pOut2, incTo );
        }
    }
    event1();
}


#endif // __DIRECT_CONV_INT16X8_GENERIC_TEMPLATE_H__
