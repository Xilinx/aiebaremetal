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

#include <fstream>
#include <xaiengine.h>

#include "../../aiebaremetal.h"
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
Mm mm;
BaremetalGraphSim gr("mm");
#else
BaremetalGraph gr("mm");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>

#define SAMPLES 256

int main(int argc, char ** argv)
{
    const int input_size_in_bytes = SIZE * sizeof(float);
    const int output_size_in_bytes = SIZE * sizeof(float);
    
    mm.init();
   
    gr.init();
    gr.run(1);
   
    auto out_bomapped = reinterpret_cast<float*>(gr.malloc(output_size_in_bytes));
    memset(out_bomapped, 0, output_size_in_bytes);

    auto in_bomappedA = reinterpret_cast<float*>(gr.malloc(input_size_in_bytes));
    auto in_bomappedB = reinterpret_cast<float*>(gr.malloc(input_size_in_bytes));
	
    //setting input data
    float *golden = (float*)malloc(output_size_in_bytes);
    for(int i = 0; i < SIZE; i++){
        in_bomappedA[i] = rand() % SIZE;
        in_bomappedB[i] = rand() % SIZE;
    }
    for(int i = 0; i < HEIGHT ; i++) {
        for(int j = 0; j < WIDTH ; j++){
            golden[i*WIDTH+j] = 0;
            for(int k=0; k <WIDTH; k++) golden[i*WIDTH+j] += in_bomappedA[i*WIDTH + k] * in_bomappedB[k+WIDTH * j];
        }
    } 

    int rval;

    std::string gmioportA, gmioportB, gmioportC;
    int gmio_id = 0;
    gmioportA = std::to_string(gmio_id++);
    
    gmioportB = std::to_string(gmio_id++);
    gr.gm2aie_nb("mm.inB", reinterpret_cast<char *>(in_bomappedB),SIZE * sizeof(float));
    std::cout << "syncAIEBO B \n";
   
    gr.gm2aie_nb("mm.inA",reinterpret_cast<char *>(in_bomappedA),SIZE * sizeof(float));
    std::cout << "syncAIEBO A \n";

    gmioportC = std::to_string(gmio_id++);
    gr.aie2gm_nb("mm.out", reinterpret_cast<char *>(out_bomapped),SIZE * sizeof(float) );
	gr.gmio_wait("mm.out");
    gr.wait();
    int match = 0;
    for (int i = 0; i < SIZE; i++) {
        if (out_bomapped[i] != golden[i]) {
            printf("ERROR: Test failed! Error found in sample %d: golden: %f, hardware: %f\n", i, golden[i], out_bomapped[i]);
            match = 1;
            break;
        }
    }

    std::cout << "Releasing remaining objects...\n";
   
    gr.free(in_bomappedA);
    gr.free(in_bomappedB);
    gr.free(out_bomapped);
   
    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl; 
    return (match ? EXIT_FAILURE :  EXIT_SUCCESS);
}