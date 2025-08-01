/**
* Copyright (C) 2024-2025 Advanced Micro Devices, Inc.
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

#include "graph.h"

myGraph gradf;
myGraph gradf1;
myGraph gradf2;
int main(int argc, char* argv[]) {
    int* outputArray = (int*)adf::GMIO::malloc(256*sizeof(int));

    int* outt;
    outt = (int*)adf::GMIO::malloc(256*sizeof(int));

    gradf2.out[0].aie2gm_nb(outt, 32*sizeof(int));
    
    gradf.init();
    gradf2.run(1);

    char a[156];
    gradf.in[2].gm2aie_nb(a, 256);
    
    const int idx = 1;

    // Showcases:
    // * Using expressions/variables in subscript
    // * Automatic curly brace insertion
    for(int i = 0; i < 1; ++i)
        gradf1.out[i+0].aie2gm_nb(&outputArray[idx*32], 32*sizeof(int));
    
    gradf.singleout.wait();
    gradf.out[0].wait();
    gradf2.outarray[1][2].wait();
    gradf.firstout.out[0].wait();
    gradf.wait();
    
    adf::GMIO::free(outputArray);
    adf::GMIO::free(outt);
}