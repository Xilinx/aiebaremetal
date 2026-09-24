// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <fstream>
#include <xaiengine.h>
#include "../../../../include/aiebaremetal.h"
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
Dct gradf;
BaremetalGraphSim gr("gradf");
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xtime_l.h"
BaremetalGraph gr("gradf");
#endif

#define NBYTES_DATA 4

#define WIDTH 16
#define HEIGHT 16
#define SIZE (WIDTH * HEIGHT)
#define BLK_DIM 8
#define BLK_SIZE (BLK_DIM * BLK_DIM)

////////////////////////////////////////////////////////////////////////////////
//// Hardcoded unrolled fast 8-point (i)DCT
//////////////////////////////////////////////////////////////////////////////////
#define C_a 1.3870398453221474618216191915664f     // a = sqrt(2) * cos(1 * pi / 16)
#define C_b 1.3065629648763765278566431734272f     // b = sqrt(2) * cos(2 * pi / 16)
#define C_c 1.1758756024193587169744671046113f     // c = sqrt(2) * cos(3 * pi / 16)
#define C_d 0.78569495838710218127789736765722f    // d = sqrt(2) * cos(5 * pi / 16)
#define C_e 0.54119610014619698439972320536639f    // e = sqrt(2) * cos(6 * pi / 16)
#define C_f 0.27589937928294301233595756366937f    // f = sqrt(2) * cos(7 * pi / 16)
#define C_norm 0.35355339059327376220042218105242f // 1 / sqrt(8)

void DCT8(float *dst, float *src, int ostride, int istride) {
    float X07P = src[0 * istride] + src[7 * istride];
    float X16P = src[1 * istride] + src[6 * istride];
    float X25P = src[2 * istride] + src[5 * istride];
    float X34P = src[3 * istride] + src[4 * istride];

    float X07M = src[0 * istride] - src[7 * istride];
    float X61M = src[6 * istride] - src[1 * istride];
    float X25M = src[2 * istride] - src[5 * istride];
    float X43M = src[4 * istride] - src[3 * istride];

    float X07P34PP = X07P + X34P;
    float X07P34PM = X07P - X34P;
    float X16P25PP = X16P + X25P;
    float X16P25PM = X16P - X25P;

    dst[0 * ostride] = C_norm * (X07P34PP + X16P25PP);
    dst[2 * ostride] = C_norm * (C_b * X07P34PM + C_e * X16P25PM);
    dst[4 * ostride] = C_norm * (X07P34PP - X16P25PP);
    dst[6 * ostride] = C_norm * (C_e * X07P34PM - C_b * X16P25PM);

    dst[1 * ostride] = C_norm * (C_a * X07M - C_c * X61M + C_d * X25M - C_f * X43M);
    dst[3 * ostride] = C_norm * (C_c * X07M + C_f * X61M - C_a * X25M + C_d * X43M);
    dst[5 * ostride] = C_norm * (C_d * X07M + C_a * X61M + C_f * X25M - C_c * X43M);
    dst[7 * ostride] = C_norm * (C_f * X07M + C_d * X61M + C_c * X25M + C_a * X43M);
}

int main(int argc, char **argv) {
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
    gradf.init();
#else
    Xil_DCacheDisable();
    Xil_ICacheDisable();

    XTime tStart, tEnd;

    XTime_GetTime(&tStart);
#endif

    gr.init();

    gr.run(1);
    gr.wait();
    return 0;
}