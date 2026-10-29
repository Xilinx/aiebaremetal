// Copyright (C) 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include <cstdio>
#include <cstdlib>
#include <string>

#include <adf/adf_api/AIEControlConfig.h>

namespace adf {

bool reserveBroadcastchannelForEnablingCores() {
    return true;
}

void initializeConfigurations(
    void * /*pDevInst*/,
    size_t /*numColumns*/, size_t /*numAIERows*/, size_t /*numMemRows*/,
    const graph_config * /*pGraphConfig*/, size_t /*GraphConfigSize*/,
    const rtp_config * /*pRTPConfig*/, size_t /*RTPConfigSize*/,
    const kernel_config * /*pKernelConfig*/, size_t /*KernelConfigSize*/,
    const dma_config * /*pDMAConfig*/, size_t /*DMAConfigSize*/,
    const dma_channel_config * /*pDMAChConfig*/, size_t /*DMAChConfigSize*/,
    const dma_channel_bd_config * /*pDMAChBDConfig*/, size_t /*DMAChBDConfigSize*/,
    const shared_buffer_config * /*pSharedBufferConfig*/, size_t /*SharedBufferConfigSize*/,
    const external_buffer_config * /*pExternalBufferConfig*/, size_t /*ExternalBufferConfigSize*/,
    const gmio_config * /*pGMIOConfig*/, size_t /*GMIOConfigSize*/,
    const plio_config * /*pPLIOConfig*/, size_t /*PLIOConfigSize*/,
    const trace_unit_config * /*pTraceUnitConfig*/, size_t /*TraceUnitConfigSize*/,
    int /*traceAieSimOption*/, return_code (* /*traceGraphInitFunc*/)(),
    bool /*aiesim_debug*/, bool /*exclude_pl_control*/, bool /*enable_ecc_scrubbing*/,
    return_code (* /*broadcastConfigForCoreEnableFunc*/)(),
    bool /*broadcast_config_for_core_enable*/,
    int /*multiLayerUcAsmSim*/, bool /*multiLayerInitCoreElfCtrlPkt*/,
    unsigned /*adfApiLogLevel*/, int /*multiLayerOptLevel*/)
{}

void setWorkDirectory(std::string workDir) {
    setenv("AIE_WORK_DIR", workDir.c_str(), 0 /* do not override if already set */);
}

} // namespace adf

__attribute__((weak)) int broadcastConfigForCoreEnable() {
    return 0;
}
