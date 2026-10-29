// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#if defined(__AEG_METADATA_ONLY__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf3;
#else
#include "aeg_baremetal_api_pl.h"
#include "common.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <xaiengine.h>
#include "aiebaremetal.h"
#if defined(AEG_HARDWARE)
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xparameters.h"

// XTime_GetTime moved to xiltimer.h in Python-flow BSPs, which drop xtime_l.h.
#if defined(__has_include) && __has_include("xtime_l.h")
#include "xtime_l.h"
#else
#include "xiltimer.h"
#endif
#endif

#if defined(__AIESIM__)
// aiecompiler recompiles this file (outside __ADF_FRONTEND__) to build the
// ps_i3 co-sim wrapper during `make compile`, before AEG_INHERITANCE_BASED_API
// (needed for AEGGraph<aeg::gradf3>/generated_graphs.h) is defined -- that
// only happens later for the real host-ELF build (script/builda72.sh). Use
// the untyped runtime API here; AEGGraph<T> is just this class + a graph name.
BaremetalGraph gr("gradf3");
#else
AEGGraph<aeg::gradf3> gr;
#endif

//                    AIE (vck190) / AIE-ML (vek280)
// MM2S_BASE / SOFTMAX_PL_BASE / S2MM_BASE: device-generated, see xparameters.h

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
// Python-flow BSPs use system-device-tree names; XSCT ones kept the
// _S_AXI_CONTROL infix. Same registers, so detect instead of assuming.
#ifdef XPAR_XMM2S_0_BASEADDR
#define MM2S_BASE XPAR_XMM2S_0_BASEADDR
#define SOFTMAX_PL_BASE XPAR_XSOFTMAX_PL_0_BASEADDR
#define S2MM_BASE XPAR_XS2MM_0_BASEADDR
#else
#define MM2S_BASE XPAR_XMM2S_0_S_AXI_CONTROL_BASEADDR
#define SOFTMAX_PL_BASE XPAR_XSOFTMAX_PL_0_S_AXI_CONTROL_BASEADDR
#define S2MM_BASE XPAR_XS2MM_0_S_AXI_CONTROL_BASEADDR
#endif

// HLS s_axilite argument offsets, confirmed from the generated driver headers
// (pl_kernels/_x/.../drivers/{mm2s,softmax_pl,s2mm}_v1_0/src/x*_hw.h) after
// running pl_kernels/build.sh:
//   mm2s/s2mm:  void (int* mem, stream, int size) -> mem @ 0x10/0x14, size @ 0x1C
//   softmax_pl: void (stream, stream, int size)    -> no m_axi arg, so size
//               is the sole s_axilite scalar and lands at 0x10, not 0x1C.
#define MEM_OFFSET 0x10
#define MEM_SIZE_OFFSET 0x1C
#define SOFTMAX_SIZE_OFFSET 0x10

aeg::pl_kernel mm2s_kernel(MM2S_BASE);
aeg::pl_kernel softmax_kernel(SOFTMAX_PL_BASE);
aeg::pl_kernel s2mm_kernel(S2MM_BASE);
#endif

void start_pl(uint32_t *in, uint32_t *out, int len) {
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    mm2s_kernel.set_arg(MEM_OFFSET, in);
    mm2s_kernel.set_arg(MEM_SIZE_OFFSET, len);
    softmax_kernel.set_arg(SOFTMAX_SIZE_OFFSET, len);
    s2mm_kernel.set_arg(MEM_OFFSET, out);
    s2mm_kernel.set_arg(MEM_SIZE_OFFSET, len);
    mm2s_kernel.start();
    softmax_kernel.start();
    s2mm_kernel.start();
#else
    (void)in;
    (void)out;
    (void)len;
#endif
}

void wait_s2mm() {
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    s2mm_kernel.wait();
#endif
}

int main(int argc, char **argv) {
#if defined(AEG_HARDWARE)
    Xil_DCacheDisable();
    Xil_ICacheDisable();
#endif
    static float indata[TOTAL_LEN];
    static float outdata[TOTAL_LEN];
    for (int i = 0; i < TOTAL_LEN; i++) {
        indata[i] = (i + 1) / 500.0f;
        outdata[i] = 0.0f;
    }

    gr.init();
    gr.run(NUM_TILES);

    // Chain: DDR -> [mm2s PL] -> [AIE passthrough] -> [softmax_pl PL] ->
    // [s2mm PL] -> DDR, as a single one-shot invocation over the whole
    // TOTAL_LEN buffer (not one start()/wait() per tile). softmax_pl.cpp
    // still computes NUM_TILES independent TILE_LEN-element softmaxes
    // internally, so results stay numerically comparable to aie_only.
    //
    // Timed window covers the same logical operation as aie_only's timed
    // window (transfer all TOTAL_LEN elements through the graph + PL
    // kernels and wait for completion) for a directly comparable
    // end-to-end latency number.
#if defined(AEG_HARDWARE)
    XTime tStart, tEnd;
    XTime_GetTime(&tStart);
#endif
    start_pl(reinterpret_cast<uint32_t *>(indata), reinterpret_cast<uint32_t *>(outdata), TOTAL_LEN);
    wait_s2mm();
    gr.wait();
#if defined(AEG_HARDWARE)
    XTime_GetTime(&tEnd);
    printf("[perf] pl_only: %d elements, %llu clock cycles.\n", TOTAL_LEN, 2 * (tEnd - tStart));
    printf("[perf] pl_only: %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));
#endif
    gr.end();

#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)
    // Correctness check runs entirely after gr.wait()/gr.end() above, so it
    // cannot perturb any future timing measurement. Reference matches
    // pl_kernels/softmax_pl.cpp and aie_only/src/softmax.cc exactly:
    // per-TILE_LEN-tile exp(x)/sum(exp(x)), no max-subtraction.
    bool all_pass = true;
    float max_abs_err = 0.0f;
    int worst_tile = -1;
    int worst_idx = -1;
    float gold_all[TOTAL_LEN];
    for (int t = 0; t < NUM_TILES; t++) {
        float gold[TILE_LEN];
        float sum = 0.0f;
        for (int i = 0; i < TILE_LEN; i++) {
            int idx = t * TILE_LEN + i;
            gold[i] = std::exp(indata[idx]);
            sum += gold[i];
        }
        for (int i = 0; i < TILE_LEN; i++) {
            int idx = t * TILE_LEN + i;
            gold[i] /= sum;
            gold_all[idx] = gold[i];
            float err = std::fabs(outdata[idx] - gold[i]);
            if (err > max_abs_err) {
                max_abs_err = err;
                worst_tile = t;
                worst_idx = i;
            }
            if (err > 1e-4f)
                all_pass = false;
        }
    }

    std::cout << "Max abs error vs CPU reference: " << max_abs_err << std::endl;
    if (!all_pass) {
        // Dump the worst tile in full so we can see the *shape* of the
        // mismatch (single bad element vs. off-by-one shift vs. uniform
        // offset).
        std::cout << "Worst tile=" << worst_tile << " worst_idx=" << worst_idx << std::endl;
        int t = worst_tile;
        for (int i = 0; i < TILE_LEN; i++) {
            int idx = t * TILE_LEN + i;
            std::cout << "  [" << i << "] in=" << indata[idx] << " out=" << outdata[idx] << " gold=" << gold_all[idx]
                       << " err=" << std::fabs(outdata[idx] - gold_all[idx]) << std::endl;
        }
    }
    if (all_pass) {
        std::cout << "Test Passed\n";
    } else {
        std::cout << "Test Failed (numeric mismatch vs CPU reference)\n";
    }
    return all_pass ? 0 : 1;
#else
    return 0;
#endif
}
#endif /* !__AEG_METADATA_ONLY__ && !__ADF_FRONTEND__ */
