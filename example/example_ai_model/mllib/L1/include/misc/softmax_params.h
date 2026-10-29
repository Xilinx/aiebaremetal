// Copyright (C) 2025 Advanced Micro Devices, Inc.

#ifndef __SOFTMAX_PARAMS_H__
#define __SOFTMAX_PARAMS_H__

#include "layer_params.h"
#include "utils.h"
#include <adf.h>
#include "aie.hpp"

#ifndef SM_SCALE_FAC
#define SM_SCALE_FAC 8 // Use 8-bit fractional part for LUTs when converting from bfloat16 to int, adjust any input scale factor using this.
#endif

struct alignas(v16uint16) softmax_internal_params {
    //softmax layer level params
    //This is just the row offset for now. The number of rows that needs to be skipped (useful in full broadcast to column mode)
    int8_t enable_exp;
    uint16_t row_offset;

    softmax_internal_params(): enable_exp(0)
    {
    }
};

typedef struct softmax_internal_params softmax_params_t ;

#endif  //__SOFTMAX_PARAMS_H__
