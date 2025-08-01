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

#include <xaiengine.h>
#include "aiebaremetal.h"
#include <string>
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
#endif
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "multincludecheck.h"
#endif 

#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
myGraph gradf; 
#endif
AbrGraph gr("gradf");
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
myGraph gradf1; 
#endif
AbrGraph gr1("gradf1");
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
myGraph gradf2; 
#endif
AbrGraph gr2("gradf2");
int main(int argc, char* argv[]) {
    int* outputArray = (int*)gr1.malloc(256 * sizeof(int));

    int* outt;
    outt = (int*)gr2.malloc(256 * sizeof(int));

    gr2.aie2gm_nb("gradf2.out[0]",reinterpret_cast<char*>(outt),32 * sizeof(int));
    
    gr.init();
    gr2.run(1);

    char a[156];
    gr.gm2aie_nb("gradf.in[2]",reinterpret_cast<char*>(a),256);
    
    const int idx = 1;

    // Showcases:
    // * Using expressions/variables in subscript
    // * Automatic curly brace insertion
    for(int i = 0; i < 1; ++i)
    {
        char *_adftoaeg_temp_0 = ("gradf1.out[" + std::to_string(i + 0) + "]").data();
        gr1.aie2gm_nb(_adftoaeg_temp_0,reinterpret_cast<char*>(outputArray[idx * 32]),32 * sizeof(int));
    }
    
    gr.gmio_wait("gradf.singleout");
    gr.gmio_wait("gradf.out[0]");
    gr2.gmio_wait("gradf2.outarray[1][2]");
    gr.gmio_wait("gradf.firstout.out[0]");
    gr.wait();
    
    gr1.free(outputArray);
    gr2.free(outt);
}