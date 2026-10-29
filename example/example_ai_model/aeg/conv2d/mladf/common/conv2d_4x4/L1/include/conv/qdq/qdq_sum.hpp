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

#ifndef __QDQ_SUM_HPP__
#define __QDQ_SUM_HPP__

#include "qdq_helpers.hpp"


/*! \brief Sum across columns in K M Ktile data structure. Kernel implementation is tested and optimized for Ta=int8, Ts=int16, Mgran=16, Kgran=8, Mtile=8, Ktile=8, inner_lr_min=8, outer_lr_min=3
    Compute: ofm^{Mx1} = srs( sum_K( ifm^{MxK} ) + !zero_init * ( ofm^{Mx1} << shift ), shift );

  @param[in] ifm        input volume. Data order: C R C8 ( K M K8 )
  @param[out] ofm       output sum (also used as partial sum buffer)
  @param[in] M_g        Size for M dimension in terms of granularity Mgran
  @param[in] K_g        Size for K dimension in terms of granularity Kgran
  @param[in] Y_g        Size for Y dimension to model YCXC8 data order used in convolution kernel. Set to 1 if unused
  @param[in] zero_init  Zero init flag to clear state (beginning of summation for multiple iterations. To be set to 1 for single iteration
  @param[in] shift      Shift factor for output
 */
template<typename Ta, typename Ts, unsigned Mgran, unsigned Kgran, unsigned Mtile, unsigned Ktile, unsigned inner_lr_min, unsigned outer_lr_min>
void sum_inner( adf::input_buffer<Ta> &ifm, adf::output_buffer<Ts> &ofm, unsigned M_g, unsigned K_g, unsigned Y_g, bool zero_init, int shift )
{

    constexpr unsigned Mi = Mgran / Mtile;
    constexpr unsigned Ki = Kgran / Ktile;
    constexpr unsigned Vs = sum_write_garbage_stride<Ts>() * Mtile;
    
    //Add3dPtr add_3d_ifm( Mi * Ki * M_g, K_g, Mi, M_g, Mi * Ki * M_g * K_g );
    //auto pI = aie::begin_vector<Mtile*Ktile>( ifm );
    Add3dElem add_3d_ifm( Mgran * Kgran * M_g, K_g, Mgran * Ktile, M_g, Mgran * Kgran * M_g * K_g );
    auto pI = ifm.data( );
    auto pT = aie::begin_vector<Vs>( ofm );
    auto pO = aie::begin_restrict_vector<Vs>( ofm );
    
    constexpr unsigned sum_outer = __AIE_ARCH__ <= 20 ? 4 : 64 / Mtile;
    using sum_mul_t = aie::mmul<sum_outer, Ktile, Mtile, Ta, Ta>;
    using tdm_t = aie::accum<aie::detail::accum_tag_for_mul_types<Ta, Ta>, Vs>;
    using acc_t = aie::accum<aie::detail::accum_tag_for_mul_types<Ta, Ta>, sum_outer * Mtile>;
    acc_t sum[Mi];
    
    #pragma unroll
    for (unsigned mi=0; mi<Mi; mi++) {
        sum[mi] = tdm_t( *pT++, shift ).template grow<sum_outer * Mtile>( );
    }
    
    #if __AIE_ARCH__ >= 21 
        v256int8_sparse sparse_in;
        sparse_in = insert( sparse_in, 0, get_sparse( 5 )); 
        sparse_in = insert( sparse_in, 1, get_sparse( 5 ));
    #endif
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE( outer_lr_min, ) 
    for ( unsigned o=0; o<M_g*Y_g; o++ ){
        //locate_in_register<2>( sum );
        #pragma unroll
        for (unsigned mic=0; mic<Mi; mic++) {
            unsigned mi = Mi - 1 - mic;
            pI = chess_copy( pI );
            
            #pragma unroll
            for (unsigned ki=0; ki<Ki; ki++) {
                //auto ifm = pI[mi + M_g * Mi * ki];
                auto ifm = load_index<Mtile*Ktile, aie_dm_resource::a>( pI, Mtile * Ktile * mi + M_g * Mgran * Ktile * ki );
    #if __AIE_ARCH__ >= 21  
                if constexpr( __AIE_ARCH__ >= 21 && Mtile == 16 && Ktile == 8 ) {
                    sparse_in = insert( sparse_in, 1, ifm.template extract<64>( 1 ));
                    sparse_in = insert( sparse_in, 0, ifm.template extract<64>( 0 )); 
                    sum[mi] = mac_4x16_16x16T_conf( aie::broadcast<Ta, sum_outer * Ktile * 2>( 1 ), sparse_in, sum[mi], zero_init, 0, 0, 0 );
                } else {
    #endif
                    auto s = sum_mul_t( aie::op_zero( sum[mi], zero_init ));
                    s.mac( aie::broadcast<Ta, sum_outer * Ktile>( 1 ), aie::transpose( ifm, Mtile, Ktile ));
                    sum[mi] = s.to_accum( );
    #if __AIE_ARCH__ >= 21  
                }
    #endif
            }
            //sum[mi] = locate_dm<2>( sum[mi] );
        }
        pI = add_3d_ifm( pI );
        locate_in_register( sum );
        
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE( inner_lr_min-1, ) 
        for ( unsigned i=1; i<K_g; i++ )
#if __AIE_ARCH__ >= 20
        chess_peel_pipelined_loop(3)
#endif
        {
            #pragma unroll
            for (unsigned mic=0; mic<Mi; mic++) {
                unsigned mi = Mi - 1 - mic;
                pI = chess_copy( pI );
                #pragma unroll
                for (unsigned ki=0; ki<Ki; ki++) {
                    //auto ifm = pI[mi + M_g * Mi * ki];
                    auto ifm = load_index<Mtile*Ktile, aie_dm_resource::a>( pI, Mtile * Ktile * mi + M_g * Mgran * Ktile * ki );
    #if __AIE_ARCH__ >= 21  
                    if constexpr( __AIE_ARCH__ >= 21 && Mtile == 16 && Ktile == 8 ) {
                        sparse_in = insert( sparse_in, 1, ifm.template extract<64>( 1 ));
                        sparse_in = insert( sparse_in, 0, ifm.template extract<64>( 0 )); 
                        sum[mi] = mac_4x16_16x16T( aie::broadcast<Ta, sum_outer * Ktile * 2>( 1 ), sparse_in, sum[mi] );
                    } else {
    #endif
                       auto s = sum_mul_t( sum[mi] );
                        s.mac( aie::broadcast<Ta, sum_outer * Ktile>( 1 ), aie::transpose( ifm, Mtile, Ktile ));
                        sum[mi] = s.to_accum( );
    #if __AIE_ARCH__ >= 21  
                    }
          #endif
                }
            }
            pI = add_3d_ifm( pI );
        }
        
        #pragma unroll
        for (unsigned mic=0; mic<Mi; mic++) {
            //unsigned mi = Mi - 1 - mic;
            unsigned mi = mic;
            //#if __AIE_ARCH__ >= 21
            auto acc = sum[mi].template extract<std::max(16u,Mtile)>( 0 );
            *pO++ = acc.template extract<Vs>( 0 ).template to_vector<Ts>( shift );
            //#else
            //auto acc = sum[mi].to_accum( );
            //*pO++ = acc.template to_vector<Ts>( shift ).template extract<Vs>( 0 );
            //#endif
            //*pO++ = acc.template extract<Mtile>( 0 ).template to_vector<Ts>( shift );
            sum[mi] = tdm_t( *pT++, shift ).template grow<sum_outer * Mtile>( );
        }
    }
}

/*! \brief Sum across columns in K M Ktile data structure. Kernel implementation is tested and optimized for Ta=int8, Ts=int16, Mgran=16, Kgran=8, Mtile=8, Ktile=8, inner_lr_min=8, outer_lr_min=3
    Compute: ofm^{Mx1} = srs( sum_K( ifm^{MxK} ) + !zero_init * ( ofm^{Mx1} << shift ) + casc_en * casc_in, shift );

  @param[in] ifm        input volume. Data order: C R C8 ( K M K8 )
  @param[in] casc_in    cascade input from previous core in cascade chain
  @param[out] ofm       output sum (also used as partial sum buffer)
  @param[in] M_g        Size for M dimension in terms of granularity Mgran
  @param[in] K_g        Size for K dimension in terms of granularity Kgran
  @param[in] Y_g        Size for Y dimension to model YCXC8 data order used in convolution kernel. Set to 1 if unused
  @param[in] zero_init  Zero init flag to clear state (beginning of summation for multiple iterations. To be set to 1 for single iteration
  @param[in] casc_en    Dynamic enable of cascade input (1 to read from cascade, 0 to omit)
  @param[in] shift      Shift factor for output
 */
template<typename Ta, typename Ts, typename Tcasc, unsigned Mgran, unsigned Kgran, unsigned Mtile, unsigned Ktile, unsigned inner_lr_min, unsigned outer_lr_min>
void sum_inner( adf::input_buffer<Ta> &ifm, input_cascade<Tcasc> &casc_in, adf::output_buffer<Ts> &ofm, unsigned M_g, unsigned K_g, unsigned Y_g, bool zero_init, bool casc_en, int shift )
{
    constexpr unsigned Mi = Mgran / Mtile;
    constexpr unsigned Ki = Kgran / Ktile;
    constexpr unsigned Vs = sum_write_garbage_stride<Ts>() * Mtile;
    
    Add3dPtr add_3d_ifm( Mi * Ki * M_g, K_g, Mi, M_g, Mi * Ki * M_g * K_g );
    auto pI = aie::begin_vector<Mtile*Ktile>( ifm );
    auto pT = aie::begin_vector<Vs>( ofm );
    auto pO = aie::begin_restrict_vector<Vs>( ofm );
    
    constexpr unsigned sum_outer = __AIE_ARCH__ <= 20 ? 4 : 8;
    using sum_mul_t = aie::mmul<sum_outer, Ktile, Mtile, Ta, Ta>;
    using acc_t = aie::accum<aie::detail::accum_tag_for_mul_types<Ta, Ta>, sum_mul_t::size_C>;
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE( outer_lr_min, ) 
    for ( unsigned o=0; o<M_g*Y_g; o++ ){
        sum_mul_t sum[Mi];
        
        #pragma unroll
        for (unsigned mic=0; mic<Mi; mic++) {
            unsigned mi = Mi - 1 - mic;
            sum[mi] = sum_mul_t( aie::op_zero( acc_t(( *pT++ ).template grow<sum_mul_t::size_C>( ), shift ), zero_init ));
            #pragma unroll
            for (unsigned ki=0; ki<Ki; ki++) {
                sum[mi].mac( aie::broadcast<Ta,sum_outer*Ktile>( 1 ), aie::transpose( pI[mi + M_g * Mi * ki], Mtile, Ktile ));
            }
        }
        pI = add_3d_ifm( pI );
        
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE( inner_lr_min-1, ) 
        for ( unsigned i=1; i<K_g; i++ )chess_peel_pipelined_loop(1){
            #pragma unroll
            for (unsigned mic=0; mic<Mi; mic++) {
                unsigned mi = Mi - 1 - mic;
                #pragma unroll
                for (unsigned ki=0; ki<Ki; ki++) {
                    sum[mi].mac( aie::broadcast<Ta,sum_outer*Ktile>( 1 ), aie::transpose( pI[mi + M_g * Mi * ki], Mtile, Ktile ));
                }
            }
            pI = add_3d_ifm( pI );
        }
        
        #pragma unroll
        for (unsigned mic=0; mic<Mi; mic++) {
            unsigned mi = Mi - 1 - mic;
            auto acc = aie::add( sum[mi].to_accum( ), get_cascade<aie::accum<Tcasc,std::max( 16u, Mtile )>>( casc_en ).template grow<sum_mul_t::size_C>( ));
            *pO++ = acc.template to_vector<Ts>( shift ).template extract<Vs>( 0 );
        }
    }
}

/*! \brief Sum across columns in K M Ktile data structure. Kernel implementation is tested and optimized for Ta=int8, Ts=int16, Mgran=16, Kgran=8, Mtile=8, Ktile=8, inner_lr_min=8, outer_lr_min=3
    Compute: casc_out = sum_K( ifm^{MxK} ) + casc_en * casc_in;

  @param[in] ifm        input volume. Data order: C R C8 ( K M K8 )
  @param[in] casc_in    cascade input from previous core in cascade chain
  @param[out] casc_out  output to next core in cascade chain
  @param[in] M_g        Size for M dimension in terms of granularity Mgran
  @param[in] K_g        Size for K dimension in terms of granularity Kgran
  @param[in] Y_g        Size for Y dimension to model YCXC8 data order used in convolution kernel. Set to 1 if unused
  @param[in] zero_init  Zero init flag to clear state (beginning of summation for multiple iterations. To be set to 1 for single iteration
  @param[in] casc_en    Dynamic enable of cascade input (1 to read from cascade, 0 to omit)
 */
template<typename Ta, typename Tcasc, unsigned Mgran, unsigned Kgran, unsigned Mtile, unsigned Ktile, unsigned inner_lr_min, unsigned outer_lr_min>
void sum_inner( adf::input_buffer<Ta> &ifm, input_cascade<Tcasc> &casc_in, output_cascade<Tcasc> &casc_out, unsigned M_g, unsigned K_g, unsigned Y_g, bool zero_init, bool casc_en )
{
    constexpr unsigned Mi = Mgran / Mtile;
    constexpr unsigned Ki = Kgran / Ktile;
    
    Add3dPtr add_3d_ifm( Mi * Ki * M_g, K_g, Mi, M_g, Mi * Ki * M_g * K_g );
    auto pI = aie::begin_vector<Mtile*Ktile>( ifm );
    
    constexpr unsigned sum_outer = __AIE_ARCH__ <= 20 ? 4 : 8;
    using sum_mul_t = aie::mmul<sum_outer, Ktile, Mtile, Ta, Ta>;
    
    AIE_PREPARE_FOR_PIPELINING 
    AIE_LOOP_RANGE( outer_lr_min, ) 
    for ( unsigned o=0; o<M_g*Y_g; o++ ){
        sum_mul_t sum[Mi];
        
        AIE_PREPARE_FOR_PIPELINING 
        AIE_LOOP_RANGE( inner_lr_min, ) 
        for ( unsigned i=0; i<K_g; i++ )chess_peel_pipelined_loop(1){
            #pragma unroll
            for (unsigned mic=0; mic<Mi; mic++) {
                unsigned mi = Mi - 1 - mic;
                #pragma unroll
                for (unsigned ki=0; ki<Ki; ki++) {
                    sum[mi].mac( aie::broadcast<Ta,sum_outer*Ktile>( 1 ), aie::transpose( pI[mi + M_g * Mi * ki], Mtile, Ktile ));
                }
            }
            pI = add_3d_ifm( pI );
        }
        
        #pragma unroll
        for (unsigned mic=0; mic<Mi; mic++) {
            unsigned mi = Mi - 1 - mic;
            auto acc = aie::add( sum[mi].to_accum( ), get_cascade<aie::accum<Tcasc,std::max( 16u, Mtile )>>( casc_en ).template grow<sum_mul_t::size_C>( ));
            writeincr( &casc_out, acc.template extract<std::max( 16u, Mtile )>( 0 ));
        }
    }
}

#endif
