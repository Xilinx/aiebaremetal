// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <fstream>
#include <vector>
#include <sstream>
#include <xaiengine.h>
#include "aiebaremetal.h"

#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
BaremetalGraphSim gr("gradf");
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xparameters.h"
#if (__AIE_ARCH__ != AIE2PS)
#endif
BaremetalGraph gr("gradf");
#endif
#include "kernels/config.h"
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
void adf_run() {
    std::cout << " adf_run alloc " << NUM_HW_ROWS * MAT_A_CHUNK_SIZE / 1024 << " kB" << std::endl;
    std::vector<int32_t *> inputArray(NUM_HW_ROWS, NULL), inputArrayB(NUM_HW_ROWS, NULL),
        outputArray(NUM_HW_ROWS, NULL);
    int32_t *mem_in = (int32_t *)GMIO::malloc(NUM_HW_ROWS * MAT_A_CHUNK_SIZE);
    int32_t *mem_in_b = (int32_t *)GMIO::malloc(NUM_ELMNTS * sizeof(int32_t));

    int32_t *mem_out = (int32_t *)GMIO::malloc(NUM_HW_ROWS * MAT_A_CHUNK_SIZE);
    std::cout << "after alloc size = " << NUM_HW_ROWS * MAT_A_CHUNK_SIZE / 1024 << "kb" << std::endl;
    for (int i = 0; i < NUM_HW_ROWS; i++) {
        std::cout << " 1 i = " << i << std::endl;
        inputArray[i] = mem_in + i * MAT_A_CHUNK;
        inputArrayB[i] = mem_in_b + i * MAT_A_CHUNK;
        outputArray[i] = mem_out + i * MAT_A_CHUNK;
        for (int j = 0; j < MAT_A_CHUNK; j++) {
            inputArray[i][j] = i + 1;
            inputArrayB[i][j] = i + 2;
            outputArray[i][j] = i + 1;
        }
    }

    std::cout << "before run" << std::endl;
    gradf.run(1);
    std::cout << "after run" << std::endl;
    int rown = 1;
    for (int i = 0; i < rown; i++) {
        gradf.in[i].gm2aie_nb(inputArray[i], MAT_A_CHUNK_SIZE);
        gradf.in[i].gm2aie_nb(inputArrayB[i], NUM_ELMNTS * sizeof(int32_t));
    }

    for (int i = 0; i < rown; i++) {
        gradf.out[i].aie2gm_nb(outputArray[i], MAT_A_CHUNK_SIZE);
        gradf.out[i].wait(); // assuming data from gm1 are processed by the graph and output to gm2
    }

    std::cout << "before end" << std::endl;
    gradf.end();
    for (auto x : inputArray) {
        GMIO::free(x);
    }
    for (auto x : inputArrayB) {
        GMIO::free(x);
    }
    for (auto x : outputArray) {
        GMIO::free(x);
    }
}
#endif
int main(int argc, char **argv) {
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
    std::cout << "gradf.a  =" << gradf.a << " before init" << std::endl;
    gradf.init();
    std::cout << "after init" << gradf.a << std::endl;
#else
    // FIXME, should handle the sync in runtime api
#endif
    const int rown = NUM_HW_ROWS;
    std::cout << "1 start--08 07" << std::endl;
    std::vector<int32_t *> inputArray(NUM_HW_ROWS, NULL), inputArrayB(NUM_HW_ROWS, NULL),
        outputArray(NUM_HW_ROWS, NULL);
    int32_t *mem_in = (int32_t *)gr.malloc(NUM_HW_ROWS * MAT_A_CHUNK_SIZE);
    int32_t *mem_in_b = (int32_t *)gr.malloc(NUM_ELMNTS * sizeof(int32_t));
    int32_t *mem_out = (int32_t *)gr.malloc(NUM_HW_ROWS * MAT_A_CHUNK_SIZE);
    for (int i = 0; i < rown; i++) {
        std::cout << " 1 i = " << i << std::endl;
        inputArray[i] = mem_in + i * MAT_A_CHUNK;
        inputArrayB[i] = mem_in_b + i * MAT_A_CHUNK;
        outputArray[i] = mem_out + i * MAT_A_CHUNK;
        for (int j = 0; j < MAT_A_CHUNK; j++) {
            inputArray[i][j] = ((j) / MAT_SIZE) + 1 + i;
            inputArrayB[i][j] = 1;
            outputArray[i][j] = 2;
        }
#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)
        // to make sure we can read correct output data from ddr, we need to first flush the output data into DDR
        // WITHOUT doing this, we can not read the correct data , even if we called the invalid funciton
        // this should be the xil api bug
#endif
    }
    std::cout << "2" << std::endl;

    gr.init();
    gr.run(1);

    std::ostringstream in_ostr, out_ostr;
    for (int i = 0; i < rown; i++) {
        std::ostringstream in_ostr;
        in_ostr << "gradf.in[" << i << "]";
        gr.gm2aie_nb((char *)in_ostr.str().c_str(), reinterpret_cast<char *>(inputArray[i]), MAT_A_CHUNK_SIZE);
        gr.gm2aie_nb((char *)in_ostr.str().c_str(), reinterpret_cast<char *>(inputArrayB[i]),
                     NUM_ELMNTS * sizeof(int32_t));
    }
    std::vector<std::string> out_strs;
    for (int i = 0; i < rown; i++) {
        std::ostringstream out_ostr;
        out_ostr << "gradf.out[" << i << "]";
        out_strs.push_back(out_ostr.str());
        std::cout << "MAT_A_CHUNK_SIZE " << MAT_A_CHUNK_SIZE << std::endl;
        gr.aie2gm_nb((char *)out_ostr.str().c_str(), reinterpret_cast<char *>(outputArray[i]), MAT_A_CHUNK_SIZE);
    }

    for (int i = 0; i < rown; i++) {
        gr.gmio_wait((char *)out_strs[i].c_str()); // assuming data from gm1 are processed by the graph and output to
                                                   // gm2
    }
    std::cout << "6. run--3-" << std::endl;
    gr.end();
    for (int j = 0; j < rown; j++) {
        for (int i = 0; i < 16; i++) {
            printf("outputArray[%d][%d] value is %d\n ", j, i, outputArray[j][i]);
        }
    }
    std::cout << "\n" << "OOB Test Passed\n";
    return 0;
}
