// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "aeg_configure.h"
#include "aeg_log.h"
#include "xaiemem.h"

extern "C" {
extern volatile int g_ut_configure_getters_force_fail;
extern volatile int g_ut_gmio_configure_fail;
extern volatile int g_ut_gmio_configure_force_code;
extern volatile int g_ut_gmio_configure_force_null_argument;
extern volatile int g_ut_configure_external_buffers_force_code;
extern volatile int g_ut_configure_rtps_force_code;
extern volatile int g_ut_configure_shared_buffers_force_code;
}
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

std::shared_ptr<XAieMem> g_ut_xaie_after_gmios;
std::shared_ptr<XAieMem> g_ut_xaie_after_ebuffers;
std::shared_ptr<abr::graph_api> g_ut_graph_after_rtp_cfg;
std::shared_ptr<abr::graph_api> g_ut_graph_after_sb_cfg;

inline aeg_err map_err_code(abr::err_code code) {
    switch (code) {
    case abr::err_code::ok:
        return aeg_err::ok;
    case abr::err_code::user_error:
        return aeg_err::user_error;
    case abr::err_code::internal_error:
        return aeg_err::internal_error;
    case abr::err_code::aie_driver_error:
        return aeg_err::aie_driver_error;
    case abr::err_code::resource_unavailable:
        return aeg_err::resource_unavailable;
    default:
        return aeg_err::aie_driver_error;
    }
}

abr::shimBDInfo make_bd(uint32_t bd_id, uint32_t buf_idx, uint32_t offset, uint64_t transaction_size) {
    abr::shimBDInfo b{};
    b.bd_id = bd_id;
    b.buf_idx = buf_idx;
    b.offset = offset;
    b.transaction_size = transaction_size;
    return b;
}

abr::shim_port_config make_port(int32_t portId, std::string portName, int32_t s2mm_or_mm2s, int16_t column,
                                int16_t channel, int32_t task_rep, bool enable_tok,
                                std::vector<abr::shimBDInfo> bds) {
    abr::shim_port_config sp{};
    sp.portId = portId;
    sp.portName = std::move(portName);
    sp.S2MMOrMM2S = s2mm_or_mm2s;
    sp.column = column;
    sp.channel = channel;
    sp.taskRepetition = task_rep;
    sp.enable_task_complete_token = enable_tok;
    sp.shimBDInfos = std::move(bds);
    return sp;
}

abr::external_buffer_config make_eb(int16_t id, std::string name, std::vector<abr::shim_port_config> ports) {
    abr::external_buffer_config e{};
    e.id = id;
    e.name = std::move(name);
    e.shimPortConfigs = std::move(ports);
    return e;
}

void seed_maps(std::vector<abr::partition_config> &partitions,
               abr::driver_config &dconfig, abr::aiecompiler_config &aiecompilerconfig,
               std::unordered_map<std::string, abr::graph_config> &gconfigs,
               std::unordered_map<std::string, abr::gmio_config> &gmioconfigs,
               std::unordered_map<std::string, abr::plio_config> &plioconfigs,
               std::unordered_map<std::string, abr::external_buffer_config> &externalbufferconfigs,
               std::unordered_map<std::string, abr::shared_buffer_config> &sharedbufferconfigs,
               std::unordered_map<std::string, abr::rtp_config> &rtpconfigs) {
    partitions.clear();
    abr::partition_config pc{};
    pc.startColumn = 0;
    pc.numColumns = 10;
    pc.uuid = "test-uuid";
    pc.aie_pl_intf_id = 0;
    pc.name_space = "test_ns";
    partitions.push_back(pc);

    dconfig = {};
    dconfig.hw_gen = 1;
    dconfig.base_address = 2199023255552ULL;
    dconfig.column_shift = 25;
    dconfig.row_shift = 20;
    dconfig.num_columns = 38;
    dconfig.num_rows = 11;
    dconfig.shim_row = 0;
    dconfig.mem_row_start = 1;
    dconfig.mem_num_rows = 2;
    dconfig.aie_tile_row_start = 3;
    dconfig.aie_tile_num_rows = 8;
    dconfig.partition_num_cols = 0;
    dconfig.partition_overlay_start_cols = {0, 1, 2, 3};

    aiecompilerconfig.broadcast_enable_core = true;
    aiecompilerconfig.graph_iterator_event = false;
    aiecompilerconfig.enable_multi_layer = false;

    abr::graph_config gc{};
    gc.id = 0;
    gc.name = "gradf2";
    gc.coreColumns = {2};
    gc.coreRows = {0};
    gc.iterMemColumns = {2};
    gc.iterMemRows = {0};
    gc.iterMemAddrs = {4};
    gc.triggered = {true};
    gc.broadcast_column = 0;
    gconfigs = {{"gradf2", gc}};

    gmioconfigs.clear();
    abr::gmio_config g0{};
    g0.id = 0;
    g0.name = "gradf.in";
    g0.logicalName = "gmio1";
    g0.type = abr::gmio_config::gmio_type::gm2aie;
    g0.shimColumn = 3;
    g0.channelNum = 0;
    g0.streamId = 3;
    g0.burstLength = 4;
    gmioconfigs["gradf.in"] = g0;
    abr::gmio_config g1{};
    g1.id = 1;
    g1.name = "gradf.out";
    g1.logicalName = "gmio2";
    g1.type = abr::gmio_config::gmio_type::aie2gm;
    g1.shimColumn = 3;
    g1.channelNum = 0;
    g1.streamId = 2;
    g1.burstLength = 4;
    gmioconfigs["gradf.out"] = g1;

    plioconfigs.clear();
    abr::plio_config p0{};
    p0.id = 0;
    p0.name = "gradf2.pin";
    p0.logicalName = "plioin1";
    p0.shimColumn = 5;
    p0.slaveOrMaster = 0;
    p0.streamId = 4;
    plioconfigs["gradf2.pin"] = p0;
    abr::plio_config p1{};
    p1.id = 1;
    p1.name = "gradf2.pout";
    p1.logicalName = "plioout1";
    p1.shimColumn = 5;
    p1.slaveOrMaster = 1;
    p1.streamId = 3;
    plioconfigs["gradf2.pout"] = p1;

    abr::gmio_config gAmb{};
    gAmb.id = 99;
    gAmb.name = "ambiguous_port";
    gAmb.logicalName = "ambiguous_logical";
    gAmb.type = abr::gmio_config::gmio_type::gm2aie;
    gAmb.shimColumn = 4;
    gAmb.channelNum = 0;
    gAmb.streamId = 1;
    gAmb.burstLength = 4;
    gmioconfigs["ambiguous_port"] = gAmb;
    abr::plio_config pAmb{};
    pAmb.id = 99;
    pAmb.name = "ambiguous_port";
    pAmb.logicalName = "ambiguous_logical";
    pAmb.shimColumn = 4;
    pAmb.slaveOrMaster = 0;
    pAmb.streamId = 1;
    plioconfigs["ambiguous_port"] = pAmb;

    externalbufferconfigs.clear();
    externalbufferconfigs["EG.ddrin"] =
        make_eb(0, "EG.ddrin",
                {make_port(6, "EG.ddrin.out[0]", 1, 15, 0, 1, false,
                           {make_bd(0, 0, 0, 108)})});
    externalbufferconfigs["EG.ddrout"] =
        make_eb(1, "EG.ddrout",
                {make_port(7, "EG.ddrout.in[0]", 0, 22, 0, 1, false,
                           {make_bd(0, 0, 0, 36)})});
    externalbufferconfigs["EG.weights_1"] =
        make_eb(2, "EG.weights_1",
                {make_port(12, "EG.weights_1.out[0]", 1, 15, 1, 1, false,
                           {make_bd(1, 0, 0, 216)})});
    externalbufferconfigs["EG.weights_2"] =
        make_eb(3, "EG.weights_2",
                {make_port(13, "EG.weights_2.out[0]", 1, 22, 0, 1, false,
                           {make_bd(1, 0, 0, 72)})});

    sharedbufferconfigs.clear();
    abr::shared_buffer_config s0{};
    s0.id = 0;
    s0.name = "EG.input";
    s0.graphId = 0;
    s0.numBytes = 108;
    s0.column = 15;
    s0.row = 0;
    s0.numInputs = 0;
    s0.numOutputs = 2;
    s0.elemType = abr::rtp_config::elementType::et_voidty;
    s0.initialized = false;
    s0.addr = {0};
    s0.producerLocks = {0};
    s0.consumerLocks = {1};
    sharedbufferconfigs["EG.input"] = s0;
    abr::shared_buffer_config s1{};
    s1.id = 1;
    s1.name = "EG.output";
    s1.graphId = 0;
    s1.numBytes = 72;
    s1.column = 19;
    s1.row = 1;
    s1.numInputs = 1;
    s1.numOutputs = 2;
    s1.elemType = abr::rtp_config::elementType::et_voidty;
    s1.initialized = false;
    s1.addr = {0};
    s1.producerLocks = {0};
    s1.consumerLocks = {1};
    sharedbufferconfigs["EG.output"] = s1;

    rtpconfigs.clear();
    {
        abr::rtp_config r{};
        r.portId = 4;
        r.aliasId = -1;
        r.portName = "gradf.sq.in[1]";
        r.aliasName = "";
        r.graphId = 0;
        r.isInput = false;
        r.isAsync = true;
        r.isConnect = false;
        r.numBytes = 4;
        r.isPL = false;
        r.hasLock = true;
        r.selectorColumn = 1;
        r.selectorRow = 0;
        r.selectorAddr = 0;
        r.selectorLockId = 0;
        r.pingColumn = 2;
        r.pingRow = 1;
        r.pingAddr = 32736;
        r.pingLockId = 0;
        r.pongColumn = 2;
        r.pongRow = 1;
        r.pongAddr = 16352;
        r.pongLockId = 1;
        rtpconfigs["gradf.sq.in[1]"] = r;
    }
}

}

extern "C" void ut_stub_reset_configure_parser_lazy_state(void) {
    g_ut_xaie_after_gmios.reset();
    g_ut_xaie_after_ebuffers.reset();
    g_ut_graph_after_rtp_cfg.reset();
    g_ut_graph_after_sb_cfg.reset();
}

ConfigureParser::ConfigureParser() {
    Load();
}

abr::driver_config &ConfigureParser::get_driver_config() & noexcept { return dconfig; }

abr::aiecompiler_config &ConfigureParser::get_aiecompiler_config() & noexcept { return aiecompilerconfig; }

abr::graph_config *ConfigureParser::get_graph_config(std::string graph_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        AEG_ERROR("%s: UT forced failure", __func__);
        return nullptr;
    }
    const auto it = gconfigs.find(graph_name);
    if (it != gconfigs.end()) {
        return &it->second;
    }
    AEG_ERROR("%s: graph '%s' does not exist", __func__, graph_name.c_str());
    return nullptr;
}

abr::gmio_config *ConfigureParser::get_gmio_config(std::string gmio_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return nullptr;
    }
    const auto it = gmioconfigs.find(gmio_name);
    if (it != gmioconfigs.end()) {
        return &it->second;
    }
    return nullptr;
}

abr::plio_config *ConfigureParser::get_plio_config_logicname(std::string graph_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return nullptr;
    }
    for (auto &e : plioconfigs) {
        if (e.second.logicalName == graph_name) {
            return &e.second;
        }
    }
    return nullptr;
}

abr::plio_config *ConfigureParser::get_plio_config(std::string plio_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return nullptr;
    }
    const auto it = plioconfigs.find(plio_name);
    if (it != plioconfigs.end()) {
        return &it->second;
    }
    return nullptr;
}

abr::external_buffer_config *ConfigureParser::get_externalbuffer_config(std::string externalbuffer_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return nullptr;
    }
    const auto it = externalbufferconfigs.find(externalbuffer_name);
    if (it != externalbufferconfigs.end()) {
        return &it->second;
    }
    return nullptr;
}

abr::shared_buffer_config *ConfigureParser::get_shared_buffer_config(std::string shared_buffer_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return nullptr;
    }
    const auto it = sharedbufferconfigs.find(shared_buffer_name);
    if (it != sharedbufferconfigs.end()) {
        return &it->second;
    }
    return nullptr;
}

abr::rtp_config *ConfigureParser::get_rtp_config(std::string rtp_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return nullptr;
    }
    const auto it = rtpconfigs.find(rtp_name);
    if (it != rtpconfigs.end()) {
        return &it->second;
    }
    return nullptr;
}

int32_t ConfigureParser::isRTPorSharedBuffer(const std::string name) const noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return -1;
    }
    if (sharedbufferconfigs.find(name) != sharedbufferconfigs.end()) {
        return 1;
    }
    if (rtpconfigs.find(name) != rtpconfigs.end()) {
        return 0;
    }
    return -1;
}

std::shared_ptr<GmioApiImpl> ConfigureParser::get_gmio_api_impl(std::string gmio_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return nullptr;
    }
    const auto hit = gmio_api_impls.find(gmio_name);
    if (hit != gmio_api_impls.end() && hit->second) {
        return hit->second;
    }
    const auto ci = gmioconfigs.find(gmio_name);
    if (ci == gmioconfigs.end() || !g_ut_xaie_after_gmios) {
        return nullptr;
    }
    auto gapi = std::make_shared<abr::gmio_api>(&ci->second);
    if (map_err_code(gapi->configure()) != aeg_err::ok) {
        return nullptr;
    }
    std::shared_ptr<GmioApiImpl> impl =
        std::make_shared<GmioApiImpl>(gapi, ci->second, g_ut_xaie_after_gmios, gmio_name);
    gmio_api_impls[gmio_name] = impl;
    return impl;
}

std::shared_ptr<ExternalBufferApiImpl> ConfigureParser::get_externalbuffer_api_impl(std::string externalbuffer_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return nullptr;
    }
    const auto hit = ebuffer_api_impls.find(externalbuffer_name);
    if (hit != ebuffer_api_impls.end() && hit->second) {
        return hit->second;
    }
    const auto ci = externalbufferconfigs.find(externalbuffer_name);
    if (ci == externalbufferconfigs.end() || !g_ut_xaie_after_ebuffers) {
        return nullptr;
    }
    std::shared_ptr<ExternalBufferApiImpl> impl =
        std::make_shared<ExternalBufferApiImpl>(ci->second, externalbuffer_name, g_ut_xaie_after_ebuffers);
    ebuffer_api_impls[externalbuffer_name] = impl;
    return impl;
}

std::shared_ptr<RtpApiImpl> ConfigureParser::get_rtp_api_impl(std::string rtp_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return nullptr;
    }
    const auto hit = rtp_api_impls.find(rtp_name);
    if (hit != rtp_api_impls.end() && hit->second) {
        return hit->second;
    }
    const auto ci = rtpconfigs.find(rtp_name);
    if (ci == rtpconfigs.end() || !g_ut_graph_after_rtp_cfg) {
        return nullptr;
    }
    std::shared_ptr<RtpApiImpl> impl =
        std::make_shared<RtpApiImpl>(ci->second, g_ut_graph_after_rtp_cfg, rtp_name);
    rtp_api_impls[rtp_name] = impl;
    return impl;
}

std::shared_ptr<SharedBufferApiImpl> ConfigureParser::get_sharedbuffer_api_impl(std::string sharedbuffer_name) noexcept {
    if (g_ut_configure_getters_force_fail != 0) {
        return nullptr;
    }
    const auto hit = sharedbuffer_api_impls.find(sharedbuffer_name);
    if (hit != sharedbuffer_api_impls.end() && hit->second) {
        return hit->second;
    }
    const auto ci = sharedbufferconfigs.find(sharedbuffer_name);
    if (ci == sharedbufferconfigs.end() || !g_ut_graph_after_sb_cfg) {
        return nullptr;
    }
    std::shared_ptr<SharedBufferApiImpl> impl =
        std::make_shared<SharedBufferApiImpl>(ci->second, g_ut_graph_after_sb_cfg, sharedbuffer_name);
    sharedbuffer_api_impls[sharedbuffer_name] = impl;
    return impl;
}

void ConfigureParser::Load() {
    seed_maps(partitions, dconfig, aiecompilerconfig, gconfigs, gmioconfigs, plioconfigs, externalbufferconfigs,
              sharedbufferconfigs, rtpconfigs);
}

template <typename T> T ConfigureParser::GetConfig(std::string name) {
#ifdef __AIESIM__
    return pt.get<T>(name);
#else
    static_cast<void>(name);
    T ret{};
    return ret;
#endif
}
aeg_err ConfigureParser::ConfigureGmios(std::shared_ptr<XAieMem> xaiemem) noexcept {
    if (g_ut_gmio_configure_force_null_argument != 0) {
        return aeg_err::null_argument;
    }
    if (!xaiemem) {
        return aeg_err::null_argument;
    }
    if (g_ut_gmio_configure_force_code != 0) {
        return map_err_code(static_cast<abr::err_code>(g_ut_gmio_configure_force_code));
    }
    if (g_ut_gmio_configure_fail != 0) {
        return aeg_err::aie_driver_error;
    }
    g_ut_xaie_after_gmios = std::move(xaiemem);
    return aeg_err::ok;
}

aeg_err ConfigureParser::ConfigureExternalBuffers(std::shared_ptr<XAieMem> xaiemem) noexcept {
    if (!xaiemem) {
        return aeg_err::null_argument;
    }
    if (g_ut_configure_external_buffers_force_code != 0) {
        return map_err_code(static_cast<abr::err_code>(g_ut_configure_external_buffers_force_code));
    }
    g_ut_xaie_after_ebuffers = std::move(xaiemem);
    return aeg_err::ok;
}

aeg_err ConfigureParser::ConfigureRtps(std::shared_ptr<abr::graph_api> graphApi) noexcept {
    if (!graphApi) {
        return aeg_err::null_argument;
    }
    if (g_ut_configure_rtps_force_code != 0) {
        return map_err_code(static_cast<abr::err_code>(g_ut_configure_rtps_force_code));
    }
    g_ut_graph_after_rtp_cfg = std::move(graphApi);
    return aeg_err::ok;
}

aeg_err ConfigureParser::ConfigureSharedBuffers(std::shared_ptr<abr::graph_api> graphApi) noexcept {
    if (!graphApi) {
        return aeg_err::null_argument;
    }
    if (g_ut_configure_shared_buffers_force_code != 0) {
        return map_err_code(static_cast<abr::err_code>(g_ut_configure_shared_buffers_force_code));
    }
    g_ut_graph_after_sb_cfg = std::move(graphApi);
    return aeg_err::ok;
}
