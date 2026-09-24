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
#ifndef __ABR_CONFIGURE__
#define __ABR_CONFIGURE__
#include <iostream>
#include <unordered_map>
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
void aie_driver_test(XAie_DevInst *);
extern const char _binary_build_driverconfig_bin_start[];
extern const char _binary_build_aiecompilerconfig_bin_start[];
extern const char _binary_build_graphconfig_bin_start[];
extern const char _binary_build_gmiosconfig_bin_start[];
extern const char _binary_build_pliosconfig_bin_start[];
extern const char _binary_build_externalbuffersconfig_bin_start[];
extern const char _binary_build_sharedbuffersconfig_bin_start[];
extern const char _binary_build_rtpsconfig_bin_start[];

extern unsigned int _binary_build_driverconfig_bin_size;
extern unsigned int _binary_build_aiecompilerconfig_bin_size;
extern unsigned int _binary_build_graphconfig_bin_size;
extern unsigned int _binary_build_gmiosconfig_bin_size;
extern unsigned int _binary_build_pliosconfig_bin_size;
extern unsigned int _binary_build_externalbuffersconfig_bin_size;
extern unsigned int _binary_build_sharedbuffersconfig_bin_size;
extern unsigned int _binary_build_rtpsconfig_bin_size;

extern const char _binary_aie_control_config_json_start[];
extern const char _binary_aie_control_config_json_end[];

static constexpr uint32_t default_start_column = 0;

uint32_t get_partition_start_column(std::vector<abr::partition_config> &partitions, int column);
class ConfigureParser {
  public:
    ConfigureParser();
    ~ConfigureParser();

    aeg_err ConfigureGmios(std::shared_ptr<XAieMem> xaiemem);
    aeg_err ConfigureExternalBuffers(std::shared_ptr<XAieMem> xaiemem);
    aeg_err ConfigureRtps(std::shared_ptr<abr::graph_api> graphApi);
    aeg_err ConfigureSharedBuffers(std::shared_ptr<abr::graph_api> graphApi);
    abr::driver_config &get_driver_config();
    abr::aiecompiler_config &get_aiecompiler_config();
    abr::graph_config *get_graph_config(std::string graph_name);
    abr::plio_config *get_plio_config_logicname(std::string graph_name);
    abr::plio_config *get_plio_config(std::string plio_name);
    abr::gmio_config *get_gmio_config(std::string gmio_name);
    abr::external_buffer_config *get_externalbuffer_config(std::string externalbuffer_name);
    abr::shared_buffer_config *get_shared_buffer_config(std::string shared_buffer_name);
    abr::rtp_config *get_rtp_config(std::string rtp_name);
    std::shared_ptr<GmioApiImpl> get_gmio_api_impl(std::string gmio_name);
    std::shared_ptr<ExternalBufferApiImpl> get_externalbuffer_api_impl(std::string externalbuffer_name);
    std::shared_ptr<RtpApiImpl> get_rtp_api_impl(std::string rtp_name);
    std::shared_ptr<SharedBufferApiImpl> get_sharedbuffer_api_impl(std::string sharedbuffer_name);
    void printDriverConfig();
    void printGraphConfig();
    void printGMIOsConfig();
    void printPLIOsConfig();
    void printExternalbufferConfig();
    void printSharedBufferConfig();
    void printRTPsConfig();
    int isRTPorSharedBuffer(std::string name); // 0-RTP, 1- shared buffer
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
    void printgraphconfig(const abr::graph_config &grconfig, int number);
    void printgmioconfig(const abr::gmio_config &gm_config, int number);
    void printplioconfig(const abr::plio_config &pl_config, int number);
    void printshimbdinfoconfig(const abr::shimBDInfo &shim_bd_info_config, int number, std::string prefix);
    void printshimportconfig(const abr::shim_port_config &shim_port_config, int number, std::string prefix);
    void printexternalbufferconfig(const abr::external_buffer_config &ebuffer_config, int number);
    void printsharedbufferconfig(const abr::shared_buffer_config &sbuffer_config, int number);
    void printrtpconfig(const abr::rtp_config &rp_config, int number);
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
