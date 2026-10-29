// Copyright (C) 2025 Advanced Micro Devices, Inc.

#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <adf.h>

typedef int8 gemm_A_t;
typedef int32 qdq_C_t;
typedef int32 qdq_C0_t;
typedef int32 qdq_I_t;
typedef int8 qdq_O_t;

//constexpr unsigned gemm_M = 64;
//constexpr unsigned gemm_K = 128;
//constexpr unsigned gemm_N = 64;
// conv_Y only used in conv addressing mode. conv_X = gemm_M / conv_Y
constexpr unsigned conv_Y = 1;

constexpr unsigned Iters = 1;

constexpr unsigned qdq_coeffs = 4;
constexpr unsigned qdq_terms = 3;

constexpr unsigned qdq_fp_split_threshold = 1;
constexpr bool sum_write_garbage = 0;
//constexpr unsigned vector_coeffs = 1;

constexpr unsigned actv_actv = 0;

constexpr unsigned Mgran_fp32 = __AIE_ARCH__ == 20 ? 4 : 8;

//constexpr unsigned Mgran = ( std::is_same_v<int16,qdq_I_t> && std::is_same_v<int16,qdq_C_t> ? 16 : ( std::is_same_v<float,qdq_C_t> ? Mgran_fp32 : 8 ));
//constexpr unsigned Mtile = std::min( Mgran, ( std::is_integral_v<qdq_C_t> && unsigned(sizeof( qdq_C_t )) + unsigned(sizeof( qdq_I_t )) > 4 ? 4u : 8u ));
//constexpr unsigned Ngran = 8;
//constexpr unsigned Ntile = 8;

#endif //__CONSTANTS_H__
