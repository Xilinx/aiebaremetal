// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
#include "graph.h"
mygraph EG;
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

AbrGraph gr("EG");

int main() {
#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    EG.init();
#else
#ifdef _RPU_
    // RPU need to disable data cache, without cache diable app will stuck
    Xil_DCacheDisable();
// Xil_ICacheDisable();
#endif
#endif

    uint32_t *inputArray = (uint32_t *)gr.malloc(128 * sizeof(uint32_t));
    uint32_t *outputArray = (uint32_t *)gr.malloc(128 * sizeof(uint32_t));
    for (int i = 0; i < 128; i++) {
        inputArray[i] = i + 1;
        outputArray[i] = 0;
    }

    gr.init();

    gr.setAddress("EG.in1", inputArray);
    gr.setAddress("EG.out1", outputArray);

    gr.gm2aie_nb("EG.in1", "EG.in1.out[0]");
    gr.aie2gm_nb("EG.out1", "EG.out1.in[0]");

    uint32_t *ping = (uint32_t *)gr.malloc(32 * sizeof(uint32_t));
    uint32_t *pong = (uint32_t *)gr.malloc(32 * sizeof(uint32_t));

    for (int i = 0; i < 32; i++) {
        ping[i] = 0;
        pong[i] = 0;
    }

    gr.setupPingPongBuffers("EG.ext", ping, pong);

    gr.run(1);
    gr.wait("EG.out1", "EG.out1.in[0]");
    gr.wait();
    gr.end();

    for (int j = 0; j < 128; j++)
        std::cout << outputArray[j] << std::endl;

    gr.free(inputArray);
    gr.free(outputArray);
    gr.free(ping);
    gr.free(pong);
    std::cout << "the test passed!!!" << std::endl;
    return 0;
}
