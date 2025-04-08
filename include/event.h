/**
* Copyright (C) 2025 Advanced Micro Devices, Inc.
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

#ifndef __EVENT_H__
#define __EVENT_H__
#include <unistd.h>
#include "abr_api_config.h"
#include "abr_profiling_api.h"
#include "abr_configure.h"
///*
enum _options_enum_init
{
  _io_profiling_option_enum_init = 0x20
};
enum _io_profiling_option
{
  _io_total_stream_running_to_idle_cycles = io_profiling_option_enum_init, /// Total accumulated cycles in between stream running to stream idle events
  _io_stream_start_to_bytes_transferred_cycles,                            /// Cycles in between the first event of stream running to the event that the specified number of bytes are transferred (assuming the stream stops right after the specified number of bytes are transferred)
  _io_stream_start_difference_cycles,                                      /// Cycles elapsed between the first stream running events of the two platform IO objects
  _io_stream_running_event_count                                           /// Number of stream running events
};
//*/
namespace abr
{
class event {
private:
    struct event_record {
      int option;
      std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquired_resources;
    };
    std::vector<event_record> event_records;
    ConfigureParser*  cfg;
    

    abr::shim_config get_shim_config(const std::string& port_name)
    {
      const abr::gmio_config * gmio = (const abr::gmio_config *)cfg->get_gmio_config(port_name);
      // For PLIO inside graph, there is no name property.
      // So we need to match logical name too
     
      auto plio = (const abr::plio_config *)cfg->get_plio_config(port_name);
      if (plio == nullptr) {
        plio = (const abr::plio_config *)cfg->get_plio_config_logicname(port_name);
      }
      

      if (gmio == nullptr && plio == nullptr)
        std::cout<< "Can't start profiling: port name '" << port_name << "' not found"<<std::endl;

      if (gmio != nullptr && plio != nullptr)
        std::cout << "Can't start profiling: ambiguous port name '"<< port_name  <<"'"<<std::endl;
      
      if (gmio != nullptr) {
        return abr::shim_config(gmio);
      } else {
        return abr::shim_config(plio);
      }
  }

  int start_profiling_run_idle(const std::string& port_name)
  {
    int handle = -1;
    std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquired_resources;
    if (abr::profiling::profile_stream_running_to_idle_cycles(get_shim_config(port_name), acquired_resources) == abr::err_code::ok)
    {
      handle = event_records.size();
      event_records.push_back({ _io_total_stream_running_to_idle_cycles, acquired_resources });
    }
    return handle;
  }

  int start_profiling_start_bytes(const std::string& port_name, uint32_t value)
  {
    int handle = -1;
    std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquired_resources;
    if (abr::profiling::profile_stream_start_to_transfer_complete_cycles(get_shim_config(port_name), value, acquired_resources) == abr::err_code::ok)
    {
      handle = event_records.size();
      event_records.push_back({ _io_stream_start_to_bytes_transferred_cycles, acquired_resources });
    }
    return handle;
  }

  int start_profiling_diff_cycles(const std::string& port1_name, const std::string& port2_name)
  {
    int handle = -1;
    std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquired_resources;
    if (abr::profiling::profile_start_time_difference_btw_two_streams(get_shim_config(port1_name), get_shim_config(port2_name), acquired_resources) == abr::err_code::ok)
    {
      handle = event_records.size();
      event_records.push_back({ _io_stream_start_difference_cycles, acquired_resources });
    }
    return handle;
  }

  int start_profiling_event_count(const std::string& port_name)
  {
    int handle = -1;
    std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquired_resources;
    if (abr::profiling::profile_stream_running_event_count(get_shim_config(port_name), acquired_resources) == abr::err_code::ok)
    {
      handle = event_records.size();
      event_records.push_back({ _io_stream_running_event_count, acquired_resources });
    }
    return handle;
  }

public:
  event(ConfigureParser* cfg): cfg(cfg) {}
  int start_profiling(const std::string& port_name, int option, uint32_t value = 0)
  {
    if (option == _io_stream_start_difference_cycles){
      printf("Start profiling fails: option requires two ports.\n");
      return -1;
    }
    return start_profiling(port_name, "", option, value);
  }
  int start_profiling(const std::string& port_name1,
															 	  const std::string& port_name2,
															    int option,
															    uint32_t value = 0) {
    switch (option) {
      case _io_total_stream_running_to_idle_cycles:
        return start_profiling_run_idle(port_name1);

      case _io_stream_start_to_bytes_transferred_cycles:
        return start_profiling_start_bytes(port_name1, value);

      case _io_stream_start_difference_cycles:
        return start_profiling_diff_cycles(port_name1, port_name2);

      case _io_stream_running_event_count:
        return start_profiling_event_count(port_name1);

      default:
        printf("Start profiling fails: unknown profiling option.\n");
      }
    return 0;

}
  long long read_profiling(int h) {
    uint64_t value = 0;
    if (event_records.size() > h)
      value = abr::profiling::read(event_records[h].acquired_resources, event_records[h].option == _io_stream_start_difference_cycles);
    else
      printf("Read profiling failed: invalid handle.\n");
    return value;
  }
  void stop_profiling(int h) {
    if (event_records.size() > h)
      abr::profiling::stop( event_records[h].acquired_resources);
    else
      printf( "Stop profiling failed: invalid handle.\n");
    return;
  }
};
};

#endif // __EVENT_H__
