// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#if defined(__AEG_METADATA_ONLY__)
#include "graph.h"
myGraph gradf;
int main(int argc, char **argv) {
    gradf.init();
    return 0;
}
#else
#include "aiebaremetal.h"
#include <sstream>
#include <vector>
#include <xaiengine.h>

#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
// BaremetalGraphSim gr("gradf");
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xparameters.h"
#if (__AIE_ARCH__ != AIE2PS)
#endif
// BaremetalGraph gr("gradf");
#endif

AEGGraph<aeg::gradf> gr;

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
        gr.in[i].gm2aie_nb(reinterpret_cast<char *>(inputArray[i]), MAT_A_CHUNK_SIZE);
        gr.in[i].gm2aie_nb(reinterpret_cast<char *>(inputArrayB[i]), NUM_ELMNTS * sizeof(int32_t));
    }
    for (int i = 0; i < rown; i++) {
        std::cout << "MAT_A_CHUNK_SIZE " << MAT_A_CHUNK_SIZE << std::endl;
        gr.out[i].aie2gm_nb(reinterpret_cast<char *>(outputArray[i]), MAT_A_CHUNK_SIZE);
    }

    for (int i = 0; i < rown; i++) {
        gr.out[i].gmio_wait();
    }
    std::cout << "6. run--3-" << std::endl;
    gr.end();

    // Validation checks
    bool test_passed = true;
    int error_count = 0;
    int unchanged_count = 0;
    int zero_count = 0;
    const int32_t INITIAL_VALUE = 2;

    std::cout << "\n=== Starting Output Validation ===" << std::endl;

    for (int j = 0; j < rown; j++) {
        for (int i = 0; i < MAT_A_CHUNK; i++) {
            if (outputArray[j][i] == INITIAL_VALUE) {
                unchanged_count++;
                if (unchanged_count <= 10) {
                    printf("ERROR: outputArray[%d][%d] unchanged from initial value %d\n", j, i, INITIAL_VALUE);
                }
            }

            if (outputArray[j][i] == 0) {
                zero_count++;
                if (zero_count <= 10) {
                    printf("WARNING: outputArray[%d][%d] is zero\n", j, i);
                }
            }

            if (i < 16) {
                printf("outputArray[%d][%d] value is %d\n", j, i, outputArray[j][i]);
            }
        }
    }

    std::cout << "\n=== Validation Results ===" << std::endl;
    std::cout << "Total elements checked: " << (rown * MAT_A_CHUNK) << std::endl;
    std::cout << "Elements unchanged from initial value: " << unchanged_count << std::endl;
    std::cout << "Elements with zero value: " << zero_count << std::endl;

    if (unchanged_count > 0) {
        test_passed = false;
        error_count++;
        std::cout << "FAIL: " << unchanged_count << " elements were not updated by the graph" << std::endl;
    }

    int nonzero_count = (rown * MAT_A_CHUNK) - zero_count;
    if (nonzero_count == 0) {
        test_passed = false;
        error_count++;
        std::cout << "FAIL: All output values are zero" << std::endl;
    } else {
        std::cout << "PASS: " << nonzero_count << " elements have non-zero values" << std::endl;
    }

    std::cout << "\n=== Test Result ===" << std::endl;
    if (test_passed) {
        std::cout << "SUCCESS: OOB Test PASSED with new api" << std::endl;
        return 0;
    } else {
        std::cout << "FAILURE: OOB Test FAILED with " << error_count << " validation error(s)" << std::endl;
        return 1;
    }
}
#endif /* !__AEG_METADATA_ONLY__ */
