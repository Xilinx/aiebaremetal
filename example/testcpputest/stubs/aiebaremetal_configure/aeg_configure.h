// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0

#ifndef ABR_CONFIGURE_H
#define ABR_CONFIGURE_H
#include <iostream>
#include <unordered_map>
#include <cstdint>
#include <cstring>
#include <xaiengine.h>
#ifdef __AIESIM__
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#endif
#include <sstream>

#include "aiebaremetal.h"

#include "common_layer/aeg_api_config.h"
#include "common_layer/aeg_runtime_api.h"
#include "externalbufferapiimpl.h"
#include "gmioapiimpl.h"
#include "rtpapiimpl.h"
#include "sharedbufferapiimpl.h"

extern const char _binary_build_driverconfig_bin_start[];
extern const char _binary_build_aiecompilerconfig_bin_start[];
extern const char _binary_build_graphconfig_bin_start[];
extern const char _binary_build_gmiosconfig_bin_start[];
extern const char _binary_build_pliosconfig_bin_start[];
extern const char _binary_build_externalbuffersconfig_bin_start[];
extern const char _binary_build_sharedbuffersconfig_bin_start[];
extern const char _binary_build_rtpsconfig_bin_start[];

extern uint32_t _binary_build_driverconfig_bin_size;
extern uint32_t _binary_build_aiecompilerconfig_bin_size;
extern uint32_t _binary_build_graphconfig_bin_size;
extern uint32_t _binary_build_gmiosconfig_bin_size;
extern uint32_t _binary_build_pliosconfig_bin_size;
extern uint32_t _binary_build_externalbuffersconfig_bin_size;
extern uint32_t _binary_build_sharedbuffersconfig_bin_size;
extern uint32_t _binary_build_rtpsconfig_bin_size;

extern const char _binary_aie_control_config_json_start[];
extern const char _binary_aie_control_config_json_end[];

static constexpr uint32_t default_start_column{0U};
uint32_t get_partition_start_column(std::vector<abr::partition_config> &partitions, int32_t column) noexcept;

class ConfigureParser {
  public:
    ConfigureParser();
    ~ConfigureParser() = default;
    ConfigureParser(const ConfigureParser &) = delete;
    ConfigureParser(ConfigureParser &&) = delete;
    ConfigureParser &operator=(const ConfigureParser &) & = delete;
    ConfigureParser &operator=(ConfigureParser &&) & = delete;

    aeg_err ConfigureGmios(std::shared_ptr<XAieMem> xaiemem) noexcept;
    aeg_err ConfigureExternalBuffers(std::shared_ptr<XAieMem> xaiemem) noexcept;
    aeg_err ConfigureRtps(std::shared_ptr<abr::graph_api> graphApi) noexcept;
    aeg_err ConfigureSharedBuffers(std::shared_ptr<abr::graph_api> graphApi) noexcept;
    abr::driver_config &get_driver_config() & noexcept;
    abr::aiecompiler_config &get_aiecompiler_config() & noexcept;
    abr::graph_config *get_graph_config(std::string graph_name) noexcept;
    abr::plio_config *get_plio_config_logicname(std::string graph_name) noexcept;
    abr::plio_config *get_plio_config(std::string plio_name) noexcept;
    abr::gmio_config *get_gmio_config(std::string gmio_name) noexcept;
    abr::external_buffer_config *get_externalbuffer_config(std::string externalbuffer_name) noexcept;
    abr::shared_buffer_config *get_shared_buffer_config(std::string shared_buffer_name) noexcept;
    abr::rtp_config *get_rtp_config(std::string rtp_name) noexcept;
    std::shared_ptr<GmioApiImpl> get_gmio_api_impl(std::string gmio_name) noexcept;
    std::shared_ptr<ExternalBufferApiImpl> get_externalbuffer_api_impl(std::string externalbuffer_name) noexcept;
    std::shared_ptr<RtpApiImpl> get_rtp_api_impl(std::string rtp_name) noexcept;
    std::shared_ptr<SharedBufferApiImpl> get_sharedbuffer_api_impl(std::string sharedbuffer_name) noexcept;
    void printDriverConfig() const noexcept;
    void printGraphConfig() const noexcept;
    void printGMIOsConfig() const noexcept;
    void printPLIOsConfig() const noexcept;
    void printExternalbufferConfig() const noexcept;
    void printSharedBufferConfig() const noexcept;
    void printRTPsConfig() const noexcept;
    int32_t isRTPorSharedBuffer(const std::string name) const noexcept;
  private:
    void Load();
    template <typename T> T GetConfig(std::string name);
    void LoadDriverConfig();
    void LoadAiecompilerConfig();
    void LoadGraphConfig();
    void LoadGmios();
    void LoadPlios();
    void LoadExternalbuffers();
    void LoadSharedBuffers();
    void LoadRtps();

#ifdef __AIESIM__
    boost::property_tree::ptree pt;
#endif
    void throw_if_error(bool err, const char *msg);
    void printgraphconfig(const abr::graph_config &grconfig, int32_t number) const;
    void printgmioconfig(const abr::gmio_config &gm_config, int32_t number) const;
    void printplioconfig(const abr::plio_config &pl_config, int32_t number) const;
    void printshimbdinfoconfig(const abr::shimBDInfo &shim_bd_info_config, int32_t number, const std::string &prefix) const;
    void printshimportconfig(const abr::shim_port_config &shim_port_config, int32_t number, const std::string &prefix) const;
    void printexternalbufferconfig(const abr::external_buffer_config &ebuffer_config, int32_t number) const;
    void printsharedbufferconfig(const abr::shared_buffer_config &sbuffer_config, int32_t number) const;
    void printrtpconfig(const abr::rtp_config &rp_config, int32_t number) const;
    std::unordered_map<std::string, abr::gmio_config> gmioconfigs;
    std::unordered_map<std::string, abr::plio_config> plioconfigs;
    std::unordered_map<std::string, abr::external_buffer_config> externalbufferconfigs;
    std::unordered_map<std::string, abr::shared_buffer_config> sharedbufferconfigs;
    std::unordered_map<std::string, abr::rtp_config> rtpconfigs;
    std::unordered_map<std::string, abr::graph_config> gconfigs;
    std::unordered_map<std::string, std::shared_ptr<GmioApiImpl>> gmio_api_impls;
    std::unordered_map<std::string, std::shared_ptr<ExternalBufferApiImpl>> ebuffer_api_impls;
    std::unordered_map<std::string, std::shared_ptr<RtpApiImpl>> rtp_api_impls;
    std::unordered_map<std::string, std::shared_ptr<SharedBufferApiImpl>> sharedbuffer_api_impls;
    abr::driver_config dconfig;
    abr::aiecompiler_config aiecompilerconfig;
    std::vector<abr::partition_config> partitions;
};
#endif
