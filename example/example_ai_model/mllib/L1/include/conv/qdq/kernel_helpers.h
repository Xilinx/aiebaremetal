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

#ifndef __KERNEL_HELPERS_H__
#define __KERNEL_HELPERS_H__


//This file mainly implements functionality currently missing in the AIE API for convenient use inside the kernel

#include <aie_api/aie.hpp>
#include "./common/constants_main.h"

#define FAST_INT16_ACCUM_BROADCAST 2
#define HAS_FLOAT ( __AIE_ARCH__ <= 20 || ( __AIE_ARCH__ == 21 && __AIE_ARCH_MODEL_VERSION__ >= 21010600 ))

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

template<typename T>
concept Integal = std::is_integral_v<T>;

template<typename T>
concept FloatingPoint = !std::is_integral_v<T>;

template<typename T>
concept BufferOrPointer = requires( T x ) { *x; aie::begin_vector( x ); };


template<typename T>
void report( T v ) {
    chess_report( v );
}
template<> void report( aie::vector< int16, 8> v ) {  v8int16  l = v; chess_report( l ); }
template<> void report( aie::vector<uint16, 8> v ) {  v8uint16 l = v; chess_report( l ); }
template<> void report( aie::vector< int32, 8> v ) {  v8int32  l = v; chess_report( l ); }
template<> void report( aie::vector< int16,16> v ) { v16int16  l = v; chess_report( l ); }
template<> void report( aie::vector<uint16,16> v ) { v16uint16 l = v; chess_report( l ); }
template<> void report( aie::vector< int32,16> v ) { v16int32  l = v; chess_report( l ); }

template<> void report( aie::accum<accfloat, 8> v ) {  v8accfloat l = v; chess_report( l ); }
template<> void report( aie::accum<accfloat,16> v ) { v16accfloat l = v; chess_report( l ); }
template<> void report( aie::accum<accfloat,32> v ) { v32accfloat l = v; chess_report( l ); }



template<typename Tx, unsigned V, typename Ty=Tx, typename Ta=aie::detail::accum_tag_for_mul_types<Tx,Ty>, unsigned float_accuracy_mode=1>
ALWAYS_INLINE aie::accum<Ta,V> mul_elew( aie::vector<Tx,V> x, aie::vector<Ty,V> y )
{
    return aie::mul( x, y );
}
#if HAS_FLOAT
#if __AIE_ARCH__ == 20
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mul_elew<float,32,float,accfloat,0>( aie::vector<float,32> x, aie::vector<float,32> y )
{
    aie::accum<accfloat,16> a0 = mul_elem_16_accuracy_low( x.template extract<16>( 0 ), y.template extract<16>( 0 ));
    aie::accum<accfloat,16> a1 = mul_elem_16_accuracy_low( x.template extract<16>( 1 ), y.template extract<16>( 1 ));
    return aie::concat( a0, a1 );
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mul_elew<float,32,float,accfloat,1>( aie::vector<float,32> x, aie::vector<float,32> y )
{
    aie::accum<accfloat,16> a0 = mul_elem_16_accuracy_fast( x.template extract<16>( 0 ), y.template extract<16>( 0 ));
    aie::accum<accfloat,16> a1 = mul_elem_16_accuracy_fast( x.template extract<16>( 1 ), y.template extract<16>( 1 ));
    return aie::concat( a0, a1 );
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mul_elew<float,32,float,accfloat,2>( aie::vector<float,32> x, aie::vector<float,32> y )
{
    aie::accum<accfloat,16> a0 = mul_elem_16_accuracy_safe( x.template extract<16>( 0 ), y.template extract<16>( 0 ));
    aie::accum<accfloat,16> a1 = mul_elem_16_accuracy_safe( x.template extract<16>( 1 ), y.template extract<16>( 1 ));
    return aie::concat( a0, a1 );
}
#else
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mul_elew<float,32,float,accfloat,0>( aie::vector<float,32> x, aie::vector<float,32> y )
{
    return mul_elem_32_accuracy_low( x, y );
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mul_elew<float,32,float,accfloat,1>( aie::vector<float,32> x, aie::vector<float,32> y )
{
    return mul_elem_32_accuracy_fast( x, y );
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mul_elew<float,32,float,accfloat,2>( aie::vector<float,32> x, aie::vector<float,32> y )
{
    return mul_elem_32_accuracy_safe( x, y );
}
#endif
#endif


template<typename Tx, unsigned V, typename Ty=Tx, typename Ta=aie::detail::accum_tag_for_mul_types<Tx,Ty>, unsigned float_accuracy_mode=1>
ALWAYS_INLINE aie::accum<Ta,V> mac_elew( aie::accum<Ta,V> acc, aie::vector<Tx,V> x, aie::vector<Ty,V> y )
{
    return aie::mac( acc, x, y );
}
#if HAS_FLOAT
#if __AIE_ARCH__ == 20
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mac_elew<float,32,float,accfloat,0>( aie::accum<accfloat,32> acc, aie::vector<float,32> x, aie::vector<float,32> y )
{
    aie::accum<accfloat,16> a0 = mac_elem_16_accuracy_low( x.template extract<16>( 0 ), y.template extract<16>( 0 ), acc.template extract<16>( 0 ));
    aie::accum<accfloat,16> a1 = mac_elem_16_accuracy_low( x.template extract<16>( 1 ), y.template extract<16>( 1 ), acc.template extract<16>( 1 ));
    return aie::concat( a0, a1 );
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mac_elew<float,32,float,accfloat,1>( aie::accum<accfloat,32> acc, aie::vector<float,32> x, aie::vector<float,32> y )
{
    aie::accum<accfloat,16> a0 = mac_elem_16_accuracy_fast( x.template extract<16>( 0 ), y.template extract<16>( 0 ), acc.template extract<16>( 0 ));
    aie::accum<accfloat,16> a1 = mac_elem_16_accuracy_fast( x.template extract<16>( 1 ), y.template extract<16>( 1 ), acc.template extract<16>( 1 ));
    return aie::concat( a0, a1 );                         
}                                                         
template<>                                                
ALWAYS_INLINE aie::accum<accfloat,32> mac_elew<float,32,float,accfloat,2>( aie::accum<accfloat,32> acc, aie::vector<float,32> x, aie::vector<float,32> y )
{                                                         
    aie::accum<accfloat,16> a0 = mac_elem_16_accuracy_safe( x.template extract<16>( 0 ), y.template extract<16>( 0 ), acc.template extract<16>( 0 ));
    aie::accum<accfloat,16> a1 = mac_elem_16_accuracy_safe( x.template extract<16>( 1 ), y.template extract<16>( 1 ), acc.template extract<16>( 1 ));
    return aie::concat( a0, a1 );
}
#else
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mac_elew<float,32,float,accfloat,0>( aie::accum<accfloat,32> acc, aie::vector<float,32> x, aie::vector<float,32> y )
{
    return mac_elem_32_accuracy_low( x, y, acc, 0, 0, 0 );
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mac_elew<float,32,float,accfloat,1>( aie::accum<accfloat,32> acc, aie::vector<float,32> x, aie::vector<float,32> y )
{
    return mac_elem_32_accuracy_fast( x, y, acc, 0, 0, 0 );
}                                                         
template<>                                                
ALWAYS_INLINE aie::accum<accfloat,32> mac_elew<float,32,float,accfloat,2>( aie::accum<accfloat,32> acc, aie::vector<float,32> x, aie::vector<float,32> y )
{      
    return mac_elem_32_accuracy_safe( x, y, acc, 0, 0, 0 );
}
#endif
#endif
#if __AIE_ARCH__ >= 21
ALWAYS_INLINE aie::accum<acc64,32> mac_elew( aie::accum<acc64,32> acc, aie::vector<int32,32> x, aie::vector<int32,32> y )
{
    //v32uint16 chess_storage( x0 ) xl = (v32uint16) shuffle( x.template extract<16>( 0 ), x.template extract<16>( 1 ), T16_32x2_lo );
    //v32int16  chess_storage( x3 ) xh = (v32int16)  shuffle( x.template extract<16>( 0 ), x.template extract<16>( 1 ), T16_32x2_hi );
    //v32uint16 chess_storage( x1 ) yl = (v32uint16) shuffle( y.template extract<16>( 0 ), y.template extract<16>( 1 ), T16_32x2_lo );
    //v32int16  chess_storage( x2 ) yh = (v32int16)  shuffle( y.template extract<16>( 0 ), y.template extract<16>( 1 ), T16_32x2_hi );
    v32uint16 chess_storage( x6 ) xl = (v32uint16) shuffle( x.template extract<16>( 0 ), x.template extract<16>( 1 ), T16_32x2_lo );
    v32int16  chess_storage( x6 ) xh = (v32int16)  shuffle( x.template extract<16>( 0 ), x.template extract<16>( 1 ), T16_32x2_hi );
    v32uint16 chess_storage( x4 ) yl = (v32uint16) shuffle( y.template extract<16>( 0 ), y.template extract<16>( 1 ), T16_32x2_lo );
    v32int16  chess_storage( x5 ) yh = (v32int16)  shuffle( y.template extract<16>( 0 ), y.template extract<16>( 1 ), T16_32x2_hi );
    v32acc64 a = mul_elem_32( xh, yh );
    //a = mac_elem_32_2_conf( concat( xl, yl ), concat( yh, xh ), a, 0, 1, 0, 0 );
    a = mac_elem_32_2_conf( yl, xh, a, 0, 1, 0, 0 );
    a = mac_elem_32_2_conf( xl, yh, a, 0, 0, 0, 0 );
    a = addmac_elem_32_conf( xl, yl, a, acc, 0, 1, 0, 0, 0 );
    //chess_separator();
    return a;
}
#endif



// vector_broadcast is obsolete once aie::broadcast supports vectors (CRVO-7495)
template<unsigned Vo, typename T, unsigned Vi>
aie::vector<T,Vo> vector_broadcast( aie::vector<T,Vi> e )
{
    return aie::broadcast( e ); //Currently not supported, implemented in LLI below
}
#if __AIE_ARCH__ == 20
template<>
aie::vector<int16,16> vector_broadcast( aie::vector<int16,8> e )
{
    v32int16 lli = e.template grow<32>( );
    lli = shuffle( lli, lli, T128_2x4_lo );
    return aie::vector<int16,32>( lli ).template extract<16>( 0 );
}
template<>
aie::vector<int16,32> vector_broadcast( aie::vector<int16,8> e )
{
    v32int16 lli = e.template grow<32>( );
    lli = shuffle( lli, lli, T128_2x4_lo );
    lli = shuffle( lli, lli, T128_2x4_lo );
    return aie::vector<int16,32>( lli );
}
#else
template<>
aie::vector<int16,16> vector_broadcast( aie::vector<int16,8> e )
{
    v32int16 lli = e.template grow<32>( );
    lli = (v32int16) broadcast_elem_128( (v16int32)lli, 0 );
    return aie::vector<int16,32>( lli ).template extract<16>( 0 );
}
template<>
aie::vector<int16,32> vector_broadcast( aie::vector<int16,8> e )
{
    v32int16 lli = e.template grow<32>( );
    lli = (v32int16) broadcast_elem_128( (v16int32)lli, 0 );
    return aie::vector<int16,32>( lli );
}
#endif
template<>
aie::vector<int16,64> vector_broadcast( aie::vector<int16,8> e )
{
    auto v = vector_broadcast<32>( e );
    return aie::concat( v, v );
}
template<>
aie::vector<int32,16> vector_broadcast( aie::vector<int32,8> e )
{
    auto v = e.template grow<16>( );
    return v.insert( 1, e );
}
template<>
aie::vector<int32,32> vector_broadcast( aie::vector<int32,8> e )
{
    auto v = vector_broadcast<16>( e );
    return aie::concat( v, v );
}
#if HAS_FLOAT
template<>
aie::vector<float,16> vector_broadcast( aie::vector<float,4> e )
{
    return vector_broadcast<32>( e.template cast_to<int16>( )).template cast_to<float>( );
}
template<>
aie::vector<float,16> vector_broadcast( aie::vector<float,8> e )
{
    auto v = e.template grow<16>( );
    return v.insert( 1, e );
}
template<>
aie::vector<float,32> vector_broadcast( aie::vector<float,8> e )
{
    auto v = vector_broadcast<16>( e );
    return aie::concat( v, v );
}
#endif
template<>
aie::vector<bfloat16,32> vector_broadcast( aie::vector<bfloat16,8> e )
{
    return vector_broadcast<32>( e.template cast_to<int16>( )).template cast_to<bfloat16>( );
}
template<>
aie::vector<bfloat16,64> vector_broadcast( aie::vector<bfloat16,8> e )
{
    auto v = vector_broadcast<32>( e );
    return aie::concat( v, v );
}



// mul_outer_prod is obsolete once aie::mmul<M,1,N> shapes are supported
template<typename Tx, unsigned Vx, typename Ty, unsigned Vy, typename Ta=aie::detail::accum_tag_for_mul_types<Tx,Ty>, unsigned fp_accuracy_mode=1>
ALWAYS_INLINE aie::accum<Ta,Vx*Vy> mul_outer_prod( aie::vector<Tx,Vx> x, aie::vector<Ty,Vy> y )
{
    aie::mmul<Vx, 1, Vy, Tx, Ty> m;
    m.mul( x, y );
    return m.to_accum();
}
#if __AIE_ARCH__ == 20
template<>
ALWAYS_INLINE aie::accum<acc32,64> mul_outer_prod( aie::vector<int16,8> x, aie::vector<int16,8> y )
{
    auto xi = x.template grow<32>( );
    xi = shuffle( xi, broadcast_zero_to_v32int16(), T16_2x32_lo );
    aie::mmul<8, 2, 8, int16, int16> m;
    m.mul( xi.template extract<16>( 0 ), y.template grow<16>( ));
    return m.to_accum();
}
template<>
ALWAYS_INLINE aie::accum<acc32,64> mul_outer_prod( aie::vector<uint16,8> x, aie::vector<int16,8> y )
{
    auto xi = x.template grow<32>( );
    xi = shuffle( xi, broadcast_zero_to_v32uint16(), T16_2x32_lo );
    aie::mmul<8, 2, 8, uint16, int16> m;
    m.mul( xi.template extract<16>( 0 ), y.template grow<16>( ));
    return m.to_accum();
}
template<>
ALWAYS_INLINE aie::accum<acc48,32> mul_outer_prod( aie::vector<int32,4> x, aie::vector<int16,8> y )
{
    v16int32 xi = x.template grow<16>( );
    v32int16 yi = y.template grow<32>( );
    xi = sel( shift( broadcast_zero_to_v16int32( ), xi, 4 ), xi, 15 );
    aie::accum<acc64,16> a0, a1;
    v16int32 chess_storage(x0) x0 = shuffle( xi, T32_4x4 );
    v16int32 chess_storage(x0) x1 = shuffle( x0, T256_2x2_hi );
    a0 = mul_4x2_2x4( x0, yi );
    a1 = mul_4x2_2x4( x1, yi );
    return aie::concat( a0, a1 );
}
template<>
ALWAYS_INLINE aie::accum<acc64,32> mul_outer_prod( aie::vector<int32,4> x, aie::vector<int32,8> y )
{
    v16int32 xi = x.template grow<16>( );
    v16int32 yi = y.template grow<16>( );
    xi = sel( shift( broadcast_zero_to_v16int32( ), xi, 4 ), xi, 15 );
    aie::accum<acc64,16> a0, a1;
    v16int32 x0 = shuffle( xi, T32_4x4 );
    v16int32 x1 = shuffle( x0, T256_2x2_hi );
    v32int16 y0 = (v32int16) shuffle( yi, T16_32x2_lo );
    v32int16 y1 = (v32int16) shuffle( yi, T16_32x2_hi );
    a0 = mul_4x2_2x4( x0, y1 );
    a1 = mul_4x2_2x4( x1, y1 );
    a0 = mac_4x2_2x4_conf( x0, true, y0, false, a0, 0, 1, 0, 0 );
    a1 = mac_4x2_2x4_conf( x1, true, y0, false, a1, 0, 1, 0, 0 );
    return aie::concat( a0, a1 );
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mul_outer_prod<float,4,float,8,accfloat,0>( aie::vector<float,4> x, aie::vector<float,8> y )
{
    auto x0 = aie::broadcast<float,8>( x[0] );
    auto x1 = aie::broadcast<float,8>( x[1] );
    auto x2 = aie::broadcast<float,8>( x[2] );
    auto x3 = aie::broadcast<float,8>( x[3] );
    return mul_elew<float,32,float,accfloat,0>( aie::concat( x0, x1, x2, x3 ), vector_broadcast<32>( y ));
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mul_outer_prod<float,4,float,8,accfloat,1>( aie::vector<float,4> x, aie::vector<float,8> y )
{
    auto x0 = aie::broadcast<float,8>( x[0] );
    auto x1 = aie::broadcast<float,8>( x[1] );
    auto x2 = aie::broadcast<float,8>( x[2] );
    auto x3 = aie::broadcast<float,8>( x[3] );
    return mul_elew<float,32,float,accfloat,1>( aie::concat( x0, x1, x2, x3 ), vector_broadcast<32>( y ));
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mul_outer_prod<float,4,float,8,accfloat,2>( aie::vector<float,4> x, aie::vector<float,8> y )
{
    auto x0 = aie::broadcast<float,8>( x[0] );
    auto x1 = aie::broadcast<float,8>( x[1] );
    auto x2 = aie::broadcast<float,8>( x[2] );
    auto x3 = aie::broadcast<float,8>( x[3] );
    return mul_elew<float,32,float,accfloat,2>( aie::concat( x0, x1, x2, x3 ), vector_broadcast<32>( y ));
}
template<>
ALWAYS_INLINE aie::accum<accfloat,16> mul_outer_prod<float,4,float,4,accfloat,0>( aie::vector<float,4> x, aie::vector<float,4> y )
{
    return mul_elew<float,16,float,accfloat,0>( aie::transpose( vector_broadcast<16>( x ), 4, 4 ), vector_broadcast<16>( y ));
}
template<>
ALWAYS_INLINE aie::accum<accfloat,16> mul_outer_prod<float,4,float,4,accfloat,1>( aie::vector<float,4> x, aie::vector<float,4> y )
{
    return mul_elew<float,16,float,accfloat,1>( aie::transpose( vector_broadcast<16>( x ), 4, 4 ), vector_broadcast<16>( y ));
}
template<>
ALWAYS_INLINE aie::accum<accfloat,16> mul_outer_prod<float,4,float,4,accfloat,2>( aie::vector<float,4> x, aie::vector<float,4> y )
{
    return mul_elew<float,16,float,accfloat,2>( aie::transpose( vector_broadcast<16>( x ), 4, 4 ), vector_broadcast<16>( y ));
}
#else
template<>
ALWAYS_INLINE aie::accum<acc32,64> mul_outer_prod( aie::vector<int16,8> x, aie::vector<int16,8> y )
{
    v32int16 xi = x.template grow<32>( );
    v32int16 yi = y.template grow<32>( );
    xi = shuffle( xi, broadcast_zero_to_v32int16(), T16_2x32_lo );
    v64acc32 acc = mul_8x2_2x8( xi, yi );
    return aie::accum<acc32,64>( acc );
}
template<>
ALWAYS_INLINE aie::accum<acc32,64> mul_outer_prod( aie::vector<uint16,8> x, aie::vector<int16,8> y )
{
    v32uint16 xi = x.template grow<32>( );
    v32int16  yi = y.template grow<32>( );
    xi = shuffle( xi, broadcast_zero_to_v32uint16(), T16_2x32_lo );
    v64acc32 acc = mul_8x2_2x8( xi, yi );
    return aie::accum<acc32,64>( acc );
}
template<>
ALWAYS_INLINE aie::accum<acc48,32> mul_outer_prod( aie::vector<int32,4> x, aie::vector<int16,8> y )
{
    v16int32 chess_storage( x2 ) xi = x.template grow<16>( );
    v32int16 yi = y.template grow<32>( );
    xi = shuffle( xi, broadcast_zero_to_v16int32( ), T32_2x16_lo );
    aie::accum<acc48,32> acc = mul_4x2_2x8( xi, yi );
    return acc;
}
template<>
ALWAYS_INLINE aie::accum<acc64,32> mul_outer_prod( aie::vector<int32,4> x, aie::vector<int32,8> y )
{
    v16int32 chess_storage( x2 ) xi = x.template grow<16>( );
    v16int32 chess_storage( x3 ) yi = y.template grow<16>( );
    aie::accum<acc64,32> a0;
    xi = shuffle( xi, broadcast_zero_to_v16int32( ), T32_2x16_lo );
    v32int16 y0 = (v32int16) shuffle( yi, T16_32x2_lo );
    v32int16 chess_storage( x3 ) y1 = (v32int16) shuffle( yi, T16_32x2_hi );
    a0 = mul_4x2_2x8( xi, y1 );
    a0 = mac_4x2_2x8_conf( xi, true, y0, false, a0, 0, 1, 0, 0 );
    return a0;
}
#endif
template<>
ALWAYS_INLINE aie::accum<accfloat,64> mul_outer_prod( aie::vector<bfloat16,8> x, aie::vector<bfloat16,8> y )
{
    return aie::mul( aie::transpose( vector_broadcast<64>( x ), 8, 8 ), vector_broadcast<64>( y ));
}

// mac_outer_prod is obsolete once aie::mmul<M,1,N> shapes are supported
template<typename Tx, unsigned Vx, typename Ty, unsigned Vy, typename Ta=aie::detail::accum_tag_for_mul_types<Tx,Ty>, unsigned fp_accuracy_mode=1>
ALWAYS_INLINE aie::accum<Ta,Vx*Vy> mac_outer_prod( aie::accum<Ta,Vx*Vy> acc, aie::vector<Tx,Vx> x, aie::vector<Ty,Vy> y )
{
    aie::mmul<Vx, 1, Vy, Tx, Ty> m( acc );
    m.mac( x, y );
    return m.to_accum();
}
#if __AIE_ARCH__ == 20
template<>
ALWAYS_INLINE aie::accum<acc32,64> mac_outer_prod( aie::accum<acc32,64> acc, aie::vector<int16,8> x, aie::vector<int16,8> y )
{
    auto xi = x.template grow<32>( );
    xi = shuffle( xi, broadcast_zero_to_v32int16(), T16_2x32_lo );
    aie::mmul<8, 2, 8, int16, int16> m( acc );
    m.mac( xi.template extract<16>( 0 ), y.template grow<16>( ));
    return m.to_accum();
}
template<>
ALWAYS_INLINE aie::accum<acc48,32> mac_outer_prod( aie::accum<acc48,32> acc, aie::vector<int32,4> x, aie::vector<int16,8> y )
{
    v16int32 xi = x.template grow<16>( );
    v32int16 yi = y.template grow<32>( );
    xi = sel( shift( broadcast_zero_to_v16int32( ), xi, 4 ), xi, 15 );
    aie::accum<acc64,16> a0, a1;
    v16int32 chess_storage(x1) x0 = shuffle( xi, T32_4x4 );
    v16int32 chess_storage(x1) x1 = shuffle( x0, T256_2x2_hi );
    a0 = mac_4x2_2x4( x0, yi, acc.template extract<16>( 0 ));
    a1 = mac_4x2_2x4( x1, yi, acc.template extract<16>( 1 ));
    return aie::concat( a0, a1 );
}
template<>
ALWAYS_INLINE aie::accum<acc64,32> mac_outer_prod( aie::accum<acc64,32> acc, aie::vector<int32,4> x, aie::vector<int32,8> y )
{
    v16int32 xi = x.template grow<16>( );
    v16int32 yi = y.template grow<16>( );
    xi = sel( shift( broadcast_zero_to_v16int32( ), xi, 4 ), xi, 15 );
    aie::accum<acc64,16> a0, a1;
    v16int32 x0 = shuffle( xi, T32_4x4 );
    v16int32 x1 = shuffle( x0, T256_2x2_hi );
    v32int16 y0 = (v32int16) shuffle( yi, T16_32x2_lo );
    v32int16 y1 = (v32int16) shuffle( yi, T16_32x2_hi );
    a0 = mul_4x2_2x4( x0, y1 );
    a1 = mul_4x2_2x4( x1, y1 );
    a0 = addmac_4x2_2x4_conf( x0, true, y0, false, a0, acc.template extract<16>( 0 ), 0, 1, 0, 0, 0 );
    a1 = addmac_4x2_2x4_conf( x1, true, y0, false, a1, acc.template extract<16>( 1 ), 0, 1, 0, 0, 0 );
    return aie::concat( a0, a1 );
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mac_outer_prod<float,4,float,8,accfloat,0>( aie::accum<accfloat,32> acc, aie::vector<float,4> x, aie::vector<float,8> y )
{
    auto x0 = aie::broadcast<float,8>( x[0] );
    auto x1 = aie::broadcast<float,8>( x[1] );
    auto x2 = aie::broadcast<float,8>( x[2] );
    auto x3 = aie::broadcast<float,8>( x[3] );
    return mac_elew<float,32,float,accfloat,0>( acc, aie::concat( x0, x1, x2, x3 ), vector_broadcast<32>( y ));
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mac_outer_prod<float,4,float,8,accfloat,1>( aie::accum<accfloat,32> acc, aie::vector<float,4> x, aie::vector<float,8> y )
{
    auto x0 = aie::broadcast<float,8>( x[0] );
    auto x1 = aie::broadcast<float,8>( x[1] );
    auto x2 = aie::broadcast<float,8>( x[2] );
    auto x3 = aie::broadcast<float,8>( x[3] );
    return mac_elew<float,32,float,accfloat,1>( acc, aie::concat( x0, x1, x2, x3 ), vector_broadcast<32>( y ));
}
template<>
ALWAYS_INLINE aie::accum<accfloat,32> mac_outer_prod<float,4,float,8,accfloat,2>( aie::accum<accfloat,32> acc, aie::vector<float,4> x, aie::vector<float,8> y )
{
    auto x0 = aie::broadcast<float,8>( x[0] );
    auto x1 = aie::broadcast<float,8>( x[1] );
    auto x2 = aie::broadcast<float,8>( x[2] );
    auto x3 = aie::broadcast<float,8>( x[3] );
    return mac_elew<float,32,float,accfloat,2>( acc, aie::concat( x0, x1, x2, x3 ), vector_broadcast<32>( y ));
}
template<>
ALWAYS_INLINE aie::accum<accfloat,16> mac_outer_prod<float,4,float,4,accfloat,0>( aie::accum<accfloat,16> acc, aie::vector<float,4> x, aie::vector<float,4> y )
{
    return mac_elew<float,16,float,accfloat,0>( acc, aie::transpose( vector_broadcast<16>( x ), 4, 4 ), vector_broadcast<16>( y ));
}
template<>
ALWAYS_INLINE aie::accum<accfloat,16> mac_outer_prod<float,4,float,4,accfloat,1>( aie::accum<accfloat,16> acc, aie::vector<float,4> x, aie::vector<float,4> y )
{
    return mac_elew<float,16,float,accfloat,1>( acc, aie::transpose( vector_broadcast<16>( x ), 4, 4 ), vector_broadcast<16>( y ));
}
template<>
ALWAYS_INLINE aie::accum<accfloat,16> mac_outer_prod<float,4,float,4,accfloat,2>( aie::accum<accfloat,16> acc, aie::vector<float,4> x, aie::vector<float,4> y )
{
    return mac_elew<float,16,float,accfloat,2>( acc, aie::transpose( vector_broadcast<16>( x ), 4, 4 ), vector_broadcast<16>( y ));
}
#else
template<>
ALWAYS_INLINE aie::accum<acc32,64> mac_outer_prod( aie::accum<acc32,64> acc, aie::vector<int16,8> x, aie::vector<int16,8> y )
{
    v32int16 xi = x.template grow<32>( );
    v32int16 yi = y.template grow<32>( );
    acc = mac_8x2_2x8( shuffle( xi, broadcast_zero_to_v32int16(), T16_2x32_lo ), yi, acc );
    return acc;
}
template<>
ALWAYS_INLINE aie::accum<acc48,32> mac_outer_prod( aie::accum<acc48,32> acc, aie::vector<int32,4> x, aie::vector<int16,8> y )
{
    v16int32 xi = x.template grow<16>( );
    v32int16 yi = y.template grow<32>( );
    xi = shuffle( xi, broadcast_zero_to_v16int32( ), T32_2x16_lo );
    acc = mac_4x2_2x8( xi, yi, acc );
    return acc;
}
template<>
ALWAYS_INLINE aie::accum<acc64,32> mac_outer_prod( aie::accum<acc64,32> acc, aie::vector<int32,4> x, aie::vector<int32,8> y )
{
    v16int32 chess_storage(x0) xi = x.template grow<16>( );
    v16int32 chess_storage(x1) yi = y.template grow<16>( );
    aie::accum<acc64,32> a0;
    xi = shuffle( xi, broadcast_zero_to_v16int32( ), T32_2x16_lo );
    v32int16 chess_storage(x11) y0 = (v32int16) shuffle( yi, T16_32x2_lo );
    v32int16 chess_storage(x11) y1 = (v32int16) shuffle( yi, T16_32x2_hi );
    a0 = mul_4x2_2x8( xi, y1 );
    a0 = addmac_4x2_2x8_conf( xi, true, y0, false, a0, acc, 0, 1, 0, 0, 0 );
    return a0;
}
#endif
template<>
ALWAYS_INLINE aie::accum<accfloat,64> mac_outer_prod( aie::accum<accfloat,64> acc, aie::vector<bfloat16,8> x, aie::vector<bfloat16,8> y )
{
    return aie::mac( acc, aie::transpose( vector_broadcast<64>( x ), 8, 8 ), vector_broadcast<64>( y ));
}



// accum_broadcast becomes obsolete when aie::accum instantiation would support boradcasting. 
// Alternative the generic implementation can be used when vector broadcasting is supported
template<unsigned Vo, typename T, unsigned Vi, typename T2=T, typename Ta=aie::detail::accum_tag_for_mul_types<T,T2>>
aie::accum<Ta,Vo> accum_broadcast( aie::vector<T,Vi> e, int shift=0 )
{
    //return aie::accum<Ta,Vo>( aie::broadcast( e ), shift ); //Currently not supported, implemented in LLI below
    return aie::accum<Ta,Vo>( vector_broadcast<Vo>( e ), shift );
}
template<>
aie::accum<acc32,32> accum_broadcast( aie::vector<int16,8> e, int shift )
{
  #if FAST_INT16_ACCUM_BROADCAST
    return mul_outer_prod( aie::broadcast<uint16,8>( 1 << shift ), e ).template extract<32>( 0 );
  #else
    return aie::accum<acc32,32>( vector_broadcast<32>( e ), shift );
  #endif
}
template<>
aie::accum<acc32,64> accum_broadcast( aie::vector<int16,8> e, int shift )
{
  #if FAST_INT16_ACCUM_BROADCAST && __AIE_ARCH__ >= 21
    return mul_outer_prod( aie::broadcast<uint16,8>( 1 << shift ), e );
  #else
    auto v = accum_broadcast<32>( e, shift );
    return aie::concat( v, v );
  #endif
}
template<>
aie::accum<acc64,16> accum_broadcast( aie::vector<int32,8> e, int shift )
{
  //#if FAST_INT16_ACCUM_BROADCAST
  //  return mul_outer_prod( aie::broadcast<uint16,8>( 1 << shift ), e ).template extract<32>( 0 );
  //#else
    return aie::accum<acc64,16>( vector_broadcast<16>( e ), shift );
  //#endif
}
template<>
aie::accum<acc64,32> accum_broadcast( aie::vector<int32,8> e, int shift )
{
  //#if FAST_INT16_ACCUM_BROADCAST && __AIE_ARCH__ >= 21
  //  return mul_outer_prod( aie::broadcast<uint16,8>( 1 << shift ), e );
  //#else
    auto v = accum_broadcast<16>( e, shift );
    return aie::concat( v, v );
  //#endif
}
#if HAS_FLOAT
template<>
aie::accum<accfloat,64> accum_broadcast( aie::vector<float,8> e, int shift )
{
    auto v = accum_broadcast<32>( e, shift );
    return aie::concat( v, v );
}
#endif



// Generic convert function for conversion between different datatypes (Not complete)
template<typename To, typename Ti, unsigned V>
ALWAYS_INLINE aie::vector<To,V> convert( aie::vector<Ti,V> in, int shift=0 )
{
    //if constexpr( __AIE_ARCH__ == 20 && V == 64 ) {
    if constexpr( V == 64 && ( sizeof( Ti ) >= 4 || sizeof( To ) >= 4 || ( std::is_same_v<bfloat16,Ti> ^ std::is_same_v<bfloat16,To> ))) {
        auto a = convert<To>( in.template extract<32>( 0 ), shift );
        auto b = convert<To>( in.template extract<32>( 1 ), shift );
        return aie::concat( a, b );
    } else if constexpr( std::is_same_v<Ti,To> )
        return in;
    else if constexpr( std::is_integral_v<To> ) {
        if constexpr( std::is_integral_v<Ti> )
            return aie::accum<acc32,V>( in, std::max( 0, -shift )).template to_vector<To>( std::max( 0, shift ));
        else if constexpr( std::is_same_v<To,int8> )
            return aie::pack( convert<int16>( in ));
        else if constexpr( std::is_same_v<bfloat16,Ti> ) {
            return aie::to_fixed<To>( aie::accum<accfloat,V>( in ).template to_vector<float>( ), shift );
        } else
            return aie::to_fixed<To>( in, shift );
    } else if constexpr( std::is_integral_v<Ti> ) {
        if constexpr( std::is_same_v<Ti,int8> )
            return convert<To>( aie::unpack( in ));
        else if constexpr( std::is_same_v<bfloat16,To> )
            return aie::accum<accfloat,V>( aie::to_float<float>( in, shift )).template to_vector<bfloat16>( );
        else
            return aie::to_float<To>( in, shift );
    } else
        return aie::accum<accfloat,V>( in ).template to_vector<To>( );
}
#if __AIE_ARCH__ == 21
inline v32acc32 add( v32acc32 a, v32acc32 b ) {
    return extract_v32acc32( add( set_v64acc32( 0, a ), set_v64acc32( 0, b )), 0 );
}

inline v32acc32 operator+( v32acc32 a, v32acc32 b ) {
    return extract_v32acc32( add( set_v64acc32( 0, a ), set_v64acc32( 0, b )), 0 );
}
inline v32acc32 operator-( v32acc32 a, v32acc32 b ) {
    return extract_v32acc32( sub( set_v64acc32( 0, a ), set_v64acc32( 0, b )), 0 );
}
inline v32acc32 operator-( v32acc32 a ) {
    return extract_v32acc32( neg( set_v64acc32( 0, a )), 0 );
}

template<>
ALWAYS_INLINE aie::vector<bfloat16,32> convert( aie::vector<int32,32> in, int shift )
{
    v64accfloat acc;
    #pragma unroll
    for (unsigned it = 0; it < 2; it++ ) {
        int16 m = 0x3f81 + 2048 * it + 128 * shift;
        v64accfloat magic = set_v64accfloat( 0, ups( broadcast_to_v32bfloat16(*( bfloat16* )&m )));
        v32int16 part = (v32int16) shuffle( in.template extract<16>( 0 ), in.template extract<16>( 1 ), T16_32x2_lo + it );
        v64acc32 part_i = add(( v64acc32 ) magic, set_v64acc32( 0, ups_to_v32acc32( part, 0, it >= 1 )));
        v64accfloat part_f = sub(( v64accfloat ) part_i, magic );
        if ( it == 0 )
            acc = part_f;
        else
            acc += part_f;
    }
    return to_v32bfloat16( extract_v32accfloat( acc, 0 ));
}
#endif



#if HAS_FLOAT// && __AIE_ARCH__ < 21

template <>
struct aie::detail::elementary_vector_bits_impl<aie::detail::ElementaryOp::Float2Fix, 32, int32, float, 8>
{
    using vector_ret_type = vector<int, 8>;
    using     vector_type = vector<float, 8>;

    __aie_inline
    static vector_ret_type run(const vector_type &v, int shift = 0)
    {
        accum<accfloat,32> magic_h, magic_l;

        const vector<bfloat16, 32> magic_h_tmp = broadcast<int16, 32>::run(0x5301 - 128 * shift).cast_to<bfloat16>();
        const vector<bfloat16, 32> magic_l_tmp = broadcast<int16, 32>::run(0x4b01 - 128 * shift).cast_to<bfloat16>();
        
        const saturation_mode sat = tile::current().get_saturation();
        //const rounding_mode   rnd = tile::current().get_saturation();
        tile::current().set_saturation(saturation_mode::saturate);

        magic_h.insert<16>(0, ::ups_to_v16accfloat(magic_h_tmp.extract<16>(0)));
        magic_l.insert<16>(0, ::ups_to_v16accfloat(magic_l_tmp.extract<16>(0)));

        accum<accfloat, 16> acc_input(v.template grow<16>());
        accum<accfloat, 16> vfp = (v16accfloat) acc_input + magic_h.extract<16>(0);

        accum<acc32,32> vint = (v32acc32)vfp.grow<32>() - (v32acc32)magic_h;

        vector<int16, 16> out_h = vint.extract<16>(0).to_vector<int16>();

        vint = ::sups(out_h.grow<32>(), 0) + (v32acc32) magic_h;
        

        vfp = acc_input - ((v16accfloat) vint.extract<16>(0) - magic_h.extract<16>(0)) + magic_l.extract<16>(0);

        vint = (v32acc32) vfp.grow<32>() - (v32acc32) magic_l;

        vector<uint16, 16> out_lp = vint.extract<16>(0).to_vector<uint16>();
        vector<uint16, 16> out_ln = ::ulsrs( extract_v16acc32( -(v32acc32)vint, 0), 0);

        vector<int32, 16> output = ::lsrs(::lups(out_h, 16) + ::lups(out_lp, 0) - ::lups(out_ln, 0), 0);
        
        //report( acc_input ); 
        //report( output );
        //report( shift );
        //report( vfp );
        //report( out_h ); 
        //report( out_lp ); 
        //report( out_ln );

        tile::current().set_saturation(sat);

        return output.template extract<8>(0);
    }
};
#endif
#if HAS_FLOAT && __AIE_ARCH__ >= 21
template<typename To, typename Ti, unsigned V>
requires( !std::is_integral_v<Ti> && std::is_integral_v<To> )
ALWAYS_INLINE aie::vector<To,V> convert( aie::vector<Ti,V> in, int shift=0 )
{
    constexpr unsigned Vop = std::max( V, 32u );
    using acc_fp_t  = aie::accum<accfloat, Vop>;
    using acc_int_t = aie::accum<acc32, Vop>;
    const acc_fp_t magic_h( aie::broadcast<int16, V>( 0x5301 - 128 * shift ).template cast_to<bfloat16>( ));
    const acc_fp_t magic_l( aie::broadcast<int16, V>( 0x4b01 - 128 * shift ).template cast_to<bfloat16>( ));
    
    const aie::saturation_mode sat = aie::tile::current().get_saturation();
    //const rounding_mode   rnd = tile::current().get_saturation();
    aie::tile::current().set_saturation(aie::saturation_mode::saturate);

    const acc_fp_t acc_input( in.template grow<Vop>( ));
    acc_fp_t vfp = acc_input;
    acc_int_t vint;
    aie::vector<int16, Vop> out_h;
    aie::vector<To, V> output;
    
    if constexpr( sizeof( To ) > 2 ) {
        vfp = vfp + magic_h;
        vint = vfp.template cast_to<acc32>( ) - magic_h.template cast_to<acc32>( );
    
        out_h = vint.template to_vector<int16>();
    
        vint = acc_int_t( out_h ) + magic_h.template cast_to<acc32>( );
        vfp = acc_input - ( vint.template cast_to<accfloat>( ) - magic_h );
    }
    vfp = vfp + magic_l;
    vint = vfp.template cast_to<acc32>( ) - magic_l.template cast_to<acc32>( );
    
    if constexpr( sizeof( To ) > 2 ) {
        auto out_lp = vint.template to_vector<uint16>().template extract<V>( 0 );
        auto out_ln = acc_int_t( -vint ).template to_vector<uint16>().template extract<V>( 0 );
    
        using acc_t = aie::accum<acc64, V>;
        auto acc = acc_t( acc_t( out_h.template extract<V>( 0 ), 16 ) + acc_t( out_lp ) - acc_t( out_ln ));
        output = acc.template to_vector<To>( );
    } else {
        output = vint.template to_vector<To>( ).template extract<V>( 0 );
    }

    aie::tile::current().set_saturation(sat);

    return output;
}

template<typename To, typename Ti, unsigned V>
requires( std::is_integral_v<Ti> && !std::is_integral_v<To> )
ALWAYS_INLINE aie::vector<To,V> convert( aie::vector<Ti,V> in, int shift=0 )
{
    constexpr unsigned Vop = std::max( V, 32u );
    using acc_fp_t  = aie::accum<accfloat, Vop>;
    using acc_int_t = aie::accum<acc32, Vop>;
    const acc_fp_t magic_h( aie::broadcast<int16, V>( 0x5301 - 128 * shift ).template cast_to<bfloat16>( ));
    const acc_fp_t magic_l( aie::broadcast<int16, V>( 0x4b01 - 128 * shift ).template cast_to<bfloat16>( ));
    
    const aie::saturation_mode sat = aie::tile::current().get_saturation();
    //const rounding_mode   rnd = tile::current().get_saturation();
    aie::tile::current().set_saturation(aie::saturation_mode::saturate);

    acc_fp_t vfp;
    if constexpr( sizeof( Ti ) > 2 ) {
        acc_int_t vint_h( aie::filter_odd(  in.template cast_to< int16>( )).template grow<Vop>( ));
        acc_int_t vint_l( aie::filter_even( in.template cast_to<uint16>( )).template grow<Vop>( ));
        
        vint_h = vint_h + magic_h.template cast_to<acc32>( );
        vint_l = vint_l + magic_l.template cast_to<acc32>( );

        vfp = vint_h.template cast_to<accfloat>( ) - magic_h;
        vfp = (vint_l.template cast_to<accfloat>( ) - magic_l ) + vfp;
    } else {
        acc_int_t vint( in.template grow<Vop>( ));
        vint = vint + magic_l.template cast_to<acc32>( );
        vfp = vint.template cast_to<accfloat>( ) - magic_l;
    }

    aie::vector<To, V> output = vfp.template extract<V>( 0 ).template to_vector<To>();
    
    aie::tile::current().set_saturation(sat);

    return output;
}
/*
template <>
struct aie::detail::elementary_vector_bits_impl<aie::detail::ElementaryOp::Float2Fix, 32, int32, float, 32>
{
    using vector_ret_type = vector<int, 32>;
    using     vector_type = vector<float, 32>;

    __aie_inline
    static vector_ret_type run(const vector_type &v, int shift = 0)
    {
        return convert<int>( v, shift );
    }
};
template <>
struct aie::detail::elementary_vector_bits_impl<aie::detail::ElementaryOp::Float2Fix, 32, int16, float, 32>
{
    using vector_ret_type = vector<int16, 32>;
    using     vector_type = vector<float, 32>;

    __aie_inline
    static vector_ret_type run(const vector_type &v, int shift = 0)
    {
        return convert<int16>( v, shift );
    }
};
template <>
struct aie::detail::elementary_vector_bits_impl<aie::detail::ElementaryOp::Float2Fix, 32, int8, float, 32>
{
    using vector_ret_type = vector<int8, 32>;
    using     vector_type = vector<float, 32>;

    __aie_inline
    static vector_ret_type run(const vector_type &v, int shift = 0)
    {
        return convert<int8>( v, shift );
    }
};
template <>
struct aie::detail::elementary_vector_bits_impl<aie::detail::ElementaryOp::Fix2Float, 32, float, int32, 32>
{
    using vector_ret_type = vector<float, 32>;
    using     vector_type = vector<int32, 32>;

    __aie_inline
    static vector_ret_type run(const vector_type &v, int shift = 0)
    {
        return convert<float>( v, shift );
    }
};
template <>
struct aie::detail::elementary_vector_bits_impl<aie::detail::ElementaryOp::Fix2Float, 32, float, int16, 32>
{
    using vector_ret_type = vector<float, 32>;
    using     vector_type = vector<int16, 32>;

    __aie_inline
    static vector_ret_type run(const vector_type &v, int shift = 0)
    {
        return convert<float>( v, shift );
    }
};
*/
#endif

// This zip wrapper commonly works, just 128b types have a performance issue (CRVO-7534)
template<typename T, unsigned V>
inline aie::vector<T,2*V> zip( aie::vector<T,V> a, aie::vector<T,V> b ) {
    auto [c,d] = aie::interleave_zip( a, b, 1 );
    return aie::concat( c, d );
}
template<>
inline aie::vector<int16,16> zip( aie::vector<int16,8> a, aie::vector<int16,8> b ) {
    aie::vector<int16,32> c = shuffle( a.template grow<32>( ), b.template grow<32>( ), T16_2x32_lo );
    return c.template extract<16>( 0 );
}
template<typename T, unsigned V>
inline aie::vector<T,2*V> zip( T a, aie::vector<T,V> b ) {
    auto [c,d] = aie::interleave_zip( aie::broadcast<T,V>( a ), b, 1 );
    return aie::concat( c, d );
}
template<>
inline aie::vector<int16,16> zip( int16 a, aie::vector<int16,8> b ) {
    aie::vector<int16,32> c = shuffle( aie::broadcast<int16,32>( a ), b.template grow<32>( ), T16_2x32_lo );
    return c.template extract<16>( 0 );
}



template<unsigned reg, typename T, unsigned V>
inline aie::vector<T,V> locate_in_register( aie::vector<T,V> vec ) {
    if constexpr( vec.bits( ) == 512 ) {
        if constexpr( reg ==  0 ) { auto __aie_register( x0  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  1 ) { auto __aie_register( x1  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  2 ) { auto __aie_register( x2  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  3 ) { auto __aie_register( x3  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  4 ) { auto __aie_register( x4  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  5 ) { auto __aie_register( x5  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  6 ) { auto __aie_register( x6  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  7 ) { auto __aie_register( x7  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  8 ) { auto __aie_register( x8  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  9 ) { auto __aie_register( x9  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg == 10 ) { auto __aie_register( x10 ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg == 11 ) { auto __aie_register( x11 ) tmp = vec; vec = __aie_copy( tmp ); }
    } else if constexpr( vec.bits( ) == 1024 ) {
        if constexpr( reg ==  0 ) { auto __aie_register( y0  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  1 ) { auto __aie_register( y1  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  2 ) { auto __aie_register( y2  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  3 ) { auto __aie_register( y3  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  4 ) { auto __aie_register( y4  ) tmp = vec; vec = __aie_copy( tmp ); }
        if constexpr( reg ==  5 ) { auto __aie_register( y5  ) tmp = vec; vec = __aie_copy( tmp ); }
    } else {
        chess_error( "locate_in_register not yet implemented for this type" );
    }
    return vec;
}
        

template<unsigned reg, aie::Accum Ta>
inline Ta locate_in_register( Ta acc ) {
    if constexpr( acc.bits( ) == 1024 ) {
      #if __AIE_ARCH__ == 20
        if constexpr( reg ==  0 ) { auto __aie_register( cm0  ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  1 ) { auto __aie_register( cm1  ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  2 ) { auto __aie_register( cm2  ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  3 ) { auto __aie_register( cm3  ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  4 ) { auto __aie_register( cm4  ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  5 ) { auto __aie_register( cm5  ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  6 ) { auto __aie_register( cm6  ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  7 ) { auto __aie_register( cm7  ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  8 ) { auto __aie_register( cm8  ) tmp = acc; acc = __aie_copy( tmp ); }
      #elif __AIE_ARCH__ >= 21
        if constexpr( reg ==  0 ) { auto __aie_register( cml0 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  1 ) { auto __aie_register( cmh0 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  2 ) { auto __aie_register( cml1 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  3 ) { auto __aie_register( cmh1 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  4 ) { auto __aie_register( cml2 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  5 ) { auto __aie_register( cmh2 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  6 ) { auto __aie_register( cml3 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  7 ) { auto __aie_register( cmh3 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  8 ) { auto __aie_register( cml4 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg ==  9 ) { auto __aie_register( cmh4 ) tmp = acc; acc = __aie_copy( tmp ); }
       //#if __AIE_ARCH__ >= 22
       // if constexpr( reg == 10 ) { auto __aie_register( cml5 ) tmp = acc; acc = __aie_copy( tmp ); }
       // if constexpr( reg == 11 ) { auto __aie_register( cmh5 ) tmp = acc; acc = __aie_copy( tmp ); }
       // if constexpr( reg == 12 ) { auto __aie_register( cml6 ) tmp = acc; acc = __aie_copy( tmp ); }
       // if constexpr( reg == 13 ) { auto __aie_register( cmh6 ) tmp = acc; acc = __aie_copy( tmp ); }
       // if constexpr( reg == 14 ) { auto __aie_register( cml7 ) tmp = acc; acc = __aie_copy( tmp ); }
       // if constexpr( reg == 15 ) { auto __aie_register( cmh7 ) tmp = acc; acc = __aie_copy( tmp ); }
       //#endif
      #endif
        
    } else if constexpr( acc.bits( ) == 2048 ) {
      #if __AIE_ARCH__ == 20
        if constexpr( reg == 0 ) { 
            auto __aie_register( cm0 ) tmp0 = acc.template extract<32>( 0 );
            auto __aie_register( cm1 ) tmp1 = acc.template extract<32>( 1 );
            acc = aie::concat( __aie_copy( tmp0 ), __aie_copy( tmp1 )); }
        if constexpr( reg == 1 ) { 
            auto __aie_register( cm2 ) tmp0 = acc.template extract<32>( 0 );
            auto __aie_register( cm3 ) tmp1 = acc.template extract<32>( 1 );
            acc = aie::concat( __aie_copy( tmp0 ), __aie_copy( tmp1 )); }
        if constexpr( reg == 2 ) { 
            auto __aie_register( cm4 ) tmp0 = acc.template extract<32>( 0 );
            auto __aie_register( cm5 ) tmp1 = acc.template extract<32>( 1 );
            acc = aie::concat( __aie_copy( tmp0 ), __aie_copy( tmp1 )); }
        if constexpr( reg == 3 ) { 
            auto __aie_register( cm6 ) tmp0 = acc.template extract<32>( 0 );
            auto __aie_register( cm7 ) tmp1 = acc.template extract<32>( 1 );
            acc = aie::concat( __aie_copy( tmp0 ), __aie_copy( tmp1 )); }
      #elif __AIE_ARCH__ >= 21
        if constexpr( reg == 0 ) { auto __aie_register( dm0 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg == 1 ) { auto __aie_register( dm1 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg == 2 ) { auto __aie_register( dm2 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg == 3 ) { auto __aie_register( dm3 ) tmp = acc; acc = __aie_copy( tmp ); }
        if constexpr( reg == 4 ) { auto __aie_register( dm4 ) tmp = acc; acc = __aie_copy( tmp ); }
       //#if __AIE_ARCH__ >= 22
       // if constexpr( reg == 5 ) { auto __aie_register( dm5 ) tmp = acc; acc = __aie_copy( tmp ); }
       // if constexpr( reg == 6 ) { auto __aie_register( dm6 ) tmp = acc; acc = __aie_copy( tmp ); }
       // if constexpr( reg == 7 ) { auto __aie_register( dm7 ) tmp = acc; acc = __aie_copy( tmp ); }
       //#endif
      #endif
    } else {
        chess_error( "locate_in_register not yet implemented for this type" );
    }
    return acc;
}


template<unsigned reg>
inline aie::mmul<4, 8, 8, int8, int8> locate_in_register( aie::mmul<4, 8, 8, int8, int8> mm ) {
    #if __AIE_ARCH__ == 20
    if constexpr( reg == 0 ) { auto __aie_register( cm0 ) tmp = mm.data; mm.data = __aie_copy( tmp ); }
    if constexpr( reg == 1 ) { auto __aie_register( cm1 ) tmp = mm.data; mm.data = __aie_copy( tmp ); }
    if constexpr( reg == 2 ) { auto __aie_register( cm2 ) tmp = mm.data; mm.data = __aie_copy( tmp ); }
    if constexpr( reg == 3 ) { auto __aie_register( cm3 ) tmp = mm.data; mm.data = __aie_copy( tmp ); }
    if constexpr( reg == 4 ) { auto __aie_register( cm4 ) tmp = mm.data; mm.data = __aie_copy( tmp ); }
    if constexpr( reg == 5 ) { auto __aie_register( cm5 ) tmp = mm.data; mm.data = __aie_copy( tmp ); }
    if constexpr( reg == 6 ) { auto __aie_register( cm6 ) tmp = mm.data; mm.data = __aie_copy( tmp ); }
    if constexpr( reg == 7 ) { auto __aie_register( cm7 ) tmp = mm.data; mm.data = __aie_copy( tmp ); }
    if constexpr( reg == 8 ) { auto __aie_register( cm8 ) tmp = mm.data; mm.data = __aie_copy( tmp ); }
    #elif __AIE_ARCH__ >= 21
    chess_error( "locate_in_register not yet implemented for this type" );
    #endif
    return mm;
}
template<unsigned reg>
inline aie::mmul<8, 8, 8, int8, int8> locate_in_register( aie::mmul<8, 8, 8, int8, int8> mm ) {
    #if __AIE_ARCH__ == 20
    if constexpr( reg == 0 ) { aie::accum<acc32,32> __aie_register( cm0 ) tmp0 = mm.data[0]; mm.data[0] = __aie_copy( tmp0 );
                               aie::accum<acc32,32> __aie_register( cm1 ) tmp1 = mm.data[1]; mm.data[1] = __aie_copy( tmp1 ); }
    if constexpr( reg == 1 ) { aie::accum<acc32,32> __aie_register( cm2 ) tmp0 = mm.data[0]; mm.data[0] = __aie_copy( tmp0 );
                               aie::accum<acc32,32> __aie_register( cm3 ) tmp1 = mm.data[1]; mm.data[1] = __aie_copy( tmp1 ); }
    if constexpr( reg == 2 ) { aie::accum<acc32,32> __aie_register( cm4 ) tmp0 = mm.data[0]; mm.data[0] = __aie_copy( tmp0 );
                               aie::accum<acc32,32> __aie_register( cm5 ) tmp1 = mm.data[1]; mm.data[1] = __aie_copy( tmp1 ); }
    if constexpr( reg == 3 ) { aie::accum<acc32,32> __aie_register( cm6 ) tmp0 = mm.data[0]; mm.data[0] = __aie_copy( tmp0 );
                               aie::accum<acc32,32> __aie_register( cm7 ) tmp1 = mm.data[1]; mm.data[1] = __aie_copy( tmp1 ); }
    #elif __AIE_ARCH__ >= 21
    if constexpr( reg == 0 ) { aie::accum<acc32,64> __aie_register( dm0 ) tmp = mm.to_accum( ); mm = aie::mmul<8, 8, 8, int8, int8>( __aie_copy( tmp )); }
    if constexpr( reg == 1 ) { aie::accum<acc32,64> __aie_register( dm1 ) tmp = mm.to_accum( ); mm = aie::mmul<8, 8, 8, int8, int8>( __aie_copy( tmp )); }
    if constexpr( reg == 2 ) { aie::accum<acc32,64> __aie_register( dm2 ) tmp = mm.to_accum( ); mm = aie::mmul<8, 8, 8, int8, int8>( __aie_copy( tmp )); }
    if constexpr( reg == 3 ) { aie::accum<acc32,64> __aie_register( dm3 ) tmp = mm.to_accum( ); mm = aie::mmul<8, 8, 8, int8, int8>( __aie_copy( tmp )); }
    if constexpr( reg == 4 ) { aie::accum<acc32,64> __aie_register( dm4 ) tmp = mm.to_accum( ); mm = aie::mmul<8, 8, 8, int8, int8>( __aie_copy( tmp )); }
    //#if __AIE_ARCH__ >= 22
    //if constexpr( reg == 5 ) { aie::accum<acc32,64> __aie_register( dm5 ) tmp = mm.to_accum( ); mm = aie::mmul<8, 8, 8, int8, int8>( __aie_copy( tmp )); }
    //if constexpr( reg == 6 ) { aie::accum<acc32,64> __aie_register( dm6 ) tmp = mm.to_accum( ); mm = aie::mmul<8, 8, 8, int8, int8>( __aie_copy( tmp )); }
    //if constexpr( reg == 7 ) { aie::accum<acc32,64> __aie_register( dm7 ) tmp = mm.to_accum( ); mm = aie::mmul<8, 8, 8, int8, int8>( __aie_copy( tmp )); }
    //#endif
    #endif
    return mm;
}

template<typename T, unsigned size, unsigned reg_start, unsigned ... Is>
inline void locate_in_register_helper( T (&arr)[size], std::integer_sequence<unsigned, Is...> const & ) {
   (( arr[Is] = locate_in_register<reg_start+Is>( arr[Is] )), ... );
}

template<unsigned reg_start=0, unsigned size, typename T>
inline void locate_in_register( T (&arr)[size] ) {
    locate_in_register_helper<T,size,reg_start>( arr, std::make_integer_sequence<unsigned, size>{} );
}


inline sparsity_t get_sparse( int mask ) property( no_debug ) {
    v16int8 sparse = extract_v16int8( broadcast_s8( 0x11 * mask ), 0 );
    return *( sparsity_t* )&sparse; // TODO: create CRVO to get this enabled as a move
}


// get_cascade function gets obsolte once input_cascade interface support dynamic cascade read.
template<typename T>
inline T get_cascade( int casc_en ) {
    chess_error("Need to define this combination of T and V");
    return T();
}
template<>
inline aie::accum<acc32,16> get_cascade( int casc_en ) {
    return aie::accum<acc32,16>( get_scd_v16acc32( casc_en ));
}


template<typename Tnew, typename Told, typename Dir, typename Config, unsigned ... Is>
constexpr auto IOBufferDimsHelper( adf::io_buffer<Told, Dir, Config> old, std::index_sequence<Is...> ) {
    return (( old.size( Is ) * sizeof( Told ) / sizeof( Tnew )), ... );
}

// Cast data type of a adf::io_buffer to a different type andor direction for local inplace use of it.
template<typename Tnew, typename DirNew, typename Told, typename DirOld, typename Config>
ALWAYS_INLINE adf::io_buffer<Tnew, DirNew, Config> local_buffer_cast( adf::io_buffer<Told, DirOld, Config> old ) {
    auto size = old.size() * sizeof( Told ) / sizeof( Tnew );
    auto dims = IOBufferDimsHelper<Tnew>( old, std::make_index_sequence<old.num_dims( )>( ));
    return adf::io_buffer<Tnew, DirNew, Config>({( Tnew * ) old.base( ), size, 0, dims});
}

// Create a new buffer with the same type with possibly offset and limited size
template<typename T, typename Dir, typename Config>
ALWAYS_INLINE adf::io_buffer<T, Dir, Config> local_buffer( adf::io_buffer<T, Dir, Config> buf, unsigned offset=0, unsigned size=0 ) {
    static_assert( buf.num_dims( ) == 1 ); // Only 1d buffers supported
    offset = std::min( offset, buf.size( ) - 1 );
    size = std::min( size ? size : buf.size( ), buf.size( ) - offset );
    return adf::io_buffer<T, Dir, Config>({ buf.base( ) + offset, size, 0, size });
}


// This set of classes will be replace by multi dimentional iterator improvements currently under development (CRVO-7516)
class Add2dElem {
  public:
    int inc0;
    int num0;
    addr_t cnt0;
    int inc1;
        
    Add2dElem( int step0, int size0, int step1 ) {
        inc0 = step0;
        num0 = size0 - 1;
        cnt0 = 0;
        inc1 = step1 - inc0 * num0;
    }
    
    template<typename T>
    inline T operator()( T it ) {
        auto tmp = *it;
        // TODO this might not be correct
        if constexpr( std::is_class_v<decltype(tmp)> && !std::is_same_v<decltype(tmp),bfloat16> ) {
            using elem_type = typename decltype(tmp)::value_type;
            using pointer = typename T::pointer;
            pointer p = &*it;
            auto ptr = (elem_type*) p;
            ptr = add_2d_ptr( ptr, inc1, num0, cnt0, inc0 );
            return T(ptr);
        } else {
            return add_2d_ptr( it, inc1, num0, cnt0, inc0 );
        }
    }
};

class Add3dElem {
  public:
    int inc0;
    int num0;
    addr_t cnt0;
    int inc1;
    int num1;
    addr_t cnt1;
    int inc2;
    
    int chain_step;
    int chain_size;
        
    Add3dElem( int step0, int size0, int step1, int size1, int step2 ) {
        inc0 = step0;
        num0 = size0 - 1;
        int reset = step0 * num0;
        cnt0 = 0;
        inc1 = step1 - reset;
        num1 = size1 - 1;
        reset += step1 * num1;
        cnt1 = 0;
        inc2 = step2 - reset;
        chain_step = step2;
        chain_size = size0 * size1;
    }
        
    Add3dElem( Add3dElem &preceed, int size0, int step1, int size1, int step2 ) {
        inc0 = 0;
        num0 = preceed.chain_size * size0 - 1;
        int reset = preceed.chain_step * size0;
        cnt0 = 0;
        inc1 = step1 - reset;
        num1 = size1 - 1;
        reset += step1 * num1;
        cnt1 = 0;
        inc2 = step2 - reset;
        chain_step = step2;
        chain_size = preceed.chain_size * size0 * size1;
    }
    
    template<typename T>
    inline T operator()( T it ) {
        auto tmp = *it;
        // TODO this might not be correct
        if constexpr( std::is_class_v<decltype(tmp)> && !std::is_same_v<decltype(tmp),bfloat16> ) {
            using elem_type = typename decltype(tmp)::value_type;
            using pointer = typename T::pointer;
            pointer p = &*it;
            auto ptr = (elem_type*) p;
            ptr = add_3d_ptr( ptr, inc2, num0, cnt0, inc0, num1, cnt1, inc1 );
            return T(ptr);
        } else {
            return add_3d_ptr( it, inc2, num0, cnt0, inc0, num1, cnt1, inc1 );
        }
    }
};

class Add2dPtr {
  public:
    int inc0;
    int num0;
    addr_t cnt0;
    int inc1;
        
    Add2dPtr( int step0, int size0, int step1 ) {
        inc0 = step0;
        num0 = size0 - 1;
        cnt0 = 0;
        inc1 = step1 - inc0 * num0;
    }
    
    template<typename T>
    inline T operator()( T it ) {
        auto tmp = *it;
        using elem_type = typename decltype(tmp)::value_type;
        using pointer = typename T::pointer;
        pointer ptr = &*it;
        ptr = add_2d_ptr( ptr, inc1, num0, cnt0, inc0 );
        return T((elem_type*)ptr);
    }
};

class Add3dPtr {
  public:
    int inc0;
    int num0;
    addr_t cnt0;
    int inc1;
    int num1;
    addr_t cnt1;
    int inc2;
        
    Add3dPtr( int step0, int size0, int step1, int size1, int step2 ) {
        inc0 = step0;
        num0 = size0 - 1;
        int reset = step0 * (size0 - 1);
        cnt0 = 0;
        inc1 = step1 - reset;
        num1 = size1 - 1;
        reset += step1 * (size1 - 1);
        cnt1 = 0;
        inc2 = step2 - reset;
    }
    
    template<typename T>
    inline T operator()( T it ) {
        auto tmp = *it;
        using elem_type = typename decltype(tmp)::value_type;
        using pointer = typename T::pointer;
        pointer ptr = &*it;
        ptr = add_3d_ptr( ptr, inc2, num0, cnt0, inc0, num1, cnt1, inc1 );
        return T((elem_type*)ptr);
    }
};

#endif //__KERNEL_HELPERS_H__
