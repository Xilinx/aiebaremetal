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
#include<math.h>
#include <adf.h>
#include<aie_api/aie.hpp>
#include<aie_api/aie_adf.hpp>

#define DATA_SIZE 32

void softmax(input_window_float * win, 
              output_window_float *out)
{ 
  const int input_len = DATA_SIZE;
  float input[32];
  float output[32];
  float exp_values[32];
  printf("Reading input inside kernel \n");
  for (unsigned int i = 0; i < input_len; i++) {
    window_readincr(win, input[i]);
    
  }
  printf("Calculating exponents inside kernel \n");
  float sum = 0.0;
  for (unsigned int i = 0; i < input_len; i++) {
    exp_values[i] = exp(input[i]);
    sum += exp_values[i];
  }

 float prob_sum = 0.0;
 printf("Writing outputs inside kernel\n");
  for (size_t i = 0; i < input_len; i++) {
    output[i] = exp_values[i]/sum;
    prob_sum += output[i];
    window_writeincr(out, output[i]);
  }

 printf("Kernel executed\n");	
 if(abs(prob_sum - 1.0f) < 1e-9){
  printf("Test Passed, prob_sum:%f\n", prob_sum);
 }
 else{
  printf("Test Failed, prob_sum:%f\n", prob_sum);
 }
 

 
}
