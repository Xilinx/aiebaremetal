// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "aiebaremetal.h"
#include "aeg_platform.h"
#include "config.h"
// Runtime CDO load is off by default: vck190_boot_image.bif already lists the
// same Work/ps/cdo/*.bin files in a `type=cdo` image, so the PLM applies them
// from the boot PDI. Applying them a second time from here is not idempotent --
// aie_cdo_init.bin pushes BD 0 onto the tile DMA start queues (0x1DE04/0x1DE14),
// which is a FIFO push, not a state write.
// To drive the config from the app instead, define AEG_HAVE_CDO_HEADER *and*
// drop the `type=cdo` image from the .bif so the config is applied exactly once.
// The load must stay ahead of gr.init() -- see the comment at the load site.
#if defined(__has_include)
    #if __has_include("cdoheader.h")
    #include "cdoheader.h"
    #define AEG_HAVE_CDO_HEADER 1
    #endif
#endif
#include <fstream>
#include <vector>
#include <string>
#include <xaiengine.h>

#if defined(__ADF_FRONTEND__)
    #include "graph.h"
    myGraph gradf;
#endif

AbrGraph gr("gradf");

int main(int argc, char **argv) {
#if defined(AEG_HARDWARE) && defined(_RPU_)
    Xil_DCacheDisable();
    Xil_ICacheDisable();
#endif


    // AEG_HAVE_CDO_HEADER selects who configures the AIE array:
    //  - defined:     this app drives it, by handing the CDO blobs embedded in
    //                 cdoheader.h to init(). init() applies them before it
    //                 configures GMIOs/RTPs/buffers, which is required -- see
    //                 AIEGraph::init(). Drop the `type=cdo` image from the .bif
    //                 so the config is applied exactly once.
    //  - not defined: the PLM already applied the CDOs from the boot PDI and
    //                 init() just does the runtime configuration.
#ifdef AEG_HAVE_CDO_HEADER
    std::vector<aeg::cdo_blob> cdos;
    cdos.reserve(cdo_blobs_count);
    for (size_t i = 0; i < cdo_blobs_count; ++i) {
        cdos.push_back(aeg::cdo_blob{cdo_blobs[i].data, cdo_blobs[i].size});
    }
    if (gr.init(cdos.data(), cdos.size()) != aeg::aeg_err::ok) {
        std::cout << "init (with CDO load) failed\n";
        return -1;
    }
#else
    if (gr.init() != aeg::aeg_err::ok) {
        std::cout << "init failed\n";
        return -1;
    }
#endif

    gr.getaiedevinst();
    PROFILE_ID handle0 = gr.start_profiling("gradf.in", io_total_stream_running_to_idle_cycles);

    XTime tStart, tEnd;
    XTime_GetTime(&tStart);

    int32_t *inputArray     = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    int32_t *outputArray    = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));
    int32_t *outputArray_gold = (int32_t *)gr.malloc(DATA_LENGTH * sizeof(int32_t));

    gr.run(1);

    for (int i = 0; i < DATA_LENGTH; i++) {
        inputArray[i]        = i + 1;
        outputArray_gold[i]  = inputArray[i] * inputArray[i];
    }

    gr.gm2aie_nb("gradf.in",  reinterpret_cast<char *>(&inputArray[0]),  DATA_LENGTH * sizeof(int32_t));
    gr.aie2gm_nb("gradf.out", reinterpret_cast<char *>(&outputArray[0]), DATA_LENGTH * sizeof(int32_t));
    gr.gmio_wait("gradf.out");

    gr.wait();

    XTime_GetTime(&tEnd);
    printf("Output took %llu clock cycles.\n", 2 * (tEnd - tStart));
    printf("Output 2 took %.2f us.\n", 1.0 * (tEnd - tStart) / (COUNTS_PER_SECOND / 1000000));

    gr.end();
    if (handle0 != -1) {
        long long cycle_count0 = gr.read_profiling(handle0);
        gr.stop_profiling(handle0);
        printf("cycle_count0 = %lld\n", cycle_count0);
    }

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
