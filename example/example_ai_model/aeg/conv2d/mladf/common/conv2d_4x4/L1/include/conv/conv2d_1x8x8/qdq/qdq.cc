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

#ifndef __QDQ_CC__
#define __QDQ_CC__

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include "qdq_helpers.hpp"

#define INLINE_DECL ALWAYS_INLINE

/*! \brief Scaling kernel for QDQ following a GEMM operation. Kernel implementation is tested and optimized for Tq=int16, Tr=int8, Mgran=16, Ngran=8, Mtile=8, Ntile=8, lr_min=6
    Compute: ofm = srs( ifm^{MxN} * coeff[2]^{1xN} + ifm_sum^{Mx1} * coeff[1]^{1xN} + ( coeff[0]^{1xN} << shift_Qb ), shift_out );

  @param[in] ifm        input to QDQ kernel (output of GEMM). Data order: C R C8 ( N M N8 )
  @param[in] ifm_sum    column sum values of ifms to GEMM kernel
  @param[in] coeffs     coefficients for QDQ compute. Data order: C coeff_step C8 ( N 4 N8 ). coeff_step = 4 while in 3 term mode only index 0, 1 and 2+coeff_skip are in use and in 2 term mode only index 0 and 1+coeff_skip are in use. If sizeof( Tq0 ) > sizeof( Tq ), then the upper indices are adjusted accordingly.
  @param[out] ofm       output of QDQ kernel in target precision (for next GEMM)
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel
 */
template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned terms, unsigned lr_min=4, unsigned coeff_step=4, unsigned coeff_skip=0, unsigned fp_accuracy_mode=1>
requires( SameValue<Ngran,Ntile> && QDQTerms<terms,3> )
INLINE_DECL void qdq( Ti * ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<Tq> &coeffs, adf::output_buffer<Tr> &ofm, QDQParams &param )
{
    constexpr unsigned Mtile_DM = 8;
    constexpr unsigned Mi = std::min( Mgran, Mtile_DM ) / Mtile;
    constexpr unsigned Mb = std::max( 1u, Mgran / Mtile_DM );
    constexpr unsigned Vo = Mtile * Ntile;
    constexpr unsigned Vb = Mgran * Ngran;
    constexpr unsigned strideS = sum_write_garbage_stride<Ti>();
    constexpr unsigned offset_c1 = std::max( 1u, unsigned(sizeof( Tq0 )) / unsigned(sizeof( Tq ))) * Ntile;
    constexpr unsigned offset_c2 = offset_c1 + (( terms > 2 ) + coeff_skip ) * Ntile;
    using Ti_op = std::conditional_t<std::is_integral_v<Ti> ^ std::is_integral_v<Tq>, Tq, Ti>;
    using acc_t = aie::detail::accum_tag_for_mul_types<Ti_op, Tq>;

    constexpr bool int16_opt = std::is_same_v<Ti,int16> && std::is_same_v<Tq,int16> && std::is_same_v<Tq,Tq0> && terms == 3 && FAST_INT16_ACCUM_BROADCAST == 2;
    constexpr bool ifm_vector_load = std::is_same_v<Ti,Tr>;

    Add3dElem add_3d_in1( param.step0, param.wrap0, param.step1, param.wrap1, param.step2 );
    Add3dElem add_3d_in2( add_3d_in1, param.wrap2, param.step3, param.wrap3, param.step4 );
    Add2dElem add_2d_sum( Mgran * strideS, param.M_g * param.Y_g, 0 );
    Add3dElem add_3d_sum( Mgran, std::max( 1u, Mtile_DM / Mgran ), Mgran * strideS, param.M_g * param.Y_g, 0 );
    Add2dElem add_2d_qnt( 0, param.M_g * param.Y_g, coeff_step * Ngran );
    Add3dElem add_3d_out( Vb, param.M_g, Vb * param.M_g * param.N_g, param.Y_g, Vb * param.M_g );

    Ti __aie_dm_resource_a * pI = (Ti __aie_dm_resource_a *) ifm;
    auto pIv = aie::begin_vector<Vo, aie_dm_resource::a>( ifm );
    auto pS = aie::begin_vector<std::max( int16_opt ? Mgran * strideS : Mtile, 16 / unsigned(sizeof( Ti ))), aie_dm_resource::b>( ifm_sum );
    Tq __aie_dm_resource_b * pQ = (Tq __aie_dm_resource_b *) coeffs.base();
    auto * restrict pOs = ofm.base();
    auto pO = aie::begin_restrict_vector<Vo, aie_dm_resource::c>( pOs );

    //printf( "\nCoeffs QDQ: c0=%lli, c1=%i, c2=%i\n", (long long)(*(Tq0*)pQ), pQ[offset_c1], pQ[offset_c2] );

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE( lr_min, )
    for ( unsigned o=0; o<param.M_g*param.N_g*param.Y_g; o++ )
        
        chess_modulo_scheduling_budget_ratio(5000)
        
    {
        auto pIb = pI;
        auto ss = zip(( Ti )( 1 << param.shift_Qb ), *pS );
        #pragma unroll
        for (unsigned mbc=0; mbc<Mb; mbc++) {
            unsigned mb = Mb - 1 - mbc;
            //unsigned mb = mbc;
            pI = pIb + param.step_Mb * mb;
            #pragma unroll
            for (unsigned mic=0; mic<Mi; mic++) {
                unsigned mi = Mi - 1 - mic;
                aie::vector<Ti,Vo> ifm;
                if constexpr( ifm_vector_load ) {
                    ifm = pIv[mi + param.step_Mb / Vo * mb];
                } else {
                    ifm = load_index<Vo>( pI, Vo * mi );
                }
                aie::accum<acc_t,Vo> acc;
                aie::vector<Tq,Ntile> q2;
                if constexpr( int16_opt ) {
                    auto q01 = aie::load_v<2*Ntile>( pQ );
                    aie::mmul<Mtile, 2, Ntile, Ti, Tq> mm;
                    mm.mul( ss.template extract<2*Mtile>( mi + Mi * mb * strideS ), q01 );
                    acc = mm.to_accum();
                } else {
                    if constexpr( std::is_same_v<int64,Tq0> ) {
                        static_assert( Ntile == 8 && Mtile == 4 ); //int64 not supported by AIE API, hardened:
                        v8acc64 __aie_register( bmll0) tmp = *( v8acc64 *) pQ;
                        aie::accum<acc_t,8> a( tmp );
                        auto b = locate_in_register<0>( aie::concat( a, a ));
                        acc = locate_in_register<0>(aie::concat( b, b ));
                    } else {
                        using T2 = std::conditional_t<std::is_integral_v<Ti_op>, Ti_op, Tq0>;
                        acc = accum_broadcast<Vo,Tq0,Ntile,T2>( aie::load_v<Ntile>(( Tq0* ) pQ ), param.shift_Qb );
                    }
                    if constexpr( terms == 3 )
                        acc = mac_outer_prod<Ti_op,Mtile,Tq,Ntile,acc_t,fp_accuracy_mode>( acc, convert<Ti_op>( pS[mi + Mi * mb * strideS] ), aie::load_v<Ntile>( pQ + offset_c1 ));
                }
                acc = mac_elew<Ti_op,Vo,Tq,acc_t,fp_accuracy_mode>( acc, convert<Ti_op>( ifm ), aie::load_v<Ntile>( pQ + offset_c2 ).template grow_replicate<Vo>());
                if constexpr( std::is_integral_v<Tr> ^ std::is_integral_v<Tq> )
                    pO[mi + Mi * mb] = convert<Tr>( acc.template to_vector<Tq>( ));
                else if constexpr( acc.value_bits() <= 32 * sizeof( Tr ))
                    pO[mi + Mi * mb] = acc.template to_vector<Tr>( param.shift_Qout );
                else {
                    auto chess_storage(x10) tmp = acc.template to_vector<typename aie::detail::utils::get_next_integer_type<Tr>::type>( param.shift_Qout );
                    pO[mi + Mi * mb] = aie::pack( tmp );
                }
            }
        }
        if constexpr( ifm_vector_load ) {
            pIv = add_3d_in1( pIv );
            pIv = add_3d_in2( pIv );
        } else {
            pI = add_3d_in1( pIb );
            pI = add_3d_in2( pI );
        }
        if constexpr( Mgran < Mtile_DM && strideS > 1 )
            pS = add_3d_sum( pS );
        else
            pS = add_2d_sum( pS );
        pQ = add_2d_qnt( pQ );
        pO = add_3d_out( pO );
    }
}


/*! \brief Scaling kernel for QDQ following a GEMM operation. Kernel implementation is tested and optimized for Tq=int16, Tr=int8, Mgran=16, Ngran=8, Mtile=8, Ntile=8, lr_min=6
    Compute: ofm = srs( ifm^{MxN} * coeff[2]^{1xN} + ifm_sum^{Mx1} * coeff[1]^{1xN} + ( coeff[0]^{1xN} << shift_Qb ), shift_out );

  @param[in] ifm        input to QDQ kernel (output of GEMM). Data order: C R C8 ( N M N8 )
  @param[in] ifm_sum    column sum values of ifms to GEMM kernel
  @param[in] coeffs     coefficients for QDQ compute. Data order: C coeff_step C8 ( N 4 N8 ). coeff_step = 4 while in 3 term mode only index 0, 1 and 2+coeff_skip are in use and in 2 term mode only index 0 and 1+coeff_skip are in use.
  @param[out] ofm       output of QDQ kernel in target precision (for next GEMM)
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel
 */
template<unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned terms, unsigned lr_min=4, unsigned coeff_step=4, unsigned coeff_skip=0, unsigned fp_accuracy_mode=1, typename Ti, typename Tq, typename Tr>
void qdq( Ti * ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<Tq> &coeffs, adf::output_buffer<Tr> &ofm, QDQParams &param ) {
    qdq<Ti, Tq, Tq, Tr, Mgran, Ngran, Mtile, Ntile, terms, lr_min, coeff_step, coeff_skip, fp_accuracy_mode>( ifm, ifm_sum, coeffs, ofm, param );
}

/*! \brief Scaling kernel for QDQ following a GEMM operation.
    Compute: ofm = srs( ifm^{MxN} * coeff[1]^{1xN} + ( coeff[0]^{1xN} << shift_Qb ), shift_out );

  @param[in] ifm        input to QDQ kernel (output of GEMM). Data order: C R C8 ( N M N8 )
  @param[in] coeffs     coefficients for QDQ compute. Data order: C coeff_step C8 ( N 4 N8 ). coeff_step = 4 while only index 0 and 1+coeff_skip are in use. If sizeof( Tq0 ) > sizeof( Tq ), then the upper indices are adjusted accordingly.
  @param[out] ofm       output of QDQ kernel in target precision (for next GEMM)
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel
 */
template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned lr_min=4, unsigned coeff_step=2, unsigned coeff_skip=0, unsigned fp_accuracy_mode=1>
void qdq( Ti * ifm, adf::input_buffer<Tq0> &coeffs, adf::output_buffer<Tr> &ofm, QDQParams &param ) {
    auto dummy = adf::input_buffer<Ti>({ ifm, 1, 0, 1 });
    qdq<Ti, Tq, Tq0, Tr, Mgran, Ngran, Mtile, Ntile, 2, lr_min, coeff_step, coeff_skip, fp_accuracy_mode>( ifm, dummy, coeffs, ofm, param );
}

/*! \brief Scaling kernel for QDQ following a GEMM operation.
    Compute: ofm = srs( ifm^{MxN} * coeff[1]^{1xN} + ( coeff[0]^{1xN} << shift_Qb ), shift_out );

  @param[in] ifm        input to QDQ kernel (output of GEMM). Data order: C R C8 ( N M N8 )
  @param[in] coeffs     coefficients for QDQ compute. Data order: C coeff_step C8 ( N 4 N8 ). coeff_step = 4 while only index 0 and 1+coeff_skip are in use.
  @param[out] ofm       output of QDQ kernel in target precision (for next GEMM)
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel
 */
template<unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned lr_min=4, unsigned coeff_step=2, unsigned coeff_skip=0, unsigned fp_accuracy_mode=1, typename Ti, typename Tq, typename Tr>
void qdq( Ti * ifm, adf::input_buffer<Tq> &coeffs, adf::output_buffer<Tr> &ofm, QDQParams &param ) {
    auto dummy = adf::input_buffer<Ti>({ ifm, 1, 0, 1 });
    qdq<Ti, Tq, Tq, Tr, Mgran, Ngran, Mtile, Ntile, 2, lr_min, coeff_step, coeff_skip, fp_accuracy_mode>( ifm, dummy, coeffs, ofm, param );
}




/*! \brief Scaling kernel for QDQ following a GEMM operation.
    Compute: ofm = srs( ifm^{MxN} * c2 + ifm_sum^{Mx1} * c1 + ( coeff[0] << shift_Qb ), shift_out );

  @param[in] ifm        input to QDQ kernel (output of GEMM). Data order: C R C8 ( N M N8 )
  @param[in] ifm_sum    column sum values of ifms to GEMM kernel
  @param[in] coeffs     coefficient 0 (channel vector) for QDQ compute. Data order: C coeff_step C8. coeff_step can be used here to operate in the same flow as other kernels e.g. ( N 4 N8 ). coeff_step = 4 while only index 0 is used.
  @param[in] c1         coefficient 1 (scalar) for QDQ compute. Unused if terms == 2
  @param[in] c2         coefficient 2 (scalar) for QDQ compute.
  @param[out] ofm       output of QDQ kernel in target precision (for next GEMM)
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel
 */
template<unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned terms, unsigned lr_min=4, unsigned coeff_step=1, unsigned fp_accuracy_mode=1, typename Ti, typename Tq, typename Tq0, typename Tr>
requires( QDQTerms<terms,3> )
INLINE_DECL void qdq( Ti * ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<Tq0> &coeff, Tq c1, Tq c2, adf::output_buffer<Tr> &ofm, QDQParams &param )
{
    constexpr unsigned Mtile_DM = 8;
    constexpr unsigned Mi = std::min( Mgran, Mtile_DM ) / Mtile;
    constexpr unsigned Mb = std::max( 1u, Mgran / Mtile_DM );
    constexpr unsigned Ni = Ngran / Ntile;
    constexpr unsigned Vo = Mtile * Ntile;
    constexpr unsigned Vb = Mgran * Ngran;
    constexpr unsigned strideS = sum_write_garbage_stride<Ti>();
    using Ti_op = std::conditional_t<std::is_integral_v<Ti> ^ std::is_integral_v<Tq>, Tq, Ti>;
    using acc_t = aie::detail::accum_tag_for_mul_types<Ti_op, Tq>;

#if 0
    if (((get_coreid() >> 16)==0) && ((get_coreid() & 0xF)==4)) {
        printf("Mi = %d, Mb = %d, Ni = %d, Vo = %d, Vb = %d, Mgran = %d, Mtile = %d, Ngran = %d, Ntile = %d\n", Mi, Mb, Ni, Vo, Vb, Mgran, Mtile, Ngran, Ntile);
    }
#endif

    constexpr bool int16_opt = std::is_same_v<Ti,int16> && std::is_same_v<Tq,int16> && std::is_same_v<Tq,Tq0> && terms == 3 && FAST_INT16_ACCUM_BROADCAST == 2;
    constexpr bool ifm_vector_load = std::is_same_v<Ti,Tr>;

    Add3dElem add_3d_in1( param.step0, param.wrap0, param.step1, param.wrap1, param.step2 );
    Add3dElem add_3d_in2( add_3d_in1, param.wrap2, param.step3, param.wrap3, param.step4 );
    Add2dElem add_2d_sum( Mgran * strideS, param.M_g * param.Y_g, 0 );
    Add3dElem add_3d_sum( Mgran, std::max( 1u, Mtile_DM / Mgran ), Mgran * strideS, param.M_g * param.Y_g, 0 );
    Add2dElem add_2d_qnt( 0, param.M_g * param.Y_g, coeff_step * Ngran );
    Add3dElem add_3d_out( Vb, param.M_g, Vb * param.M_g * param.N_g, param.Y_g, Vb * param.M_g );

    Ti __aie_dm_resource_a * pI = (Ti __aie_dm_resource_a *) ifm;
    auto pIv = aie::begin_vector<Vo, aie_dm_resource::a>( ifm );
    auto pS = aie::begin_vector<std::max( int16_opt ? Mgran * strideS : Mtile, 16 / unsigned(sizeof( Ti ))), aie_dm_resource::b>( ifm_sum );
    Tq0 __aie_dm_resource_b * pC0 = (Tq0 __aie_dm_resource_b *) coeff.data( );
    auto * restrict pOs = ofm.base();
    auto pO = aie::begin_restrict_vector<Vo, aie_dm_resource::c>( pOs );


    //printf("\n coeff c1=%i",c1);;

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE( lr_min, )
    for ( unsigned o=0; o<param.M_g*param.N_g*param.Y_g; o++ )
        
        chess_modulo_scheduling_budget_ratio(5000)
        
    {
        auto pIb = pI;
        #pragma unroll
        for (unsigned ni=0; ni<Ni; ni++) {
          auto ss = zip(( Ti )( 1 << param.shift_Qb ), *pS );
          pI = pIb;
          #pragma unroll
          for (unsigned mbc=0; mbc<Mb; mbc++) {
            unsigned mb = Mb - 1 - mbc;
            //unsigned mb = mbc;
            pI = pIb + param.step_Mb * mb;
            #pragma unroll
            for (unsigned mic=0; mic<Mi; mic++) {
                unsigned mi = Mi - 1 - mic;
                aie::vector<Ti,Vo> ifm;
                if constexpr( ifm_vector_load ) {
                    ifm = pIv[mi + param.step_Mb / Vo * mb + param.M_g * Mi * ni];
                } else {
                    //auto offset = Vo * mi + param.step_Mb * mb + param.M_g * Mgran * Ntile * ni;
                    auto offset = Vo * mi + param.M_g * Mgran * Ntile * ni;
                    ifm = load_index<Vo>( pI, offset );
                    #if 0
                    //if (((get_coreid() >> 16)==0) && ((get_coreid() & 0xF)==4) && (o==32)) {
                    //    printf("V0 = %d, param.M_g = %d, Mgran = %d, Ntile = %d, mi = %d, mic = %d, mbc = %d, ni = %d, o = %d, offset = %d, pi = %x\n", Vo, param.M_g, Mgran, Ntile, mi, mic, mbc, ni, o, offset, pI);
                    //    aie::print(aie::vector<Ti, Vo>(ifm), true, "ifm = ");
                    //}
                    #endif
                }
                aie::accum<acc_t,Vo> acc;
                if constexpr( int16_opt ) {
                    auto q01 = aie::concat( aie::load_v<Ntile>( pC0 + coeff_step * Ntile * ni ), aie::broadcast<Tq,Ntile>( c1 ));
                    aie::mmul<Mtile, 2, Ntile, Ti, Tq> mm;
                    mm.mul( ss.template extract<2*Mtile>( mi + Mi * mb * strideS ), q01 );
                    acc = mm.to_accum();
                } else {
                    if constexpr( std::is_same_v<int64,Tq0> ) {
                        static_assert( Ntile == 8 && Mtile == 4 ); //int64 not supported by AIE API, hardened:
                        aie::accum<acc_t,8> a((( v8acc64* )pC0 )[coeff_step * ni] );
                        auto b = locate_in_register<0>( aie::concat( a, a ));
                        acc = aie::concat( b, b );
                    } else {
                        using T2 = std::conditional_t<std::is_integral_v<Ti_op>, Ti_op, Tq0>;
                        //acc = accum_broadcast<Vo,Tq0,Ntile,T2>( aie::load_v<Ntile>( pC0 + coeff_step * Ntile * ni ), param.shift_Qb );
                        acc = accum_broadcast<Vo,Tq0,Ntile,T2>( aie::load_v<Ntile>( pC0 + coeff_step * Ntile * ni ), param.shift_Qb );
                        #if 0
                        //if (((get_coreid() >> 16)==0) && ((get_coreid() & 0xF)==4)) {
                        //    printf("pC0 = %x ",  pC0 + coeff_step * Ntile * ni);
                        //    aie::print((aie::load_v<Ntile>( pC0 + coeff_step * Ntile * ni )), true, "pC0* = ");
                            //aie::print(aie::accum<acc_t, Vo>(acc), true, "acc = ");
                        //}
                        #endif
                    }
                    if constexpr( terms == 3 )
                        acc = mac_outer_prod<Ti_op,Mtile,Tq,Ntile,acc_t,fp_accuracy_mode>( acc, convert<Ti_op>( pS[mi + Mi * mb * strideS] ), aie::broadcast<Tq,Ntile>( c1 ));
                }
                acc = mac_elew<Ti_op,Vo,Tq,acc_t,fp_accuracy_mode>( acc, convert<Ti_op>( ifm ), aie::broadcast<Tq,Vo>( c2 ));
                if constexpr( std::is_integral_v<Tr> ^ std::is_integral_v<Tq> ) {
                    pO[mi + Mi * mb + param.M_g * Mi * ni] = convert<Tr>( acc.template to_vector<Tq>( ));
                } else if constexpr( acc.value_bits() <= 32 * sizeof( Tr )) {
                    pO[mi + Mi * mb + param.M_g * Mi * ni] = acc.template to_vector<Tr>( param.shift_Qout );
                } else {
                    auto chess_storage(x10) tmp = acc.template to_vector<typename aie::detail::utils::get_next_integer_type<Tr>::type>( param.shift_Qout );
                    pO[mi + Mi * mb + param.M_g * Mi * ni] = aie::pack( tmp );
                    #if 0
                    //if (((get_coreid() >> 16)==0) && ((get_coreid() & 0xF)==4))
                    //    aie::print(aie::vector<Tr, Vo>(pO[mi + Mi * mb + param.M_g * Mi * ni]), true, "ofm = ");
                    #endif
                }
            }
          }
        }
        if constexpr( ifm_vector_load ) {
            pIv = add_3d_in1( pIv );
            pIv = add_3d_in2( pIv );
        } else {
            pI = add_3d_in1( pIb );
            pI = add_3d_in2( pI );
        }
        if constexpr( Mgran < Mtile_DM && strideS > 1 )
            pS = add_3d_sum( pS );
        else
            pS = add_2d_sum( pS );
        pC0 = add_2d_qnt( pC0 );
        pO = add_3d_out( pO );
    }
}

/*! \brief Scaling kernel for QDQ following a GEMM operation.
    Compute: ofm = srs( ifm^{MxN} * c1 + ( coeff[0] << shift_Qb ), shift_out );

  @param[in] ifm        input to QDQ kernel (output of GEMM). Data order: C R C8 ( N M N8 )
  @param[in] coeffs     coefficient 0 (channel vector) for QDQ compute. Data order: C coeff_step C8. coeff_step can be used here to operate in the same flow as other kernels e.g. ( N 4 N8 ). coeff_step = 4 while only index 0 is used.
  @param[in] c1         coefficient 1 (scalar) for QDQ compute
  @param[out] ofm       output of QDQ kernel in target precision (for next GEMM)
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel
 */
template<unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned lr_min, unsigned coeff_step=1, unsigned fp_accuracy_mode=1, typename Ti, typename Tq, typename Tq0, typename Tr>
void qdq( Ti * ifm, adf::input_buffer<Tq0> &c0, Tq c1, adf::output_buffer<Tr> &ofm, QDQParams &param ) {
    auto dummy = adf::input_buffer<Ti>({ ifm, 1, 0, 1 });
    qdq<Mgran, Ngran, Mtile, Ntile, 2, lr_min, coeff_step, fp_accuracy_mode>( ifm, dummy, c0, c1, c1, ofm, param );
}



/*! \brief Scaling kernel for standalone QDQ oprtaions.
    Compute: ofm = srs( ifm^{MxN} * c1 + ( c0 << shift_Qb ), shift_out );

  @param[in] ifm        input to QDQ kernel (output of GEMM). Data order: C R C8 ( N M N8 )
  @param[in] c0         coefficient 0 (scalar) for QDQ compute.
  @param[in] c1         coefficient 1 (scalar) for QDQ compute.
  @param[out] ofm       output of QDQ kernel in target precision (for next GEMM)
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel
 */
template<unsigned Mgran, unsigned Mtile, unsigned Ntile, unsigned lr_min=4, unsigned fp_accuracy_mode=1, typename Ti, typename Tq, typename Tq0, typename Tr>
INLINE_DECL void qdq( Ti * ifm, Tq0 c0, Tq c1, adf::output_buffer<Tr> &ofm, QDQParams &param )
{
    constexpr unsigned Mtile_DM = 8;
    constexpr unsigned Mi = std::min( Mgran, Mtile_DM ) / Mtile;
    constexpr unsigned Mb = std::max( 1u, Mgran / Mtile_DM );
    constexpr unsigned Vo = Mtile * Ntile;
    constexpr unsigned Vb = Mgran * Ntile;
    constexpr unsigned strideS = sum_write_garbage_stride<Ti>();
    using Ti_op = std::conditional_t<std::is_integral_v<Ti> ^ std::is_integral_v<Tq>, Tq, Ti>;
    using acc_t = aie::detail::accum_tag_for_mul_types<Ti_op, Tq>;

    constexpr bool ifm_vector_load = std::is_same_v<Ti,Tr>;

    Add3dElem add_3d_in1( param.step0, param.wrap0, param.step1, param.wrap1, param.step2 );
    Add3dElem add_3d_in2( add_3d_in1, param.wrap2, param.step3, param.wrap3, param.step4 );
    Add3dElem add_3d_out( Vb, param.M_g, Vb * param.M_g * param.N_g, param.Y_g, Vb * param.M_g );

    Ti __aie_dm_resource_a * pI = (Ti __aie_dm_resource_a *) ifm;
    auto pIv = aie::begin_vector<Vo, aie_dm_resource::a>( ifm );
    auto * restrict pOs = ofm.base();
    auto pO = aie::begin_restrict_vector<Vo, aie_dm_resource::c>( pOs );
    //printf("\n 2 coeff c1=%i",c1);

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE( lr_min, )
    for ( unsigned o=0; o<param.M_g*param.N_g*param.Y_g; o++ )
        
        
    {
        auto pIb = pI;
        #pragma unroll
        for (unsigned mbc=0; mbc<Mb; mbc++) {
            unsigned mb = Mb - 1 - mbc;
            //unsigned mb = mbc;
            pI = pIb + param.step_Mb * mb;
            #pragma unroll
            for (unsigned mic=0; mic<Mi; mic++) {
                unsigned mi = Mi - 1 - mic;
                aie::vector<Ti,Vo> ifm;
                //aie::print(aie::vector<Ti, Vo>(ifm), true, "ifm = ");
                if constexpr( ifm_vector_load ) {
                    ifm = pIv[mi + param.step_Mb / Vo * mb];
                } else {
                    ifm = load_index<Vo>( pI, Vo * mi );
                }
                aie::accum<acc_t,Vo> acc;
                if constexpr( std::is_same_v<int64,Tq0> ) {
                    static_assert( Ntile == 8 ); //int64 not supported by AIE API, hardened:
                    aie::accum<acc_t,8> a(( v8acc64 ) broadcast_s64( c0 ));
                    auto b = locate_in_register<0>( aie::concat( a, a ));
                    acc = aie::concat( b, b );
                } else if constexpr( sizeof( Tq0 ) * Vo > 128 ) {
                    aie::accum<acc_t,Vo/4> a( aie::broadcast<Tq0,Vo/4>( c0 ), param.shift_Qb );
                    auto b = aie::concat( a, a );
                    acc = aie::concat( b, b );
                } else
                    acc.from_vector( aie::broadcast<Tq0,Vo>( c0 ), param.shift_Qb );
                acc = mac_elew<Ti_op,Vo,Tq,acc_t,fp_accuracy_mode>( acc, convert<Ti_op>( ifm ), aie::broadcast<Tq,Vo>( c1 ));
                if constexpr( std::is_integral_v<Tr> ^ std::is_integral_v<Tq> )
                    pO[mi + Mi * mb] = convert<Tr>( acc.template to_vector<Tq>( ));
                else if constexpr( acc.value_bits() <= 32 * sizeof( Tr ))
                    pO[mi + Mi * mb] = acc.template to_vector<Tr>( param.shift_Qout );
                else {
                    auto chess_storage(x10) tmp = acc.template to_vector<typename aie::detail::utils::get_next_integer_type<Tr>::type>( param.shift_Qout );
                    pO[mi + Mi * mb] = aie::pack( tmp );
                }
            }
        }
        if constexpr( ifm_vector_load ) {
            pIv = add_3d_in1( pIv );
            pIv = add_3d_in2( pIv );
        } else {
            pI = add_3d_in1( pIb );
            pI = add_3d_in2( pI );
        }
        pO = add_3d_out( pO );
    }
}


#if HAS_FLOAT && __AIE_ARCH__ >= 21
#include "qdq_v64float.hpp"
#endif

/*! \brief Scaling kernel for the second activation * activation sum (1xN shape) and combining with C0. Implementation tailored for Ti=int32, Tq=int32, Tq0=int64, Ngran=8/16/32, Ntile=8, lr_min=4
    Compute: c0_out = srs( sum_out^{1xN} * coeff[1]^{1xN} + ( coeff[0]^{1xN} << shift_Qb ), shift_Qb );

  @param[in] ifm        input to QDQ kernel (output of GEMM). Data order: C R C8 ( N M N8 )
  @param[in] sum_out    row sum values of second activations to GEMM kernel
  @param[in] coeffs     coefficients for QDQ compute. Data order: C coeff_step C8 ( N 4 N8 ). coeff_step = 4
  @param[out] c0_out    output of QDQ kernel in target precision (for next GEMM; can be inplace if coeff_step_out = coeff_step)
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel (can be shared with QDQ kernel if Ngran=8)
 */
template<typename Ti, typename Tq, typename Tq0, unsigned Ngran, unsigned Ntile, unsigned lr_min=4, unsigned coeff_step_out=1, unsigned coeff_step=4, unsigned coeff_skip=0, unsigned fp_accuracy_mode=1>
INLINE_DECL void qdq_sum_to_c0( adf::input_buffer<Ti> &sum_out, adf::input_buffer<Tq> &coeffs, adf::output_buffer<Tq> &c0_out, QDQParams &param )
{
    constexpr unsigned strideS = sum_write_garbage_stride<Ti>();
    constexpr unsigned offset_c1 = std::max( 1u, unsigned(sizeof( Tq0 )) / unsigned(sizeof( Tq )) + coeff_skip ) * Ntile;
    constexpr unsigned Vo = std::is_same_v<int64,Tq0> ? Ntile * 2 : Ngran;
    using Ti_op = std::conditional_t<std::is_integral_v<Ti> ^ std::is_integral_v<Tq>, Tq, Ti>;
    using acc_t = aie::detail::accum_tag_for_mul_types<Ti_op, Tq>;
    auto pS = (Ti __aie_dm_resource_b *) sum_out.base();
    Tq __aie_dm_resource_c * pQ0 = (Tq __aie_dm_resource_c *) __aie_copy( coeffs.base( ));
    Tq __aie_dm_resource_c * pQ1 = (Tq __aie_dm_resource_c *) __aie_copy( coeffs.base( ));
    auto * restrict pOs = c0_out.base();
    auto pO = aie::begin_restrict_vector<Vo, aie_dm_resource::b>( pOs );
    //printf("\n 3 coeff c1=%i",c1);

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE( lr_min, )
    for ( unsigned o=0; o<param.N_g; o++ )
        
        
    {
        aie::accum<acc_t,Ngran> acc;
        aie::vector<Ti,Ngran> sm;
        aie::vector<Tq,Ngran> c1;
        #pragma unroll
        for (unsigned ni=0; ni<Ngran/Ntile; ni++) {
            if constexpr( std::is_same_v<int64,Tq0> ) {
                static_assert( Ntile == 8 ); //int64 not supported by AIE API, hardened:
                v8acc64 tmp = *( v8acc64 __aie_dm_resource_c *)pQ0;
                acc = acc.insert( ni, aie::accum<acc_t,8>( tmp ));
            } else {
                acc = acc.insert( ni, aie::accum<acc_t,Ntile>( aie::load_v<Ntile>(( Tq0* ) pQ0 ), param.shift_Qb ));
            }
            c1 = c1.insert( ni, aie::load_v<Ntile>( pQ1 + offset_c1 ));
            sm = sm.insert( ni, aie::load_v<Ntile>( pS ));
            pS += Ntile * strideS;
            pQ0 += Ntile * coeff_step;
            pQ1 += Ntile * coeff_step;
        }

        acc = aie::mac( acc, sm, c1 );

        #pragma unroll
        for (unsigned ni=0; ni<Ngran/Ntile; ni++) {
            if constexpr( std::is_same_v<int64,Tq0> ) { //int64 not supported by AIE API, hardened:
                *pO = acc.template extract<Ntile>( ni ).template cast_to<acc32>( ).template to_vector<int32>( );
            } else
                *pO = acc.template to_vector<Tq0>( param.shift_Qb );
            pO = byte_incr( pO, Ntile * sizeof( Tq ) * coeff_step_out );
        }
    }
}

/*! \brief Scaling kernel for the second activation * activation sum (1xN shape) and combining with C0. Implementation tailored for Ti=int32, Tq=int32, Tq0=int64, Ngran=8/16/32, Ntile=8, lr_min=4
    Compute: c0_out = srs( sum_out^{1xN} * coeff[1]^{1xN} + ( coeff[0]^{1xN} << shift_Qb ), shift_Qb );

  @param[in] ifm        input to QDQ kernel (output of GEMM). Data order: C R C8 ( N M N8 )
  @param[in] sum_out    row sum values of second activations to GEMM kernel
  @param[in] coeffs     coefficients for QDQ compute. Data order: C coeff_step C8 ( N 4 N8 ). coeff_step = 4
  @param[out] c0_out    output of QDQ kernel in target precision (for next GEMM; can be inplace if coeff_step_out = coeff_step)
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel (can be shared with QDQ kernel if Ngran=8)
 */
template<typename Ti, typename Tq, typename Tq0, unsigned Ngran, unsigned Ntile, unsigned lr_min=4, unsigned fp_accuracy_mode=1>
INLINE_DECL void qdq_sum_to_c0( adf::input_buffer<Ti> &sum_out, Tq0 c0, Tq c1, adf::output_buffer<Tq> &c0_out, QDQParams &param )
{
    constexpr unsigned strideS = sum_write_garbage_stride<Ti>();
    constexpr unsigned Vo = std::is_same_v<int64,Tq0> ? Ntile * 2 : Ngran;
    using Ti_op = std::conditional_t<std::is_integral_v<Ti> ^ std::is_integral_v<Tq>, Tq, Ti>;
    using acc_t = aie::detail::accum_tag_for_mul_types<Ti_op, Tq>;
    auto pS = (Ti __aie_dm_resource_b *) sum_out.base();
    auto * restrict pOs = c0_out.base();
    auto pO = aie::begin_restrict_vector<Vo, aie_dm_resource::b>( pOs );
    //v16acc64* pO = (v16acc64*)pOs;

    //printf("\n 3 coeff c1=%i",c1);

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE( lr_min, )
    for ( unsigned o=0; o<param.N_g; o++ )
        
        
    {
        aie::accum<acc_t,Ngran> acc;
        if constexpr( std::is_same_v<int64,Tq0> ) {
            static_assert( Ntile == 8 ); //int64 not supported by AIE API, hardened:
            aie::accum<acc_t,8> a(( v8acc64 ) broadcast_s64( c0 ));
            //acc = aie::concat( a, a );
            if constexpr( Ngran == Ntile ) acc = a;
            else if constexpr( Ngran == 2*Ntile ) acc = aie::concat( a, a );
            else {
                auto b = aie::concat( a, a );
                acc = aie::concat( b, b );
            }
        } else if constexpr( sizeof( Tq0 ) * Ngran > 128 ) {
            aie::accum<acc_t,Ngran/4> a( aie::broadcast<Tq0,Ngran/4>( c0 ), param.shift_Qb );
            auto b = aie::concat( a, a );
            acc = aie::concat( b, b );
        } else
            acc.from_vector( aie::broadcast<Tq0,Ngran>( c0 ), param.shift_Qb );
        aie::vector<Ti,Ngran> sm;
        #pragma unroll
        for (unsigned ni=0; ni<Ngran/Ntile; ni++) {
            sm = sm.insert( ni, aie::load_v<Ntile>( pS ));
            pS += Ntile * strideS;
        }
        acc = aie::mac( acc, sm, aie::broadcast<Tq,Ngran>( c1 ));

        #pragma unroll
        for (unsigned ni=0; ni<Ngran/Ntile; ni++) {
            if constexpr( std::is_same_v<int64,Tq0> ) { //int64 not supported by AIE API, hardened:
                *pO++ = acc.template extract<Ntile>( ni ).template cast_to<acc32>( ).template to_vector<int32>( );
                //*pO++ = acc.to_native();
            } else
                *pO++ = acc.template to_vector<Tq0>( param.shift_Qb );
        }
    }
}


/*! \brief Sum term for scaling kernel for QDQ following a GEMM operation.
    Compute: ofm = srs( ifm_sum^{Mx1} * coeff[coeff_idx]^{1xN} + ifm^{MxN}, shift_out );

  @param[in] ifm        input to QDQ kernel (from previous terms). Data order: C R C8 ( N M N8 )
  @param[in] ifm_sum    column sum values of ifms to GEMM kernel
  @param[in] coeffs     coefficients for QDQ compute. Data order: C N_coeffs C8 ( N 4 N8 ). N_coeffs = 4 while only one index is in use given by coeff_idx.
  @param[out] ofm       output of QDQ kernel in target precision (for next GEMM) or in ifm precision to connect more terms
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel
  @param[in] coeff_idx  Index used for scaling term. Allows to use same kernel instance for multiple terms.
 */
template<typename Ti, typename Ts, typename Tq, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned lr_min, unsigned fp_accuracy_mode=1>
void qdq_term_outer( Ti * ifm, adf::input_buffer<Ts> &ifm_sum, adf::input_buffer<Tq> &coeffs, adf::output_buffer<Tr> &ofm, QDQParams &param, unsigned coeff_idx, unsigned coeff_step=4 )
{
    constexpr unsigned Mtile_DM = 8;
    constexpr unsigned Mi = std::min( Mgran, Mtile_DM ) / Mtile;
    constexpr unsigned Mb = std::max( 1u, Mgran / Mtile_DM );
    constexpr unsigned Ni = Ngran / Ntile;
    constexpr unsigned Vo = Mtile * Ntile;
    constexpr unsigned Vb = Mgran * Ngran;
    constexpr unsigned strideS = sum_write_garbage_stride<Ti>();
    using Ts_op = std::conditional_t<std::is_integral_v<Ts> ^ std::is_integral_v<Tq>, Tq, Ts>;
    using acc_t = aie::detail::accum_tag_for_mul_types<Ts_op, Tq>;

    constexpr bool ifm_vector_load = false;//std::is_same_v<Ti,Tr>;

    Add3dElem add_3d_in1( param.step0, param.wrap0, param.step1, param.wrap1, param.step2 );
    Add3dElem add_3d_in2( add_3d_in1, param.wrap2, param.step3, param.wrap3, param.step4 );
    Add2dElem add_2d_sum( Mgran * strideS, param.M_g * param.Y_g, 0 );
    Add3dElem add_3d_sum( Mgran, std::max( 1u, Mtile_DM / Mgran ), Mgran * strideS, param.M_g * param.Y_g, 0 );
    Add2dElem add_2d_qnt( 0, param.M_g * param.Y_g, coeff_step * Ngran );
    //Add3dElem add_3d_out( Vb, param.M_g, Vb * param.M_g * param.N_g, param.Y_g, Vb * param.M_g );
    Add3dElem add_3d_out1( param.step0, param.wrap0, param.step1, param.wrap1, param.step2 );
    Add3dElem add_3d_out2( add_3d_out1, param.wrap2, param.step3, param.wrap3, param.step4 );

    Ti __aie_dm_resource_a * pI = (Ti __aie_dm_resource_a *) ifm;
    auto pIv = aie::begin_vector<Vo>( ifm );
    auto pS = aie::begin_vector<Mtile>( ifm_sum );
    auto pQ = aie::begin_vector<Ntile>( coeffs );
    auto * restrict pOs = ofm.base();
    auto pO = aie::begin_restrict_vector<Vo>( pOs );

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE( lr_min, )
    for ( unsigned o=0; o<param.M_g*param.N_g*param.Y_g; o++ )
        
        chess_modulo_scheduling_budget_ratio(5000)
        
    {
        auto pIb = pI;
        #pragma unroll
        for (unsigned ni=0; ni<Ni; ni++) {
          #pragma unroll
          for (unsigned mbc=0; mbc<Mb; mbc++) {
            unsigned mb = Mb - 1 - mbc;
            //unsigned mb = mbc;
            pI = pIb + param.step_Mb * mb;
            #pragma unroll
            for (unsigned mic=0; mic<Mi; mic++) {
                unsigned mi = Mi - 1 - mic;
                //aie::vector<Ti,Vo> ifm;
                aie::accum<acc_t,Vo> ifm;
                //if constexpr( ifm_vector_load ) {
                //    ifm = pIv[mi + param.step_Mb / Vo * mb + param.M_g * Mi * ni];
                //} else {
                    //auto offset = Vo * mi + param.step_Mb * mb + param.M_g * Mgran * Ntile * ni;
                    pI = chess_copy( pI );
                    auto offset = Vo * mi + param.M_g * Mgran * Ntile * ni;
                    ifm = load_accum<acc_t,Vo>( pI, offset );
                //}
                auto q = pQ[coeff_step*ni+coeff_idx];
                auto acc = mac_outer_prod<Ts_op,Mtile,Tq,Ntile,acc_t,fp_accuracy_mode>( aie::accum<acc_t,Vo>( ifm ), convert<Ts_op>( pS[mi + Mi * mb * strideS] ), q );
                if constexpr( std::is_integral_v<Tr> ^ std::is_integral_v<Tq> )
                    pO[mi + Mi * mb + param.M_g * Mi * ni] = convert<Tr>( acc.template to_vector<Tq>( ));
                else if constexpr( acc.value_bits() <= 32 * sizeof( Tr ))
                    pO[mi + Mi * mb + param.M_g * Mi * ni] = acc.template to_vector<Tr>( param.shift_Qout );
                else {
                    auto chess_storage(x10) tmp = acc.template to_vector<typename aie::detail::utils::get_next_integer_type<Tr>::type>( param.shift_Qout );
                    pO[mi + Mi * mb + param.M_g * Mi * ni] = aie::pack( tmp );
                }
            }
          }
          if constexpr( Mb > 1 ) pI = pIb;
        }
        //if constexpr( ifm_vector_load ) {
        //    pIv = add_3d_in1( pIv );
        //    pIv = add_3d_in2( pIv );
        //} else {
            pI = add_3d_in1( pI );
            pI = add_3d_in2( pI );
        //}
        pQ = add_2d_qnt( pQ );
        if constexpr( Mgran < Mtile_DM && strideS > 1 )
            pS = add_3d_sum( pS );
        else
            pS = add_2d_sum( pS );
        pO = add_3d_out1( pO );
        //pO = add_3d_out2( pO );
    }
}


static void dq_dynamic( int8 * in, bool is_int16, int16 zp, bfloat16 scale, bfloat16 * restrict out, unsigned elements ) {
    constexpr unsigned N = 64;
    int8 * pI = in;
    bfloat16 * pO = out;;

    bfloat16 magicf = as_float( 0xcbc00000 );
    int32 magici = 0x4bc00000 - zp;
    aie::accum<acc32,N> magici_a = aie::concat( aie::accum<acc32,32>( aie::broadcast<int32,32>( magici )), aie::accum<acc32,32>( aie::broadcast<int32,32>( magici )));

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE(4,)
    for (unsigned i = 0; i < elements/N; ++i)
        
         //4 as performance PM trade-off
    {
        aie::vector<int16,N> inp_vec = aie::select( aie::unpack( aie::load_v<N>( pI )), aie::load_v<N>(( int16 * ) pI ), aie::mask<N>( is_int16 ));
        aie::accum<acc32,N> acc = aie::mac( magici_a, inp_vec, int16( 1 ));
        aie::accum<accfloat,N> af = aie::add( acc.cast_to<accfloat>( ), magicf );
        aie::vector<bfloat16,N> vec_bf16 = af.to_vector<bfloat16>( );
        vec_bf16 = aie::mul( vec_bf16, scale ).to_vector<bfloat16>( );
        aie::store_v( pO, vec_bf16 );
        pI += N << is_int16;
        pO += N;
    }
}



static void __attribute__((noinline)) q_dynamic( bfloat16 * in, bfloat16 scale, int16 zp, bool is_int16, int8 * restrict out, unsigned elements ) {
    constexpr unsigned N = 64;
    bfloat16 * pI = in;
    int8 * pO = out;
    bfloat16 magicf = as_float( 0xcbc00000 );
    int32 magici = 0x4bc00000 + zp;
    aie::accum<acc32,N> magici_a = aie::concat( aie::accum<acc32,32>( aie::broadcast<int32,32>( magici )), aie::accum<acc32,32>( aie::broadcast<int32,32>( magici )));
    aie::accum<accfloat,N> magicf_a( aie::broadcast<bfloat16,N>( magicf ));

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE(4,)
    for (unsigned i = 0; i < elements/N; ++i)
        
         //4 as performance PM trade-off
    {
        aie::vector<int16,N> vec = aie::add( aie::mac( magicf_a, aie::load_v<N>( pI ), scale ).cast_to<acc32>( ), magici_a ).to_vector<int16>( );
        aie::vector<int8,N> vec8 = aie::pack( vec );
        vec = aie::select( vec8.cast_to<int16>( ).grow_replicate<N>( ), vec, aie::mask<N>( is_int16 ));
        aie::store_v( pO + N*is_int16, vec.cast_to<int8>().extract<N>( 1 ));
        aie::store_v( pO, vec.cast_to<int8>().extract<N>( 0 ));
        pI += N;
        pO += N << is_int16;
    }
}


/*! \brief Convert input buffer Ti to output buffer To.

  @param[in] ifm    input of type Ti
  @param[out] ofm   output of type To
  @param[in] iters  Number of tiles contained in the buffers
  @param[in] shift  Shift factor for conversion
 */
template<typename Ti, typename To, unsigned tile, unsigned lr_min>
void convert( adf::input_buffer<Ti> &ifm, adf::output_buffer<To> &ofm, unsigned iters, int shift=0 ) {
    auto sat_mode = get_satmode();
    set_sat();
    auto pI = aie::begin_vector<tile>( ifm );
    auto pO = aie::begin_restrict_vector<tile>( ofm );
    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE( lr_min, )
    for ( unsigned i = 0; i < iters; i++ )
        
        
    {
        *pO++ = convert<To>( *pI++, shift );
    }
    set_satmode( sat_mode );
}
template<typename Ti, typename To, unsigned tile, unsigned lr_min>
void convert( Ti * ifm, To * ofm, unsigned iters, int shift=0 ) {
    auto sat_mode = get_satmode();
    set_sat();
    auto pI = aie::begin_vector<tile>( ifm );
    auto pO = aie::begin_restrict_vector<tile>( ofm );
    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_RANGE( lr_min, )
    for ( unsigned i = 0; i < iters; i++ )
        
        
    {
        *pO++ = convert<To>( *pI++, shift );
    }
    set_satmode( sat_mode );
}



/*! \brief Scaling kernel for QDQ following a GEMM operation. Kernel calls convert to operate with data type Tq as a standalone kernel to limit the complexity of each loop
    Compute: ofm = convert<Tr>( convert<Tq>( ifm^{MxN} ) * coeff[2]^{1xN} + convert<Tq>( ifm_sum^{Mx1} ) * coeff[1]^{1xN} + coeff[0]^{1xN} );

  @param[in] ifm        input to QDQ kernel (output of GEMM). Data order: C R C8 ( N M N8 )
  @param[in] ifm_sum    column sum values of ifms to GEMM kernel
  @param[in] coeffs     coefficients for QDQ compute. Data order: C N_coeffs C8 ( N 4 N8 ). N_coeffs = 4 while only index 0, 1 and 2 are in use.
  @param[out] ofm       output of QDQ kernel in target precision (for next GEMM)
  @param[in] param      QDQParam structure for runtime parameters and addressing information of the kernel
 */
template<typename Ti, typename Tq, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned convert_tile, unsigned terms, unsigned lr_min, unsigned fp_split_threshold=2, unsigned fp_accuracy_mode=1>
__attribute__((noinline)) void qdq_split( adf::input_buffer<Ti> &ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<Tq> &coeffs, adf::output_buffer<Tr> &ofm, QDQParams &param )
{
    unsigned len_ifm = param.M_g * param.Y_g * param.N_g * Mtile * Ntile;
    unsigned len_sum = param.M_g * param.Y_g * Mtile;
    auto ifm_Tq_in  = local_buffer_cast<Tq,adf::direction::in>(  ifm );
    auto ifm_Tq_out = local_buffer_cast<Tq,adf::direction::out>( ifm );
    auto sum_Tq_in  = local_buffer_cast<Tq,adf::direction::in>(  ifm_sum );
    auto sum_Tq_out = local_buffer_cast<Tq,adf::direction::out>( ifm_sum );
    convert<Ti, Tq, convert_tile, lr_min>( ifm, ifm_Tq_out, len_ifm / convert_tile, 0 );

    if constexpr( terms == 2 ) {
         qdq<Tq, Tq, Mgran, Ngran, Mtile, Ntile, lr_min, fp_accuracy_mode>( ifm_Tq_in.data( ), coeffs, ifm_Tq_out, param );

    } else if constexpr( terms == 3 ) {
        convert<Ti, Tq, convert_tile, 1>( ifm_sum, sum_Tq_out, std::max( 1u, len_sum / convert_tile ), 0 );

        if constexpr( sizeof( Tq ) <= fp_split_threshold ) {
            qdq<Tq, Tq, Mgran, Ngran, Mtile, Ntile, 3, lr_min, fp_accuracy_mode>( ifm_Tq_in.data( ), sum_Tq_in, coeffs, ifm_Tq_out, param );
        } else {
            qdq<Tq, Tq, Mgran, Ngran, Mtile, Ntile, lr_min, fp_accuracy_mode, 1>( ifm_Tq_in.data( ), coeffs, ifm_Tq_out, param );
            qdq_term_outer<Tq, Tq, Tq, Tq, Mgran, Ngran, Mtile, Ntile, lr_min, fp_accuracy_mode>( ifm_Tq_in.data( ), sum_Tq_in, coeffs, ifm_Tq_out, param, 1 );
        }
    }

    convert<Tq, Tr, convert_tile, lr_min>( ifm_Tq_in, ofm, len_ifm / convert_tile, 0 );
}
#endif
