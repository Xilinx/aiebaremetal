// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
myGraph gradf;
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
// #include "xtime_l.h"
#endif

#include "aiebaremetal.h"
#include "config.h"
#include <fstream>
#include <xaiengine.h>

#ifdef _RPU_
// set rpu support
uint8_t rpu = 1;
#else
uint8_t rpu = 0;
#endif

AbrGraph gr("gradf");
#define DATA_LENGTH 256

#define HW_GEN 5
#define XAIE_BASE_ADDR 2199023255552
#define XAIE_COL_SHIFT 25
#define XAIE_ROW_SHIFT 20
#define XAIE_NUM_ROWS 7
#define XAIE_NUM_COLS 36
#define XAIE_SHIM_ROW 0
#define XAIE_MEM_TILE_ROW_START 1
#define XAIE_MEM_TILE_NUM_ROWS 2
#define XAIE_AIE_TILE_ROW_START 3
#define XAIE_AIE_TILE_NUM_ROWS 4

int main(int argc, char **argv) {
    printf("adfr--2---\n");
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    gradf.init();
#else
#ifdef _RPU_
    // RPU need to disable data cache, without cache diable app will stuck
    Xil_DCacheDisable();
    // Xil_ICacheDisable();
#endif
#endif

    gr.init();
    // gr.getaiedevinst();
    printf("***********gr.init() successful************\n");
    int32_t *inputArray = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    int32_t *outputArray_gold = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    printf("Successfully allocated memory\n");
    // int handle0 = gr.start_profiling("gradf.in", 1);

    printf("*************1\n");
    gr.run(1);
    printf("**************2\n");
    for (int i = 0; i < DATA_LENGTH; i++) {
        inputArray[i] = i + 1;
        outputArray_gold[i] = inputArray[i] * inputArray[i];
    }
    gr.gm2aie_nb("gradf.in", reinterpret_cast<char *>(&inputArray[0]), DATA_LENGTH * sizeof(int32_t));
    gr.aie2gm_nb("gradf.out", reinterpret_cast<char *>(&outputArray[0]), DATA_LENGTH * sizeof(int32_t));
    std::cout << "before gmio_wait ---" << std::endl;
    XAie_SetupConfig(ConfigPtr, HW_GEN, XAIE_BASE_ADDR, XAIE_COL_SHIFT, XAIE_ROW_SHIFT, XAIE_NUM_COLS, XAIE_NUM_ROWS,
                     XAIE_SHIM_ROW, XAIE_MEM_TILE_ROW_START, XAIE_MEM_TILE_NUM_ROWS, XAIE_AIE_TILE_ROW_START,
                     XAIE_AIE_TILE_NUM_ROWS);

    XAie_InstDeclare(DevInst, &ConfigPtr);

    AieRC RC = XAie_CfgInitialize(&DevInst, &ConfigPtr);

    uint32_t dma_mm2s_status_0;
    XAie_Read32(&DevInst, XAie_GetTileAddr(&DevInst, 0, 0) + 0x00009328, &dma_mm2s_status_0);

    printf("dma_mm2s_status_0 :0x%lx\n", dma_mm2s_status_0);
    gr.gmio_wait("gradf.out"); // assuming data from gm1 are processed by the graph and output to gm2
    printf("*********3");

    gr.wait();
    gr.end();

    // if (handle0 != 0)
    // {
    //     long long cycle_count0 = gr.read_profiling(handle0);
    //     gr.stop_profiling(handle0);
    // 	printf("cycle_count0 = %lld\n", cycle_count0);
    // }

    for (int i = 0; i < DATA_LENGTH; i++) {
        std::cout << "value " << outputArray[i] << ", ";
        if (outputArray[i] != outputArray_gold[i]) {
            std::cout << "\n at data[" << i << "] incorrect Test Failed \n";
            return -1;
        }
    }

    std::cout << "\n" << "Test Passed\n";

    gr.free(inputArray);
    gr.free(outputArray);
    gr.free(outputArray_gold);
    return 0;
}
