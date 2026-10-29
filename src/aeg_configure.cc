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
#include "aeg_configure.h"
#include "aeg_log.h"
#include "xaiemem.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string_view>
namespace {
class UnpackUtils {
  public:
    static void copy_bytes(const char *src, char *dst, std::size_t n) {
        std::size_t i = 0U;
        while (i < n) {
            dst[i] = src[i];
            ++i;
        }
    }

    static uint8_t unpackUInt8(const char *&data_ptr) {
        uint8_t value = 0U;
        copy_bytes(data_ptr, reinterpret_cast<char *>(&value), sizeof(value));
        data_ptr += sizeof(value);
        return value;
    }

    static std::string_view unpackString(const char *&data_ptr) {
        int32_t str_length = 0;
        static_cast<void>(std::copy_n(data_ptr, sizeof(int32_t), reinterpret_cast<char*>(&str_length)));
        data_ptr += sizeof(int32_t);
        std::string_view value(data_ptr, str_length);
        data_ptr += str_length;
        return value;
    }

    static uint64_t unpackUInt64(const char *&data_ptr) {
        uint64_t value = 0ULL;
        copy_bytes(data_ptr, reinterpret_cast<char *>(&value), sizeof(value));
        data_ptr += sizeof(value);
        return value;
    }

    static int32_t unpackUInt32(const char *&data_ptr) {
        uint32_t value = 0U;
        copy_bytes(data_ptr, reinterpret_cast<char *>(&value), sizeof(value));
        data_ptr += sizeof(value);
        return static_cast<int32_t>(value);
    }

    static int16_t unpackShort(const char *&data_ptr) {
        // unpacks as uint16
        uint16_t value = 0U;
        copy_bytes(data_ptr, reinterpret_cast<char *>(&value), sizeof(value));
        data_ptr += sizeof(value);
        return static_cast<int16_t>(value);
    }

    static bool unpackBool(const char *&data_ptr) {
        // bool unpacks as int8
        int8_t value = 0;
        copy_bytes(data_ptr, reinterpret_cast<char *>(&value), sizeof(value));
        data_ptr += sizeof(value);
        return (value != 0);
    }

    // Unpack a vector of a specified type and length
    template <typename T> static std::vector<T> unpackVector(size_t length, const char *&data_ptr) {
        std::vector<T> vec(length);
        for (std::size_t i = 0; i < length; ++i) {
            vec[i] = unpackValue<T>(data_ptr);
        }
        return vec;
    }

    // Function to unpack a value of a specific type
    template <typename T> static T unpackValue(const char *&data_ptr) {
        T value{};
        copy_bytes(data_ptr, reinterpret_cast<char *>(&value), sizeof(value));
        data_ptr += sizeof(value);
        return value;
    }
};

class DriverConfigStaticParser {
  public:
    abr::driver_config load(std::vector<abr::partition_config> &partitions) const {
        const char *bin_ptr = binary_build_driverconfig_bin_start;
        abr::driver_config dconfig{};

        dconfig.hw_gen = UnpackUtils::unpackUInt8(bin_ptr);
        dconfig.base_address = UnpackUtils::unpackUInt64(bin_ptr);
        dconfig.column_shift = UnpackUtils::unpackUInt8(bin_ptr);
        dconfig.row_shift = UnpackUtils::unpackUInt8(bin_ptr);
        dconfig.num_columns = UnpackUtils::unpackUInt8(bin_ptr);
        dconfig.num_rows = UnpackUtils::unpackUInt8(bin_ptr);
        dconfig.shim_row = UnpackUtils::unpackUInt8(bin_ptr);
        dconfig.mem_row_start = UnpackUtils::unpackUInt8(bin_ptr);
        dconfig.mem_num_rows = UnpackUtils::unpackUInt8(bin_ptr);
        dconfig.aie_tile_row_start = UnpackUtils::unpackUInt8(bin_ptr);
        dconfig.aie_tile_num_rows = UnpackUtils::unpackUInt8(bin_ptr);
        dconfig.partition_num_cols = UnpackUtils::unpackUInt8(bin_ptr);

        int32_t partition_overlay_start_cols_len = UnpackUtils::unpackUInt32(bin_ptr);
        dconfig.partition_overlay_start_cols =
            UnpackUtils::unpackVector<int16_t>(partition_overlay_start_cols_len, bin_ptr);

        // Check if partition information is available
        if (static_cast<std::uint32_t>(bin_ptr - binary_build_driverconfig_bin_start) < binary_build_driverconfig_bin_size) {
            static_cast<void>(UnpackUtils::unpackUInt32(bin_ptr)); /* start_column consumed */
            static_cast<void>(UnpackUtils::unpackUInt32(bin_ptr)); /* num_columns consumed */
            int32_t partition_len = UnpackUtils::unpackUInt32(bin_ptr);
            AEG_LOG("partition_len %d\n", partition_len);
            abr::partition_config pconfig{};
            int32_t j = 0;
            while (j < partition_len) {
                pconfig.startColumn = static_cast<std::uint32_t>(UnpackUtils::unpackUInt32(bin_ptr));
                pconfig.numColumns = static_cast<std::uint32_t>(UnpackUtils::unpackUInt32(bin_ptr));
                pconfig.uuid = UnpackUtils::unpackString(bin_ptr);
                pconfig.aie_pl_intf_id = static_cast<std::uint32_t>(UnpackUtils::unpackUInt32(bin_ptr));
                pconfig.name_space = UnpackUtils::unpackString(bin_ptr);
                partitions.push_back(pconfig);
                j++;
            }
        }
        return dconfig;
    }
};

} // namespace

namespace abr {

std::uint32_t get_partition_start_column(const std::vector<partition_config> &partitions, std::int32_t column) {
    if (partitions.empty()) {
        return default_start_column;
    }
    auto itr = std::find_if(partitions.begin(), partitions.end(), [column](const auto &part) {
        const std::uint32_t start_column = part.startColumn;
        const std::uint32_t num_columns = part.numColumns;
        return (start_column <= static_cast<std::uint32_t>(column)) &&
               (static_cast<std::uint32_t>(column) < (start_column + num_columns));
    });

    if (itr != partitions.end()) {
        return itr->startColumn;
    }
    return default_start_column;
}

} // namespace abr

namespace {
class AiecompilerConfigStaticParser {
  public:
    abr::aiecompiler_config load() const {
        const char *bin_ptr = binary_build_aiecompilerconfig_bin_start;
        abr::aiecompiler_config aiecompilerconfig{};

        aiecompilerconfig.broadcast_enable_core = UnpackUtils::unpackBool(bin_ptr);
        aiecompilerconfig.graph_iterator_event = UnpackUtils::unpackBool(bin_ptr);
        aiecompilerconfig.enable_multi_layer = UnpackUtils::unpackBool(bin_ptr);

        return aiecompilerconfig;
    }
};

class GraphConfigStaticParser {
  public:
    std::unordered_map<std::string_view, abr::graph_config> loadAll(const std::vector<abr::partition_config> &partitions,
                                                               int32_t partition_start_column) const {
        const char *bin_ptr = binary_build_graphconfig_bin_start;
        std::unordered_map<std::string_view, abr::graph_config> grconfigMap;
        uint32_t data_size = binary_build_graphconfig_bin_size;

        while (static_cast<uint32_t>(bin_ptr - binary_build_graphconfig_bin_start) < data_size) {
            abr::graph_config grconfig = load(bin_ptr, partitions, partition_start_column);
            grconfigMap[grconfig.name] = grconfig;
        }
        return grconfigMap;
    }

  private:
    abr::graph_config load(const char *&bin_ptr, const std::vector<abr::partition_config> &partitions,
                           const int32_t partition_start_column) const {
        abr::graph_config grconfig{};

        grconfig.id = static_cast<int32_t>(UnpackUtils::unpackUInt32(bin_ptr));
        grconfig.name = UnpackUtils::unpackString(bin_ptr);

        int32_t coreColumns_len = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.coreColumns = UnpackUtils::unpackVector<int16_t>(coreColumns_len, bin_ptr);

        int32_t coreRows_len = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.coreRows = UnpackUtils::unpackVector<int16_t>(coreRows_len, bin_ptr);

        int32_t iterMemColumns_len = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.iterMemColumns = UnpackUtils::unpackVector<int16_t>(iterMemColumns_len, bin_ptr);

        int32_t iterMemRows_len = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.iterMemRows = UnpackUtils::unpackVector<int16_t>(iterMemRows_len, bin_ptr);

        int32_t iterMemAddrs_len = UnpackUtils::unpackUInt32(bin_ptr);
        // itermemaddrs saved as unsigned int
        std::vector<uint32_t> iterMemAddrs_uint_vec =
            UnpackUtils::unpackVector<uint32_t>(iterMemAddrs_len, bin_ptr);
        grconfig.iterMemAddrs.reserve(iterMemAddrs_len);

        std::size_t iter_addr_idx = 0U;
        while (iter_addr_idx < iterMemAddrs_uint_vec.size()) {
            uint32_t value = iterMemAddrs_uint_vec[iter_addr_idx];
            grconfig.iterMemAddrs.push_back(static_cast<int32_t>(value));
            ++iter_addr_idx;
        }
        iterMemAddrs_uint_vec.clear();
        iterMemAddrs_uint_vec.shrink_to_fit();

        int32_t triggered_len = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.triggered = UnpackUtils::unpackVector<std::uint8_t>(triggered_len, bin_ptr);

        grconfig.broadcast_column = 0;
        if (!partitions.empty()) {
            grconfig.broadcast_column =
                abr::get_partition_start_column(partitions, static_cast<int32_t>(static_cast<int32_t>(grconfig.coreColumns[0]) + partition_start_column)) -
                static_cast<uint32_t>(partition_start_column);
        }

        return grconfig;
    }
};

class GMIOsConfigStaticParser {
  public:
    std::unordered_map<std::string_view, abr::gmio_config> loadAll() const {
        const char *bin_ptr = binary_build_gmiosconfig_bin_start;
        std::unordered_map<std::string_view, abr::gmio_config> gmconfigMap;

        uint32_t data_size = binary_build_gmiosconfig_bin_size;

        while (static_cast<uint32_t>(bin_ptr - binary_build_gmiosconfig_bin_start) < data_size) {
            abr::gmio_config gmconfig = load(bin_ptr);
            gmconfigMap[gmconfig.name] = gmconfig;
        }
        return gmconfigMap;
    }

  private:
    abr::gmio_config load(const char *&bin_ptr) const {
        abr::gmio_config gmconfig{};

        gmconfig.id = static_cast<int32_t>(UnpackUtils::unpackUInt32(bin_ptr));
        gmconfig.name = UnpackUtils::unpackString(bin_ptr);
        gmconfig.logicalName = UnpackUtils::unpackString(bin_ptr);

        int16_t gm_type = UnpackUtils::unpackShort(bin_ptr);
        gmconfig.type = static_cast<abr::gmio_config::gmio_type>(gm_type);

        gmconfig.shimColumn = UnpackUtils::unpackShort(bin_ptr);
        gmconfig.channelNum = UnpackUtils::unpackShort(bin_ptr);
        gmconfig.streamId = UnpackUtils::unpackShort(bin_ptr);
        gmconfig.burstLength = UnpackUtils::unpackShort(bin_ptr);

        return gmconfig;
    }
};

class PLIOsConfigStaticParser {
  public:
    std::unordered_map<std::string_view, abr::plio_config> loadAll() const {
        const char *bin_ptr = binary_build_pliosconfig_bin_start;
        std::unordered_map<std::string_view, abr::plio_config> plconfigMap;

        uint32_t data_size = binary_build_pliosconfig_bin_size;

        while (static_cast<std::uint32_t>(bin_ptr - binary_build_pliosconfig_bin_start) < data_size) {
            abr::plio_config plconfig = load(bin_ptr);
            plconfigMap[plconfig.name] = plconfig;
        }
        return plconfigMap;
    }

  private:
    abr::plio_config load(const char *&bin_ptr) const {
        abr::plio_config plconfig{};

        plconfig.id = static_cast<int32_t>(UnpackUtils::unpackUInt32(bin_ptr));
        plconfig.name = UnpackUtils::unpackString(bin_ptr);
        plconfig.logicalName = UnpackUtils::unpackString(bin_ptr);

        plconfig.shimColumn = UnpackUtils::unpackShort(bin_ptr);
        plconfig.slaveOrMaster = UnpackUtils::unpackShort(bin_ptr);
        plconfig.streamId = UnpackUtils::unpackShort(bin_ptr);

        return plconfig;
    }
};

class ExternalbuffersConfigStaticParser {
  public:
    std::unordered_map<std::string_view, abr::external_buffer_config> loadAll() const {
        const char *bin_ptr = binary_build_externalbuffersconfig_bin_start;
        std::unordered_map<std::string_view, abr::external_buffer_config> ebufferconfigMap;

        uint32_t data_size = binary_build_externalbuffersconfig_bin_size;
        AEG_LOG("binary_build_externalbuffersconfig_bin_start : %p\n",
                static_cast<const void *>(bin_ptr));
        AEG_LOG("binary_build_externalbuffersconfig_bin_size:  %u\n", binary_build_externalbuffersconfig_bin_size);
        AEG_LOG("data size  %u\n", data_size);
        while (static_cast<uint32_t>(bin_ptr - binary_build_externalbuffersconfig_bin_start) < data_size) {
            abr::external_buffer_config ebufferconfig = load(bin_ptr);
            ebufferconfigMap[ebufferconfig.name] = ebufferconfig;
            AEG_LOG("**************Static  parser External Buffer*********\n");
            AEG_LOG("ebufferconfig.name %.*s\n", static_cast<int32_t>(ebufferconfig.name.size()), ebufferconfig.name.data());
            AEG_LOG("read bytes %zu\n", static_cast<std::size_t>(bin_ptr - binary_build_externalbuffersconfig_bin_start));
        }
        return ebufferconfigMap;
    }

  private:
    abr::external_buffer_config load(const char *&bin_ptr) const {
        abr::external_buffer_config ebufferconfig{};

        ebufferconfig.id = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("ebufferconfig.id %d\n", static_cast<int32_t>(ebufferconfig.id));

        ebufferconfig.name = UnpackUtils::unpackString(bin_ptr);
        AEG_LOG("ebufferconfig.name %.*s\n", static_cast<int32_t>(ebufferconfig.name.size()), ebufferconfig.name.data());
        int32_t shim_port_config_len = UnpackUtils::unpackUInt32(bin_ptr);
        AEG_LOG("shim_port_config_len %d\n", shim_port_config_len);

        int32_t i = 0;
        while (i < shim_port_config_len) {
            abr::shim_port_config shimportconfig{};
            shimportconfig.portId = UnpackUtils::unpackUInt32(bin_ptr);
            shimportconfig.portName = UnpackUtils::unpackString(bin_ptr);
            shimportconfig.S2MMOrMM2S = UnpackUtils::unpackUInt32(bin_ptr);
            shimportconfig.column = UnpackUtils::unpackShort(bin_ptr);

            shimportconfig.channel = UnpackUtils::unpackShort(bin_ptr);
            shimportconfig.taskRepetition = UnpackUtils::unpackUInt32(bin_ptr);
            shimportconfig.enable_task_complete_token = UnpackUtils::unpackBool(bin_ptr);
            int32_t shim_bd_info_len = UnpackUtils::unpackUInt32(bin_ptr);
            int32_t j = 0;
            while (j < shim_bd_info_len) {
                abr::shimBDInfo shim_bd_info{};
                shim_bd_info.bd_id = static_cast<uint32_t>(UnpackUtils::unpackUInt32(bin_ptr));
                shim_bd_info.buf_idx = static_cast<uint32_t>(UnpackUtils::unpackUInt32(bin_ptr));
                shim_bd_info.offset = static_cast<uint32_t>(UnpackUtils::unpackUInt32(bin_ptr));
                shim_bd_info.transaction_size = UnpackUtils::unpackUInt64(bin_ptr);
                shimportconfig.shimBDInfos.push_back(shim_bd_info);
                j++;
            }
            ebufferconfig.shimPortConfigs.push_back(shimportconfig);
            i++;
        }

        return ebufferconfig;
    }
};

class SharedBuffersConfigStaticParser {
  public:
    std::unordered_map<std::string_view, abr::shared_buffer_config> loadAll() const {
        AEG_LOG("Load All Shared Buffers Config\n");
        const char *bin_ptr = binary_build_sharedbuffersconfig_bin_start;
        std::unordered_map<std::string_view, abr::shared_buffer_config> sbufferconfigMap;

        uint32_t data_size = binary_build_sharedbuffersconfig_bin_size;
        AEG_LOG("binary_build_sharedbuffersconfig_bin_start : %p\n",static_cast<const void *>(bin_ptr));
        AEG_LOG("binary_build_sharedbuffersconfig_bin_size:  %u\n", binary_build_sharedbuffersconfig_bin_size);
        AEG_LOG("data size  %u\n", data_size);
        while (static_cast<uint32_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start) < data_size) {
            abr::shared_buffer_config sbufferconfig = load(bin_ptr);
            sbufferconfigMap[sbufferconfig.name] = sbufferconfig;
            AEG_LOG("**************Static Parser Shared Buffer*********\n");
            AEG_LOG("sbufferconfig.name %.*s\n", static_cast<int32_t>(sbufferconfig.name.size()), sbufferconfig.name.data());
            AEG_LOG("read bytes %zu\n", static_cast<std::size_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));
        }
        return sbufferconfigMap;
    }

  private:
    abr::shared_buffer_config load(const char *&bin_ptr) const {
        AEG_LOG("Load Shared Buffer Config\n");
        abr::shared_buffer_config sbufferconfig{};

        sbufferconfig.id = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("sbufferconfig.id %d\n", static_cast<int32_t>(sbufferconfig.id));
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.name = UnpackUtils::unpackString(bin_ptr);
        AEG_LOG("sbufferconfig.name %.*s\n", static_cast<int32_t>(sbufferconfig.name.size()), sbufferconfig.name.data());
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.graphId = UnpackUtils::unpackUInt32(bin_ptr);
        AEG_LOG("sbufferconfig.graphId %u\n", sbufferconfig.graphId);
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.numBytes = UnpackUtils::unpackUInt64(bin_ptr);
        AEG_LOG("sbufferconfig.numBytes %llu\n",
                static_cast<std::uint64_t>(sbufferconfig.numBytes));
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.column = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("sbufferconfig.column %d\n", static_cast<int32_t>(sbufferconfig.column));
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.row = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("sbufferconfig.row %d\n", static_cast<int32_t>(sbufferconfig.row));
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.numInputs = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("sbufferconfig.numInputs %d\n", static_cast<int32_t>(sbufferconfig.numInputs));
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));
        sbufferconfig.numOutputs = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("sbufferconfig.numOutputs %d\n", static_cast<int32_t>(sbufferconfig.numOutputs));
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.initialized = UnpackUtils::unpackBool(bin_ptr);
        AEG_LOG("sbufferconfig.initialized %d\n",static_cast<int32_t>(sbufferconfig.initialized ? 1 : 0));
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        int32_t addr_len = UnpackUtils::unpackUInt32(bin_ptr);
        AEG_LOG("addr_len %d\n", addr_len);
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));
        sbufferconfig.addr = UnpackUtils::unpackVector<std::size_t>(addr_len, bin_ptr);
        AEG_LOG("sbufferconfig.addr ");
        std::size_t addr_idx = 0U;
        while (addr_idx < sbufferconfig.addr.size()) {
            const auto &addr = sbufferconfig.addr[addr_idx];
            static_cast<void>(addr);
            AEG_LOG("%zu ", addr);
            ++addr_idx;
        }
        AEG_LOG("\n");
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<std::size_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        int32_t producer_locks_len = UnpackUtils::unpackUInt32(bin_ptr);
        AEG_LOG("producer_locks_len %d\n", producer_locks_len);
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));
        sbufferconfig.producerLocks = UnpackUtils::unpackVector<int32_t>(producer_locks_len, bin_ptr);
        AEG_LOG("sbufferconfig.producerLocks ");
        std::size_t p_lock_idx = 0U;
        while (p_lock_idx < sbufferconfig.producerLocks.size()) {
            const auto &p_lock = sbufferconfig.producerLocks[p_lock_idx];
            static_cast<void>(p_lock);
            AEG_LOG("%d ", p_lock);
            ++p_lock_idx;
        }
        AEG_LOG("\n");
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        int32_t consumer_locks_len = UnpackUtils::unpackUInt32(bin_ptr);
        AEG_LOG("consumer_locks_len %d\n", consumer_locks_len);
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));
        sbufferconfig.consumerLocks = UnpackUtils::unpackVector<int32_t>(consumer_locks_len, bin_ptr);
        AEG_LOG("sbufferconfig.consumerLocks ");
        std::size_t c_lock_idx = 0U;
        while (c_lock_idx < sbufferconfig.consumerLocks.size()) {
            const auto &c_lock = sbufferconfig.consumerLocks[c_lock_idx];
            static_cast<void>(c_lock);
            AEG_LOG("%d ", c_lock);
            ++c_lock_idx;
        }
        AEG_LOG("\n");
        AEG_LOG("bin_ptr: 0x%lx\n", static_cast<uintptr_t>(bin_ptr - binary_build_sharedbuffersconfig_bin_start));

        return sbufferconfig;
    }
};

class RTPsConfigStaticParser {
  public:
    std::unordered_map<std::string_view, abr::rtp_config> loadAll() const {
        const char *bin_ptr = binary_build_rtpsconfig_bin_start;
        std::unordered_map<std::string_view, abr::rtp_config> rtpconfigMap;

        uint32_t data_size = binary_build_rtpsconfig_bin_size;

        while (static_cast<uint32_t>(bin_ptr - binary_build_rtpsconfig_bin_start) < data_size) {
            abr::rtp_config rtpconfig = load(bin_ptr);
            rtpconfigMap[rtpconfig.portName] = rtpconfig;
            if (!rtpconfig.aliasName.empty()) {
                rtpconfigMap[rtpconfig.aliasName] = rtpconfig;
            }
        }
        return rtpconfigMap;
    }

  private:
    abr::rtp_config load(const char *&bin_ptr) const {
        abr::rtp_config rtpconfig{};

        rtpconfig.portId = UnpackUtils::unpackUInt32(bin_ptr);
        rtpconfig.aliasId = UnpackUtils::unpackUInt32(bin_ptr);
        rtpconfig.portName = UnpackUtils::unpackString(bin_ptr);
        rtpconfig.aliasName = UnpackUtils::unpackString(bin_ptr);
        rtpconfig.graphId = UnpackUtils::unpackUInt32(bin_ptr);
        rtpconfig.isInput = UnpackUtils::unpackBool(bin_ptr);
        rtpconfig.isAsync = UnpackUtils::unpackBool(bin_ptr);
        rtpconfig.isConnect = UnpackUtils::unpackBool(bin_ptr);
        uint32_t num_bytes = static_cast<uint32_t>(UnpackUtils::unpackUInt32(bin_ptr));
        rtpconfig.numBytes = static_cast<size_t>(num_bytes);
        rtpconfig.isPL = UnpackUtils::unpackBool(bin_ptr);
        rtpconfig.hasLock = UnpackUtils::unpackBool(bin_ptr);
        rtpconfig.selectorColumn = UnpackUtils::unpackShort(bin_ptr);
        rtpconfig.selectorRow = UnpackUtils::unpackShort(bin_ptr);
        uint32_t selector_address = static_cast<uint32_t>(UnpackUtils::unpackUInt32(bin_ptr));
        rtpconfig.selectorAddr = static_cast<size_t>(selector_address);
        rtpconfig.selectorLockId = static_cast<uint16_t>(UnpackUtils::unpackShort(bin_ptr));
        rtpconfig.pingColumn = UnpackUtils::unpackShort(bin_ptr);
        rtpconfig.pingRow = UnpackUtils::unpackShort(bin_ptr);
        uint32_t ping_address = static_cast<uint32_t>(UnpackUtils::unpackUInt32(bin_ptr));
        rtpconfig.pingAddr = static_cast<size_t>(ping_address);
        rtpconfig.pingLockId = static_cast<uint16_t>(UnpackUtils::unpackShort(bin_ptr));
        rtpconfig.pongColumn = UnpackUtils::unpackShort(bin_ptr);
        rtpconfig.pongRow = UnpackUtils::unpackShort(bin_ptr);
        uint32_t pong_address = static_cast<uint32_t>(UnpackUtils::unpackUInt32(bin_ptr));
        rtpconfig.pongAddr = static_cast<size_t>(pong_address);
        rtpconfig.pongLockId = static_cast<uint16_t>(UnpackUtils::unpackShort(bin_ptr));

        return rtpconfig;
    }
};

} // namespace

void aeg_configureparser::ConfigureParser::printDriverConfig() const {
    AEG_LOG("Driver Config Starts: \n");
    AEG_LOG("aie_metadata.driver_config.hw_gen: %d\n", static_cast<int32_t>(dconfig.hw_gen));
    AEG_LOG("aie_metadata.driver_config.base_address: %llu\n", dconfig.base_address);
    AEG_LOG("aie_metadata.driver_config.column_shift: %d\n", static_cast<int32_t>(dconfig.column_shift));
    AEG_LOG("aie_metadata.driver_config.row_shift: %d\n", static_cast<int32_t>(dconfig.row_shift));
    AEG_LOG("aie_metadata.driver_config.num_columns: %d\n", static_cast<int32_t>(dconfig.num_columns));
    AEG_LOG("aie_metadata.driver_config.num_rows: %d\n", static_cast<int32_t>(dconfig.num_rows));
    AEG_LOG("aie_metadata.driver_config.shim_row: %d\n", static_cast<int32_t>(dconfig.shim_row));
    AEG_LOG("aie_metadata.driver_config.mem_row_start: %d\n", static_cast<int32_t>(dconfig.mem_row_start));
    AEG_LOG("aie_metadata.driver_config.mem_num_rows: %d\n", static_cast<int32_t>(dconfig.mem_num_rows));
    AEG_LOG("aie_metadata.driver_config.aie_tile_row_start: %d\n", static_cast<int32_t>(dconfig.aie_tile_row_start));
    AEG_LOG("aie_metadata.driver_config.aie_tile_num_rows: %d\n", static_cast<int32_t>(dconfig.aie_tile_num_rows));
    AEG_LOG("aie_metadata.driver_config.partition_num_cols: %d\n", static_cast<int32_t>(dconfig.partition_num_cols));

    // Print partitions information if available
    AEG_LOG("\n");
    AEG_LOG("Partitions.size() %zu\n", partitions.size());
    const size_t num_partitions = partitions.size();
    size_t i = 0;
    while (i < num_partitions) {
        AEG_LOG("Partitions[%zu].startColumn: %u\n", i, partitions[i].startColumn);
        AEG_LOG("Partitions[%zu].numColumns: %u\n", i, partitions[i].numColumns);
        AEG_LOG("Partitions[%zu].uuid: %.*s\n", i, static_cast<int32_t>(partitions[i].uuid.size()), partitions[i].uuid.data());
        AEG_LOG("Partitions[%zu].aie_pl_intf_id: %u\n", i, partitions[i].aie_pl_intf_id);
        AEG_LOG("Partitions[%zu].namespace: %.*s\n", i, static_cast<int32_t>(partitions[i].name_space.size()), partitions[i].name_space.data());
        i++;
    }

    AEG_LOG("\n");
    AEG_LOG("Driver Config Ends\n");
}

void aeg_configureparser::ConfigureParser::printGraphConfig() const {
    AEG_LOG("Graph Config Starts: \n");
    auto data_pair = gconfigs.begin();
    while (data_pair != gconfigs.end()) {
        const abr::graph_config &grconfig = data_pair->second;
        AEG_LOG("Graph ID %d values: \n", grconfig.id);
        printgraphconfig(grconfig, grconfig.id);
        ++data_pair;
    }
    AEG_LOG("Graph Config ends\n"); // Indicate the end of details
}

void aeg_configureparser::ConfigureParser::printGMIOsConfig() const {
    AEG_LOG("GMIOs Config Starts: \n");
    auto data_pair = gmioconfigs.begin();
    while (data_pair != gmioconfigs.end()) {
        const abr::gmio_config &gmconfig = data_pair->second;
        AEG_LOG("GMIO type %d values: \n", static_cast<int32_t>(gmconfig.type));
        printgmioconfig(gmconfig, gmconfig.id);
        ++data_pair;
    }
    AEG_LOG("GMIOs Config ends\n");
}

void aeg_configureparser::ConfigureParser::printPLIOsConfig() const {
    AEG_LOG("PLIOs Config Starts: \n");
    auto data_pair = plioconfigs.begin();
    while (data_pair != plioconfigs.end()) {
        const abr::plio_config &plconfig = data_pair->second;
        AEG_LOG("PLIO slaveOrMaster %d values: \n", static_cast<int32_t>(plconfig.slaveOrMaster));
        printplioconfig(plconfig, plconfig.id);
        ++data_pair;
    }
    AEG_LOG("PLIOs Config ends \n");
}

void aeg_configureparser::ConfigureParser::printExternalbufferConfig() const {
    AEG_LOG("External Buffers Config Starts: \n");
    auto data_pair = externalbufferconfigs.begin();
    while (data_pair != externalbufferconfigs.end()) {
        const abr::external_buffer_config &ebufferconfig = data_pair->second;
        printexternalbufferconfig(ebufferconfig, static_cast<int32_t>(ebufferconfig.id));
        ++data_pair;
    }
    AEG_LOG("External Buffers Config ends \n");
}

void aeg_configureparser::ConfigureParser::printSharedBufferConfig() const {
    AEG_LOG("Shared Buffer Config Starts: \n");
    auto data_pair = sharedbufferconfigs.begin();
    while (data_pair != sharedbufferconfigs.end()) {
        const abr::shared_buffer_config &shared_buffer_config = data_pair->second;
        AEG_LOG("Shared Buffer port name %.*s values: \n", static_cast<int32_t>(shared_buffer_config.name.size()), shared_buffer_config.name.data());
        printsharedbufferconfig(shared_buffer_config, static_cast<int32_t>(shared_buffer_config.id));
        ++data_pair;
    }
    AEG_LOG("Shared Buffer Config ends \n");
}

void aeg_configureparser::ConfigureParser::printRTPsConfig() const {
    AEG_LOG("RTPs Config Starts: \n");
    auto data_pair = rtpconfigs.begin();
    while (data_pair != rtpconfigs.end()) {
        const abr::rtp_config &rtpconfig = data_pair->second;
        AEG_LOG("RTP port name %.*s values: \n", static_cast<int32_t>(rtpconfig.portName.size()), rtpconfig.portName.data());
        printrtpconfig(rtpconfig, rtpconfig.portId);
        ++data_pair;
    }
    AEG_LOG("RTPs Config ends\n");
}

void aeg_configureparser::ConfigureParser::printgraphconfig(const abr::graph_config &grconfig, int32_t number) const {
    static_cast<void>(number);
    AEG_LOG("aie_metadata.graph_config.%d.id: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(grconfig.id));
    AEG_LOG("aie_metadata.graph_config.%d.name: %.*s\n", static_cast<int32_t>(number), static_cast<int32_t>(grconfig.name.size()), grconfig.name.data());

    AEG_LOG("aie_metadata.graph_config.%d.coreColumns: ", static_cast<int32_t>(number));
    std::size_t core_col_idx = 0U;
    while (core_col_idx < grconfig.coreColumns.size()) {
        const int16_t value = grconfig.coreColumns[core_col_idx];
        AEG_LOG("%d ", static_cast<int32_t>(value));
        ++core_col_idx;
    }
    AEG_LOG("\n");

    AEG_LOG("aie_metadata.graph_config.%d.coreRows: ", static_cast<int32_t>(number));
    std::size_t core_row_idx = 0U;
    while (core_row_idx < grconfig.coreRows.size()) {
        const int16_t value = grconfig.coreRows[core_row_idx];
        AEG_LOG("%d ", static_cast<int32_t>(value));
        ++core_row_idx;
    }
    AEG_LOG("\n");

    AEG_LOG("aie_metadata.graph_config.%d.iterMemColumns: ", static_cast<int32_t>(number));
    std::size_t iter_col_idx = 0U;
    while (iter_col_idx < grconfig.iterMemColumns.size()) {
        const int16_t value = grconfig.iterMemColumns[iter_col_idx];
        AEG_LOG("%d ", static_cast<int32_t>(value));
        ++iter_col_idx;
    }
    AEG_LOG("\n");

    AEG_LOG("aie_metadata.graph_config.%d.iterMemRows: ", static_cast<int32_t>(number));
    std::size_t iter_row_idx = 0U;
    while (iter_row_idx < grconfig.iterMemRows.size()) {
        const int16_t value = grconfig.iterMemRows[iter_row_idx];
        AEG_LOG("%d ", static_cast<int32_t>(value));
        ++iter_row_idx;
    }
    AEG_LOG("\n");

    AEG_LOG("aie_metadata.graph_config.%d.iterMemAddrs: ", static_cast<int32_t>(number));
    std::size_t iter_addr_idx = 0U;
    while (iter_addr_idx < grconfig.iterMemAddrs.size()) {
        const size_t value = grconfig.iterMemAddrs[iter_addr_idx];
        AEG_LOG("%zu ", value);
        ++iter_addr_idx;
    }
    AEG_LOG("\n");

    AEG_LOG("aie_metadata.graph_config.%d.triggered: ", static_cast<int32_t>(number));
    std::size_t triggered_idx = 0U;
    while (triggered_idx < grconfig.triggered.size()) {
        const bool value = (grconfig.triggered[triggered_idx] != 0U);
        AEG_LOG("%d ", (value ? 1 : 0));
        ++triggered_idx;
    }
    AEG_LOG("\n");

    AEG_LOG("aie_metadata.graph_config.%d.broadcast_column: %u\n", static_cast<int32_t>(number),
            static_cast<uint32_t>(grconfig.broadcast_column));
}

void aeg_configureparser::ConfigureParser::printgmioconfig(const abr::gmio_config &gm_config, int32_t number) const {
    static_cast<void>(gm_config);
    static_cast<void>(number);
    AEG_LOG("aie_metadata.gmio_config.%d.id: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(gm_config.id));
    AEG_LOG("aie_metadata.gmio_config.%d.name: %.*s\n", static_cast<int32_t>(number), static_cast<int32_t>(gm_config.name.size()), gm_config.name.data());
    AEG_LOG("aie_metadata.gmio_config.%d.logicalName: %.*s\n", static_cast<int32_t>(number), static_cast<int32_t>(gm_config.logicalName.size()), gm_config.logicalName.data());
    AEG_LOG("aie_metadata.gmio_config.%d.type: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(gm_config.type));
    AEG_LOG("aie_metadata.gmio_config.%d.shimColumn: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(gm_config.shimColumn));
    AEG_LOG("aie_metadata.gmio_config.%d.channelNum: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(gm_config.channelNum));
    AEG_LOG("aie_metadata.gmio_config.%d.streamId: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(gm_config.streamId));
    AEG_LOG("aie_metadata.gmio_config.%d.burstLength: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(gm_config.burstLength));
}

void aeg_configureparser::ConfigureParser::printplioconfig(const abr::plio_config &pl_config, int32_t number) const {
    static_cast<void>(pl_config);
    static_cast<void>(number);
    AEG_LOG("aie_metadata.plio_config.%d.id: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(pl_config.id));
    AEG_LOG("aie_metadata.plio_config.%d.name: %.*s\n", static_cast<int32_t>(number), static_cast<int32_t>(pl_config.name.size()), pl_config.name.data());
    AEG_LOG("aie_metadata.plio_config.%d.logicalName: %.*s\n", static_cast<int32_t>(number), static_cast<int32_t>(pl_config.logicalName.size()), pl_config.logicalName.data());
    AEG_LOG("aie_metadata.plio_config.%d.shimColumn: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(pl_config.shimColumn));
    AEG_LOG("aie_metadata.plio_config.%d.slaveOrMaster: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(pl_config.slaveOrMaster));
    AEG_LOG("aie_metadata.plio_config.%d.streamId: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(pl_config.streamId));
}

void aeg_configureparser::ConfigureParser::printshimbdinfoconfig(const abr::shimBDInfo &shim_bd_info_config, int32_t number,
                                            const std::string &prefix) const {
    static_cast<void>(shim_bd_info_config);
    static_cast<void>(number);
    static_cast<void>(prefix);
    AEG_LOG("%sshim_bd_info_config%d.bd_id: %u\n", prefix.c_str(), static_cast<int32_t>(number),
            static_cast<uint32_t>(shim_bd_info_config.bd_id));
    AEG_LOG("%sshim_bd_info_config%d.buf_idx: %u\n", prefix.c_str(), static_cast<int32_t>(number),
            static_cast<uint32_t>(shim_bd_info_config.buf_idx));
    AEG_LOG("%sshim_bd_info_config%d.offset: %u\n", prefix.c_str(), static_cast<int32_t>(number),
            static_cast<uint32_t>(shim_bd_info_config.offset));
    AEG_LOG("%sshim_bd_info_config%d.transaction_size: %zu\n", prefix.c_str(), static_cast<int32_t>(number),
            static_cast<uint64_t>(shim_bd_info_config.transaction_size));
}

void aeg_configureparser::ConfigureParser::printshimportconfig(const abr::shim_port_config &shim_port_config, int32_t number,
                                          const std::string &prefix) const {
    static_cast<void>(number);
    AEG_LOG("%sshim_port_config%d.portId: %d\n", prefix.c_str(), static_cast<int32_t>(number), static_cast<int32_t>(shim_port_config.portId));
    AEG_LOG("%sshim_port_config%d.portName: %.*s\n", prefix.c_str(), static_cast<int32_t>(number),
            static_cast<int32_t>(shim_port_config.portName.size()), shim_port_config.portName.data());
    AEG_LOG("%sshim_port_config%d.column: %d\n", prefix.c_str(), static_cast<int32_t>(number),
            static_cast<int32_t>(shim_port_config.column));
    AEG_LOG("%sshim_port_config%d.S2MMOrMM2S: %d\n", prefix.c_str(), static_cast<int32_t>(number),
            static_cast<int32_t>(shim_port_config.S2MMOrMM2S));
    AEG_LOG("%sshim_port_config%d.channel: %d\n", prefix.c_str(), static_cast<int32_t>(number),
            static_cast<int32_t>(shim_port_config.channel));
    AEG_LOG("%sshim_port_config%d.taskRepetition: %d\n", prefix.c_str(), static_cast<int32_t>(number),
            static_cast<int32_t>(shim_port_config.taskRepetition));
    AEG_LOG("%sshim_port_config%d.enable_task_complete_token: %d\n", prefix.c_str(), static_cast<int32_t>(number),
            static_cast<int32_t>(shim_port_config.enable_task_complete_token ? 1 : 0));

    int32_t i = static_cast<int32_t>(0);
    std::size_t bd_idx = 0U;
    while (bd_idx < shim_port_config.shimBDInfos.size()) {
        auto shim_bd_info = shim_port_config.shimBDInfos[bd_idx];
        printshimbdinfoconfig(shim_bd_info, i, prefix + "shim_port_config");
        i = static_cast<int32_t>(i + 1);
        ++bd_idx;
    }
}

void aeg_configureparser::ConfigureParser::printexternalbufferconfig(const abr::external_buffer_config &ebuffer_config, int32_t number) const {
    static_cast<void>(number);
    AEG_LOG("external_buffer_config%d.id: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(ebuffer_config.id));
    AEG_LOG("external_buffer_config%d.name: %.*s\n", static_cast<int32_t>(number), static_cast<int32_t>(ebuffer_config.name.size()), ebuffer_config.name.data());

    int32_t i = static_cast<int32_t>(0);
    std::string prefix = "external_buffer_config.shim_port_config";
    std::size_t p_idx = 0U;
    while (p_idx < ebuffer_config.shimPortConfigs.size()) {
        const auto &shim_port_config = ebuffer_config.shimPortConfigs[p_idx];
        printshimportconfig(shim_port_config, i, prefix);
        i = static_cast<int32_t>(i + 1);
        ++p_idx;
    }
}

void aeg_configureparser::ConfigureParser::printsharedbufferconfig(const abr::shared_buffer_config &sbuffer_config, int32_t number) const {
    static_cast<void>(number);
    AEG_LOG("shared_buffer_config%d.id: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(sbuffer_config.id));
    AEG_LOG("shared_buffer_config%d.name: %.*s\n", static_cast<int32_t>(number), static_cast<int32_t>(sbuffer_config.name.size()), sbuffer_config.name.data());
    AEG_LOG("shared_buffer_config%d.graphId: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(sbuffer_config.graphId));
    AEG_LOG("shared_buffer_config%d.numBytes: %zu\n", static_cast<int32_t>(number), sbuffer_config.numBytes);
    AEG_LOG("shared_buffer_config%d.column: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(sbuffer_config.column));
    AEG_LOG("shared_buffer_config%d.row: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(sbuffer_config.row));
    AEG_LOG("shared_buffer_config%d.numInputs: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(sbuffer_config.numInputs));
    AEG_LOG("shared_buffer_config%d.numOutputs: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(sbuffer_config.numOutputs));
    AEG_LOG("shared_buffer_config%d.elemType: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(sbuffer_config.elemType));
    AEG_LOG("shared_buffer_config%d.initialized: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(sbuffer_config.initialized ? 1 : 0));

    AEG_LOG("shared_buffer_config%d.addr: ", static_cast<int32_t>(number));
    size_t ia = 0U;
    while (ia < sbuffer_config.addr.size()) {
        AEG_LOG("%zu ", sbuffer_config.addr[ia]);
        ia++;
    }
    AEG_LOG("\n");

    AEG_LOG("shared_buffer_config%d.producerLocks: ", static_cast<int32_t>(number));
    size_t ipl = 0U;
    while (ipl < sbuffer_config.producerLocks.size()) {
        AEG_LOG("%d ", static_cast<int32_t>(sbuffer_config.producerLocks[ipl]));
        ipl++;
    }
    AEG_LOG("\n");

    AEG_LOG("shared_buffer_config%d.consumerLocks: ", static_cast<int32_t>(number));
    size_t icl = 0U;
    while (icl < sbuffer_config.consumerLocks.size()) {
        AEG_LOG("%d ", static_cast<int32_t>(sbuffer_config.consumerLocks[icl]));
        icl++;
    }
    AEG_LOG("\n");
}

void aeg_configureparser::ConfigureParser::printrtpconfig(const abr::rtp_config &rp_config, int32_t number) const {
    static_cast<void>(rp_config);
    static_cast<void>(number);
    AEG_LOG("aie_metadata.rtp_config.%d.port_id: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(rp_config.portId));
    AEG_LOG("aie_metadata.rtp_config.%d.alias_id: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(rp_config.aliasId));
    AEG_LOG("aie_metadata.rtp_config.%d.port_name: %.*s\n", static_cast<int32_t>(number), static_cast<int32_t>(rp_config.portName.size()), rp_config.portName.data());
    AEG_LOG("aie_metadata.rtp_config.%d.alias_name: %.*s\n", static_cast<int32_t>(number), static_cast<int32_t>(rp_config.aliasName.size()), rp_config.aliasName.data());
    AEG_LOG("aie_metadata.rtp_config.%d.graph_id: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(rp_config.graphId));
    AEG_LOG("aie_metadata.rtp_config.%d.is_input: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(rp_config.isInput ? 1 : 0));
    AEG_LOG("aie_metadata.rtp_config.%d.is_asynchronous: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(rp_config.isAsync ? 1 : 0));
    AEG_LOG("aie_metadata.rtp_config.%d.is_connected: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(rp_config.isConnect ? 1 : 0));
    AEG_LOG("aie_metadata.rtp_config.%d.number_of_bytes: %zu\n", static_cast<int32_t>(number), rp_config.numBytes);
    AEG_LOG("aie_metadata.rtp_config.%d.is_PL_RTP: %d\n", static_cast<int32_t>(number), static_cast<int32_t>(rp_config.isPL ? 1 : 0));
    AEG_LOG("aie_metadata.rtp_config.%d.requires_lock: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(rp_config.hasLock ? 1 : 0));
    AEG_LOG("aie_metadata.rtp_config.%d.selector_column: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(rp_config.selectorColumn));
    AEG_LOG("aie_metadata.rtp_config.%d.selector_row: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(rp_config.selectorRow));
    AEG_LOG("aie_metadata.rtp_config.%d.selector_address: %zu\n", static_cast<int32_t>(number), rp_config.selectorAddr);
    AEG_LOG("aie_metadata.rtp_config.%d.selector_lock_id: %u\n", static_cast<int32_t>(number),
            static_cast<uint32_t>(rp_config.selectorLockId));
    AEG_LOG("aie_metadata.rtp_config.%d.ping_buffer_column: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(rp_config.pingColumn));
    AEG_LOG("aie_metadata.rtp_config.%d.ping_buffer_row: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(rp_config.pingRow));
    AEG_LOG("aie_metadata.rtp_config.%d.ping_buffer_address: %zu\n", static_cast<int32_t>(number), rp_config.pingAddr);
    AEG_LOG("aie_metadata.rtp_config.%d.ping_buffer_lock_id: %u\n", static_cast<int32_t>(number),
            static_cast<uint32_t>(rp_config.pingLockId));
    AEG_LOG("aie_metadata.rtp_config.%d.pong_buffer_column: %d\n", static_cast<int32_t>(number),
                static_cast<int32_t>(rp_config.pongColumn));
    AEG_LOG("aie_metadata.rtp_config.%d.pong_buffer_row: %d\n", static_cast<int32_t>(number),
            static_cast<int32_t>(rp_config.pongRow));
    AEG_LOG("aie_metadata.rtp_config.%d.pong_buffer_address: %zu\n", static_cast<int32_t>(number), rp_config.pongAddr);
    AEG_LOG("aie_metadata.rtp_config.%d.pong_buffer_lock_id: %u\n", static_cast<int32_t>(number),
            static_cast<uint32_t>(rp_config.pongLockId));
}

aeg_configureparser::ConfigureParser::ConfigureParser() {
    Load();
    #ifdef AEG_DEBUG
    AEG_LOG("************Print configurations**********\n");
    printDriverConfig();
    printGraphConfig();
    printGMIOsConfig();
    printExternalbufferConfig();
    printSharedBufferConfig();
    printRTPsConfig();
    #endif
};

abr::driver_config &aeg_configureparser::ConfigureParser::get_driver_config() & { return dconfig; }

abr::aiecompiler_config &aeg_configureparser::ConfigureParser::get_aiecompiler_config() & { return aiecompilerconfig; }

abr::graph_config *aeg_configureparser::ConfigureParser::get_graph_config(std::string_view graph_name) {
    auto it = gconfigs.find(graph_name);
    if (it == gconfigs.end()) {
        AEG_ERROR("%s: graph '%.*s' does not exist", "get_graph_config", static_cast<int32_t>(graph_name.size()), graph_name.data());
        return nullptr;
    }
    return &it->second;
}

abr::gmio_config *aeg_configureparser::ConfigureParser::get_gmio_config(std::string_view gmio_name, bool silent) {
    auto it = gmioconfigs.find(gmio_name);
    if (it == gmioconfigs.end()) {
        if (!silent) {
            AEG_ERROR("%s: gmio '%.*s' does not exist", "get_gmio_config", static_cast<int32_t>(gmio_name.size()), gmio_name.data());
        }
        return nullptr;
    }
    return &it->second;
}

abr::plio_config *aeg_configureparser::ConfigureParser::get_plio_config_logicname(std::string_view graph_name, bool silent) {
    auto it = std::find_if(plioconfigs.begin(), plioconfigs.end(),
                           [&graph_name](const auto &entry) { return entry.second.logicalName.compare(graph_name) == 0; });
    if (it == plioconfigs.end()) {
        if (!silent) {
            AEG_ERROR("%s: plio (logical) '%.*s' does not exist", "get_plio_config_logicname", static_cast<int32_t>(graph_name.size()), graph_name.data());
        }
        return nullptr;
    }
    return &it->second;
}

abr::plio_config *aeg_configureparser::ConfigureParser::get_plio_config(std::string_view plio_name, bool silent) {
    auto it = plioconfigs.find(plio_name);
    if (it == plioconfigs.end()) {
        if (!silent) {
            AEG_ERROR("%s: plio '%.*s' does not exist", "get_plio_config", static_cast<int32_t>(plio_name.size()), plio_name.data());
        }
        return nullptr;
    }
    return &it->second;
}

abr::external_buffer_config *aeg_configureparser::ConfigureParser::get_externalbuffer_config(std::string_view externalbuffer_name) {
    auto it = externalbufferconfigs.find(externalbuffer_name);
    if (it == externalbufferconfigs.end()) {
        AEG_ERROR("%s: external buffer '%.*s' does not exist", "get_externalbuffer_config", static_cast<int32_t>(externalbuffer_name.size()), externalbuffer_name.data());
        return nullptr;
    }
    return &it->second;
}

abr::shared_buffer_config *aeg_configureparser::ConfigureParser::get_shared_buffer_config(std::string_view shared_buffer_name) {
    auto it = sharedbufferconfigs.find(shared_buffer_name);
    AEG_LOG("%zu shared buffers found\n", sharedbufferconfigs.size());
    auto pair = sharedbufferconfigs.begin();
    while (pair != sharedbufferconfigs.end()) {
        static_cast<void>(pair);
        AEG_LOG("Shared buffer: %.*s\n", static_cast<int32_t>(pair->first.size()), pair->first.data());
        ++pair;
    }
    if (it == sharedbufferconfigs.end()) {
        AEG_ERROR("%s: shared buffer '%.*s' does not exist", "get_shared_buffer_config", static_cast<int32_t>(shared_buffer_name.size()), shared_buffer_name.data());
        return nullptr;
    }
    return &it->second;
}

abr::rtp_config *aeg_configureparser::ConfigureParser::get_rtp_config(std::string_view rtp_name) {
    auto it = rtpconfigs.find(rtp_name);
    if (it == rtpconfigs.end()) {
        AEG_ERROR("%s: RTP '%.*s' does not exist", "get_rtp_config", static_cast<int32_t>(rtp_name.size()), rtp_name.data());
        return nullptr;
    }
    return &it->second;
}

int32_t aeg_configureparser::ConfigureParser::isRTPorSharedBuffer(const std::string_view name) const { // 0-RTP, 1- shared buffer
    if (sharedbufferconfigs.find(name) != sharedbufferconfigs.end()) {
        return 1;
    }
    if (rtpconfigs.find(name) != rtpconfigs.end()) {
        return 0;
    }
    return -1;
}

std::shared_ptr<gmioapiimpl::GmioApiImpl> aeg_configureparser::ConfigureParser::get_gmio_api_impl(std::string_view gmio_name) {
    auto it = gmio_api_impls.find(gmio_name);
    if (it == gmio_api_impls.end() || it->second == nullptr) {
        AEG_ERROR("%s: GMIO API impl for '%.*s' does not exist", "get_gmio_api_impl", static_cast<int32_t>(gmio_name.size()), gmio_name.data());
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<externalbufferapiimpl::ExternalBufferApiImpl> aeg_configureparser::ConfigureParser::get_externalbuffer_api_impl(std::string_view externalbuffer_name) {
    auto it = ebuffer_api_impls.find(externalbuffer_name);
    if (it == ebuffer_api_impls.end() || it->second == nullptr) {
        AEG_ERROR("%s: external buffer API impl for '%.*s' does not exist", "get_externalbuffer_api_impl", static_cast<int32_t>(externalbuffer_name.size()), externalbuffer_name.data());
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<rtpapiimpl::RtpApiImpl> aeg_configureparser::ConfigureParser::get_rtp_api_impl(std::string_view rtp_name) {
    auto &ret = rtp_api_impls[rtp_name];
    if (ret == nullptr) {
        AEG_ERROR("%s: RTP API impl for '%.*s' does not exist", "get_rtp_api_impl", static_cast<int32_t>(rtp_name.size()), rtp_name.data());
    }
    return ret;
}

std::shared_ptr<sharedbufferapiimpl::SharedBufferApiImpl> aeg_configureparser::ConfigureParser::get_sharedbuffer_api_impl(std::string_view sharedbuffer_name) {
    auto &ret = sharedbuffer_api_impls[sharedbuffer_name];
    if (ret == nullptr) {
        AEG_ERROR("%s: shared buffer API impl for '%.*s' does not exist", "get_sharedbuffer_api_impl", static_cast<int32_t>(sharedbuffer_name.size()), sharedbuffer_name.data());
    }
    return ret;
}

void aeg_configureparser::ConfigureParser::Load() {
    LoadDriverConfig();
    LoadAiecompilerConfig();
    LoadGraphConfig();
    AEG_LOG("Inside Load: Before LoadGMIOs\n");
    LoadGmios();
    LoadPlios();
    AEG_LOG("Inside Load: Before LoadExternalbuffers\n");
    LoadExternalbuffers();
    AEG_LOG("Inside Load: Before LoadSharedBuffers\n");
    LoadSharedBuffers();
    AEG_LOG("Inside Load: After LoadSharedBuffers\n");
    LoadRtps();
}

template <typename T> T aeg_configureparser::ConfigureParser::GetConfig(std::string_view name) {
#ifdef __AIESIM__
    return pt.get<T>(name);
#else
    T ret;
    return ret;
#endif
}

void aeg_configureparser::ConfigureParser::LoadDriverConfig() {
    DriverConfigStaticParser dconfigparser;
    dconfig = dconfigparser.load(partitions);
};

void aeg_configureparser::ConfigureParser::LoadAiecompilerConfig() {
    AiecompilerConfigStaticParser aiecompilerconfigparser;
    aiecompilerconfig = aiecompilerconfigparser.load();
};

void aeg_configureparser::ConfigureParser::LoadGraphConfig() {
    GraphConfigStaticParser grconfigMapparser;
    gconfigs = grconfigMapparser.loadAll(partitions, static_cast<int32_t>(dconfig.partition_overlay_start_cols[0]));
}

void aeg_configureparser::ConfigureParser::LoadGmios() {
    GMIOsConfigStaticParser gmconfigMapparser;
    gmioconfigs = gmconfigMapparser.loadAll();
}

void aeg_configureparser::ConfigureParser::LoadPlios() {
    PLIOsConfigStaticParser plconfigMapparser;
    plioconfigs = plconfigMapparser.loadAll();
}

void aeg_configureparser::ConfigureParser::LoadExternalbuffers() {
    ExternalbuffersConfigStaticParser ebufferconfigMapparser;
    externalbufferconfigs = ebufferconfigMapparser.loadAll();
}

void aeg_configureparser::ConfigureParser::LoadSharedBuffers() {
    AEG_LOG("LoadSharedBuffers called\n");
    SharedBuffersConfigStaticParser sbufferconfigMapparser;
    sharedbufferconfigs = sbufferconfigMapparser.loadAll();
    AEG_LOG("LoadSharedBuffers exit\n");
}

void aeg_configureparser::ConfigureParser::LoadRtps() {
    RTPsConfigStaticParser rtconfigMapparser;
    rtpconfigs = rtconfigMapparser.loadAll();
}

namespace {
inline aeg_err map_err_code(abr::err_code code) {
    switch (code) {
    case abr::err_code::ok:
    {
        return aeg_err::ok;
    }
    case abr::err_code::user_error:
    {
        return aeg_err::user_error;
    }
    case abr::err_code::internal_error:
    {
        return aeg_err::internal_error;
    }
    case abr::err_code::aie_driver_error:
    {
        return aeg_err::aie_driver_error;
    }
    case abr::err_code::resource_unavailable:
    {
        return aeg_err::resource_unavailable;
    }
    default:
    {
        return aeg_err::aie_driver_error;
    }
    }
}
}

aeg_err aeg_configureparser::ConfigureParser::ConfigureGmios(std::shared_ptr<xaiemem::XAieMem> xaiemem) {
    if (!xaiemem) {
        AEG_ERROR("%s: XAieMem backend is null", "ConfigureGmios");
        return aeg_err::null_argument;
    }
    auto it = gmioconfigs.begin();
    while (it != gmioconfigs.end()) {
        auto &item = *it;
        auto gmio_api = std::make_shared<abr::gmio_api>(&item.second);
        aeg_err configure_status = map_err_code(gmio_api->configure());
        if (configure_status != aeg_err::ok) {
            AEG_ERROR("%s: failed to configure GMIO '%.*s' status=%d", "ConfigureGmios",
                      static_cast<int32_t>(item.first.size()), item.first.data(),
                      static_cast<int32_t>(configure_status));
            return configure_status;
        }
        // new gmioapimpl class---
        auto gmio_api_impl = std::make_shared<gmioapiimpl::GmioApiImpl>(gmio_api, item.second, xaiemem, item.first);
        gmio_api_impls[item.first] = gmio_api_impl;
        ++it;
    }
    return aeg_err::ok;
}

aeg_err aeg_configureparser::ConfigureParser::ConfigureExternalBuffers(std::shared_ptr<xaiemem::XAieMem> xaiemem) {
    if (!xaiemem) {
        AEG_ERROR("%s: XAieMem backend is null", "ConfigureExternalBuffers");
        return aeg_err::null_argument;
    }
    auto it = externalbufferconfigs.begin();
    while (it != externalbufferconfigs.end()) {
        auto &item = *it;
        AEG_LOG("******Configuring external buffer*******\n");
        AEG_LOG("e buffer name : %.*s\n", static_cast<int32_t>(item.first.size()), item.first.data());
        auto ebuffer_api_impl = std::make_shared<externalbufferapiimpl::ExternalBufferApiImpl>(item.second, item.first, xaiemem);
        ebuffer_api_impls[item.first] = ebuffer_api_impl;
        ++it;
    }
    return aeg_err::ok;
}

aeg_err aeg_configureparser::ConfigureParser::ConfigureRtps(std::shared_ptr<abr::graph_api> graphApi) {
    if (!graphApi) {
        AEG_ERROR("%s: graph API backend is null", "ConfigureRtps");
        return aeg_err::null_argument;
    }
    auto it = rtpconfigs.begin();
    while (it != rtpconfigs.end()) {
        auto &item = *it;
        auto rtp_api_impl = std::make_shared<rtpapiimpl::RtpApiImpl>(item.second, graphApi, item.first);
        rtp_api_impls[item.first] = rtp_api_impl;
        ++it;
    }
    return aeg_err::ok;
}

aeg_err aeg_configureparser::ConfigureParser::ConfigureSharedBuffers(std::shared_ptr<abr::graph_api> graphApi) {
    if (!graphApi) {
        AEG_ERROR("%s: graph API backend is null", "ConfigureSharedBuffers");
        return aeg_err::null_argument;
    }
    auto it = sharedbufferconfigs.begin();
    while (it != sharedbufferconfigs.end()) {
        auto &item = *it;
        auto sharedbuffer_api_impl = std::make_shared<sharedbufferapiimpl::SharedBufferApiImpl>(item.second, graphApi, item.first);
        sharedbuffer_api_impls[item.first] = sharedbuffer_api_impl;
        ++it;
    }
    return aeg_err::ok;
}
