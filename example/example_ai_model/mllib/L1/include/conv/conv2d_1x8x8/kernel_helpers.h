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

#include <stdint.h>

#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>
#include <aie_api/utils.hpp>

#define uns4  undef_v128int4( )
#define uns8  undef_v64int8( )
#define uns16 undef_v32int16( )
#define uns32 undef_v16int32( )

#define unu4  undef_v128uint4( )
#define unu8  undef_v64uint8( )
#define unu16 undef_v32uint16( )
#define unu32 undef_v16uint32( )

struct Vecs2_i8 property( keep_in_registers ) {
    v64int8 x0;
    v64int8 x1;
};
struct Vecs2_ui8 property( keep_in_registers ) {
    v64uint8 x0;
    v64uint8 x1;
};
struct Vecs2_i16 property( keep_in_registers ) {
    v32int16 x0;
    v32int16 x1;
};
struct Vecs2_ui16 property( keep_in_registers ) {
    v32uint16 x0;
    v32uint16 x1;
};
struct Vecs2_i32 property( keep_in_registers ) {
    v16int32 x0;
    v16int32 x1;
};
struct Vecs2_ui32 property( keep_in_registers ) {
    v16uint32 x0;
    v16uint32 x1;
};
struct Vecs2_bf16 property( keep_in_registers ) {
    v32bfloat16 x0;
    v32bfloat16 x1;
};

struct Vecs4_i8 property( keep_in_registers ) {
    v64int8 x0;
    v64int8 x1;
    v64int8 x2;
    v64int8 x3;
};
struct Vecs4_ui8 property( keep_in_registers ) {
    v64uint8 x0;
    v64uint8 x1;
    v64uint8 x2;
    v64uint8 x3;
};
struct Vecs4_i16 property( keep_in_registers ) {
    v32int16 x0;
    v32int16 x1;
    v32int16 x2;
    v32int16 x3;
};
struct Vecs4_ui16 property( keep_in_registers ) {
    v32uint16 x0;
    v32uint16 x1;
    v32uint16 x2;
    v32uint16 x3;
};
struct Vecs4_i32 property( keep_in_registers ) {
    v16int32 x0;
    v16int32 x1;
    v16int32 x2;
    v16int32 x3;
};
struct Vecs4_ui32 property( keep_in_registers ) {
    v16uint32 x0;
    v16uint32 x1;
    v16uint32 x2;
    v16uint32 x3;
};
struct Vecs4_bf16 property( keep_in_registers ) {
    v32bfloat16 x0;
    v32bfloat16 x1;
    v32bfloat16 x2;
    v32bfloat16 x3;
};

inline void undef( Vecs2_i8 &vecs ) {
    vecs.x0 = undef_v64int8( );
    vecs.x1 = undef_v64int8( );
}
inline void undef( Vecs2_ui8 &vecs ) {
    vecs.x0 = undef_v64uint8( );
    vecs.x1 = undef_v64uint8( );
}
inline void undef( Vecs2_i16 &vecs ) {
    vecs.x0 = undef_v32int16( );
    vecs.x1 = undef_v32int16( );
}
inline void undef( Vecs2_ui16 &vecs ) {
    vecs.x0 = undef_v32uint16( );
    vecs.x1 = undef_v32uint16( );
}
inline void undef( Vecs2_i32 &vecs ) {
    vecs.x0 = undef_v16int32( );
    vecs.x1 = undef_v16int32( );
}
inline void undef( Vecs2_ui32 &vecs ) {
    vecs.x0 = undef_v16uint32( );
    vecs.x1 = undef_v16uint32( );
}
inline void undef( Vecs2_bf16 &vecs ) {
    vecs.x0 = undef_v32bfloat16( );
    vecs.x1 = undef_v32bfloat16( );
}

inline void undef( Vecs4_i8 &vecs ) {
    vecs.x0 = undef_v64int8( );
    vecs.x1 = undef_v64int8( );
    vecs.x2 = undef_v64int8( );
    vecs.x3 = undef_v64int8( );
}
inline void undef( Vecs4_ui8 &vecs ) {
    vecs.x0 = undef_v64uint8( );
    vecs.x1 = undef_v64uint8( );
    vecs.x2 = undef_v64uint8( );
    vecs.x3 = undef_v64uint8( );
}
inline void undef( Vecs4_i16 &vecs ) {
    vecs.x0 = undef_v32int16( );
    vecs.x1 = undef_v32int16( );
    vecs.x2 = undef_v32int16( );
    vecs.x3 = undef_v32int16( );
}
inline void undef( Vecs4_ui16 &vecs ) {
    vecs.x0 = undef_v32uint16( );
    vecs.x1 = undef_v32uint16( );
    vecs.x2 = undef_v32uint16( );
    vecs.x3 = undef_v32uint16( );
}
inline void undef( Vecs4_i32 &vecs ) {
    vecs.x0 = undef_v16int32( );
    vecs.x1 = undef_v16int32( );
    vecs.x2 = undef_v16int32( );
    vecs.x3 = undef_v16int32( );
}
inline void undef( Vecs4_ui32 &vecs ) {
    vecs.x0 = undef_v16uint32( );
    vecs.x1 = undef_v16uint32( );
    vecs.x2 = undef_v16uint32( );
    vecs.x3 = undef_v16uint32( );
}
inline void undef( Vecs4_bf16 &vecs ) {
    vecs.x0 = undef_v32bfloat16( );
    vecs.x1 = undef_v32bfloat16( );
    vecs.x2 = undef_v32bfloat16( );
    vecs.x3 = undef_v32bfloat16( );
}





struct Accs2_i32 property( keep_in_registers ) {
    v64acc32 a0;
    v64acc32 a1;
};
struct Accs4_i32 property( keep_in_registers ) {
    v64acc32 a0;
    v64acc32 a1;
    v64acc32 a2;
    v64acc32 a3;
};
struct Accs8_i32 property( keep_in_registers ) {
    v64acc32 a0;
    v64acc32 a1;
    v64acc32 a2;
    v64acc32 a3;
    v64acc32 a4;
    v64acc32 a5;
    v64acc32 a6;
    v64acc32 a7;
};

struct Accs2_i64 property( keep_in_registers ) {
    v32acc64 a0;
    v32acc64 a1;
};
struct Accs4_i64 property( keep_in_registers ) {
    v32acc64 a0;
    v32acc64 a1;
    v32acc64 a2;
    v32acc64 a3;
};
struct Accs8_i64 property( keep_in_registers ) {
    v32acc64 a0;
    v32acc64 a1;
    v32acc64 a2;
    v32acc64 a3;
    v32acc64 a4;
    v32acc64 a5;
    v32acc64 a6;
    v32acc64 a7;
};

struct Accs4_float property( keep_in_registers ) {
    v64accfloat a0;
    v64accfloat a1;
    v64accfloat a2;
    v64accfloat a3;
};
struct Accs4_float32 property( keep_in_registers ) {
    v32accfloat a0;
    v32accfloat a1;
    v32accfloat a2;
    v32accfloat a3;
};
struct Accs8_float property( keep_in_registers ) {
    v64accfloat a0;
    v64accfloat a1;
    v64accfloat a2;
    v64accfloat a3;
    v64accfloat a4;
    v64accfloat a5;
    v64accfloat a6;
    v64accfloat a7;
};


inline void undef( Accs2_i32 &accs ) {
    accs.a0 = undef_v64acc32( );
    accs.a1 = undef_v64acc32( );
}
inline void undef( Accs4_i32 &accs ) {
    accs.a0 = undef_v64acc32( );
    accs.a1 = undef_v64acc32( );
    accs.a2 = undef_v64acc32( );
    accs.a3 = undef_v64acc32( );
}
inline void undef( Accs8_i32 &accs ) {
    accs.a0 = undef_v64acc32( );
    accs.a1 = undef_v64acc32( );
    accs.a2 = undef_v64acc32( );
    accs.a3 = undef_v64acc32( );
    accs.a4 = undef_v64acc32( );
    accs.a5 = undef_v64acc32( );
    accs.a6 = undef_v64acc32( );
    accs.a7 = undef_v64acc32( );
}

inline void undef( Accs2_i64 &accs ) {
    accs.a0 = undef_v32acc64( );
    accs.a1 = undef_v32acc64( );
}
inline void undef( Accs4_i64 &accs ) {
    accs.a0 = undef_v32acc64( );
    accs.a1 = undef_v32acc64( );
    accs.a2 = undef_v32acc64( );
    accs.a3 = undef_v32acc64( );
}
inline void undef( Accs8_i64 &accs ) {
    accs.a0 = undef_v32acc64( );
    accs.a1 = undef_v32acc64( );
    accs.a2 = undef_v32acc64( );
    accs.a3 = undef_v32acc64( );
    accs.a4 = undef_v32acc64( );
    accs.a5 = undef_v32acc64( );
    accs.a6 = undef_v32acc64( );
    accs.a7 = undef_v32acc64( );
}

inline void undef( Accs4_float &accs ) {
    accs.a0  = undef_v64accfloat( );
    accs.a1  = undef_v64accfloat( );
    accs.a2  = undef_v64accfloat( );
    accs.a3  = undef_v64accfloat( );
}
inline void undef( Accs4_float32 &accs ) {
    accs.a0  = undef_v32accfloat( );
    accs.a1  = undef_v32accfloat( );
    accs.a2  = undef_v32accfloat( );
    accs.a3  = undef_v32accfloat( );
}
inline void undef( Accs8_float &accs ) {
    accs.a0  = undef_v64accfloat( );
    accs.a1  = undef_v64accfloat( );
    accs.a2  = undef_v64accfloat( );
    accs.a3  = undef_v64accfloat( );
    accs.a4  = undef_v64accfloat( );
    accs.a5  = undef_v64accfloat( );
    accs.a6  = undef_v64accfloat( );
    accs.a7  = undef_v64accfloat( );
}


inline v32acc32 to_v32acc32( v32int16 v, int shift, int sign ) {
    return sups( v, shift, sign );
}
inline v16acc64 to_v16acc64( v16int32 v, int shift, int sign ) {
    return lups( v, shift, sign );
}
inline v32int16 to_v32int16( v32acc32 a, int shift, int sign ) {
    return lsrs( a, shift, sign );
}
inline v16int32 to_v16int32( v16acc64 a, int shift, int sign ) {
    return lsrs( a, shift, sign );
}
inline v32accfloat to_v32accfloat( v32bfloat16 v ) { return ups( v ); }

inline v64accfloat to_v64accfloat( v64bfloat16 v ) {
    return concat( ups( extract_v32bfloat16( v, 0 )), ups( extract_v32bfloat16( v, 1 )));
}

inline v64int16 lsrs( v64acc32 acc, int shift, int sign ) {
    v64int16 vec = set_v64int16( 0, lsrs( extract_v32acc32( acc, 0 ), shift, sign ));
    return insert( vec, 1, lsrs( extract_v32acc32( acc, 1 ), shift, sign ));
}
inline v64int16 lsrs( v64acc32 acc, int shift ) {
    return lsrs( acc, shift, 1 );
}
inline v64uint16 ulsrs( v64acc32 acc, int shift ) {
    return ( v64uint16 ) lsrs( acc, shift, 0 );
}
inline v32int32 lsrs( v32acc64 acc, int shift, int sign ) {
    v32int32 vec = set_v32int32( 0, lsrs( extract_v16acc64( acc, 0 ), shift, sign ));
    return insert( vec, 1, lsrs( extract_v16acc64( acc, 1 ), shift, sign ));
}
inline v32int32 lsrs( v32acc64 acc, int shift ) {
    return lsrs( acc, shift, 1 );
}

inline v64acc32 sups( v64int16 vec, int shift, int sign ) {
    v64acc32 acc = set_v64acc32( 0, sups( extract_v32int16( vec, 0 ), shift, sign ));
    return insert( acc, 1, sups( extract_v32int16( vec, 1 ), shift, sign ));
}
inline v64acc32 sups( v64int16 vec, int shift ) {
    v64acc32 acc = set_v64acc32( 0, sups( extract_v32int16( vec, 0 ), shift ));
    return insert( acc, 1, sups( extract_v32int16( vec, 1 ), shift ));
}
inline v64acc32 sups( v64uint16 vec, int shift ) {
    v64acc32 acc = set_v64acc32( 0, sups( extract_v32uint16( vec, 0 ), shift ));
    return insert( acc, 1, sups( extract_v32uint16( vec, 1 ), shift ));
}

inline v16cacc64 lups( v16cint32 vec, int shift ) {
    v16cacc64 acc = set_v16cacc64( 0, lups( extract_v8cint32( vec, 0 ), shift ));
    return insert( acc, 1, lups( extract_v8cint32( vec, 1 ), shift ));
}


inline v64bfloat16 to_v64bfloat16( v64accfloat acc ) {
    v64bfloat16 vec = set_v64bfloat16( 0, to_v32bfloat16( extract_v32accfloat( acc, 0 )));
    return insert( vec, 1, to_v32bfloat16( extract_v32accfloat( acc, 1 )));
}


#ifdef __clang__
#define RSTRCT __restrict
#else
#define RSTRCT
#endif

#include "pp.h"
#define SHIFT_SIGN_SIG( BF ) PP_IF( BF, PP_EMPTY, PP_COMMA )( ) PP_IF( BF, PP_EMPTY, int shift PP_COMMA )( ) PP_IF( BF, ,bool sign )
#define SHIFT_SIGN( BF ) PP_IF( BF, PP_EMPTY, PP_COMMA )( ) PP_IF( BF, PP_EMPTY, shift PP_COMMA )( ) PP_IF( BF, ,sign )

#define LOAD_STORE_TEMPLATE_LOC( T1, T2, T4, T2h, T4h, BF, LOC )                            \
inline void load( T4 &acc, T4 LOC *& ptr ) {                                          \
    ptr = chess_copy( ptr );                                                          \
    T1 LOC * pIn = ( T1 LOC* ) ptr;                                                   \
    acc = insert( acc, 3, pIn[3] );                                                   \
    pIn = chess_copy( pIn );                                                          \
    acc = insert( acc, 2, pIn[2] );                                                   \
    pIn = chess_copy( pIn );                                                          \
    acc = insert( acc, 1, pIn[1] );                                                   \
    acc = insert( acc, 0, pIn[0] );                                                   \
}                                                                                   \
inline void store( T4 LOC * RSTRCT & ptr, T4 acc ) {                                  \
    ptr = chess_copy( ptr );                                                          \
    T1 LOC * RSTRCT pOut = ( T1 LOC* ) ptr;                                           \
    pOut[3] = extract_##T1( acc, 3 );                                                 \
    pOut = chess_copy( pOut );                                                        \
    pOut[2] = extract_##T1( acc, 2 );                                                 \
    pOut = chess_copy( pOut );                                                        \
    pOut[1] = extract_##T1( acc, 1 );                                                 \
    pOut[0] = extract_##T1( acc, 0 );                                                 \
}                                                                                   \
inline void load( T4 &acc, T4h LOC *& ptr SHIFT_SIGN_SIG( BF )) {                      \
    ptr = chess_copy( ptr );                                                          \
    T2h LOC * pIn = ( T2h LOC* ) ptr;                                                 \
    acc = insert( acc, 1, to_##T2( pIn[1] SHIFT_SIGN( BF )));                           \
    acc = insert( acc, 0, to_##T2( pIn[0] SHIFT_SIGN( BF )));                           \
}                                                                                   \
inline void store( T4h LOC * RSTRCT & ptr, T4 acc SHIFT_SIGN_SIG( BF )) {              \
    T2h LOC * RSTRCT pOut = ( T2h LOC* ) ptr;                                         \
    pOut[1] = to_##T2h( extract_##T2( acc, 1 ) SHIFT_SIGN( BF ));                        \
    pOut[0] = to_##T2h( extract_##T2( acc, 0 ) SHIFT_SIGN( BF ));                        \
}

#if defined(__chess__) || defined(__AIECC__)
#define LOAD_STORE_TEMPLATE( T1, T2, T4, T2h, T4h, BF )                         \
    LOAD_STORE_TEMPLATE_LOC( T1, T2, T4, T2h, T4h, BF, )                        \
    LOAD_STORE_TEMPLATE_LOC( T1, T2, T4, T2h, T4h, BF, __aie_dm_resource_a) \
    LOAD_STORE_TEMPLATE_LOC( T1, T2, T4, T2h, T4h, BF, __aie_dm_resource_b) \
    LOAD_STORE_TEMPLATE_LOC( T1, T2, T4, T2h, T4h, BF, __aie_dm_resource_c) \
    LOAD_STORE_TEMPLATE_LOC( T1, T2, T4, T2h, T4h, BF, __aie_dm_resource_d)
#else
#define LOAD_STORE_TEMPLATE( T1, T2, T4, T2h, T4h, BF )                         \
    LOAD_STORE_TEMPLATE_LOC( T1, T2, T4, T2h, T4h, BF, )
#endif

LOAD_STORE_TEMPLATE( v16acc32, v32acc32, v64acc32, v32int16, v64int16, 0 )
LOAD_STORE_TEMPLATE( v8acc64, v16acc64, v32acc64, v16int32, v32int32, 0 )
LOAD_STORE_TEMPLATE( v16accfloat, v32accfloat, v64accfloat, v32bfloat16, v64bfloat16, 1 )



#define LOAD_STORE_TEMPLATE_HALF_LOC( T1, T2, T4, T2h, BF, LOC )                           \
inline void load( T4 &acc, T2 LOC *& ptr, int half ) {                                \
    ptr = chess_copy( ptr );                                                          \
    T1 LOC * pIn = ( T1 LOC* ) ptr;                                                   \
    acc = insert( acc, 1+2*half, pIn[1] );                                            \
    acc = insert( acc, 0+2*half, pIn[0] );                                            \
}                                                                                   \
inline void load( T2 &acc, T2 LOC *& ptr ) {                                          \
    ptr = chess_copy( ptr );                                                          \
    T1 LOC * pIn = ( T1 LOC* ) ptr;                                                   \
    acc = insert( acc, 1, pIn[1] );                                                   \
    acc = insert( acc, 0, pIn[0] );                                                   \
}                                                                                   \
inline void store( T2 LOC * RSTRCT & ptr, T4 acc, int half ) {                        \
    T1 LOC * RSTRCT pOut = ( T1 LOC* ) ptr;                                           \
    pOut[1] = extract_##T1( acc, 1+2*half );                                          \
    pOut[0] = extract_##T1( acc, 0+2*half );                                          \
}                                                                                   \
inline void store( T2 LOC * RSTRCT & ptr, T2 acc ) {                                  \
    T1 LOC * RSTRCT pOut = ( T1 LOC* ) ptr;                                           \
    pOut[1] = extract_##T1( acc, 1 );                                                 \
    pOut[0] = extract_##T1( acc, 0 );                                                 \
}                                                                                   \
inline void load( T4 &acc, T2h LOC *& ptr, int half SHIFT_SIGN_SIG( BF )) {            \
    ptr = chess_copy( ptr );                                                          \
    acc = insert( acc, 0+half, to_##T2( ptr[0] SHIFT_SIGN( BF )));                      \
}                                                                                   \
inline void store( T2h LOC * RSTRCT & ptr, T4 acc, int half SHIFT_SIGN_SIG( BF )) {    \
    ptr[0] = to_##T2h( extract_##T2( acc, 0+half ) SHIFT_SIGN( BF ));                    \
}

#if defined(__chess__) || defined(__AIECC__)
#define LOAD_STORE_TEMPLATE_HALF( T1, T2, T4, T2h, BF )                         \
    LOAD_STORE_TEMPLATE_HALF_LOC( T1, T2, T4, T2h, BF, )                        \
    LOAD_STORE_TEMPLATE_HALF_LOC( T1, T2, T4, T2h, BF, __aie_dm_resource_a) \
    LOAD_STORE_TEMPLATE_HALF_LOC( T1, T2, T4, T2h, BF, __aie_dm_resource_b) \
    LOAD_STORE_TEMPLATE_HALF_LOC( T1, T2, T4, T2h, BF, __aie_dm_resource_c) \
    LOAD_STORE_TEMPLATE_HALF_LOC( T1, T2, T4, T2h, BF, __aie_dm_resource_d)
#else
#define LOAD_STORE_TEMPLATE_HALF( T1, T2, T4, T2h, BF )                         \
    LOAD_STORE_TEMPLATE_HALF_LOC( T1, T2, T4, T2h, BF, )
#endif

LOAD_STORE_TEMPLATE_HALF( v16acc32, v32acc32, v64acc32, v32int16, 0 )
LOAD_STORE_TEMPLATE_HALF( v8acc64, v16acc64, v32acc64, v16int32, 0 )
LOAD_STORE_TEMPLATE_HALF( v16accfloat, v32accfloat, v64accfloat, v32bfloat16, 1 )


inline void load_strided( v64acc32 &acc, v64acc32 *& ptr, int stride ) {
    ptr = chess_copy( ptr );
    v16acc32 * pIn = ( v16acc32* ) ptr;
    acc = insert( acc, 0, *pIn );        pIn = byte_incr( pIn, stride );
    acc = insert( acc, 1, *pIn );        pIn = byte_incr( pIn, stride );
    acc = insert( acc, 2, *pIn );        pIn = byte_incr( pIn, stride );
    acc = insert( acc, 3, *pIn );
    ptr = ( v64acc32* ) pIn;
}
inline void store_strided( v64acc32 * RSTRCT & ptr, v64acc32 acc, int stride ) {
    v16acc32 * pOut = ( v16acc32* ) ptr;
    *pOut = extract_v16acc32( acc, 0 );        pOut = byte_incr( pOut, stride );
    *pOut = extract_v16acc32( acc, 1 );        pOut = byte_incr( pOut, stride );
    *pOut = extract_v16acc32( acc, 2 );        pOut = byte_incr( pOut, stride );
    *pOut = extract_v16acc32( acc, 3 );
    ptr = ( v64acc32* ) pOut;
}
inline void load_strided( v64acc32 &acc, v64int16 *& ptr, int shift, bool sign, int stride ) {
    ptr = chess_copy( ptr );
    v32int16 * pIn = ( v32int16* ) ptr;
    acc = insert( acc, 0, sups( *pIn, shift, sign ));        pIn = byte_incr( pIn, stride );
    acc = insert( acc, 1, sups( *pIn, shift, sign ));
    ptr = ( v64int16* ) pIn;
}
inline void store_strided( v64int16 * RSTRCT & ptr, v64acc32 acc, int shift, bool sign, int stride ) {
    v32int16 * pOut = ( v32int16* ) ptr;
    *pOut = lsrs( extract_v32acc32( acc, 0 ), shift, sign );        pOut = byte_incr( pOut, stride );
    *pOut = lsrs( extract_v32acc32( acc, 1 ), shift, sign );
    ptr = ( v64int16* ) pOut;
}


inline void load_2x2_itlv( v64acc32 &acc0, v64acc32 &acc1, v64acc32 *& ptr, int incr0, int incr1 ) {
    ptr = chess_copy( ptr );
    v16acc32 * pIn = ( v16acc32* ) ptr;
    acc0 = insert( acc0, 1, pIn[1] );
    acc0 = insert( acc0, 0, pIn[0] );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 1, pIn[1] );
    acc1 = insert( acc1, 0, pIn[0] );        pIn = byte_incr( pIn, incr1 );
    acc0 = insert( acc0, 3, pIn[1] );
    acc0 = insert( acc0, 2, pIn[0] );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 3, pIn[1] );
    acc1 = insert( acc1, 2, pIn[0] );
    ptr = ( v64acc32* ) pIn;
}
inline void store_2x2_itlv( v64acc32 * RSTRCT & ptr, v64acc32 acc0, v64acc32 acc1, int incr0, int incr1 ) {
    v16acc32 * pOut = ( v16acc32* ) ptr;
    pOut[1] = extract_v16acc32( acc0, 1 );
    pOut[0] = extract_v16acc32( acc0, 0 );        pOut = byte_incr( pOut, incr0 );
    pOut[1] = extract_v16acc32( acc1, 1 );
    pOut[0] = extract_v16acc32( acc1, 0 );        pOut = byte_incr( pOut, incr1 );
    pOut[1] = extract_v16acc32( acc0, 3 );
    pOut[0] = extract_v16acc32( acc0, 2 );        pOut = byte_incr( pOut, incr0 );
    pOut[1] = extract_v16acc32( acc1, 3 );
    pOut[0] = extract_v16acc32( acc1, 2 );
    ptr = ( v64acc32* ) pOut;
}
inline void load_2x4_itlv( v64acc32 &acc0, v64acc32 &acc1, v64acc32 *& ptr, int incr0, int incr1 ) {
    ptr = chess_copy( ptr );
    v16acc32 * pIn = ( v16acc32* ) ptr;
    acc0 = insert( acc0, 0, *pIn );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 0, *pIn );        pIn = byte_incr( pIn, incr1 );
    acc0 = insert( acc0, 1, *pIn );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 1, *pIn );        pIn = byte_incr( pIn, incr1 );
    acc0 = insert( acc0, 2, *pIn );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 2, *pIn );        pIn = byte_incr( pIn, incr1 );
    acc0 = insert( acc0, 3, *pIn );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 3, *pIn );
    ptr = ( v64acc32* ) pIn;
}
inline void store_2x4_itlv( v64acc32 * RSTRCT & ptr, v64acc32 acc0, v64acc32 acc1, int incr0, int incr1 ) {
    v16acc32 * pOut = ( v16acc32* ) ptr;
    *pOut = extract_v16acc32( acc0, 0 );        pOut = byte_incr( pOut, incr0 );
    *pOut = extract_v16acc32( acc1, 0 );        pOut = byte_incr( pOut, incr1 );
    *pOut = extract_v16acc32( acc0, 1 );        pOut = byte_incr( pOut, incr0 );
    *pOut = extract_v16acc32( acc1, 1 );        pOut = byte_incr( pOut, incr1 );
    *pOut = extract_v16acc32( acc0, 2 );        pOut = byte_incr( pOut, incr0 );
    *pOut = extract_v16acc32( acc1, 2 );        pOut = byte_incr( pOut, incr1 );
    *pOut = extract_v16acc32( acc0, 3 );        pOut = byte_incr( pOut, incr0 );
    *pOut = extract_v16acc32( acc1, 3 );
    ptr = ( v64acc32* ) pOut;
}
inline void load_2x2_itlv( v64acc32 &acc0, v64acc32 &acc1, v64int16 *& ptr, int shift, bool sign, int incr0, int incr1 ) {
    ptr = chess_copy( ptr );
    v32int16 * pIn = ( v32int16* ) ptr;
    acc0 = insert( acc0, 0, sups( *pIn, shift, sign ));        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 0, sups( *pIn, shift, sign ));        pIn = byte_incr( pIn, incr1 );
    acc0 = insert( acc0, 1, sups( *pIn, shift, sign ));        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 1, sups( *pIn, shift, sign ));
    ptr = ( v64int16* ) pIn;
}
inline void store_2x2_itlv( v64int16 * RSTRCT & ptr, v64acc32 acc0, v64acc32 &acc1, int shift, bool sign, int incr0, int incr1 ) {
    v32int16 * pOut = ( v32int16* ) ptr;
    *pOut = lsrs( extract_v32acc32( acc0, 0 ), shift, sign );        pOut = byte_incr( pOut, incr0 );
    *pOut = lsrs( extract_v32acc32( acc1, 0 ), shift, sign );        pOut = byte_incr( pOut, incr1 );
    *pOut = lsrs( extract_v32acc32( acc0, 1 ), shift, sign );        pOut = byte_incr( pOut, incr0 );
    *pOut = lsrs( extract_v32acc32( acc1, 1 ), shift, sign );
    ptr = ( v64int16* ) pOut;
}


inline void load_2x2_itlv( v64accfloat &acc0, v64accfloat &acc1, v64accfloat *& ptr, int incr0, int incr1 ) {
    ptr = chess_copy( ptr );
    v16accfloat * pIn = ( v16accfloat* ) ptr;
    acc0 = insert( acc0, 1, pIn[1] );
    acc0 = insert( acc0, 0, pIn[0] );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 1, pIn[1] );
    acc1 = insert( acc1, 0, pIn[0] );        pIn = byte_incr( pIn, incr1 );
    acc0 = insert( acc0, 3, pIn[1] );
    acc0 = insert( acc0, 2, pIn[0] );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 3, pIn[1] );
    acc1 = insert( acc1, 2, pIn[0] );
    ptr = ( v64accfloat* ) pIn;
}
inline void store_2x2_itlv( v64accfloat * RSTRCT & ptr, v64accfloat acc0, v64accfloat acc1, int incr0, int incr1 ) {
    v16accfloat * pOut = ( v16accfloat* ) ptr;
    pOut[1] = extract_v16accfloat( acc0, 1 );
    pOut[0] = extract_v16accfloat( acc0, 0 );        pOut = byte_incr( pOut, incr0 );
    pOut[1] = extract_v16accfloat( acc1, 1 );
    pOut[0] = extract_v16accfloat( acc1, 0 );        pOut = byte_incr( pOut, incr1 );
    pOut[1] = extract_v16accfloat( acc0, 3 );
    pOut[0] = extract_v16accfloat( acc0, 2 );        pOut = byte_incr( pOut, incr0 );
    pOut[1] = extract_v16accfloat( acc1, 3 );
    pOut[0] = extract_v16accfloat( acc1, 2 );
    ptr = ( v64accfloat* ) pOut;
}
inline void load_2x4_itlv( v64accfloat &acc0, v64accfloat &acc1, v64accfloat *& ptr, int incr0, int incr1 ) {
    ptr = chess_copy( ptr );
    v16accfloat * pIn = ( v16accfloat* ) ptr;
    acc0 = insert( acc0, 0, *pIn );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 0, *pIn );        pIn = byte_incr( pIn, incr1 );
    acc0 = insert( acc0, 1, *pIn );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 1, *pIn );        pIn = byte_incr( pIn, incr1 );
    acc0 = insert( acc0, 2, *pIn );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 2, *pIn );        pIn = byte_incr( pIn, incr1 );
    acc0 = insert( acc0, 3, *pIn );        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 3, *pIn );
    ptr = ( v64accfloat* ) pIn;
}
inline void store_2x4_itlv( v64accfloat * RSTRCT & ptr, v64accfloat acc0, v64accfloat acc1, int incr0, int incr1 ) {
    v16accfloat * pOut = ( v16accfloat* ) ptr;
    *pOut = extract_v16accfloat( acc0, 0 );        pOut = byte_incr( pOut, incr0 );
    *pOut = extract_v16accfloat( acc1, 0 );        pOut = byte_incr( pOut, incr1 );
    *pOut = extract_v16accfloat( acc0, 1 );        pOut = byte_incr( pOut, incr0 );
    *pOut = extract_v16accfloat( acc1, 1 );        pOut = byte_incr( pOut, incr1 );
    *pOut = extract_v16accfloat( acc0, 2 );        pOut = byte_incr( pOut, incr0 );
    *pOut = extract_v16accfloat( acc1, 2 );        pOut = byte_incr( pOut, incr1 );
    *pOut = extract_v16accfloat( acc0, 3 );        pOut = byte_incr( pOut, incr0 );
    *pOut = extract_v16accfloat( acc1, 3 );
    ptr = ( v64accfloat* ) pOut;
}
inline void load_2x2_itlv( v64accfloat &acc0, v64accfloat &acc1, v64bfloat16 *& ptr, int incr0, int incr1 ) {
    ptr = chess_copy( ptr );
    v32bfloat16 * pIn = ( v32bfloat16* ) ptr;
    acc0 = insert( acc0, 0, to_v32accfloat( *pIn ));        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 0, to_v32accfloat( *pIn ));        pIn = byte_incr( pIn, incr1 );
    acc0 = insert( acc0, 1, to_v32accfloat( *pIn ));        pIn = byte_incr( pIn, incr0 );
    acc1 = insert( acc1, 1, to_v32accfloat( *pIn ));
    ptr = ( v64bfloat16* ) pIn;
}
inline void store_2x2_itlv( v64bfloat16 * RSTRCT & ptr, v64accfloat acc0, v64accfloat &acc1, int incr0, int incr1 ) {
    v32bfloat16 * pOut = ( v32bfloat16* ) ptr;
    *pOut = to_v32bfloat16( extract_v32accfloat( acc0, 0 ));        pOut = byte_incr( pOut, incr0 );
    *pOut = to_v32bfloat16( extract_v32accfloat( acc1, 0 ));        pOut = byte_incr( pOut, incr1 );
    *pOut = to_v32bfloat16( extract_v32accfloat( acc0, 1 ));        pOut = byte_incr( pOut, incr0 );
    *pOut = to_v32bfloat16( extract_v32accfloat( acc1, 1 ));
    ptr = ( v64bfloat16* ) pOut;
}




inline v64acc32 null_v64acc32( v64acc32 acc ) { return neg_conf( acc, 1, 0, 0 ); }
inline v32acc64 null_v32acc64( v32acc64 acc ) { return neg_conf( acc, 1, 0, 0 ); }
inline v64acc32 null_v64acc32( ) { return clr64( ); }
inline v32acc64 null_v32acc64( ) { return clr32( ); }
inline v64accfloat null_v64accfloat( v64accfloat acc ) { v64accfloat chess_storage( dm4 ) n = clr64f( ); chess_separator_scheduler( ); return n; } //add_conf( acc, acc, 0, 0, 1 ); }//neg_conf( acc, 1, 0 ); }
inline v64accfloat null_v64accfloat( ) { return clr64f( ); }



inline int add_dimension( int &reset, int count, int step ) {
    int incr = reset + step;
    reset -= count * step;
    return incr;
}

inline int get_expo( int offset ) {
    return 0x1010101 * ( 127+offset );
}
inline sparsity_t get_sparse( int mask ) {
    v16int8 sparse = extract_v16int8( broadcast_s8( 0x11 * mask ), 0 );
    return *( sparsity_t* )&sparse;
}


inline v32int16 shuffle( v64int16 a, int mode ){
    return shuffle( extract_v32int16( a, 0 ), extract_v32int16( a, 1 ), mode );
}

inline v64bfloat16 shuffle( v64bfloat16 x, int mode ) {
    v64bfloat16 y = set_v64bfloat16( 0, shuffle( extract_v32bfloat16( x, 0 ), extract_v32bfloat16( x, 1 ), mode ));
    return insert( y, 1, shuffle( extract_v32bfloat16( x, 0 ), extract_v32bfloat16( x, 1 ), mode+1 ));
}


inline v32int16 interleave_T16_4x2x4( v32int16 a ) {
    return shuffle( shuffle( a, T16_8x4 ), T16_4x2 );
}


inline v8acc64 shuffle( v8acc64 a, v8acc64 b, int mode ) {
    return ( v8acc64 ) shuffle(( v16int32 )a, ( v16int32 )b, mode );
}

inline v16accfloat shuffle( v16accfloat a, v16accfloat b, int mode ) {
    return ( v16accfloat ) shuffle(( v16int32 )a, ( v16int32 )b, mode );
}
inline v16accfloat shuffle( v16accfloat a, int mode ) {
    return ( v16accfloat ) shuffle(( v16int32 )a, mode );
}


inline float extract_elem( v64accfloat v, int idx ) {
    return as_float( extract_elem(( v16int32 )extract_v16accfloat( v, idx/16 ), idx&15 ));
}
inline float extract_elem( v32accfloat v, int idx ) {
    return as_float( extract_elem(( v16int32 )extract_v16accfloat( v, idx/16 ), idx&15 ));
}
inline float extract_elem( v16accfloat v, int idx ) {
    return as_float( extract_elem(( v16int32 )v, idx ));
}


inline v64int16 broadcast_to_v64int16( int16_t s ) {
    v32int16 x = broadcast_to_v32int16( s );
    return concat( x, x );
}
inline v64uint16 broadcast_to_v64uint16( uint16_t s ) {
    v32uint16 x = broadcast_to_v32uint16( s );
    return concat( x, x );
}


inline v32accfloat broadcast_to_v32accfloat( float s ) {
    v16accfloat bm = broadcast_to_v16accfloat( s );
    return concat( bm, bm );
}
inline v64accfloat broadcast_to_v64accfloat( float s ) {
    v32accfloat cm = broadcast_to_v32accfloat( s );
    return concat( cm, cm );
}
inline v32float broadcast_to_v32float( float s ) {
    v16float bm = broadcast_to_v16float( s );
    return (v32float)concat( (v16accfloat)bm, (v16accfloat)bm );
}
inline v64float broadcast_to_v64float( float s ) {
    v32float cm = broadcast_to_v32float( s );
    return (v64float)concat( (v32accfloat)cm, (v32accfloat)cm );
}


inline v32bfloat16 broadcast_extract_v8bfloat16_to_v32bfloat16( v32bfloat16 v, int idx ) {
    return ( v32bfloat16 ) broadcast_elem_128(( v16int32 )v, idx );
}
inline v32bfloat16 broadcast_extract_v16bfloat16_to_v32bfloat16( v32bfloat16 v, int idx ) {
    return shuffle( v, v, T256_2x2_lo+( idx&1 ));
}


inline v2float extract_v2float( v32float acc, int idx ) {
    return extract_v2float(( v16float )extract_v16accfloat( (v32accfloat)acc, ( idx/16 )&1 ), idx&15 );
}
inline v2float extract_v2float( v64float acc, int idx ) {
    return extract_v2float(( v16float )extract_v16accfloat( (v64accfloat)acc, ( idx/16 )&3 ), idx&15 );
}

inline v2float extract_v2float( v16accfloat acc, int idx ) {
    return extract_v2float(( v16float )acc, idx );
}
inline v2float extract_v2float( v32accfloat acc, int idx ) {
    return extract_v2float(( v32float )acc, idx );
}
inline v2float extract_v2float( v64accfloat acc, int idx ) {
    return extract_v2float(( v64float )acc, idx );
}

inline v16accfloat insert( v16accfloat acc, int idx, v2float elem ) {
    return ( v16accfloat ) insert(( v16float )acc, idx, elem );
}

inline v2float inv( v2float vec ) {
    v2float ret;
    ret = set_v2float( 0, inv( extract_elem( vec, 0 )));
    ret = insert( ret, 1, inv( extract_elem( vec, 1 )));
    return ret;
}
inline v2float invsqrt( v2float vec ) {
    v2float ret;
    ret = set_v2float( 0, invsqrt( extract_elem( vec, 0 )));
    ret = insert( ret, 1, invsqrt( extract_elem( vec, 1 )));
    return ret;
}

inline v16float insert_invsqrt_extract_v2float( v16float acc, int idx, v16float vec ) {
    acc = insert(acc, 2*idx+0, invsqrt( extract_elem(vec, 2*idx+0 )));
    acc = insert(acc, 2*idx+1, invsqrt( extract_elem(vec, 2*idx+1 )));
    return acc;
}
inline v16float insert_invsqrt_extract_v2float( v16float acc, int idx, v32float vec ) {
    acc = insert(acc, 2*idx+0, invsqrt( extract_elem(extract_v16float( vec, 0 ), 2*idx+0 )));
    acc = insert(acc, 2*idx+1, invsqrt( extract_elem(extract_v16float( vec, 0 ), 2*idx+1 )));
    return acc;
}

inline v32accfloat set_v32accfloat (int idx, v16float b) { return set_v32accfloat(idx, (v16accfloat)b); }

inline v16accfloat operator+ ( v16accfloat a, v16float b ) { return a + (v16accfloat)b; }
inline v16accfloat operator+ ( v16float a, v16accfloat b ) { return (v16accfloat)a + b; }

inline v32acc32 operator- ( v32acc32 a, v32acc32 b )
#ifdef __ndl__
property( non_functional )
#endif
{
    return extract_v32acc32( set_v64acc32( 0, a ) - set_v64acc32( 0, b ), 0 );
}

inline v32acc32 operator- ( v32acc32 a )
#ifdef __ndl__
property( non_functional )
#endif
{
    return extract_v32acc32( -set_v64acc32( 0, a ), 0 );
}


inline v32acc64 mul_8x4_4x4( v32int16 x, bool sgn_x, v32int16 y, bool sgn_y ) {
    v32acc64 acc = mul_4x4_4x8( shuffle( y, T16_4x4 ), sgn_y, shuffle( x, T16_8x4 ), sgn_x );
    v8acc64 a0 = extract_v8acc64( acc, 0 );
    v8acc64 a1 = extract_v8acc64( acc, 1 );
    v8acc64 a2 = extract_v8acc64( acc, 2 );
    v8acc64 a3 = extract_v8acc64( acc, 3 );
    v8acc64 b0 = shuffle( a0, a2, T64_2x8_lo );
    v8acc64 b1 = shuffle( a0, a2, T64_2x8_hi );
    v8acc64 b2 = shuffle( a1, a3, T64_2x8_lo );
    v8acc64 b3 = shuffle( a1, a3, T64_2x8_hi );
            a0 = shuffle( b0, b2, T64_2x8_lo );
            a1 = shuffle( b0, b2, T64_2x8_hi );
            a2 = shuffle( b1, b3, T64_2x8_lo );
            a3 = shuffle( b1, b3, T64_2x8_hi );
    return insert( insert( insert( set_v32acc64( 0, a0 ), 1, a1 ), 2, a2 ), 3, a3 );
}


inline v16acc64 mul_elem_16( v16int32 a, v16int32 b ) {
    v32int16 a_hi  = shuffle(( v32int16 )  a, T16_32x2_hi );
    v32uint16 a_lo = shuffle(( v32uint16 ) a, T16_32x2_lo );
    v32int16 b_hi  = shuffle(( v32int16 )  b, T16_32x2_hi );
    v32uint16 b_lo = shuffle(( v32uint16 ) b, T16_32x2_lo );

    v32acc64 acc = mul_elem_32( a_hi, b_hi );
    acc = mac_elem_32_conf( a_hi, b_lo, acc, 0, 1, 0, 0 );
    acc = mac_elem_32(      a_lo, b_hi, acc );
    acc = mac_elem_32_conf( a_lo, b_lo, acc, 0, 1, 0, 0 );
    return extract_v16acc64( acc, 0 );
}
inline v32acc64 mul_elem_32( v32int32 a, v32int32 b ) {
    v32int16 a_hi  = ( v32int16 )  shuffle( extract_v16int32( a, 0 ), extract_v16int32( a, 1 ), T16_32x2_hi );
    v32uint16 a_lo = ( v32uint16 ) shuffle( extract_v16int32( a, 0 ), extract_v16int32( a, 1 ), T16_32x2_lo );
    v32int16 b_hi  = ( v32int16 )  shuffle( extract_v16int32( b, 0 ), extract_v16int32( b, 1 ), T16_32x2_hi );
    v32uint16 b_lo = ( v32uint16 ) shuffle( extract_v16int32( b, 0 ), extract_v16int32( b, 1 ), T16_32x2_lo );

    v32acc64 acc = mul_elem_32( a_hi, b_hi );
    acc = mac_elem_32_conf( a_hi, b_lo, acc, 0, 1, 0, 0 );
    acc = mac_elem_32(      a_lo, b_hi, acc );
    acc = mac_elem_32_conf( a_lo, b_lo, acc, 0, 1, 0, 0 );
    return acc;
}


inline v32acc32 mac_elem_32_narrow( v32int16 x, bool sgn_x, v32int16 y, bool sgn_y, v32acc32 acc ) {
    return extract_v32acc32( mac_elem_64( x, sgn_x, y, sgn_y, set_v64acc32( 0, acc )), 0 );
}
inline v32acc32 mac_elem_32_narrow( v32int16 x, v32int16 y, v32acc32 acc ) {
    return extract_v32acc32( mac_elem_64( x, y, set_v64acc32( 0, acc )), 0 );
}


inline v64accfloat upshift_to_float( v64acc32 acc ) {
    return ( v64accfloat ) neg_conf( acc, 0, 1, 1 );
}


inline v32int32 band( v32int32 a, v16int32 b ) {
    return concat( band( extract_v16int32( a, 0 ), b ), band( extract_v16int32( a, 1 ), b ));
}
inline v32int32 bor( v32int32 a, v32int32 b ) {
    return concat( bor( extract_v16int32( a, 0 ), extract_v16int32( b, 0 )), bor( extract_v16int32( a, 1 ), extract_v16int32( b, 1 )));
}


#ifndef __ndl__
template<typename T>
inline T fifo_ld_pop_3d_byte( T *& ptr, fifo_state_t & fifo, dims_3d_t & dims ) {
    return fifo_ld_pop_3d_byte( ptr, fifo, dims.inc3, dims.num1, dims.count1, dims.inc1, dims.num2, dims.count2, dims.inc2 );
}
#endif


#if defined(__chess__) || defined(__AIECC__)
#define PTR_CEIL_LOC( T, loc )                \
inline T loc * ceil( T loc * ptr ) {          \
    return byte_incr( ptr, 63 );              \
}
#define PTR_CEIL( T )                         \
PTR_CEIL_LOC( T, __aie_dm_resource_a)    \
PTR_CEIL_LOC( T, __aie_dm_resource_b)    \
PTR_CEIL_LOC( T, __aie_dm_resource_c)    \
PTR_CEIL_LOC( T, __aie_dm_resource_d)    \
PTR_CEIL_LOC( T, )
#else
#define PTR_CEIL( T )                         \
inline T * ceil( T * ptr ) {                  \
    return ( T* )(((( long )ptr ) + 63 ) & ~63 );  \
}
#endif

PTR_CEIL( v128int8 )
PTR_CEIL( v64int16 )
PTR_CEIL( v32int32 )
PTR_CEIL( v64bfloat16 )
PTR_CEIL( v64int8 )
PTR_CEIL( v32int16 )
PTR_CEIL( v16int32 )
PTR_CEIL( v32bfloat16 )

#undef PTR_CEIL_LOC
#undef PTR_CEIL


#include "pp.h"
#define CHESS_STORAGE_1( T, acc, N ) {          \
    T chess_storage( PP_CAT( dm, N )) a = acc;  \
    acc = a;                                \
}


#define CHESS_STORAGE( T, accs ) {         \
    T chess_storage( dm0 ) a0 = accs.a0;  \
    T chess_storage( dm1 ) a1 = accs.a1;  \
    T chess_storage( dm2 ) a2 = accs.a2;  \
    T chess_storage( dm3 ) a3 = accs.a3;  \
                                        \
    accs.a0 = a0;                       \
    accs.a1 = a1;                       \
    accs.a2 = a2;                       \
    accs.a3 = a3;                       \
}

inline v32bfloat16 exp2_v32( v32accfloat a ) {
    v16accfloat extract_lo = extract_v16accfloat( a, 0 );
    v16accfloat extract_hi = extract_v16accfloat( a, 1 );
    v16bfloat16 lo = exp2( extract_lo );
    v16bfloat16 hi = exp2( extract_hi );
    v32bfloat16 r = concat(lo, hi);
    return r;
}

#if defined(__AIENGINE__)

#include <aie_api/aie.hpp>
#include <aie_api/aie_adf.hpp>

inline void AIE_API_store_int8_from_accfloat(  v32int8 * p8,  v32accfloat acc32float, int shift)
{
    aie::accum<accfloat,32> v_acc32float;
    v_acc32float = acc32float;
    aie::vector<float,32> v_float;
    aie::vector<int8,32> v_int8;
    v_float = v_acc32float.to_vector<float>();
    v_int8 = aie::to_fixed<int8>(v_float, shift);
    *p8 = v_int8.to_native();
}

#endif

static void* conv_to_local_ptr(uint32_t addr)
{
    uint32_t constexpr core_local_offset = 0x70000;
    return reinterpret_cast<void*>(core_local_offset + addr);
}


template <typename T>
void print_buf(T* buf, int32_t num_elements, int32_t col)
{
    for (int32_t i = 0; i < num_elements; i++)
    {
        printf("%lld ", buf[i]);
        if(i % col == (col-1))
            printf("\n");
    }
    printf("\n\n");
}


#define ACT1_SUM_ADDR 53888
#define ACT2_SUM_ADDR 54400
#define C0_ADDR 54912

#endif //__KERNEL_HELPERS_H__
