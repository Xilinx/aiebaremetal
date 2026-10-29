// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include "aiebaremetal.h"
#include "aeg_platform.h"
#if defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
#endif

#include <fstream>
#include <math.h>
#include <xaiengine.h>

AbrGraph gr("gradf");

#if defined(__ADF_FRONTEND__)
void adf_run() {
    int32_t *inputArray = (int32 *)GMIO::malloc(256 * sizeof(int32));
    int32_t *outputArray = (int32 *)GMIO::malloc(256 * sizeof(int32));

    for (int i = 0; i < 256; i++)
        inputArray[i] = i + 1;

    std::ofstream ofs;

    gradf.run(1);
    for (int i = 0; i < 1; i++) {
        gradf.in.gm2aie_nb(&inputArray[i * 32], 32 * sizeof(int32));
        gradf.out.aie2gm_nb(&outputArray[i * 32], 32 * sizeof(int32));
    }
    gradf.out.wait(); // assuming data from gm1 are processed by the graph and output to gm2

    for (int i = 0; i < 256; i++) {
        std::cout << outputArray[i] << ", ";
    }

    std::cout << "before end" << std::endl;
    gradf.end();

    GMIO::free(inputArray);
    GMIO::free(outputArray);
}
#endif

int main(int argc, char **argv) {
    // 256 elements = 8 tiles of 32, matching pl_only's data volume for a
    // directly comparable end-to-end timing.
    const int kNumTiles = 8;
    const int kTileLen = 32;
    const int kTotalLen = kNumTiles * kTileLen;

    float *inputArray = (float *)gr.malloc(kTotalLen * sizeof(float));
    float *outputArray = (float *)gr.malloc(kTotalLen * sizeof(float));
    float *outputArray_gold = (float *)gr.malloc(kTotalLen * sizeof(float));

    gr.init();
    gr.run(kNumTiles);

    for (int i = 0; i < kTotalLen; i++)
        inputArray[i] = (i + 1) / 500.0;
    std::cout << "before:run---" << std::endl;

#if defined(AEG_HARDWARE) && (__AIE_ARCH__ != 22)
    XTime tStart, tEnd;
    XTime_GetTime(&tStart);
#endif
    // Batch non-blocking GMIO transfers in groups of kBatchTiles; queuing
    // all tiles back-to-back before a single wait overflows the GMIO
    // non-blocking queue depth.
    const int kBatchTiles = 2;
    for (int b = 0; b < kNumTiles; b += kBatchTiles) {
        for (int i = b; i < b + kBatchTiles; i++) {
            gr.gm2aie_nb("gradf.in", reinterpret_cast<char *>(&inputArray[i * kTileLen]), kTileLen * sizeof(float));
            gr.aie2gm_nb("gradf.out", reinterpret_cast<char *>(&outputArray[i * kTileLen]), kTileLen * sizeof(float));
        }
        gr.gmio_wait("gradf.out"); // assuming data from gm1 are processed by the graph and output to gm2
    }
    gr.wait();
#if defined(AEG_HARDWARE) && (__AIE_ARCH__ != 22)
    XTime_GetTime(&tEnd);
    printf("[perf] aie_only: %d elements, %llu clock cycles.\n", kTotalLen, 2 * (tEnd - tStart));
    printf("[perf] aie_only: %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));
#endif
    std::cout << "run---" << std::endl;

    // Correctness check runs entirely after gr.wait() returns, so it cannot
    // perturb the timing measurement taken around the run/wait window
    // above. Reference matches softmax.cc exactly: per-32-element-tile
    // exp(x)/sum(exp(x)), no max-subtraction.
    const float kAbsTol = 1e-4f;
    bool all_pass = true;
    float max_abs_err = 0.0f;
    for (int t = 0; t < kNumTiles; t++) {
        float sum = 0.0f;
        for (int i = 0; i < kTileLen; i++) {
            int idx = t * kTileLen + i;
            outputArray_gold[idx] = exp(inputArray[idx]);
            sum += outputArray_gold[idx];
        }
        // Per-tile breakdown: a single worst-tile summary can mask other
        // tiles failing, since every tile's gold pattern is numerically
        // identical (constant per-tile offset cancels out in softmax).
        float tile_max_err = 0.0f;
        for (int i = 0; i < kTileLen; i++) {
            int idx = t * kTileLen + i;
            outputArray_gold[idx] /= sum;
            float err = fabs(outputArray[idx] - outputArray_gold[idx]);
            if (err > tile_max_err)
                tile_max_err = err;
            if (err > max_abs_err)
                max_abs_err = err;
            if (err > kAbsTol)
                all_pass = false;
        }
        std::cout << "  tile" << t << " max_abs_err=" << tile_max_err << std::endl;
    }
    std::cout << "Max abs error vs CPU reference: " << max_abs_err << std::endl;

    if (all_pass) {
        std::cout << "Test Passed\n";
    } else {
        std::cout << "Test Failed (numeric mismatch vs CPU reference)\n";
    }

    return all_pass ? 0 : 1;
}
