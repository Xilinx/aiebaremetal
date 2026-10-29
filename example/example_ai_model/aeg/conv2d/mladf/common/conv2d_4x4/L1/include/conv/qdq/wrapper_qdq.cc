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

#include <adf.h>
#include <aie_api/aie.hpp>
#include "./common/constants_main.h"
#include "wrapper_qdq.hpp"
#include "qdq_sum.hpp"
#include "qdq.cc"

template<typename Ta, typename Ts, unsigned Mgran, unsigned Kgran, unsigned Mtile, unsigned Ktile, unsigned inner_lr_min, unsigned outer_lr_min>
__attribute__((noinline)) void sum_top( adf::input_buffer<Ta> &ifm, adf::output_buffer<Ts> &ofm, unsigned address_config ) {
    const unsigned Y = ( address_config >= ADDRCFG_YCXC8 ? conv_Y : 1 );
    sum_inner<Ta, Ts, Mgran, Kgran, Mtile, Ktile, inner_lr_min, outer_lr_min>( ifm, ofm, gemm_M / Y / Mgran, gemm_K / Kgran, Y, 1, 0 );
}



template<typename Tq, unsigned Ntile, unsigned terms>
void convert_coefficients( float * in, Tq * out, int * shift, unsigned N, int shift_gemm=0 ) {
    auto pQ = aie::begin_vector<qdq_coeffs*Ntile>(( int32* ) in);
    auto mv = aie::bit_and( *pQ++, aie::broadcast<int32,qdq_coeffs*Ntile>( 0x7fffffff ));
    for ( unsigned i = 1; i < N / Ntile; i++ ) {
        //mv = aie::max( mv, aie::abs( *pQ++ ).template cast_to<int32>( ));
        mv = aie::max( mv, aie::bit_and( *pQ++, aie::broadcast<int32,qdq_coeffs*Ntile>( 0x7fffffff )));
    }
    for ( unsigned i = 0; i < terms; i++ ) {
        int32 val = aie::reduce_max( mv.template extract<Ntile>( i ));
        shift[i] = 127 - (( val >> 23 ) & 255 ) + ( 8 * sizeof( Tq ) - 2 );
    }

    if ( terms == 3 ) {
        shift[1] = std::min( shift[1], shift[2] - shift_gemm );
        shift[2] = shift[1] + shift_gemm;
    }

  #if HAS_FLOAT
    auto pQi = aie::begin_vector<Ntile>( in );
    auto pQo = aie::begin_restrict_vector<Ntile>( out );
    AIE_LOOP_RANGE( 1, ) 
    for ( unsigned t = 0; t < N / Ntile; t++ ){
        #pragma unroll
        for ( unsigned i = 0; i < terms; i++ ) {
            shift[i] = chess_copy( shift[i] );
            pQo[i] = aie::to_fixed<Tq>( pQi[i], shift[i] );
        }
        pQi += qdq_coeffs;
        pQo += qdq_coeffs;
    }
  #else
    auto pQi = in;
    auto pQo = out;
    AIE_LOOP_RANGE( 1, ) 
    for ( unsigned t = 0; t < N / Ntile; t++ ){
        for ( unsigned i = 0; i < terms; i++ ) {
            float s = as_float(( 127 + shift[i] ) << 23 );
            for ( unsigned n = 0; n < Ntile; n++ ) {
                pQo[i * Ntile + n] = pQi[i * Ntile + n] * s;
            }
        }
        pQi += qdq_coeffs * Ntile;
        pQo += qdq_coeffs * Ntile;
    }
  #endif
}
 

   
template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned vector_coeffs, unsigned fp_accuracy_mode, unsigned fp_split_threshold>
inline __attribute__((noinline)) void wrapper_sym( adf::input_buffer<Ti> &ifm, adf::input_buffer<Tq> &coeffs, adf::output_buffer<Tr> &ofm, QDQParams &param)
{
    adf::input_buffer<Ti> dummy({ ( Ti* ) ifm.data( ), 1, 0, 1 });
    wrapper_asym<Ti, Tq, Tq0, Tr, Mgran, Ngran, Mtile, Ntile, vector_coeffs, fp_accuracy_mode, fp_split_threshold, qdq_terms>( ifm, dummy, coeffs, ofm, param);
}


template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned vector_coeffs, unsigned fp_accuracy_mode, unsigned fp_split_threshold, unsigned terms>
inline __attribute__((noinline)) void wrapper_asym( adf::input_buffer<Ti> &ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<Tq> &coeffs_c, adf::output_buffer<Tr> &ofm, QDQParams &param)
{
    constexpr unsigned lr_min = 6;
    auto coeffs = local_buffer( coeffs_c, 64 / sizeof( Tq ));
    auto ifm_Ti = (Ti*) ifm.data( );
    static unsigned iteration=0;
    
    auto config = ( int32* ) coeffs_c.data( );
    //auto M = config[0];
    //auto N = config[1];
    set_rnd( rnd_conv_even );
    //auto address_config = config[2];
    //auto shift_Qb   = config[4];
    auto shift_Qout = config[6];
    Tq0 c0 = *( Tq0* )( config + 8 );
    Tq c1 = config[10];
    Tq c2 = config[11];
    //Tq  c1 = (( Tq*  )( config + 8 ))[sizeof( Tq0 ) / sizeof( Tq )];
    //Tq  c2 = (( Tq*  )( config + 8 ))[terms - 2 + sizeof( Tq0 ) / sizeof( Tq )];
    
    //param.Y_g = ( address_config >= ADDRCFG_YCXC8 ? conv_Y : 1 );
    //param.M_g = M / param.Y_g / Mgran;
    //param.N_g = N / Ngran;
    QDQParams param_loc = param;
    //param_loc.shift_Qb = shift_Qb;
    param_loc.shift_Qout = shift_Qout;
    address_setup_qdq( param_loc, Mgran, Ngran, Mtile, Ntile, 0, Half_DM); 

    set_sat( );    
   if constexpr( std::is_integral_v<Tq> || HAS_FLOAT ) {
    
    //printf( "Start kernel on core %i,%i, iteration %i\n", get_coreid() >> 16, get_coreid() - 2 & 15, iteration );
    if constexpr( vector_coeffs > 1 ) {//|| __AIE_ARCH__ >= 21 ) {
        if constexpr( !std::is_integral_v<Tq> && fp_split_threshold > 1 && sizeof( Tq ) <= sizeof( Ti )) {
            //assert( address_config == ADDRCFG_linear || address_config == ADDRCFG_YCXC8 );
            auto ifm_Ti = local_buffer_cast<Ti, adf::direction::in>( ifm );
            qdq_split<Ti, Tq, Tr, Mgran, Ngran, Mtile, Ntile, 32, terms, lr_min, fp_split_threshold, fp_accuracy_mode>( ifm_Ti, ifm_sum, coeffs, ofm, param_loc );
        } else
            qdq<Ti, Tq, Tq0, Tr, Mgran, Ngran, Mtile, Ntile, terms, lr_min, qdq_coeffs, 0, fp_accuracy_mode>( ifm_Ti, ifm_sum, coeffs, ofm, param_loc );
    } else if constexpr( vector_coeffs == 1 || terms > 2 ) {
        auto coeffs_Tq0 = local_buffer_cast<Tq0,adf::direction::in>( coeffs );
        qdq<Mgran, Ngran, Mtile, Ntile, terms, lr_min, qdq_coeffs * sizeof( Tq ) / sizeof( Tq0 ), fp_accuracy_mode>( ifm_Ti, ifm_sum, coeffs_Tq0, c1, c2, ofm, param_loc );
    } else {
        static_assert( Ngran == Ntile );
        qdq<Mgran, Mtile, Ntile, lr_min, fp_accuracy_mode>( ifm_Ti, c0, c2, ofm, param_loc );
    }
   }
    iteration++;
}



template<typename Ti, typename Tq, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned fp_split_threshold, unsigned fp_accuracy_mode, typename T1>
__attribute__((noinline)) void wrapper_sym_conv( adf::input_buffer<T1> &ifm, adf::input_buffer<T1> &coeffs_c, adf::output_buffer<Tr> &ofm, QDQParams &param)
{
    constexpr unsigned lr_min = 6;
    auto config = aie::load_v<16>(( int32* ) coeffs_c.data( ));
    //auto M = config[0];
    //auto N = config[1];
    //auto address_config = config[2];
  	auto M = 64;
  	auto N = 64;
  	//auto address_config = 4;
    float c0 = as_float( config[8] );
    float c1 = as_float( config[9] );
    auto coeffs = local_buffer( coeffs_c, 16 );
    auto ifm_Ti = (Ti*) ifm.data( );
    static unsigned iteration=0;

    int shift_gemm = std::max( 0, (int)( 24 - 8 * unsigned(sizeof( Ti ))));
    if constexpr( !std::is_same_v<T1,Ti> ) {
        constexpr unsigned Vo = std::min( 32u, Mtile * Ntile );
        convert<T1, Ti, Vo, Mgran / Mtile * lr_min>( ifm.data( ), ifm_Ti, gemm_M * gemm_N / Vo, shift_gemm );
    }
    
    //QDQParams param;
    //
    //param.Y_g = ( address_config >= ADDRCFG_YCXC8 ? conv_Y : 1 );
    //param.M_g = M / param.Y_g / Mgran;
    //param.N_g = N / Ngran;
    address_setup_qdq( param, Mgran, Ngran, Mtile, Ntile, 0, Half_DM);

    set_sat( );
    
    if constexpr( std::is_same_v<float,Tq> || std::is_same_v<bfloat16,Tq> ) {
      if constexpr( HAS_FLOAT ) {
        auto coeffs_Tq = local_buffer_cast<Tq,adf::direction::in>( coeffs );
        printf( "Start kernel on core %i,%i, iteration %i\n", get_coreid() >> 16, get_coreid() - 2 & 15, iteration );
        if constexpr( vector_coeffs > 1 || __AIE_ARCH__ >= 21 ) {
            if constexpr( fp_split_threshold > 1 && sizeof( Tq ) <= sizeof( Ti )) {
                //assert( address_config == ADDRCFG_linear || address_config == ADDRCFG_YCXC8 );
                qdq_split<Ti, Tq, Tr, Mgran, Ngran, Mtile, Ntile, 32, 2, lr_min, fp_split_threshold, fp_accuracy_mode>( ifm_Ti, ifm, coeffs_Tq, ofm, param );
            } else
                qdq<Mgran, Ngran, Mtile, Ntile, lr_min, qdq_coeffs, 0, fp_accuracy_mode>( ifm_Ti, coeffs_Tq, ofm, param );
        } else if constexpr( vector_coeffs == 1 ) {
            qdq<Mgran, Ngran, Mtile, Ntile, lr_min, qdq_coeffs, fp_accuracy_mode>( ifm_Ti, coeffs_Tq, c1, ofm, param );
        } else {
            static_assert( Ngran == Ntile );
            qdq<Mgran, Mtile, Ntile, lr_min, fp_accuracy_mode>( ifm_Ti, c0, c1, ofm, param );
        }
      }
    } else {
        int shift[2];
        auto coeffs_Tq = local_buffer_cast<Tq,adf::direction::in>( coeffs );
        //convert_coefficients<Tq, Ntile, 2>(( float* ) coeffs.data( ), coeffs_Tq.data( ), shift, N );
        shift[0] = 26;
        shift[1] = 40;
        int shift_out = shift[1] - shift_gemm;
        printf( "Start kernel on core %i,%i, iteration %i\n", get_coreid() >> 16, get_coreid() - 2 & 15, iteration );
        
        set_rnd( rnd_conv_even );
        param.shift_Qb = shift_out - shift[0];
        param.shift_Qout = shift_out;
        
        if constexpr( vector_coeffs > 1 ) {
            qdq<Mgran, Ngran, Mtile, Ntile, lr_min, qdq_coeffs, 0, fp_accuracy_mode>( ifm_Ti, coeffs_Tq, ofm, param );
        } else if constexpr( vector_coeffs == 1 ) {
            Tq c1i = coeffs_Tq.data( )[8];
            qdq<Mgran, Ngran, Mtile, Ntile, lr_min, qdq_coeffs, fp_accuracy_mode>( ifm_Ti, coeffs_Tq, c1i, ofm, param );
        } else {
            static_assert( Ngran == Ntile );
            Tq c0i = coeffs_Tq.data( )[0];
            Tq c1i = coeffs_Tq.data( )[8];
            qdq<Mgran, Mtile, Ntile, lr_min, fp_accuracy_mode>( ifm_Ti, c0i, c1i, ofm, param );
        }
    }
    iteration++;
}


template<typename Ti, typename Tq, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned fp_split_threshold, unsigned fp_accuracy_mode>
__attribute__((noinline)) void wrapper_asym_conv( adf::input_buffer<int32> &ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<qdq_C_tt> &coeffs_c, adf::output_buffer<Tr> &ofm )
{
    constexpr unsigned lr_min = 6;    
    constexpr bool term_3_preadd = false;
    
    auto config = aie::load_v<16>(( int32* ) coeffs_c.data( ));
    auto M = config[0];
    auto N = config[1];
    auto address_config = config[2];
    float c1 = as_float( config[9] );
    float c2 = as_float( config[10] );
    int16 zpw = config[11];
    auto coeffs = local_buffer( coeffs_c, 16 );
    auto ifm_Ti = (Ti*) ifm.data( );
    static unsigned iteration=0;

    int shift_gemm = std::max( 0, (int)( 24 - 8 * unsigned(sizeof( Ti ))));
    if constexpr( !std::is_same_v<int32,Ti> ) {
        constexpr unsigned Vo = std::min( 32u, Mtile * Ntile );
        convert<int32, Ti, Vo, Mgran / Mtile * lr_min>( ifm.data( ), ifm_Ti, gemm_M * gemm_N / Vo, shift_gemm );
    }
    
    if constexpr( term_3_preadd ) {
        constexpr unsigned Mt = 16 / sizeof( Ti );
        constexpr unsigned Nt = 8;
        constexpr unsigned Mg = 2 * Mt;
        constexpr unsigned Ng = 8;
        constexpr unsigned step = sizeof( qdq_C_tt ) / sizeof( int16 );
        
        QDQParams prm;
        prm.Y_g = ( address_config >= ADDRCFG_YCXC8 ? conv_Y : 1 );
        prm.M_g = M / prm.Y_g / Mg;
        prm.N_g = N / Ng;
        address_setup_qdq( prm, Mg, Ng, Mt, Nt, address_config, ifm.size( ) / 2 );
        
        auto ifm_o = local_buffer_cast<Ti, adf::direction::out>( ifm );
        auto cf_i16 = local_buffer_cast<int16, adf::direction::in>( coeffs );
        
        qdq_term_outer<Ti, Ti, int16, Ti, Mg, Ng, Mt, Nt, lr_min * Mgran / Mg>( ifm_Ti, ifm_sum, cf_i16, ifm_o, prm, 3 * step, qdq_coeffs * step );
    }
    constexpr unsigned terms = 3 - term_3_preadd;
    
    QDQParams param;
    param.Y_g = ( address_config >= ADDRCFG_YCXC8 ? conv_Y : 1 );
    param.M_g = M / param.Y_g / Mgran;
    param.N_g = N / Ngran;
    address_setup_qdq( param, Mgran, Ngran, Mtile, Ntile, address_config, ifm.size( ) / 2 );

    set_sat( );
    
    if constexpr( std::is_same_v<float,Tq> || std::is_same_v<bfloat16,Tq> ) {
      if constexpr( HAS_FLOAT ) {
        printf( "Start kernel on core %i,%i, iteration %i\n", get_coreid() >> 16, get_coreid() - 2 & 15, iteration );
        auto coeffs_Tq = local_buffer_cast<Tq,adf::direction::in>( coeffs );
        if constexpr( vector_coeffs > 1 || __AIE_ARCH__ >= 21 ) {
            if constexpr( fp_split_threshold > 1 && sizeof( Tq ) <= sizeof( Ti )) {
                assert( address_config == ADDRCFG_linear || address_config == ADDRCFG_YCXC8 );
                qdq_split<Ti, Tq, Tr, Mgran, Ngran, Mtile, Ntile, 32, terms, lr_min, fp_split_threshold, fp_accuracy_mode>( ifm, ifm_sum, coeffs_Tq, ofm, param );
            } else
                qdq<Mgran, Ngran, Mtile, Ntile, terms, lr_min, qdq_coeffs, 3-terms, fp_accuracy_mode>( ifm_Ti, ifm_sum, coeffs_Tq, ofm, param );
        } else {
            qdq<Mgran, Ngran, Mtile, Ntile, terms, lr_min, qdq_coeffs, fp_accuracy_mode>( ifm_Ti, ifm_sum, coeffs_Tq, c1, c2, ofm, param );
        }
      }
    } else {
        int shift[3];
        auto coeffs_Tq = local_buffer_cast<Tq,adf::direction::in>( coeffs );
        convert_coefficients<Tq, Ntile, terms>(( float* ) coeffs.data( ), coeffs_Tq.data( ), shift, N, shift_gemm );
        
        printf( "Start kernel on core %i,%i, iteration %i\n", get_coreid() >> 16, get_coreid() - 2 & 15, iteration );
        
        set_rnd( rnd_conv_even );
        param.shift_Qb = shift[1] - shift[0];
        param.shift_Qout = shift[1];

        if constexpr( vector_coeffs > 1 ) {
            qdq<Mgran, Ngran, Mtile, Ntile, terms, lr_min, qdq_coeffs, 3-terms, fp_accuracy_mode>( ifm_Ti, ifm_sum, coeffs_Tq, ofm, param );
        } else {
            Tq c1i = coeffs_Tq.data( )[8];
            Tq c2i = coeffs_Tq.data( )[16];
            qdq<Mgran, Ngran, Mtile, Ntile, terms, lr_min, qdq_coeffs, fp_accuracy_mode>( ifm_Ti, ifm_sum, coeffs_Tq, c1i, c2i, ofm, param );
        }
    }
    iteration++;
}


template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned vector_coeffs, unsigned fp_accuracy_mode, unsigned fp_split_threshold>
void wrapper_sym_wa( adf::input_buffer<qdq_I_tt> &ifm, adf::input_buffer<qdq_C_tt> &coeffs, adf::output_buffer<qdq_O_tt> &ofm )
{
    auto ifm_l   = local_buffer_cast<Ti, adf::direction::in>( ifm );
    auto coeff_l = local_buffer_cast<Tq, adf::direction::in>( coeffs );
    auto ofm_l   = local_buffer_cast<Tr, adf::direction::out>( ofm );
    wrapper_sym<Ti, Tq, Tq0, Tr, Mgran, Ngran, Mtile, Ntile, vector_coeffs, fp_accuracy_mode, fp_split_threshold>( ifm_l, coeff_l, ofm_l );
}
template<typename Ti, typename Tq, typename Tq0, typename Tr, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned vector_coeffs, unsigned fp_accuracy_mode, unsigned fp_split_threshold, unsigned terms>
void wrapper_asym_wa( adf::input_buffer<qdq_I_tt> &ifm, adf::input_buffer<Ti> &ifm_sum, adf::input_buffer<qdq_C_tt> &coeffs, adf::output_buffer<qdq_O_tt> &ofm )
{
    auto ifm_l   = local_buffer_cast<Ti, adf::direction::in>( ifm );
    auto coeff_l = local_buffer_cast<Tq, adf::direction::in>( coeffs );
    auto ofm_l   = local_buffer_cast<Tr, adf::direction::out>( ofm );
    wrapper_asym<Ti, Tq, Tq0, Tr, Mgran, Ngran, Mtile, Ntile, vector_coeffs, fp_accuracy_mode, fp_split_threshold, terms>( ifm_l, ifm_sum, coeff_l, ofm_l );
}

