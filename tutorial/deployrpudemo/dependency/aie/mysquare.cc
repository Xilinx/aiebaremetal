// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <adf.h>
#define DATA_LENGTH 128
void mysquare(input_window_int32 * win, 
              output_window_int32 *out, int param) { 

  
  for (unsigned i = 0 ; i < DATA_LENGTH ; i++) {
    int j = window_readincr(win);
    window_writeincr(out,j*j + param  );
  }

}
