/**
* Copyright (C) 2024 Advanced Micro Devices, Inc.
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
*/
#ifndef __ABR_CONFIGURE__
#define __ABR_CONFIGURE__
#include <iostream>
#include <unordered_map>
//#include "hw_config.h"
#include <xaiengine.h>
#include <cstring>
#ifdef __AIESIM__
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#endif
#include <sstream>

#include "aiebaremetal.h"

#include "common_layer/abr_api_config.h"
#include "common_layer/abr_runtime_api.h"
#include "gmioapiimpl.h"
#include "externalbufferapiimpl.h"

extern const char _binary_build_driverconfig_bin_start[];
extern const char _binary_build_graphconfig_bin_start[];
extern const char _binary_build_gmiosconfig_bin_start[];
extern const char _binary_build_pliosconfig_bin_start[];
extern const char _binary_build_externalbuffersconfig_bin_start[];
extern const char _binary_build_rtpsconfig_bin_start[];

//#ifdef NO_JSON
extern unsigned int _binary_build_driverconfig_bin_size;
extern unsigned int _binary_build_graphconfig_bin_size;
extern unsigned int _binary_build_gmiosconfig_bin_size;
extern unsigned int _binary_build_pliosconfig_bin_size;
extern unsigned int _binary_build_externalbuffersconfig_bin_size;
extern unsigned int _binary_build_rtpsconfig_bin_size;
/*
#else
	extern const char _binary_build_driverconfig_bin_end[];
	extern const char _binary_build_graphconfig_bin_end[];
	extern const char _binary_build_gmiosconfig_bin_end[];
	extern const char _binary_build_rtpsconfig_bin_end[];
#endif
*/
extern const char _binary_aie_control_config_json_start[];
extern const char _binary_aie_control_config_json_end[];

static constexpr uint32_t default_start_column = 0;

//#if !defined(__AIESIM__) && !defined(__ADF_FRONTEND__)
class ConfigureParser {
public:
	ConfigureParser();
	~ConfigureParser();
		
	void ConfigureGmios(std::shared_ptr<XAieMem> xaiemem);
	void ConfigureExternalBuffers(std::shared_ptr<XAieMem> xaiemem);
	abr::driver_config& get_driver_config();
	abr::graph_config& get_graph_config(std::string graph_name);
	abr::plio_config* get_plio_config_logicname(std::string graph_name);
	abr::plio_config* get_plio_config(std::string plio_name);
	abr::gmio_config* get_gmio_config(std::string gmio_name);
	abr::external_buffer_config& get_externalbuffer_config(std::string externalbuffer_name);
	abr::rtp_config& get_rtp_config(std::string rtp_name);
	std::shared_ptr<GmioApiImpl> get_gmio_api_impl(std::string gmio_name);
	std::shared_ptr<ExternalBufferApiImpl> get_externalbuffer_api_impl(std::string externalbuffer_name);
	void printDriverConfig();
	void printGraphConfig();
	void printGMIOsConfig();
	void printPLIOsConfig();
	void printExternalbufferConfig();
	void printRTPsConfig();
private:
	void Load();
	template<typename T> T GetConfig(std::string name);
	void LoadDriverConfig();
	void LoadGraphConfig();
	void LoadGmios();
	void LoadPlios();
	void LoadExternalbuffers();
	void LoadRtps();

#ifdef __AIESIM__
	boost::property_tree::ptree pt;
#endif
	void throw_if_error(bool err, const char* msg);
	void printgraphconfig(const abr::graph_config& grconfig, int number);
	void printgmioconfig(const abr::gmio_config& gm_config, int number);
	void printplioconfig(const abr::plio_config& pl_config, int number);
	void printshimbdinfoconfig(const abr::shimBDInfo& shim_bd_info_config, int number, std::string prefix);
	void printshimportconfig(const abr::shim_port_config& shim_port_config, int number, std::string prefix);
	void printexternalbufferconfig(const abr::external_buffer_config& ebuffer_config, int number);
	void printrtpconfig(const abr::rtp_config& rp_config, int number);
	std::unordered_map<std::string, abr::gmio_config> gmioconfigs;
	std::unordered_map<std::string, abr::plio_config> plioconfigs;
	std::unordered_map<std::string, abr::external_buffer_config> externalbufferconfigs;
	std::unordered_map<std::string, abr::rtp_config> rtpconfigs;
	std::unordered_map<std::string, abr::graph_config> gconfigs;
	std::unordered_map<std::string, std::shared_ptr<GmioApiImpl>> gmio_api_impls;
	std::unordered_map<std::string, std::shared_ptr<ExternalBufferApiImpl>> ebuffer_api_impls;
	abr::driver_config dconfig;
	std::vector<abr::partition_config> partitions;
};
#endif

