// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef EVENT_H
#define EVENT_H
#include "aeg_api_config.h"
#include "aeg_configure.h"
#include "aeg_profiling_api.h"
#include "aeg_baremetal_api.h"
#include <cstdint>
#include <unistd.h>
#include "aeg_log.h"
namespace abr {
class event {
  private:
    struct event_record {
        int32_t option;
        std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquired_resources;
    };
    std::vector<event_record> event_records;
    aeg_configureparser::ConfigureParser *cfg;

    abr::shim_config get_shim_config(const std::string_view port_name) const {
        const abr::gmio_config *gmio = static_cast<const abr::gmio_config *>(cfg->get_gmio_config(port_name, true));
        abr::plio_config *plio = static_cast<abr::plio_config *>(cfg->get_plio_config(port_name, true));
        if (plio == nullptr) {
            plio = static_cast<abr::plio_config *>(cfg->get_plio_config_logicname(port_name, true));
        }

        if ((gmio == nullptr) && (plio == nullptr)) {
            AEG_ERROR("Can't start profiling: port name '%.*s' not found\n", static_cast<int32_t>(port_name.size()), port_name.data());
        }

        if ((gmio != nullptr) && (plio != nullptr)) {
            AEG_ERROR("Can't start profiling: ambiguous port name '%.*s'\n", static_cast<int32_t>(port_name.size()), port_name.data());
        }

        if (gmio != nullptr) {
            return abr::shim_config(gmio);
        }
        return abr::shim_config(plio);
    }

    int32_t start_profiling_run_idle(const std::string_view port_name) {
        int32_t handle = -1;
        std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquired_resources;
        if (abr::profiling::profile_stream_running_to_idle_cycles(get_shim_config(port_name), acquired_resources) ==
            abr::err_code::ok) {
            handle = static_cast<int32_t>(event_records.size());
            event_records.push_back(
                {static_cast<int32_t>(aeg::io_profiling_option::io_total_stream_running_to_idle_cycles), acquired_resources});
        }
        return handle;
    }

    int32_t start_profiling_start_bytes(const std::string_view port_name, uint32_t value) {
        int32_t handle = -1;
        std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquired_resources;
        if (abr::profiling::profile_stream_start_to_transfer_complete_cycles(get_shim_config(port_name), value,
                                                                             acquired_resources) == abr::err_code::ok) {
            handle = static_cast<int32_t>(event_records.size());
            event_records.push_back(
                {static_cast<int32_t>(aeg::io_profiling_option::io_stream_start_to_bytes_transferred_cycles), acquired_resources});
        }
        return handle;
    }

    int32_t start_profiling_diff_cycles(const std::string_view port1_name, const std::string_view port2_name) {
        int32_t handle = -1;
        std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquired_resources;
        const abr::shim_config shim_config1 = get_shim_config(port1_name);
        const abr::shim_config shim_config2 = get_shim_config(port2_name);
        if (abr::profiling::profile_start_time_difference_btw_two_streams(
                shim_config1, shim_config2, acquired_resources) == abr::err_code::ok) {
            handle = static_cast<int32_t>(event_records.size());
            event_records.push_back(
                {static_cast<int32_t>(aeg::io_profiling_option::io_stream_start_difference_cycles), acquired_resources});
        }
        return handle;
    }

    int32_t start_profiling_event_count(const std::string_view port_name) {
        int32_t handle = -1;
        std::vector<std::shared_ptr<xaiefal::XAieRsc>> acquired_resources;
        if (abr::profiling::profile_stream_running_event_count(get_shim_config(port_name), acquired_resources) ==
            abr::err_code::ok) {
            handle = static_cast<int32_t>(event_records.size());
            event_records.push_back(
                {static_cast<int32_t>(aeg::io_profiling_option::io_stream_running_event_count), acquired_resources});
        }
        return handle;
    }

  public:
    explicit event(aeg_configureparser::ConfigureParser *cfg) : cfg(cfg) {}
    int32_t start_profiling(const std::string_view port_name, int32_t option, uint32_t value = 0) {
        if (option == static_cast<int32_t>(aeg::io_profiling_option::io_stream_start_difference_cycles)) {
            AEG_ERROR("Start profiling fails: option requires two ports.\n");
            return -1;
        }
        return start_profiling(port_name, "", option, value);
    }
    int32_t start_profiling(const std::string_view port_name1, const std::string_view port_name2, int32_t option,
                                 std::uint32_t value = 0) {
        switch (option) {
        case static_cast<int32_t>(aeg::io_profiling_option::io_total_stream_running_to_idle_cycles):
        {
            return start_profiling_run_idle(port_name1);
        }
        case static_cast<int32_t>(aeg::io_profiling_option::io_stream_start_to_bytes_transferred_cycles):
        {
            return start_profiling_start_bytes(port_name1, value);
        }
        case static_cast<int32_t>(aeg::io_profiling_option::io_stream_start_difference_cycles):
        {
            return start_profiling_diff_cycles(port_name1, port_name2);
        }
        case static_cast<int32_t>(aeg::io_profiling_option::io_stream_running_event_count):
        {
            return start_profiling_event_count(port_name1);
        }
        default:
        {
            AEG_ERROR("Start profiling fails: unknown profiling option.\n");
            break;
        }
        }
        return 0;
    }
    uint64_t read_profiling(int32_t h) {
        uint64_t value = 0;
        if (event_records.size() > static_cast<size_t>(h)) {
            value = abr::profiling::read(event_records[static_cast<size_t>(h)].acquired_resources,
                                        event_records[static_cast<size_t>(h)].option == static_cast<int32_t>(aeg::io_profiling_option::io_stream_start_difference_cycles));
        } else {
            AEG_ERROR("Read profiling failed: invalid handle.\n");
            return 0;
        }
        return value;
    }
    void stop_profiling(int32_t h) {
        if (event_records.size() > static_cast<size_t>(h))
        {
            static_cast<void>(abr::profiling::stop(event_records[static_cast<size_t>(h)].acquired_resources));
        }
        else {
            AEG_ERROR("Stop profiling failed: invalid handle.\n");
        }
    }
};
}; // namespace abr

#endif // EVENT_H
