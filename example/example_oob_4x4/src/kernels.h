// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef __KERNELS__
#define __KERNELS__
#include "kernels/config.h"
#include <adf.h>
#if (__AIE_ARCH__ == ARCH_AIE)
void OneInput(input_window<int32> *__restrict dataIn, output_window<int32> *__restrict aOut,
              output_window<int32> *__restrict result, output_cascade<acc48> *bOut);

void TwoInputs(input_window<int32> *__restrict aIn, input_window<int32> *__restrict bypassResult,
               input_cascade<acc48> *bIn, output_window<int32> *__restrict aOut,
               output_window<int32> *__restrict result, output_cascade<acc48> *bOut);

void OneOutput(input_window<int32> *__restrict aIn, input_window<int32> *__restrict bypassResult,
               input_cascade<acc48> *bIn, output_window<int32> *__restrict result);
#else
// INT8 datapath: A/B stream in as int8; each tile saturates its full-K int32
// result to int8, so the gathered C stream is int8 in and int8 out.
void OneInput(input_window_int8 *dataIn, output_window_int8 *dataOut, output_window_int8 *result);

void TwoInputs(input_window_int8 *dataIn, input_window_int8 *bypassResult, output_window_int8 *dataOut,
               output_window_int8 *result);

void OneOutput(input_window_int8 *dataIn, input_window_int8 *bypassResult, output_window_int8 *result);
#endif
#endif
