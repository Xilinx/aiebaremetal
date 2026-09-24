// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#if defined(__AEG_METADATA_ONLY__)
#include "graph.h"
myGraph gradf2;
int main(int argc, char **argv) {
    gradf2.init();
    return 0;
}
#else
#include "aiebaremetal.h"
#include "common.h"
#include "log/klogr.h"
#include <fstream>
#include <xaiengine.h>
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf2;
#else
#include "xil_cache.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xparameters.h"

#if __AIE_ARCH__ != 22
#include "xtime_l.h"
#else
#include "xiltimer.h"
#endif
#endif

AEGGraph<aeg::gradf2> gr;

#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
void adf_run() {
    int32_t *inputArray = (int32_t *)GMIO::malloc(256 * sizeof(int32_t));
    int32_t *outputArray = (int32_t *)GMIO::malloc(256 * sizeof(int32_t));

    for (int i = 0; i < 256; i++)
        inputArray[i] = i + 1;

    std::ofstream ofs;

    gradf2.run(1);
    std::cout << "before end" << std::endl;
    gradf2.end();

    GMIO::free(inputArray);
    GMIO::free(outputArray);
}
#endif
//            AIE (vck190) / AIE-ML (vek280)
// MM2S_BASE: 0xA4050000   / 0xA4010000
// S2MM_BASE: 0xA4060000   / 0XA4020000

#if __AIE_ARCH__ != 22
#define MM2S_BASE XPAR_XMM2S_0_S_AXI_CONTROL_BASEADDR
#define S2MM_BASE XPAR_XS2MM_0_S_AXI_CONTROL_BASEADDR
#else
// For Vek385
#define MM2S_BASE XPAR_XMM2S_0_BASEADDR
#define S2MM_BASE XPAR_XS2MM_0_BASEADDR
#endif

#define CTRL_OFFSET 0x0
#define SIZE_OFFSET 0x1C
#define MEM_OFFSET 0x10
void start_pl(uint32_t *in, uint32_t *out, int len) {
#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)

    int i;
    int errCount = 0;
    uint64_t memAddr = (uint64_t)in;
    uint64_t mem_outAddr = (uint64_t)out;

    printf("Starting test w/ cu\n");
    printf("Starting mm2s & s2mm - %x\n  ", MM2S_BASE);
    Xil_Out32(MM2S_BASE + MEM_OFFSET, (uint32_t)memAddr);
    u32 reg_val;
    reg_val = Xil_In32(MM2S_BASE + MEM_OFFSET);
    printf(" reg_val %x, memAddr %x \n", reg_val, (uint32_t)memAddr);

    Xil_Out32(MM2S_BASE + MEM_OFFSET + 4, 0);
    Xil_Out32(S2MM_BASE + MEM_OFFSET, (uint32_t)mem_outAddr);
    Xil_Out32(S2MM_BASE + MEM_OFFSET + 4, 0);
    Xil_Out32(MM2S_BASE + SIZE_OFFSET, len);
    Xil_Out32(S2MM_BASE + SIZE_OFFSET, len);
    Xil_Out32(MM2S_BASE + CTRL_OFFSET, 1);
    Xil_Out32(S2MM_BASE + CTRL_OFFSET, 1);
#endif
}

void wait_s2mm() {
#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)
    while (1) {
        uint32_t v = Xil_In32(S2MM_BASE + CTRL_OFFSET);
        if (v & 6) {
            break;
        }
        // printf("wait_s2mm\n");
    }
#endif
}

int main(int argc, char **argv) {
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
    std::cout << "gradf2.a  =" << gradf2.a << std::endl;
    gradf2.init();
#else
    Xil_DCacheDisable();
    Xil_ICacheDisable();
    XTime tStart, tEnd;
#endif
    uint32_t indata[SQUARE_BUF_LEN], outdata[SQUARE_BUF_LEN];
    bool test_passed = true;

    for (int i = 0; i < SQUARE_BUF_LEN; i++) {
        indata[i] = i;
        outdata[i] = 0;
    }
    // PROFILE_ID handle0 = gr.start_profiling("gradf2.pin", io_total_stream_running_to_idle_cycles);
    PROFILE_ID handle0 = gr.pin.start_profiling(io_total_stream_running_to_idle_cycles);
    start_pl(indata, outdata, SQUARE_BUF_LEN);

    // g_start_plio = 1;
    printf("main-1216----------****** handle0 = %d \n", handle0);
    gr.init();
    printf("main------before run-----******\n");
    gr.run(1);

    wait_s2mm();
    printf("after run-----------******\n");
    gr.wait();
    char *log = gr.dumpcorelog(3, 2);
    printf("kernel log is %s\n", log);

#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)
    int error_count = 0;
    int mismatch_count = 0;

    std::cout << "\n=== Starting Output Validation ===" << std::endl;
    std::cout << "Checking that output = input^2 (squared values)" << std::endl;

    for (int i = 0; i < 16 && i < SQUARE_BUF_LEN; i++) {
        printf("outdata[%d] is %d (expected: %d)\n", i, outdata[i], indata[i] * indata[i]);
    }

    for (int i = 0; i < SQUARE_BUF_LEN; i++) {
        int32_t expected = indata[i] * indata[i]; // i * i

        if (outdata[i] != expected) {
            mismatch_count++;
            if (mismatch_count <= 10) {
                printf("ERROR: outdata[%d] = %d, expected %d (input was %d)\n", i, outdata[i], expected, indata[i]);
            }
        }
    }

    std::cout << "\n=== Validation Results ===" << std::endl;
    std::cout << "Total elements checked: " << SQUARE_BUF_LEN << std::endl;
    std::cout << "Elements with incorrect values: " << mismatch_count << std::endl;

    if (mismatch_count > 0) {
        test_passed = false;
        error_count++;
        std::cout << "FAIL: " << mismatch_count << " elements have incorrect squared values" << std::endl;
    } else {
        std::cout << "PASS: All " << SQUARE_BUF_LEN << " elements have correct squared values" << std::endl;
    }

    int nonzero_count = 0;
    for (int i = 1; i < SQUARE_BUF_LEN; i++) {
        if (outdata[i] != 0) {
            nonzero_count++;
        }
    }

    if (nonzero_count == 0) {
        test_passed = false;
        error_count++;
        std::cout << "FAIL: All non-zero input values produced zero output" << std::endl;
    } else {
        std::cout << "PASS: " << nonzero_count << " non-zero outputs detected" << std::endl;
    }

    std::cout << "\nLast 4 values:" << std::endl;
    for (int i = SQUARE_BUF_LEN - 4; i < SQUARE_BUF_LEN; i++) {
        printf("outdata[%d] = %d (expected: %d)\n", i, outdata[i], indata[i] * indata[i]);
    }

    std::cout << "\n=== Test Result (new api) ===" << std::endl;
    if (test_passed) {
        std::cout << "SUCCESS: Square Kernel Test PASSED" << std::endl;
    } else {
        std::cout << "FAILURE: Square Kernel Test FAILED with " << error_count << " validation error(s)" << std::endl;
    }
#endif

    gr.end();
    if (handle0 != -1) {
        long long cycle_count0 = gr.read_profiling(handle0);
        gr.stop_profiling(handle0);
        printf("cycle_count0 = %lld\n", cycle_count0);
    }

#if not defined(__AIESIM__) && not defined(__ADF_FRONTEND__)
    if (test_passed) {
        return 0;
    } else {
        return 1;
    }
#else
    return 0;
#endif
}
#endif /* !__AEG_METADATA_ONLY__ */
