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

#ifndef __QDQ_HELPERS_H__
#define __QDQ_HELPERS_H__

#include <adf.h>
#include <aie_api/aie.hpp>
#include "constants.h"

template<typename Ts>
constexpr unsigned sum_write_garbage_stride() {
    return 1;
    /*
    unsigned accum_access_width = aie::vector_decl_align / 4;
    unsigned srs_access_width = 16;
    unsigned stride = ( std::is_same_v<int16,Ts> ? srs_access_width : accum_access_width ) / Mtile;
    return sum_write_garbage ? stride : 1;
    */
}


enum AddressConfig {
    ADDRCFG_linear,             // linear
    ADDRCFG_C2r1Cl1RC8,         // D0: RC8,  D1: C2>1, D2: C<1                      out: linear (same as Y==1)        order matches tdn (tn / tdn) * (tm * m0) n0 with n0=8 & tdn=2
    ADDRCFG_R2r1CRl1C2R8C8,     // D0: R8C8, D1: R2>1, D2: R<1,  D3: C2, D4: C      out: linear (same as Y==1)
    ADDRCFG_YCXC8,              // D0: XC8,  D1: Y,    D2: C                        out: D0: XC8, D1: Y, D2: C
    ADDRCFG_C2r1YXCl1X8C8,      // D0: X8C8, D1: YX,   D2: C2>1, D3: C<1            out: D0: XC8, D1: Y, D2: C
    ADDRCFG_X2r1YXl1CX8C8,      // D0: X8C8, D1: X2>1, D2: YX<1, D3: C              out: D0: XC8, D1: Y, D2: C
};

struct QDQKernelParams {
    int64 c0;
    int32 c1;
    int32 c2;
    int32 c3;
    int32 M;
    int32 N;
    int32 shift_Qb;
    int32 shift_Qout;
    // Shift for output of GEMM before qdq
    int32 shift_tdm;
    //reserve bits to not flow into stack during vector copy
    int32 r0;
    int32 r1;
    int32 r2;
    int32 r3;
    int32 r4;
    int32 r5;
};





struct QDQParams {
    uint8 M_g;
    uint8 N_g;
    uint8 Y_g=1;
    uint8 reserved;
    int8 shift_Qb=0;
    int8 shift_Qout=0;
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
};


/*! \brief Function to compute addressing parameters for QDQ kernel based on address configuration (predefined data layout).
    As optimization, this function could be moved to an external tool and results streamed to the core or if only a single mode is used, inlined with constant propagation

  @param[inout] param   Parameter structure. Contains already the size parameters and addressing parameters are added
  @param[in] Mgran      QDQ kernel granularity for M dimension
  @param[in] Ngran      QDQ kernel granularity for N dimension
  @param[in] Mtile      Tiling used in QDQ kernel for M dimension
  @param[in] Ntile      Tiling used in QDQ kernel for N dimension
  @param[in] address_config Value of enum AddressConfig to select the used addressing with a single parameter
  @param[in] psum_delta_offset  Delta between the 2 psum buffers (if used in split TDM layout; psum2 - psum1), given in terms of elements of input type to QDQ kernel (Ti)
 */
static void address_setup_qdq( QDQParams &param, unsigned Mgran, unsigned Ngran, unsigned Mtile, unsigned Ntile, unsigned address_config, int psum_delta_offset=0 )
{
    unsigned Mtile_DM = 8;
    unsigned Mi = std::min( Mgran, Mtile_DM ) / Mtile;
    unsigned Mb = std::max( 1u, Mgran / Mtile_DM );
    unsigned Ni = Ngran / Ntile;
    unsigned Vo = Mtile * Ntile;
    unsigned Vb = Mgran * Ngran;

    assert( Ntile == Ngran );
    assert( address_config >= ADDRCFG_YCXC8 || param.Y_g == 1 );

    unsigned wrap0 = 1;
    unsigned wrap1 = 1;
    unsigned wrap2 = 1;
    unsigned wrap3 = 1;
    int step_Mb = Mtile_DM * Ntile;
    int step0 = 0;
    int step1 = 0;
    int step2 = 0;
    int step3 = 0;
    int step4 = Vb;
    if ( address_config == ADDRCFG_C2r1Cl1RC8 ) {
        wrap0 = param.M_g;
        wrap1 = 2;
        step0 = Vb;
        step1 = psum_delta_offset;
        step4 = Vb * param.M_g;
    }

    param.wrap0 = wrap0;
    param.wrap1 = wrap1;
    param.wrap2 = wrap2;
    param.wrap3 = wrap3;
    param.step0 = step0;
    param.step1 = step1;
    param.step2 = step2;
    param.step3 = step3;
    param.step4 = step4;
    param.step_Mb = step_Mb;
}

#include "qdq_kernel_helpers.h"

template<unsigned V, aie_dm_resource Resource=aie_dm_resource::none, typename T>
inline auto load_index( T * &ptr, int offset=0 ) -> aie::vector<aie_dm_resource_remove_t<T>, V>
{
    aie::vector<aie_dm_resource_remove_t<T>,V> vec;
    if constexpr( V * sizeof( T ) <= aie::vector_decl_align ) {
        vec = aie::load_v<V, Resource>( ptr + offset );
    } else if constexpr( V * sizeof( T ) <= 2 * aie::vector_decl_align ) {
        vec.insert( 1, aie::load_v<V/2, Resource>( ptr + offset + V / 2 ));       if constexpr( sizeof( T ) <= 1 ) ptr = chess_copy( ptr );
        vec.insert( 0, aie::load_v<V/2, Resource>( ptr + offset ));
    } else if constexpr( V * sizeof( T ) <= 4 * aie::vector_decl_align ) {
        vec.insert( 3, aie::load_v<V/4, Resource>( ptr + offset + 3 * V / 4 ));   if constexpr( sizeof( T ) <= 2 ) ptr = chess_copy( ptr );
        vec.insert( 2, aie::load_v<V/4, Resource>( ptr + offset + 2 * V / 4 ));   if constexpr( sizeof( T ) <= 2 ) ptr = chess_copy( ptr );
        vec.insert( 1, aie::load_v<V/4, Resource>( ptr + offset + V / 4 ));       if constexpr( sizeof( T ) <= 2 ) ptr = chess_copy( ptr );
        vec.insert( 0, aie::load_v<V/4, Resource>( ptr + offset ));
    } else {
        chess_error( "Unknown combintation of tile size and memory interface width in qdq implementation" );
    }
    return vec;
}


template<typename Ta, unsigned V, typename T>
inline auto load_accum( T * &ptr, int offset=0, int shift=0 )// -> aie::vector<aie_dm_resource_remove_t<T>, V>
{
    aie::accum<Ta,V> acc;
    if constexpr( V * sizeof( T ) <= aie::vector_decl_align ) {
        acc = aie::accum<Ta,V>( aie::load_v<V>( ptr + offset ), shift );
    } else if constexpr( V * sizeof( T ) <= 2 * aie::vector_decl_align ) {
        acc.insert( 1, aie::accum<Ta,V/2>( aie::load_v<V/2>( ptr + offset + V / 2 ), shift ));       if constexpr( sizeof( T ) <= 1 ) ptr = chess_copy( ptr );
        acc.insert( 0, aie::accum<Ta,V/2>( aie::load_v<V/2>( ptr + offset ), shift ));
    } else if constexpr( V * sizeof( T ) <= 4 * aie::vector_decl_align ) {
        acc.insert( 3, aie::accum<Ta,V/4>( aie::load_v<V/4>( ptr + offset + 3 * V / 4 ), shift ));   if constexpr( sizeof( T ) <= 2 ) ptr = chess_copy( ptr );
        acc.insert( 2, aie::accum<Ta,V/4>( aie::load_v<V/4>( ptr + offset + 2 * V / 4 ), shift ));   if constexpr( sizeof( T ) <= 2 ) ptr = chess_copy( ptr );
        acc.insert( 1, aie::accum<Ta,V/4>( aie::load_v<V/4>( ptr + offset + V / 4 ), shift ));       if constexpr( sizeof( T ) <= 2 ) ptr = chess_copy( ptr );
        acc.insert( 0, aie::accum<Ta,V/4>( aie::load_v<V/4>( ptr + offset ), shift ));
    } else {
        chess_error( "Unknown combintation of tile size and memory interface width in qdq implementation" );
    }
    return acc;
}


template<unsigned terms, unsigned max_terms>
concept QDQTerms = terms >= 2 && terms <= max_terms;

template<unsigned a, unsigned b>
concept SameValue = a == b;

#endif
