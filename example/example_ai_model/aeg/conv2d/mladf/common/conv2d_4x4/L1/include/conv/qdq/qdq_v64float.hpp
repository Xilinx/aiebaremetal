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

inline v64float concat( v32float a, v32float b ) {
    return ( v64float ) concat(( v32accfloat ) a, ( v32accfloat ) b );
}


template<unsigned fp_accuracy_mode=1>
ALWAYS_INLINE v64accfloat mac_elew( v64accfloat acc, v64float a, v64float b ) {
    return mac_elem_64_accuracy_fast( a, b, acc, 0, 0, 0 );
}
template<>
ALWAYS_INLINE v64accfloat mac_elew<0>( v64accfloat acc, v64float a, v64float b ) {
    return mac_elem_64_accuracy_low( a, b, acc, 0, 0, 0 );
}
template<>
ALWAYS_INLINE v64accfloat mac_elew<2>( v64accfloat acc, v64float a, v64float b ) {
    return mac_elem_64_accuracy_safe( a, b, acc, 0, 0, 0 );
}


template<unsigned terms, unsigned lr_min, unsigned fp_accuracy_mode=1, unsigned coeff_step=4, unsigned coeff_skip=0, unsigned vector_coeffs=0, typename Ti, typename Tr>
void qdq_v64float( Ti * ifm, adf::input_buffer<int32> &ifm_sum, adf::input_buffer<float> &coeffs, adf::output_buffer<Tr> &ofm, QDQParams &param, float c2=0, float c1=0, float c0=0 )
{
    constexpr unsigned Mgran = 8;
    constexpr unsigned Ngran = 8;
    constexpr unsigned Mtile = 8;
    constexpr unsigned Ntile = 8;
    constexpr unsigned Mtile_DM = 8;
    constexpr unsigned Mi = std::min( Mgran, Mtile_DM ) / Mtile;
    constexpr unsigned Mb = std::max( 1u, Mgran / Mtile_DM );
    constexpr unsigned Ni = Ngran / Ntile;
    constexpr unsigned V = Mtile * Ntile;
    constexpr unsigned Vb = Mgran * Ngran;
    constexpr unsigned Vi = std::min( V, 128 / unsigned(sizeof( Ti )));
    constexpr unsigned Vo = std::min( V, 128 / unsigned(sizeof( Tr )));
    constexpr unsigned strideS = sum_write_garbage_stride<int32>();
    
    Add3dElem add_3d_in1( param.step0, param.wrap0, param.step1, param.wrap1, param.step2 );
    Add3dElem add_3d_in2( add_3d_in1, param.wrap2, param.step3, param.wrap3, param.step4 );
    Add2dElem add_2d_sum( Mgran * strideS, param.M_g * param.Y_g, 0 );
    Add2dElem add_2d_qnt( 0, param.M_g * param.Y_g, coeff_step * Ngran );
    Add3dElem add_3d_out( Vb, param.M_g, Vb * param.M_g * param.N_g, param.Y_g, Vb * param.M_g );
    
    auto pI = aie::begin_vector<Vi>( ifm );
    auto pS = aie::begin_vector<Mtile/2>( ifm_sum );
    auto pQ = aie::begin_vector<Ntile>( coeffs );
    auto * restrict pOs = ofm.base();
    auto pO = aie::begin_restrict_vector<Vo>( pOs );
        
    const aie::saturation_mode sat = aie::tile::current().get_saturation();
    aie::tile::current().set_saturation(aie::saturation_mode::saturate);

    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE( lr_min, ) 
    for ( unsigned o=0; o<param.M_g*param.N_g*param.Y_g; o++ )chess_modulo_scheduling_budget_ratio(5000){
        auto ifm0 = pI[0];
        auto ifm1 = pI[1];

        static_assert( terms < 3 || ( vector_coeffs > 1 && std::is_integral_v<Ti> )); //This need to be implemented
        if constexpr( terms == 3 ) {
            auto q1 = pQ[3].cast_to<int16>( ).extract<8>( 0 );
            aie::accum<acc48,32> acc;
            
            acc.from_vector( ifm0 );
            acc = mac_outer_prod<int32,Mtile/2,int16,Ntile,acc48>( acc, pS[0], q1 );            
            ifm0 = acc.to_vector<int32>( );
            acc.from_vector( ifm1 );
            acc = mac_outer_prod<int32,Mtile/2,int16,Ntile,acc48>( acc, pS[1], q1 );            
            ifm1 = acc.to_vector<int32>( );
        }
        
        v64float ifm;
        aie::accum<accfloat, 64> magic_h( aie::broadcast<int16, 64>( 0x5301 ).cast_to<bfloat16>( ));
        aie::accum<accfloat, 64> magic_l( aie::broadcast<int16, 64>( 0x4b01 ).cast_to<bfloat16>( ));
            
        if constexpr( sizeof( Ti ) < 4 ) {
            aie::accum<acc32, 64> vint( ifm0 );
            vint = (v64acc32) vint + (v64acc32)magic_l;
            v64accfloat vfp = (v64accfloat)vint - magic_l;
            ifm = (v64float) vfp;
            
        } else if constexpr( std::is_integral_v<Ti> ) {
            auto ifm0_h = aie::filter_odd( ifm0.template cast_to<int16>( ));
            auto ifm1_h = aie::filter_odd( ifm1.template cast_to<int16>( ));
            auto ifm0_l = aie::filter_even( ifm0.template cast_to<uint16>( ));
            auto ifm1_l = aie::filter_even( ifm1.template cast_to<uint16>( ));
    
            aie::accum<acc32, 64> vint_h( aie::concat( ifm0_h, ifm1_h ));
            aie::accum<acc32, 64> vint_l( aie::concat( ifm0_l, ifm1_l ));
            
            vint_h = (v64acc32) vint_h + (v64acc32)magic_h;
            vint_l = (v64acc32) vint_l + (v64acc32)magic_l;
    
            v64accfloat vfp = (v64accfloat)vint_h - magic_h;
            vfp = ((v64accfloat)vint_l - magic_l ) + vfp;
    
            ifm = (v64float) vfp;
            
        } else {
            ifm = concat(( v32float )ifm0, ( v32float )ifm1 );
        }

        auto q0 = vector_coeffs > 0 ? pQ[0] : aie::broadcast<float,Ntile>( c0 );
        auto q2 = vector_coeffs > 1 ? pQ[terms-1+coeff_skip] : aie::broadcast<float,Ntile>( terms >= 3 ? c2 : c1 );
        auto q0i = accum_broadcast<32,float,Ntile,float>( q0 );
        v32float q2i = vector_broadcast<32>( q2 );
        v64accfloat acc = concat( q0i, q0i );
        acc = mac_elew<fp_accuracy_mode>( acc, ifm, concat( q2i, q2i ));
        
        if constexpr( std::is_integral_v<Tr> ) {
            v64accfloat vfp = acc + magic_l;
            aie::accum<acc32, 64> vint = (v64acc32) vfp - (v64acc32) magic_l;
            
            pO[0] = vint.template to_vector<Tr>( );
        } else {
            #pragma unroll
            for ( unsigned i = 0; i < V / Vo; i++ )
                pO[i] = aie::accum<accfloat,64>( acc ).extract<Vo>( i ).template to_vector<Tr>( );
        }
        
        pI = add_3d_in1( pI );
        pI = add_3d_in2( pI );
        pQ = add_2d_qnt( pQ );
        pS = add_2d_sum( pS );
        pO = add_3d_out( pO );
    }
    aie::tile::current().set_saturation(sat);
}


template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned terms, unsigned lr_min=4, unsigned coeff_step=4, unsigned coeff_skip=0, unsigned fp_accuracy_mode=1>
requires( std::is_same_v<Tq,float> && std::is_same_v<Tq0,float> && Mgran==8 && SameValue<Ngran,Ntile> && Mtile==8 && Ntile==8 && QDQTerms<terms,3> )
INLINE_DECL void qdq( Ti * ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<Tq> &coeffs, adf::output_buffer<Tr> &ofm, QDQParams &param )
{
    auto ifms = local_buffer_cast<int32, adf::direction::in>( ifm_sum );
    qdq_v64float<terms, lr_min, fp_accuracy_mode, coeff_step, coeff_skip, 2>( ifm, ifms, coeffs, ofm, param );
}

template<unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned terms, unsigned lr_min=4, unsigned coeff_step=1, unsigned fp_accuracy_mode=1, typename Ti, typename Tq, typename Tq0, typename Tr>
requires( std::is_same_v<Tq,float> && std::is_same_v<Tq0,float> && Mgran==8 && SameValue<Ngran,Ntile> && Mtile==8 && Ntile==8 && QDQTerms<terms,3> )
INLINE_DECL void qdq( Ti * ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<Tq0> &coeff, Tq c1, Tq c2, adf::output_buffer<Tr> &ofm, QDQParams &param )
{
    auto ifms = local_buffer_cast<int32, adf::direction::in>( ifm_sum );
    qdq_v64float<terms, lr_min, fp_accuracy_mode, coeff_step, 0, 1>( ifm, ifms, coeff, ofm, param, c2, c1 );
}
    
template<unsigned Mgran, unsigned Mtile, unsigned Ntile, unsigned lr_min=4, unsigned fp_accuracy_mode=1, typename Ti, typename Tq, typename Tq0, typename Tr>
requires( std::is_same_v<Tq,float> && std::is_same_v<Tq0,float> && Mgran==8 && Mtile==8 && Ntile==8 )
INLINE_DECL void qdq( Ti * ifm, Tq0 c0, Tq c1, adf::output_buffer<Tr> &ofm, QDQParams &param )
{
    auto dummy1 = adf::input_buffer<int32>({( int32* )ifm, 1, 0, 1 });
    auto dummy2 = adf::input_buffer<Tq>({( Tq* )ifm, 1, 0, 1 });
    qdq_v64float<2, lr_min, fp_accuracy_mode>( ifm, dummy1, dummy2, ofm, param, c1, c1, c0 );
}

