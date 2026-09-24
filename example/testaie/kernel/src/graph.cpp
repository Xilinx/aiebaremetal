// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#include <fstream>
#include <vector>
#include <xaiengine.h>

#include "aiebaremetal.h"
#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)
#include "graph.h"
myGraph gradf;
BaremetalGraphSim gr("gradf");
#include "app.h"
#else
BaremetalGraph gr("gradf");
#endif

int num_err_backtracked = 0;

void graph_error_handling_cb(std::vector<XAie_ErrorPayload> payloads, void *priv) {
    num_err_backtracked += payloads.size();

    for (XAie_ErrorPayload pl : payloads) {
        printf("CB: [%d, %d]: Mod: %d Error: %d\n", pl.Loc.Col, pl.Loc.Row, static_cast<int>(pl.Module), pl.EventId);
    }
}

int main(int argc, char **argv) {
    setbuf(stdout, NULL); // Disable buffering
    // trigger the ps_ix.so load and the elf load
    gr.init();
    gr.error_handling_cb_init((void *)graph_error_handling_cb, (void *)&gr);
    gr.selftestexit();

#if (defined(__AIESIM__) || defined(__ADF_FRONTEND__))
    gr.BacktrackErrors();
#endif
    if (num_err_backtracked == 0) {
        printf("Backtracking Errors failed\n");
        return -1;
    } else {
        printf("Total errors backtracked: %d\n", num_err_backtracked);
    }
    return 0;
}
