/**
* Copyright (C) 2025 Advanced Micro Devices, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include <stdio.h>
#include <adf.h>
void square(input_window_int32 * win, 
              output_window_int32 *out) { 

  printf("square log\n");
  for (unsigned i = 0 ; i < 32 ; i++) {
    int j = window_readincr(win);
		if (i==0)printf("%d ", j);
    window_writeincr(out,j*j);
  }
  printf("\nsquare log end \n");
}
