/**<!--
 * Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may
 * not use this file except in compliance with the License. A copy of the
 * License is located at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 * -->
 */
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

#include "aiebaremetal.h"

#include "common_layer/aeg_api_config.h"
#include "common_layer/aeg_runtime_api.h"
#include "externalbufferapiimpl.h"
#include "gmioapiimpl.h"
#include "rtpapiimpl.h"
#include "sharedbufferapiimpl.h"

extern "C" {
extern const char binary_build_driverconfig_bin_start[];
extern const char binary_build_aiecompilerconfig_bin_start[];
extern const char binary_build_graphconfig_bin_start[];
extern const char binary_build_gmiosconfig_bin_start[];
extern const char binary_build_pliosconfig_bin_start[];
extern const char binary_build_externalbuffersconfig_bin_start[];
extern const char binary_build_sharedbuffersconfig_bin_start[];
extern const char binary_build_rtpsconfig_bin_start[];

extern std::uint32_t binary_build_driverconfig_bin_size;
extern std::uint32_t binary_build_aiecompilerconfig_bin_size;
extern std::uint32_t binary_build_graphconfig_bin_size;
extern std::uint32_t binary_build_gmiosconfig_bin_size;
extern std::uint32_t binary_build_pliosconfig_bin_size;
extern std::uint32_t binary_build_externalbuffersconfig_bin_size;
extern std::uint32_t binary_build_sharedbuffersconfig_bin_size;
extern std::uint32_t binary_build_rtpsconfig_bin_size;

// extern const char binary_aie_control_config_json_start[];
// extern const char binary_aie_control_config_json_end[];
}
namespace abr {

static constexpr std::uint32_t default_start_column{0U};
std::uint32_t get_partition_start_column(const std::vector<partition_config> &partitions, std::int32_t column);

} // namespace abr
namespace aeg_configureparser {
class ConfigureParser {
  public:
    ConfigureParser();
    ~ConfigureParser() = default;
    ConfigureParser(const ConfigureParser &) = delete;
    ConfigureParser(ConfigureParser &&) = delete;
    ConfigureParser &operator=(const ConfigureParser &) & = delete;
    ConfigureParser &operator=(ConfigureParser &&) & = delete;

    aeg_err ConfigureGmios(std::shared_ptr<xaiemem::XAieMem> xaiemem);
    aeg_err ConfigureExternalBuffers(std::shared_ptr<xaiemem::XAieMem> xaiemem);
    aeg_err ConfigureRtps(std::shared_ptr<abr::graph_api> graphApi);
    aeg_err ConfigureSharedBuffers(std::shared_ptr<abr::graph_api> graphApi);
    abr::driver_config &get_driver_config() &;
    abr::aiecompiler_config &get_aiecompiler_config() &;
    abr::graph_config *get_graph_config(std::string_view graph_name);
    abr::plio_config *get_plio_config_logicname(std::string_view graph_name, bool silent = false);
    abr::plio_config *get_plio_config(std::string_view plio_name, bool silent = false);
    abr::gmio_config *get_gmio_config(std::string_view gmio_name, bool silent = false);
    abr::external_buffer_config *get_externalbuffer_config(std::string_view externalbuffer_name);
    abr::shared_buffer_config *get_shared_buffer_config(std::string_view shared_buffer_name);
    abr::rtp_config *get_rtp_config(std::string_view rtp_name);
    std::shared_ptr<gmioapiimpl::GmioApiImpl> get_gmio_api_impl(std::string_view gmio_name);
    std::shared_ptr<externalbufferapiimpl::ExternalBufferApiImpl> get_externalbuffer_api_impl(std::string_view externalbuffer_name);
    std::shared_ptr<rtpapiimpl::RtpApiImpl> get_rtp_api_impl(std::string_view rtp_name);
    std::shared_ptr<sharedbufferapiimpl::SharedBufferApiImpl> get_sharedbuffer_api_impl(std::string_view sharedbuffer_name);
    void printDriverConfig() const;
    void printGraphConfig() const;
    void printGMIOsConfig() const;
    void printPLIOsConfig() const;
    void printExternalbufferConfig() const;
    void printSharedBufferConfig() const;
    void printRTPsConfig() const;
    int32_t isRTPorSharedBuffer(const std::string_view name) const; // 0-RTP, 1- shared buffer
  private:
    void Load();
    template <typename T> T GetConfig(std::string_view name);
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
    void printgraphconfig(const abr::graph_config &grconfig, int32_t number) const;
    void printgmioconfig(const abr::gmio_config &gm_config, int32_t number) const;
    void printplioconfig(const abr::plio_config &pl_config, int32_t number) const;
    void printshimbdinfoconfig(const abr::shimBDInfo &shim_bd_info_config, int32_t number, const std::string &prefix) const;
    void printshimportconfig(const abr::shim_port_config &shim_port_config, int32_t number, const std::string &prefix) const;
    void printexternalbufferconfig(const abr::external_buffer_config &ebuffer_config, int32_t number) const;
    void printsharedbufferconfig(const abr::shared_buffer_config &sbuffer_config, int32_t number) const;
    void printrtpconfig(const abr::rtp_config &rp_config, int32_t number) const;
    std::unordered_map<std::string_view, abr::gmio_config> gmioconfigs;
    std::unordered_map<std::string_view, abr::plio_config> plioconfigs;
    std::unordered_map<std::string_view, abr::external_buffer_config> externalbufferconfigs;
    std::unordered_map<std::string_view, abr::shared_buffer_config> sharedbufferconfigs;
    std::unordered_map<std::string_view, abr::rtp_config> rtpconfigs;
    std::unordered_map<std::string_view, abr::graph_config> gconfigs;
    std::unordered_map<std::string_view, std::shared_ptr<gmioapiimpl::GmioApiImpl>> gmio_api_impls;
    std::unordered_map<std::string_view, std::shared_ptr<externalbufferapiimpl::ExternalBufferApiImpl>> ebuffer_api_impls;
    std::unordered_map<std::string_view, std::shared_ptr<rtpapiimpl::RtpApiImpl>> rtp_api_impls;
    std::unordered_map<std::string_view, std::shared_ptr<sharedbufferapiimpl::SharedBufferApiImpl>> sharedbuffer_api_impls;
    abr::driver_config dconfig;
    abr::aiecompiler_config aiecompilerconfig;
    std::vector<abr::partition_config> partitions;
};
} // namespace aeg_configureparser
#endif