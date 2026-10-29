// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
//
// Radar 2D-FFT host code using typed AEG API.
// Refactored from graph.cpp: removed shim_config1-4(), shim_perf_config(),
// XAie_SetupConfig/CfgInitialize, and all direct XAie_Write32 calls.

#if defined(__AEG_METADATA_ONLY__) || defined(__ADF_FRONTEND__)
#include "graph_new_api.h"
FFT2D_graphall gradf;
#else
#include "aiebaremetal.h"
#include "aeg_platform.h"
#include "perf.h"

#include <fstream>
#include <stdio.h>

#include "golden.data"
#include "input.data"

#define CHECK 1
#define ITER_CNT 1

AEGGraph<aeg::gradf> gr;

int main(int argc, char **argv) {
#if defined(__ADF_FRONTEND__)
    gradf.init();
#else
#ifdef _RPU_
    Xil_DCacheDisable();
#endif

    unsigned int itrc = ITER_CNT;
    PERF_DECLARE();
    PERF_RUN_BEGIN("new_api", itrc);

    PERF_TIC();
    gr.init();
    PERF_TOC("init");
    printf("Initialising Graph ...\n");

    uint32_t *buffer_in_mm2s_map_1;
    uint32_t *buffer_in_mm2s_map_2;
    uint32_t *buffer_in_mm2s_map_3;
    uint32_t *buffer_in_mm2s_map_4;

    int32_t *buffer_in_s2mm_map_1;
    int32_t *buffer_in_s2mm_map_2;
    int32_t *buffer_in_s2mm_map_3;
    int32_t *buffer_in_s2mm_map_4;

    uint32_t temp_buffer[2048 * 64 * 4];
    memcpy(temp_buffer, m_fft_buffer_mapped, 2048 * 64 * 4);

    PERF_TIC();
    buffer_in_mm2s_map_1 = (uint32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_mm2s_map_2 = (uint32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_mm2s_map_3 = (uint32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_mm2s_map_4 = (uint32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);

    buffer_in_s2mm_map_1 = (int32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_s2mm_map_2 = (int32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_s2mm_map_3 = (int32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);
    buffer_in_s2mm_map_4 = (int32_t *)gr.malloc(2048 * 64 * 4 * 8 * 24);

    // DDR bounce buffers: 12,582,912 cint16 elements = ~48 MB each, 4 instances.
    // The old code used hardcoded high-DDR addresses (0x500_xxxxxxxx) which don't
    // consume heap. Here we use gr.malloc from the 1 GB heap (see lscript.ld).
    //
    // For multi-iteration (ITER_CNT > 1), separate ping/pong buffers are needed
    // so S2MM can write iteration N+1 while MM2S reads iteration N:
    //
    //   void *bounce_ping[4], *bounce_pong[4];
    //   for (int i = 0; i < 4; i++) {
    //       bounce_ping[i] = gr.malloc(bounce_size);
    //       bounce_pong[i] = gr.malloc(bounce_size);
    //       gr.ddr_bounce[i].setupPingPongBuffers(bounce_ping[i], bounce_pong[i], false);
    //   }
    //
    // This doubles bounce allocation to 384 MB, requiring a larger heap
    // (_HEAP_SIZE in thirdparty/arch/ps/psv_cortexa72_0/lscript.ld).
    //
    // For ITER_CNT=1 only the ping BD executes, so we reuse the same buffer
    // for both ping and pong to keep total bounce allocation at 192 MB.
    const size_t bounce_size = 12582912 * 4;
    void *bounce_buf[4];
    for (int i = 0; i < 4; i++) {
        bounce_buf[i] = gr.malloc(bounce_size);
    }
    PERF_TOC("malloc");

    PERF_TIC();
    // Chirp rotation preprocessing: rotate each chirp left by 4 samples.
    unsigned int k = 0;
    int ind = 0;
    int shft_cnt = 0;
    size_t total_samples = 2048 * 8 * 24;
    unsigned int size = total_samples / 4;

    uint32_t ptr[2048];
    for (int i = 0; i < 64; i++) {
        shft_cnt = 0;
        ind = 0;

        for (int p = 0; p < 2048; p++) {
            ptr[p] = temp_buffer[p + 2048 * i];
        }
        uint32_t index = 0;
        for (int q = 0; q < size; q++) {
            if ((q != 0) && (q % 512 == 0)) {
                ind = 0;
                shft_cnt++;
            }
            index = (ind + shft_cnt) % 512;

            buffer_in_mm2s_map_1[k++] = ptr[index * 4];
            buffer_in_mm2s_map_1[k++] = ptr[index * 4 + 1];
            buffer_in_mm2s_map_1[k++] = ptr[index * 4 + 2];
            buffer_in_mm2s_map_1[k++] = ptr[index * 4 + 3];

            ind++;
        }
    }

    memcpy(buffer_in_mm2s_map_2, buffer_in_mm2s_map_1, 2048 * 64 * 4 * 8 * 24);
    memcpy(buffer_in_mm2s_map_3, buffer_in_mm2s_map_1, 2048 * 64 * 4 * 8 * 24);
    memcpy(buffer_in_mm2s_map_4, buffer_in_mm2s_map_1, 2048 * 64 * 4 * 8 * 24);
    PERF_TOC("preprocess");

    printf("K is :%d\n", k);
    printf("memcopy done\n");
    printf("Data transfer through GMIO is starting\n");

    PERF_TIC();
    gr.gm_in[0].gm2aie_nb(reinterpret_cast<char *>(buffer_in_mm2s_map_1), 64 * 4 * 8 * 24 * 2048);
    gr.gm_in[1].gm2aie_nb(reinterpret_cast<char *>(buffer_in_mm2s_map_2), 64 * 4 * 8 * 24 * 2048);
    gr.gm_in[2].gm2aie_nb(reinterpret_cast<char *>(buffer_in_mm2s_map_3), 64 * 4 * 8 * 24 * 2048);
    gr.gm_in[3].gm2aie_nb(reinterpret_cast<char *>(buffer_in_mm2s_map_4), 64 * 4 * 8 * 24 * 2048);
    PERF_TOC("gm2aie");

    printf("Data transfer through GMIO in progress\n");

    PERF_TIC();
    // Set up DDR bounce ping-pong buffers (replaces manual shim_config1-4)
    for (int i = 0; i < 4; i++) {
        gr.ddr_bounce[i].setupPingPongBuffers(bounce_buf[i], bounce_buf[i], false);
    }
    PERF_TOC("dma_config");

    PERF_TIC();
    gr.run(2048 * 64 * 8 * 24);

    gr.gm_out[0].aie2gm_nb(reinterpret_cast<char *>(buffer_in_s2mm_map_1), 2048 * 64 * 4 * 8 * 24);
    gr.gm_out[1].aie2gm_nb(reinterpret_cast<char *>(buffer_in_s2mm_map_2), 2048 * 64 * 4 * 8 * 24);
    gr.gm_out[2].aie2gm_nb(reinterpret_cast<char *>(buffer_in_s2mm_map_3), 2048 * 64 * 4 * 8 * 24);
    gr.gm_out[3].aie2gm_nb(reinterpret_cast<char *>(buffer_in_s2mm_map_4), 2048 * 64 * 4 * 8 * 24);

    gr.gm_out[0].gmio_wait();
    gr.gm_out[1].gmio_wait();
    gr.gm_out[2].gmio_wait();
    gr.gm_out[3].gmio_wait();

    gr.wait();
    PERF_TOC("run_output_wait");

    int errCnt = 0;
    printf("errCnt before running value %d...\n", errCnt);

    if (itrc == 1) {
        printf("Kernels are running for 1 iteration...\n");

        printf("Kernels execution completed ...\n");

        PERF_TIC();
        for (int j = 0; j < 2048 * 64 * 8 * 24; j++) {
            if (buffer_in_s2mm_map_1[j] != golden_data[j] || buffer_in_s2mm_map_2[j] != golden_data[j] ||
                buffer_in_s2mm_map_3[j] != golden_data[j] || buffer_in_s2mm_map_4[j] != golden_data[j]) {
                errCnt++;
            }
        }
        PERF_TOC("golden_check");
        printf("errCnt after running value %d...\n", errCnt);
        if (errCnt == 0) {
            printf("\n ******** Golden Check Passed ******** \n");
        } else {
            printf("\n ******** Golden Check Failed with error count %d ******** \n", errCnt);
        }
    } else {
        printf("Kernels are running ...\n");
    }

    for (int j = 0; j < 10; j++) {
        printf("Final output data :%d \n", buffer_in_s2mm_map_1[j]);
    }

    std::cout << "Closing Graph...\n";
    gr.end();
    PERF_RUN_END();

    gr.free(buffer_in_mm2s_map_1);
    gr.free(buffer_in_mm2s_map_2);
    gr.free(buffer_in_mm2s_map_3);
    gr.free(buffer_in_mm2s_map_4);

    gr.free(buffer_in_s2mm_map_1);
    gr.free(buffer_in_s2mm_map_2);
    gr.free(buffer_in_s2mm_map_3);
    gr.free(buffer_in_s2mm_map_4);

    for (int i = 0; i < 4; i++) {
        gr.free(bounce_buf[i]);
    }

    return 0;
#endif /* !defined(__ADF_FRONTEND__) */
}
#endif /* !__AEG_METADATA_ONLY__ && !__ADF_FRONTEND__ */
