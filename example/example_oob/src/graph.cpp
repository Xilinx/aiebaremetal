// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <fstream>
#include <vector>
#include <sstream>
#include <xaiengine.h>
#include "aiebaremetal.h"
#include "aeg_platform.h"

#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
BaremetalGraphSim gr("gradf");
#else
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
    for (int i = 0; i < NUM_HW_ROWS; i++) {
        gradf.in[i].gm2aie_nb(inputArray[i], MAT_A_CHUNK_SIZE);
        gradf.in[i].gm2aie_nb(inputArrayB[i], NUM_ELMNTS * sizeof(int32_t));
    }

    for (int i = 0; i < NUM_HW_ROWS; i++) {
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
#ifdef _RPU_
    // RPU need to disable data cache, without cache diable app will stuck
#if !defined(__ZEPHYR__)
    Xil_DCacheDisable();
    Xil_ICacheDisable();
    printf("Cache disabled for RPU\n");
#endif
#endif

#endif
    gr.init();

    int32_t **input_a;
    int32_t **input_a_hw_rows;
    int32_t **input_b;
    int32_t **input_b_hw_rows;
    int32_t **output_aie;
    int32_t **output_aie_hw_rows;
    int32_t **output_cpu;

    std::cout << "Allocating memory for input and output matrices" << std::endl;
    int32_t *mem_a = (int32_t *)gr.malloc(NUM_ROWS * sizeof(int32_t *) +
                                          NUM_HW_ROWS * sizeof(int32_t *) +
                                          NUM_ROWS * NUM_COLS * sizeof(int32_t));
    int32_t *mem_b = (int32_t *)gr.malloc(NUM_ROWS * sizeof(int32_t *) +
                                          NUM_HW_ROWS * sizeof(int32_t *) +
                                          NUM_ROWS * NUM_COLS * sizeof(int32_t));
    int32_t *mem_out = (int32_t *)gr.malloc(NUM_ROWS * sizeof(int32_t *) +
                                            NUM_HW_ROWS * sizeof(int32_t *) +
                                            NUM_ROWS * NUM_COLS * sizeof(int32_t));
    output_cpu = (int32_t **) malloc(NUM_ROWS * sizeof(int32_t *) +
                                     NUM_HW_ROWS * sizeof(int32_t *) +
                                     NUM_ROWS * NUM_COLS * sizeof(int32_t));
    if (!mem_a || !mem_b || !mem_out || !output_cpu) {
        std::cout << "[INFO] Failed to allocate memory" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Inidializing input and output matrices" << std::endl;
    input_a = (int32_t **) mem_a;
    input_a_hw_rows = (int32_t **) mem_a + NUM_ROWS;
    input_b = (int32_t **) mem_b;
    input_b_hw_rows = (int32_t **) mem_b + NUM_ROWS;
    output_aie = (int32_t **) mem_out;
    output_aie_hw_rows = (int32_t **) mem_out + NUM_ROWS;
    for (int i = 0; i < NUM_ROWS; i++) {
        input_a[i] = (int32_t *)(input_a + NUM_ROWS + NUM_HW_ROWS) + i * NUM_COLS;
        input_b[i] = (int32_t *)(input_b + NUM_ROWS + NUM_HW_ROWS) + i * NUM_COLS;
        output_aie[i] = (int32_t *)(output_aie + NUM_ROWS + NUM_HW_ROWS) + i * NUM_COLS;
        output_cpu[i] = (int32_t *)(output_cpu + NUM_ROWS + NUM_HW_ROWS) + i * NUM_COLS;
    }

    for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_COLS; j++) {
            int32_t value = (i * NUM_COLS + j) % 10;

            value = value == 0 ? 1 : value;
            input_a[i][j] = value;
            input_b[i][j] = 1;
            output_aie[i][j] = 0;
            output_cpu[i][j] = 0;
        }
    }

    for (int i = 0; i < NUM_HW_ROWS; i++) {
        input_a_hw_rows[i] = (int32_t *)(input_a + NUM_ROWS + NUM_HW_ROWS) + i * (NUM_ROWS_PER_HW_ROW * NUM_COLS);
        input_b_hw_rows[i] = (int32_t *)(input_b + NUM_ROWS + NUM_HW_ROWS) + i * (NUM_ROWS_PER_HW_ROW * NUM_COLS);
        output_aie_hw_rows[i] = (int32_t *)(output_aie + NUM_ROWS + NUM_HW_ROWS) + i * (NUM_ROWS_PER_HW_ROW * NUM_COLS);
    }

#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)
        // to make sure we can read correct output data from ddr, we need to first flush the output data into DDR
        // WITHOUT doing this, we can not read the correct data , even if we called the invalid funciton
        // this should be the xil api bug
#endif

    std::cout << "Starting graph execution" << std::endl;
    gr.run(1);
    std::cout << "graph started" << std::endl;

    std::ostringstream in_ostr, out_ostr;
    for (int i = 0; i < NUM_HW_ROWS; i++) {
        std::ostringstream in_ostr;

        in_ostr << "gradf.in[" << i << "]";
        gr.gm2aie_nb((char *)in_ostr.str().c_str(),
                     reinterpret_cast<char *>(input_a_hw_rows[i]),
                     NUM_ROWS_PER_HW_ROW * NUM_COLS * sizeof(int32_t));
        gr.gm2aie_nb((char *)in_ostr.str().c_str(),
                     reinterpret_cast<char *>(input_b_hw_rows[0]),
                     NUM_ROWS * NUM_COLS * sizeof(int32_t));
    }
    std::vector<std::string> out_strs;
    for (int i = 0; i < NUM_HW_ROWS; i++) {
        std::ostringstream out_ostr;
        out_ostr << "gradf.out[" << i << "]";
        out_strs.push_back(out_ostr.str());
        std::cout << "Issuing gmio read for " << out_ostr.str() << std::endl;
        gr.aie2gm((char *)out_ostr.str().c_str(),
                  reinterpret_cast<char *>(output_aie_hw_rows[i]),
                  NUM_ROWS_PER_HW_ROW * NUM_COLS * sizeof(int32_t));
    }

    for (int i = 0; i < NUM_HW_ROWS; i++) {
        std::cout << "Waiting for output from " << out_strs[i] << std::endl;
        gr.gmio_wait((char *)out_strs[i].c_str()); // assuming data from gm1 are processed by the graph and output to
                                                   // gm2
    }
    std::cout << "Ending graph execution" << std::endl;
    gr.end();
    std::cout << "Graph execution ended" << std::endl;

    printf("Computing matrix multiplication on CPU for verification...\n");
    /* Compute matrix multiplication on CPU for result verification */
    for (int i = 0; i < NUM_ROWS; i++) {
        for (int j = 0; j < NUM_ROWS; j++) {
            if (!(j % 128)) {
                printf("\ri=%d j=%d", i, j);
                fflush(stdout);
            }
            for (int k = 0; k < NUM_COLS; k++) {
                output_cpu[i][j] += input_a[i][k] * input_b[k][j];
            }
        }
    }
    printf("\n");
    printf("Verifying results...\n");
    /* Z-ordering and compare with CPU */
    for (int i = 0; i < NUM_COLS / (WIN_SIZE / NUM_ROWS_PER_TILE); i++) {
        for (int j = 0; j < NUM_HW_ROWS; j++) {
            for (int k = 0; k < NUM_HW_COLS; k++) {
                for (int m = 0; m < WIN_SIZE / NUM_ROWS_PER_TILE; m++) {
                    for (int l = 0; l < NUM_ROWS_PER_TILE; l++) {
                        int dst_row = j * NUM_ROWS_PER_HW_ROW + k * NUM_ROWS_PER_TILE + l;
                        int dst_col = i * (WIN_SIZE / NUM_ROWS_PER_TILE) + m;
                        int src_idx = j * MAT_A_CHUNK + i * NUM_ROWS_PER_HW_ROW * (WIN_SIZE / NUM_ROWS_PER_TILE) + k * WIN_SIZE + m * NUM_ROWS_PER_TILE + l;
                        int *src = (int *)((int **)output_aie_hw_rows + NUM_HW_ROWS);
                        if (!(dst_col % 128)) {
                            printf("\rChecking dst_row=%d dst_col=%d src_idx=%d", dst_row, dst_col, src_idx);
                            fflush(stdout);
                        }
                        if (src[src_idx] != output_cpu[dst_row][dst_col]) {
                            printf("\n[ERROR] Data mismatch at [%d][%d]: AIE = %d, CPU = %d\n", dst_row, dst_col, src[src_idx], output_cpu[dst_row][dst_col]);
                            printf("Test Failed!\n");
                            return -1;
                        }
                    }
                }
            }
        }
    }
    printf("\n");

    std::cout << "OOB Test Passed\n";
    return 0;
}
