// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
Mm gradf;
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"

#if __AIE_ARCH__ == 22
#include "xiltimer.h"
#else
#include "xtime_l.h"
#endif
#endif

#include "aiebaremetal.h"
#include <fstream>
#include <xaiengine.h>

AbrGraph gr("gradf");

#define SAMPLES 256
#define NBYTES_DATA 4

#define WIDTH 8
#define HEIGHT 8
#define SIZE (WIDTH * HEIGHT)

int main(int argc, char **argv) {
    const int input_size_in_bytes = SIZE * sizeof(float);
    const int output_size_in_bytes = SIZE * sizeof(float);

#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    gradf.init();
#endif
    gr.init();

    float increment[1] = {1};
    char *inVect = reinterpret_cast<char *>(increment);
    gr.update("gradf.mm0.in[2]", inVect, sizeof(float));

    auto out_bomapped = reinterpret_cast<float *>(gr.malloc(output_size_in_bytes));
    memset(out_bomapped, 0, output_size_in_bytes);

    auto in_bomappedA = reinterpret_cast<float *>(gr.malloc(input_size_in_bytes));
    auto in_bomappedB = reinterpret_cast<float *>(gr.malloc(input_size_in_bytes));

    // setting input data
    float *golden = (float *)malloc(output_size_in_bytes);
    // std::ofstream fout("data/inputSource.txt");

    for (int i = 0; i < SIZE; i++) {
        in_bomappedA[i] = rand() % SIZE;
        // in_bomappedB[i] = rand() % SIZE;
        in_bomappedB[i] = in_bomappedA[i];
        // std::fout<<in_bomappedA[i]<<std::endl;
    }
    // fout.close();

    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            golden[i * WIDTH + j] = 0;
            for (int k = 0; k < WIDTH; k++)
                golden[i * WIDTH + j] += in_bomappedA[i * WIDTH + k] * in_bomappedB[k + HEIGHT * j];
        }
    }

    gr.run(1);
    // std::ifstream fin("aiesimulator_output/data/OutputSink.txt");
    //  for(int i = 0; i < SIZE; i++){
    //      fin>>out_bomapped[i];
    //  }
    // fin.close();
    gr.gm2aie_nb("gradf.inB", reinterpret_cast<char *>(in_bomappedB), SIZE * sizeof(float));
    std::cout << "syncAIEBO B \n";

    gr.gm2aie_nb("gradf.inA", reinterpret_cast<char *>(in_bomappedA), SIZE * sizeof(float));
    std::cout << "syncAIEBO A \n";

    gr.aie2gm_nb("gradf.out", reinterpret_cast<char *>(out_bomapped), SIZE * sizeof(float));
    gr.gmio_wait("gradf.out");
    gr.wait();
    int match = 0;
    for (int i = 0; i < SIZE; i++) {
        if (out_bomapped[i] != golden[i]) {
            printf("ERROR: Test failed! Error found in sample %d: golden: %f, hardware: %f\n", i, golden[i],
                   out_bomapped[i]);
            match = 1;
            break;
        }
    }

    float increment_out[1] = {1};
    char *outVect = reinterpret_cast<char *>(increment_out);
    gr.read("gradf.mm0.inout[0]", outVect, sizeof(float));
    if (increment_out[0] != 2) {
        printf("ERROR: ReadRTP may failed! %f\n", increment_out[0]);
        match = 1;
    }

    std::cout << "Releasing remaining  objects...\n";
    gr.end();
    gr.free(in_bomappedA);
    gr.free(in_bomappedB);
    gr.free(out_bomapped);

    std::cout << "TEST " << (match ? "FAILED" : "PASSED") << std::endl;
    return (match ? EXIT_FAILURE : EXIT_SUCCESS);
}