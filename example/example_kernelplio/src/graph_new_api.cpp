// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#if defined(__AEG_METADATA_ONLY__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf2;
#else
#include "aeg_baremetal_api_pl.h"
#include "common.h"
#include "log/klogr.h"
#include <fstream>
#include <xaiengine.h>
#include "aiebaremetal.h"
#include "aeg_platform.h"

AEGGraph<aeg::gradf2> gr;

#if 0
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

#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#if __AIE_ARCH__ != 22
#define MM2S_BASE XPAR_XMM2S_0_S_AXI_CONTROL_BASEADDR
#define S2MM_BASE XPAR_XS2MM_0_S_AXI_CONTROL_BASEADDR
#else
// For Vek385
#define MM2S_BASE XPAR_XMM2S_0_BASEADDR
#define S2MM_BASE XPAR_XS2MM_0_BASEADDR
#endif

// HLS s_axilite argument offsets for the kernels in pl_kernels/{mm2s,s2mm}.cpp:
//   void mm2s(int* mem, int size); void s2mm(int* mem, int size);
// Layout per the HLS-generated driver header:
//   0x10  mem  (low 32 bits)   <- m_axi pointer arg
//   0x14  mem  (high 32 bits)
//   0x1C  size                 <- 32-bit scalar arg
#define MEM_OFFSET 0x10
#define SIZE_OFFSET 0x1C

aeg::pl_kernel mm2s_kernel(MM2S_BASE);
aeg::pl_kernel s2mm_kernel(S2MM_BASE);
#endif

void start_pl(uint32_t *in, uint32_t *out, int len) {
#if !(defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    printf("Starting test w/ cu\n");
    printf("Starting mm2s & s2mm - %x\n  ", MM2S_BASE);

    mm2s_kernel.set_arg(MEM_OFFSET, in);
    mm2s_kernel.set_arg(SIZE_OFFSET, len);
    s2mm_kernel.set_arg(MEM_OFFSET, out);
    s2mm_kernel.set_arg(SIZE_OFFSET, len);
    mm2s_kernel.start();
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
#if defined(__ADF_FRONTEND__)
    std::cout << "gradf2.a  =" << gradf2.a << std::endl;
    gradf2.init();
#elif defined(AEG_HARDWARE)
    Xil_DCacheDisable();
    Xil_ICacheDisable();
#endif
    XTime tStart, tEnd;
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

    if (handle0 != -1) {
        long long cycle_count0 = gr.read_profiling(handle0);
        gr.stop_profiling(handle0);
        printf("cycle_count0 = %lld\n", cycle_count0);
    }
    gr.end();

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
#endif /* !__AEG_METADATA_ONLY__ && !__ADF_FRONTEND__ */
