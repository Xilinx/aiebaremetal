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
#include <cstdlib>

class UnpackUtils {
  public:
    static uint8_t unpackUInt8(const char *&data_ptr) {
        uint8_t value;
        std::memcpy(&value, data_ptr, sizeof(uint8_t));
        data_ptr += sizeof(uint8_t);
        return value;
    }

    static std::string unpackString(const char *&data_ptr) {
        int str_length;
        std::memcpy(&str_length, data_ptr, sizeof(int));
        data_ptr += sizeof(int);
        std::string value(data_ptr, str_length);
        data_ptr += str_length;
        return value;
    }

    static uint64_t unpackUInt64(const char *&data_ptr) {
        uint64_t value;
        std::memcpy(&value, data_ptr, sizeof(uint64_t));
        data_ptr += sizeof(uint64_t);
        return value;
    }

    static int unpackUInt32(const char *&data_ptr) {
        uint32_t value;
        std::memcpy(&value, data_ptr, sizeof(uint32_t));
        data_ptr += sizeof(uint32_t);
        return value;
    }

    static short unpackShort(const char *&data_ptr) {
        // unpacks as uint16
        uint16_t value;
        std::memcpy(&value, data_ptr, sizeof(uint16_t));
        data_ptr += sizeof(uint16_t);
        return value;
    }

    static bool unpackBool(const char *&data_ptr) {
        // bool unpacks as int8
        int8_t value;
        std::memcpy(&value, data_ptr, sizeof(int8_t));
        data_ptr += sizeof(int8_t);
        return value;
    }

    // Unpack a vector of a specified type and length
    template <typename T> static std::vector<T> unpackVector(size_t length, const char *&data_ptr) {
        std::vector<T> vec(length);
        for (size_t i = 0; i < length; ++i) {
            vec[i] = unpackValue<T>(data_ptr);
        }
        return vec;
    }

    // Function to unpack a value of a specific type
    template <typename T> static T unpackValue(const char *&data_ptr) {
        T value;
        std::memcpy(&value, data_ptr, sizeof(T));
        data_ptr += sizeof(T);
        return value;
    }
};

class DriverConfigStaticParser {
  public:
    abr::driver_config load(std::vector<abr::partition_config> &partitions) {
        const char *bin_ptr = _binary_build_driverconfig_bin_start;
        abr::driver_config dconfig;

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

        int partition_overlay_start_cols_len = UnpackUtils::unpackUInt32(bin_ptr);
        dconfig.partition_overlay_start_cols =
            UnpackUtils::unpackVector<short>(partition_overlay_start_cols_len, bin_ptr);

        // Check if partition information is available
        if ((bin_ptr - _binary_build_driverconfig_bin_start) < _binary_build_driverconfig_bin_size) {
            int start_column = UnpackUtils::unpackUInt32(bin_ptr);
            int num_columns = UnpackUtils::unpackUInt32(bin_ptr);
            int partition_len = UnpackUtils::unpackUInt32(bin_ptr);
            AEG_LOG("partition_len %d\n", partition_len);
            abr::partition_config pconfig;
            for (int j = 0; j < partition_len; j++) {
                pconfig.startColumn = UnpackUtils::unpackUInt32(bin_ptr);
                pconfig.numColumns = UnpackUtils::unpackUInt32(bin_ptr);
                pconfig.uuid = UnpackUtils::unpackString(bin_ptr);
                pconfig.aie_pl_intf_id = UnpackUtils::unpackUInt32(bin_ptr);
                pconfig.name_space = UnpackUtils::unpackString(bin_ptr);
                partitions.push_back(pconfig);
            }
        }
        return dconfig;
    }
};

uint32_t get_partition_start_column(std::vector<abr::partition_config> &partitions, int column) {
    if (partitions.size() == 0)
        return default_start_column;

    auto itr = std::find_if(partitions.begin(), partitions.end(), [column](const auto &part) {
        uint32_t start_column = part.startColumn;
        uint32_t num_columns = part.numColumns;
        return (start_column <= column) && (column < (start_column + num_columns));
    });

    if (itr != partitions.end())
        return itr->startColumn;

    return default_start_column;
}

class AiecompilerConfigStaticParser {
  public:
    abr::aiecompiler_config load() {
        const char *bin_ptr = _binary_build_aiecompilerconfig_bin_start;
        abr::aiecompiler_config aiecompilerconfig;

        aiecompilerconfig.broadcast_enable_core = UnpackUtils::unpackBool(bin_ptr);
        aiecompilerconfig.graph_iterator_event = UnpackUtils::unpackBool(bin_ptr);
        aiecompilerconfig.enable_multi_layer = UnpackUtils::unpackBool(bin_ptr);

        return aiecompilerconfig;
    }
};

class GraphConfigStaticParser {
  public:
    std::unordered_map<std::string, abr::graph_config> loadAll(std::vector<abr::partition_config> &partitions,
                                                               int partition_start_column) {
        const char *bin_ptr = _binary_build_graphconfig_bin_start;
        std::unordered_map<std::string, abr::graph_config> grconfigMap;
        unsigned int data_size = 0;

        data_size = _binary_build_graphconfig_bin_size;

        while ((bin_ptr - _binary_build_graphconfig_bin_start) < data_size) {
            AEG_LOG("bin_ptr :%p\n", (void *)bin_ptr);
            AEG_LOG("Graph config : Line 119 \n");
            abr::graph_config grconfig = load(bin_ptr, partitions, partition_start_column);
            AEG_LOG("Graph config : Line 121 \n");
            grconfigMap[grconfig.name] = grconfig;
            AEG_LOG("Graph config : Line 123 \n");
        }
        AEG_LOG("returning graph config\n");
        return grconfigMap;
    }

  private:
    abr::graph_config load(const char *&bin_ptr, std::vector<abr::partition_config> partitions,
                           int partition_start_column) {
        abr::graph_config grconfig;

        grconfig.id = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.name = UnpackUtils::unpackString(bin_ptr);

        int coreColumns_len = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.coreColumns = UnpackUtils::unpackVector<short>(coreColumns_len, bin_ptr);

        int coreRows_len = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.coreRows = UnpackUtils::unpackVector<short>(coreRows_len, bin_ptr);

        int iterMemColumns_len = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.iterMemColumns = UnpackUtils::unpackVector<short>(iterMemColumns_len, bin_ptr);

        int iterMemRows_len = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.iterMemRows = UnpackUtils::unpackVector<short>(iterMemRows_len, bin_ptr);

        int iterMemAddrs_len = UnpackUtils::unpackUInt32(bin_ptr);
        // itermemaddrs saved as unsigned int
        std::vector<unsigned int> iterMemAddrs_uint_vec =
            UnpackUtils::unpackVector<unsigned int>(iterMemAddrs_len, bin_ptr);
        grconfig.iterMemAddrs.reserve(iterMemAddrs_len);

        for (unsigned int value : iterMemAddrs_uint_vec) {
            grconfig.iterMemAddrs.push_back(static_cast<int>(value));
        }
        iterMemAddrs_uint_vec.clear();
        iterMemAddrs_uint_vec.shrink_to_fit();

        int triggered_len = UnpackUtils::unpackUInt32(bin_ptr);
        grconfig.triggered = UnpackUtils::unpackVector<bool>(triggered_len, bin_ptr);

        grconfig.broadcast_column = 0;
        if (partitions.size()) {
            grconfig.broadcast_column =
                get_partition_start_column(partitions, grconfig.coreColumns[0] + partition_start_column) -
                partition_start_column;
        }

        return grconfig;
    }
};

class GMIOsConfigStaticParser {
  public:
    std::unordered_map<std::string, abr::gmio_config> loadAll() {
        const char *bin_ptr = _binary_build_gmiosconfig_bin_start;
        std::unordered_map<std::string, abr::gmio_config> gmconfigMap;

        unsigned int data_size = 0;

        data_size = _binary_build_gmiosconfig_bin_size;

        while ((bin_ptr - _binary_build_gmiosconfig_bin_start) < data_size) {
            abr::gmio_config gmconfig = load(bin_ptr);
            gmconfigMap[gmconfig.name] = gmconfig;
        }
        return gmconfigMap;
    }

  private:
    abr::gmio_config load(const char *&bin_ptr) {
        abr::gmio_config gmconfig;

        gmconfig.id = UnpackUtils::unpackUInt32(bin_ptr);
        gmconfig.name = UnpackUtils::unpackString(bin_ptr);
        gmconfig.logicalName = UnpackUtils::unpackString(bin_ptr);

        short gm_type = UnpackUtils::unpackShort(bin_ptr);
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
    std::unordered_map<std::string, abr::plio_config> loadAll() {
        const char *bin_ptr = _binary_build_pliosconfig_bin_start;
        std::unordered_map<std::string, abr::plio_config> plconfigMap;

        unsigned int data_size = 0;

        data_size = _binary_build_pliosconfig_bin_size;

        while ((bin_ptr - _binary_build_pliosconfig_bin_start) < data_size) {
            abr::plio_config plconfig = load(bin_ptr);
            plconfigMap[plconfig.name] = plconfig;
        }
        return plconfigMap;
    }

  private:
    abr::plio_config load(const char *&bin_ptr) {
        abr::plio_config plconfig;

        plconfig.id = UnpackUtils::unpackUInt32(bin_ptr);
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
    std::unordered_map<std::string, abr::external_buffer_config> loadAll() {
        const char *bin_ptr = _binary_build_externalbuffersconfig_bin_start;
        std::unordered_map<std::string, abr::external_buffer_config> ebufferconfigMap;

        unsigned int data_size = _binary_build_externalbuffersconfig_bin_size;
        AEG_LOG("_binary_build_externalbuffersconfig_bin_start : %p\n",
                (void *)_binary_build_externalbuffersconfig_bin_start);
        AEG_LOG("_binary_build_externalbuffersconfig_bin_size:  %u\n", _binary_build_externalbuffersconfig_bin_size);
        AEG_LOG("data size  %u\n", data_size);
        while ((bin_ptr - _binary_build_externalbuffersconfig_bin_start) < data_size) {
            abr::external_buffer_config ebufferconfig = load(bin_ptr);
            ebufferconfigMap[ebufferconfig.name] = ebufferconfig;
            AEG_LOG("**************Static  parser External Buffer*********\n");
            AEG_LOG("ebufferconfig.name %s\n", ebufferconfig.name.c_str());
            AEG_LOG("read bytes %zu\n", (size_t)(bin_ptr - _binary_build_externalbuffersconfig_bin_start));
        }
        return ebufferconfigMap;
    }

  private:
    abr::external_buffer_config load(const char *&bin_ptr) {
        abr::external_buffer_config ebufferconfig;

        ebufferconfig.id = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("ebufferconfig.id %d\n", (int)ebufferconfig.id);

        ebufferconfig.name = UnpackUtils::unpackString(bin_ptr);
        AEG_LOG("ebufferconfig.name %s\n", ebufferconfig.name.c_str());
        int shim_port_config_len = UnpackUtils::unpackUInt32(bin_ptr);
        AEG_LOG("shim_port_config_len %d\n", shim_port_config_len);

        for (int i = 0; i < shim_port_config_len; i++) {
            abr::shim_port_config shimportconfig;
            shimportconfig.portId = UnpackUtils::unpackUInt32(bin_ptr);
            shimportconfig.portName = UnpackUtils::unpackString(bin_ptr);
            shimportconfig.S2MMOrMM2S = UnpackUtils::unpackUInt32(bin_ptr);
            shimportconfig.column = UnpackUtils::unpackShort(bin_ptr);

            shimportconfig.channel = UnpackUtils::unpackShort(bin_ptr);
            shimportconfig.taskRepetition = UnpackUtils::unpackUInt32(bin_ptr);
            shimportconfig.enable_task_complete_token = UnpackUtils::unpackBool(bin_ptr);
            int shim_bd_info_len = UnpackUtils::unpackUInt32(bin_ptr);
            for (int j = 0; j < shim_bd_info_len; j++) {
                abr::shimBDInfo shim_bd_info;
                shim_bd_info.bd_id = UnpackUtils::unpackUInt32(bin_ptr);
                shim_bd_info.buf_idx = UnpackUtils::unpackUInt32(bin_ptr);
                shim_bd_info.offset = UnpackUtils::unpackUInt32(bin_ptr);
                shim_bd_info.transaction_size = UnpackUtils::unpackUInt64(bin_ptr);
                shimportconfig.shimBDInfos.push_back(shim_bd_info);
            }
            ebufferconfig.shimPortConfigs.push_back(shimportconfig);
        }

        return ebufferconfig;
    }
};

class SharedBuffersConfigStaticParser {
  public:
    std::unordered_map<std::string, abr::shared_buffer_config> loadAll() {
        AEG_LOG("Load All Shared Buffers Config\n");
        const char *bin_ptr = _binary_build_sharedbuffersconfig_bin_start;
        std::unordered_map<std::string, abr::shared_buffer_config> sbufferconfigMap;

        unsigned int data_size = _binary_build_sharedbuffersconfig_bin_size;
        AEG_LOG("_binary_build_sharedbuffersconfig_bin_start : %p\n", (void *)_binary_build_sharedbuffersconfig_bin_start);
        AEG_LOG("_binary_build_sharedbuffersconfig_bin_size:  %u\n", _binary_build_sharedbuffersconfig_bin_size);
        AEG_LOG("data size  %u\n", data_size);
        while ((bin_ptr - _binary_build_sharedbuffersconfig_bin_start) < data_size) {
            abr::shared_buffer_config sbufferconfig = load(bin_ptr);
            sbufferconfigMap[sbufferconfig.name] = sbufferconfig;
            AEG_LOG("**************Static Parser Shared Buffer*********\n");
            AEG_LOG("sbufferconfig.name %s\n", sbufferconfig.name.c_str());
            AEG_LOG("read bytes %zu\n", (size_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));
        }
        return sbufferconfigMap;
    }

  private:
    abr::shared_buffer_config load(const char *&bin_ptr) {
        AEG_LOG("Load Shared Buffer Config\n");
        abr::shared_buffer_config sbufferconfig;

        sbufferconfig.id = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("sbufferconfig.id %d\n", (int)sbufferconfig.id);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.name = UnpackUtils::unpackString(bin_ptr);
        AEG_LOG("sbufferconfig.name %s\n", sbufferconfig.name.c_str());
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.graphId = UnpackUtils::unpackUInt32(bin_ptr);
        AEG_LOG("sbufferconfig.graphId %u\n", sbufferconfig.graphId);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.numBytes = UnpackUtils::unpackUInt64(bin_ptr);
        AEG_LOG("sbufferconfig.numBytes %llu\n", (unsigned long long)sbufferconfig.numBytes);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.column = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("sbufferconfig.column %d\n", (int)sbufferconfig.column);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.row = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("sbufferconfig.row %d\n", (int)sbufferconfig.row);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.numInputs = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("sbufferconfig.numInputs %d\n", (int)sbufferconfig.numInputs);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));
        sbufferconfig.numOutputs = UnpackUtils::unpackShort(bin_ptr);
        AEG_LOG("sbufferconfig.numOutputs %d\n", (int)sbufferconfig.numOutputs);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        sbufferconfig.initialized = UnpackUtils::unpackBool(bin_ptr);
        AEG_LOG("sbufferconfig.initialized %d\n", (int)sbufferconfig.initialized);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        int addr_len = UnpackUtils::unpackUInt32(bin_ptr);
        AEG_LOG("addr_len %d\n", addr_len);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));
        sbufferconfig.addr = UnpackUtils::unpackVector<size_t>(addr_len, bin_ptr);
        AEG_LOG("sbufferconfig.addr ");
        for (const auto &addr : sbufferconfig.addr) {
            AEG_LOG("%zu ", addr);
        }
        AEG_LOG("\n");
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        int producer_locks_len = UnpackUtils::unpackUInt32(bin_ptr);
        AEG_LOG("producer_locks_len %d\n", producer_locks_len);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));
        sbufferconfig.producerLocks = UnpackUtils::unpackVector<int>(producer_locks_len, bin_ptr);
        AEG_LOG("sbufferconfig.producerLocks ");
        for (const auto &p_lock : sbufferconfig.producerLocks) {
            AEG_LOG("%d ", p_lock);
        }
        AEG_LOG("\n");
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        int consumer_locks_len = UnpackUtils::unpackUInt32(bin_ptr);
        AEG_LOG("consumer_locks_len %d\n", consumer_locks_len);
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));
        sbufferconfig.consumerLocks = UnpackUtils::unpackVector<int>(consumer_locks_len, bin_ptr);
        AEG_LOG("sbufferconfig.consumerLocks ");
        for (const auto &c_lock : sbufferconfig.consumerLocks) {
            AEG_LOG("%d ", c_lock);
        }
        AEG_LOG("\n");
        AEG_LOG("bin_ptr: 0x%lx\n", (unsigned long)(uintptr_t)(bin_ptr - _binary_build_sharedbuffersconfig_bin_start));

        return sbufferconfig;
    }
};

class RTPsConfigStaticParser {
  public:
    std::unordered_map<std::string, abr::rtp_config> loadAll() {
        const char *bin_ptr = _binary_build_rtpsconfig_bin_start;
        std::unordered_map<std::string, abr::rtp_config> rtpconfigMap;

        unsigned int data_size = 0;

        data_size = _binary_build_rtpsconfig_bin_size;

        while ((bin_ptr - _binary_build_rtpsconfig_bin_start) < data_size) {
            abr::rtp_config rtpconfig = load(bin_ptr);
            rtpconfigMap[rtpconfig.portName] = rtpconfig;
        }
        return rtpconfigMap;
    }

  private:
    abr::rtp_config load(const char *&bin_ptr) {
        abr::rtp_config rtpconfig;

        rtpconfig.portId = UnpackUtils::unpackUInt32(bin_ptr);
        rtpconfig.aliasId = UnpackUtils::unpackUInt32(bin_ptr);
        rtpconfig.portName = UnpackUtils::unpackString(bin_ptr);
        rtpconfig.aliasName = UnpackUtils::unpackString(bin_ptr);
        rtpconfig.graphId = UnpackUtils::unpackUInt32(bin_ptr);
        rtpconfig.isInput = UnpackUtils::unpackBool(bin_ptr);
        rtpconfig.isAsync = UnpackUtils::unpackBool(bin_ptr);
        rtpconfig.isConnect = UnpackUtils::unpackBool(bin_ptr);
        uint32_t num_bytes = UnpackUtils::unpackUInt32(bin_ptr);
        rtpconfig.numBytes = static_cast<size_t>(num_bytes);
        rtpconfig.isPL = UnpackUtils::unpackBool(bin_ptr);
        rtpconfig.hasLock = UnpackUtils::unpackBool(bin_ptr);
        rtpconfig.selectorColumn = UnpackUtils::unpackShort(bin_ptr);
        rtpconfig.selectorRow = UnpackUtils::unpackShort(bin_ptr);
        uint32_t selector_address = UnpackUtils::unpackUInt32(bin_ptr);
        rtpconfig.selectorAddr = static_cast<size_t>(selector_address);
        rtpconfig.selectorLockId = UnpackUtils::unpackShort(bin_ptr);
        rtpconfig.pingColumn = UnpackUtils::unpackShort(bin_ptr);
        rtpconfig.pingRow = UnpackUtils::unpackShort(bin_ptr);
        uint32_t ping_address = UnpackUtils::unpackUInt32(bin_ptr);
        rtpconfig.pingAddr = static_cast<size_t>(ping_address);
        rtpconfig.pingLockId = UnpackUtils::unpackShort(bin_ptr);
        rtpconfig.pongColumn = UnpackUtils::unpackShort(bin_ptr);
        rtpconfig.pongRow = UnpackUtils::unpackShort(bin_ptr);
        uint32_t pong_address = UnpackUtils::unpackUInt32(bin_ptr);
        rtpconfig.pongAddr = static_cast<size_t>(pong_address);
        rtpconfig.pongLockId = UnpackUtils::unpackShort(bin_ptr);

        return rtpconfig;
    }
};

void ConfigureParser::printDriverConfig() {
    std::cout << "Driver Config Starts: " << std::endl;
    std::cout << "aie_metadata.driver_config.hw_gen: " << (int)dconfig.hw_gen << std::endl;
    std::cout << "aie_metadata.driver_config.base_address: " << dconfig.base_address << std::endl;
    std::cout << "aie_metadata.driver_config.column_shift: " << (int)dconfig.column_shift << std::endl;
    std::cout << "aie_metadata.driver_config.row_shift: " << (int)dconfig.row_shift << std::endl;
    std::cout << "aie_metadata.driver_config.num_columns: " << (int)dconfig.num_columns << std::endl;
    std::cout << "aie_metadata.driver_config.num_rows: " << (int)dconfig.num_rows << std::endl;
    std::cout << "aie_metadata.driver_config.shim_row: " << (int)dconfig.shim_row << std::endl;
    std::cout << "aie_metadata.driver_config.mem_row_start: " << (int)dconfig.mem_row_start << std::endl;
    std::cout << "aie_metadata.driver_config.mem_num_rows: " << (int)dconfig.mem_num_rows << std::endl;
    std::cout << "aie_metadata.driver_config.aie_tile_row_start: " << (int)dconfig.aie_tile_row_start << std::endl;
    std::cout << "aie_metadata.driver_config.aie_tile_num_rows: " << (int)dconfig.aie_tile_num_rows << std::endl;
    std::cout << "aie_metadata.driver_config.partition_num_cols: " << (int)dconfig.partition_num_cols << std::endl;

    std::cout << "aie_metadata.driver_config.partition_overlay_start_cols: ";
    for (const short &value : dconfig.partition_overlay_start_cols) {
        std::cout << value << " ";
    }

    // Print partitions information if available
    std::cout << std::endl;
    std::cout << "Partitions.size()" << partitions.size() << std::endl;
    for (size_t i = 0; i < partitions.size(); i++) {
        std::cout << "Partitions[" << i << "].startColumn: " << partitions[i].startColumn << std::endl;
        std::cout << "Partitions[" << i << "].numColumns: " << partitions[i].numColumns << std::endl;
        std::cout << "Partitions[" << i << "].uuid: " << partitions[i].uuid << std::endl;
        std::cout << "Partitions[" << i << "].aie_pl_intf_id: " << partitions[i].aie_pl_intf_id << std::endl;
        std::cout << "Partitions[" << i << "].namespace: " << partitions[i].name_space << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Driver Config Ends" << std::endl;
}

void ConfigureParser::printGraphConfig() {
    std::cout << "Graph Config Starts: " << std::endl;
    for (const auto &data_pair : gconfigs) {
        const std::string &name = data_pair.first;
        const abr::graph_config &grconfig = data_pair.second;
        std::cout << "Graph ID " << grconfig.id << " values: " << std::endl;
        printgraphconfig(grconfig, grconfig.id);
    }
    std::cout << "Graph Config ends" << std::endl; // Indicate the end of details
}

void ConfigureParser::printGMIOsConfig() {
    std::cout << "GMIOs Config Starts: " << std::endl;
    for (const auto &data_pair : gmioconfigs) {
        const std::string &name = data_pair.first;
        const abr::gmio_config &gmconfig = data_pair.second;
        std::cout << "GMIO type " << gmconfig.type << " values: " << std::endl;
        printgmioconfig(gmconfig, gmconfig.id);
    }
    std::cout << "GMIOs Config ends" << std::endl;
}

void ConfigureParser::printPLIOsConfig() {
    std::cout << "PLIOs Config Starts: " << std::endl;
    for (const auto &data_pair : plioconfigs) {
        const std::string &name = data_pair.first;
        const abr::plio_config &plconfig = data_pair.second;
        std::cout << "PLIO slaveOrMaster " << plconfig.slaveOrMaster << " values: " << std::endl;
        printplioconfig(plconfig, plconfig.id);
    }
    std::cout << "PLIOs Config ends" << std::endl;
}

void ConfigureParser::printExternalbufferConfig() {
    std::cout << "External Buffers Config Starts: " << std::endl;
    for (const auto &data_pair : externalbufferconfigs) {
        const std::string &name = data_pair.first;
        const abr::external_buffer_config &ebufferconfig = data_pair.second;
        printexternalbufferconfig(ebufferconfig, ebufferconfig.id);
    }
    std::cout << "External Buffers Config ends" << std::endl;
}

void ConfigureParser::printSharedBufferConfig() {
    std::cout << "Shared Buffer Config Starts: " << std::endl;
    for (const auto &data_pair : sharedbufferconfigs) {
        const std::string &name = data_pair.first;
        const abr::shared_buffer_config &shared_buffer_config = data_pair.second;
        std::cout << "Shared Buffer port name " << shared_buffer_config.name << " values: " << std::endl;
        printsharedbufferconfig(shared_buffer_config, shared_buffer_config.id);
    }
    std::cout << "Shared Buffer Config ends" << std::endl;
}

void ConfigureParser::printRTPsConfig() {
    std::cout << "RTPs Config Starts: " << std::endl;
    for (const auto &data_pair : rtpconfigs) {
        const std::string &name = data_pair.first;
        const abr::rtp_config &rtpconfig = data_pair.second;
        std::cout << "RTP port name " << rtpconfig.portName << " values: " << std::endl;
        printrtpconfig(rtpconfig, rtpconfig.portId);
    }
    std::cout << "RTPs Config ends" << std::endl;
}

void ConfigureParser::printgraphconfig(const abr::graph_config &grconfig, int number) {
    std::cout << "aie_metadata.graph_config." << number << ".id: " << grconfig.id << std::endl;
    std::cout << "aie_metadata.graph_config." << number << ".name: " << grconfig.name << std::endl;

    std::cout << "aie_metadata.graph_config." << number << ".coreColumns: ";
    for (const short &value : grconfig.coreColumns) {
        std::cout << value << " ";
    }
    std::cout << std::endl;

    std::cout << "aie_metadata.graph_config." << number << ".coreRows: ";
    for (const short &value : grconfig.coreRows) {
        std::cout << value << " ";
    }
    std::cout << std::endl;

    std::cout << "aie_metadata.graph_config." << number << ".iterMemColumns: ";
    for (const short &value : grconfig.iterMemColumns) {
        std::cout << value << " ";
    }
    std::cout << std::endl;

    std::cout << "aie_metadata.graph_config." << number << ".iterMemRows: ";
    for (const short &value : grconfig.iterMemRows) {
        std::cout << value << " ";
    }
    std::cout << std::endl;

    std::cout << "aie_metadata.graph_config." << number << ".iterMemAddrs: ";
    for (const size_t &value : grconfig.iterMemAddrs) {
        std::cout << value << " ";
    }
    std::cout << std::endl;

    std::cout << "aie_metadata.graph_config." << number << ".triggered: ";
    for (const bool &value : grconfig.triggered) {
        std::cout << value << " ";
    }
    std::cout << std::endl;

    std::cout << "aie_metadata.graph_config." << number << ".broadcast_column: " << grconfig.broadcast_column
              << std::endl;
}

void ConfigureParser::printgmioconfig(const abr::gmio_config &gm_config, int number) {
    std::cout << "aie_metadata.gmio_config." << number << ".id: " << gm_config.id << std::endl;
    std::cout << "aie_metadata.gmio_config." << number << ".name: " << gm_config.name << std::endl;
    std::cout << "aie_metadata.gmio_config." << number << ".logicalName: " << gm_config.logicalName << std::endl;
    std::cout << "aie_metadata.gmio_config." << number << ".type: " << gm_config.type << std::endl;
    std::cout << "aie_metadata.gmio_config." << number << ".shimColumn: " << gm_config.shimColumn << std::endl;
    std::cout << "aie_metadata.gmio_config." << number << ".channelNum: " << gm_config.channelNum << std::endl;
    std::cout << "aie_metadata.gmio_config." << number << ".streamId: " << gm_config.streamId << std::endl;
    std::cout << "aie_metadata.gmio_config." << number << ".burstLength: " << gm_config.burstLength << std::endl;
}

void ConfigureParser::printplioconfig(const abr::plio_config &pl_config, int number) {
    std::cout << "aie_metadata.plio_config." << number << ".id: " << pl_config.id << std::endl;
    std::cout << "aie_metadata.plio_config." << number << ".name: " << pl_config.name << std::endl;
    std::cout << "aie_metadata.plio_config." << number << ".logicalName: " << pl_config.logicalName << std::endl;
    std::cout << "aie_metadata.plio_config." << number << ".shimColumn: " << pl_config.shimColumn << std::endl;
    std::cout << "aie_metadata.plio_config." << number << ".slaveOrMaster: " << pl_config.slaveOrMaster << std::endl;
    std::cout << "aie_metadata.plio_config." << number << ".streamId: " << pl_config.streamId << std::endl;
}

void ConfigureParser::printshimbdinfoconfig(const abr::shimBDInfo &shim_bd_info_config, int number,
                                            std::string prefix) {
    std::cout << prefix << "shim_bd_info_config" << number << ".bd_id: " << shim_bd_info_config.bd_id << std::endl;
    std::cout << prefix << "shim_bd_info_config" << number << ".buf_idx: " << shim_bd_info_config.buf_idx << std::endl;
    std::cout << prefix << "shim_bd_info_config" << number << ".offset: " << shim_bd_info_config.offset << std::endl;
    std::cout << prefix << "shim_bd_info_config" << number
              << ".transaction_size: " << shim_bd_info_config.transaction_size << std::endl;
}

void ConfigureParser::printshimportconfig(const abr::shim_port_config &shim_port_config, int number,
                                          std::string prefix) {
    std::cout << prefix << "shim_port_config" << number << ".portId: " << shim_port_config.portId << std::endl;
    std::cout << prefix << "shim_port_config" << number << ".portName: " << shim_port_config.portName << std::endl;
    std::cout << prefix << "shim_port_config" << number << ".column: " << shim_port_config.column << std::endl;
    std::cout << prefix << "shim_port_config" << number << ".S2MMOrMM2S: " << shim_port_config.S2MMOrMM2S << std::endl;
    std::cout << prefix << "shim_port_config" << number << ".channel: " << shim_port_config.channel << std::endl;
    std::cout << prefix << "shim_port_config" << number << ".taskRepetition: " << shim_port_config.taskRepetition
              << std::endl;
    std::cout << prefix << "shim_port_config" << number
              << ".enable_task_complete_token: " << shim_port_config.enable_task_complete_token << std::endl;

    int i = 0;
    for (auto shim_bd_info : shim_port_config.shimBDInfos) {
        printshimbdinfoconfig(shim_bd_info, i, prefix + "shim_port_config");
        i++;
    }
}

void ConfigureParser::printexternalbufferconfig(const abr::external_buffer_config &ebuffer_config, int number) {
    std::cout << "external_buffer_config" << number << ".id: " << ebuffer_config.id << std::endl;
    std::cout << "external_buffer_config" << number << ".name: " << ebuffer_config.name << std::endl;

    int i = 0;
    std::string prefix = "external_buffer_config.shim_port_config";
    for (auto shim_port_config : ebuffer_config.shimPortConfigs) {
        printshimportconfig(shim_port_config, i, prefix);
        i++;
    }
}

void ConfigureParser::printsharedbufferconfig(const abr::shared_buffer_config &sbuffer_config, int number) {
    std::cout << "shared_buffer_config" << number << ".id: " << sbuffer_config.id << std::endl;
    std::cout << "shared_buffer_config" << number << ".name: " << sbuffer_config.name << std::endl;
    std::cout << "shared_buffer_config" << number << ".graphId: " << sbuffer_config.graphId << std::endl;
    std::cout << "shared_buffer_config" << number << ".numBytes: " << sbuffer_config.numBytes << std::endl;
    std::cout << "shared_buffer_config" << number << ".column: " << sbuffer_config.column << std::endl;
    std::cout << "shared_buffer_config" << number << ".row: " << sbuffer_config.row << std::endl;
    std::cout << "shared_buffer_config" << number << ".numInputs: " << sbuffer_config.numInputs << std::endl;
    std::cout << "shared_buffer_config" << number << ".numOutputs: " << sbuffer_config.numOutputs << std::endl;
    std::cout << "shared_buffer_config" << number << ".elemType: " << sbuffer_config.elemType << std::endl;
    std::cout << "shared_buffer_config" << number << ".initialized: " << sbuffer_config.initialized << std::endl;

    std::cout << "shared_buffer_config" << number << ".addr: ";
    for (auto a : sbuffer_config.addr) {
        std::cout << a << " ";
    }
    std::cout << std::endl;

    std::cout << "shared_buffer_config" << number << ".producerLocks: ";
    for (auto pl : sbuffer_config.producerLocks) {
        std::cout << pl << " ";
    }
    std::cout << std::endl;

    std::cout << "shared_buffer_config" << number << ".consumerLocks: ";
    for (auto cl : sbuffer_config.consumerLocks) {
        std::cout << cl << " ";
    }
    std::cout << std::endl;
}

void ConfigureParser::printrtpconfig(const abr::rtp_config &rt_config, int number) {
    std::cout << "aie_metadata.rtp_config." << number << ".port_id: " << rt_config.portId << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".alias_id: " << rt_config.aliasId << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".port_name: " << rt_config.portName << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".alias_name: " << rt_config.aliasName << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".graph_id: " << rt_config.graphId << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".is_input: " << rt_config.isInput << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".is_asynchronous: " << rt_config.isAsync << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".is_connected: " << rt_config.isConnect << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".number_of_bytes: " << rt_config.numBytes << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".is_PL_RTP: " << rt_config.isPL << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".requires_lock: " << rt_config.hasLock << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".selector_column: " << rt_config.selectorColumn << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".selector_row: " << rt_config.selectorRow << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".selector_address: " << rt_config.selectorAddr << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".selector_lock_id: " << rt_config.selectorLockId << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".ping_buffer_column: " << rt_config.pingColumn << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".ping_buffer_row: " << rt_config.pingRow << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".ping_buffer_address: " << rt_config.pingAddr << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".ping_buffer_lock_id: " << rt_config.pingLockId << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".pong_buffer_column: " << rt_config.pongColumn << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".pong_buffer_row: " << rt_config.pongRow << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".pong_buffer_address: " << rt_config.pongAddr << std::endl;
    std::cout << "aie_metadata.rtp_config." << number << ".pong_buffer_lock_id: " << rt_config.pongLockId << std::endl;
}

ConfigureParser::ConfigureParser() {
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

ConfigureParser::~ConfigureParser() {};

abr::driver_config &ConfigureParser::get_driver_config() { return dconfig; }

abr::aiecompiler_config &ConfigureParser::get_aiecompiler_config() { return aiecompilerconfig; }

abr::graph_config *ConfigureParser::get_graph_config(std::string graph_name) {
    auto it = gconfigs.find(graph_name);
    if (it == gconfigs.end()) {
        AEG_ERROR("%s: graph '%s' does not exist", __func__, graph_name.c_str());
        return nullptr;
    }
    return &it->second;
}

abr::gmio_config *ConfigureParser::get_gmio_config(std::string gmio_name) {
    auto it = gmioconfigs.find(gmio_name);
    if (it == gmioconfigs.end()) {
        AEG_ERROR("%s: gmio '%s' does not exist", __func__, gmio_name.c_str());
        return nullptr;
    }
    return &it->second;
}

abr::plio_config *ConfigureParser::get_plio_config_logicname(std::string plio_name) {
    auto it = std::find_if(plioconfigs.begin(), plioconfigs.end(),
                           [&plio_name](auto &it) { return it.second.logicalName.compare(plio_name) == 0; });
    if (it == plioconfigs.end()) {
        AEG_ERROR("%s: plio (logical) '%s' does not exist", __func__, plio_name.c_str());
        return nullptr;
    }
    return &it->second;
}

abr::plio_config *ConfigureParser::get_plio_config(std::string plio_name) {
    auto it = plioconfigs.find(plio_name);
    if (it == plioconfigs.end()) {
        AEG_ERROR("%s: plio '%s' does not exist", __func__, plio_name.c_str());
        return nullptr;
    }
    return &it->second;
}

abr::external_buffer_config *ConfigureParser::get_externalbuffer_config(std::string ebuffer_name) {
    auto it = externalbufferconfigs.find(ebuffer_name);
    if (it == externalbufferconfigs.end()) {
        AEG_ERROR("%s: external buffer '%s' does not exist", __func__, ebuffer_name.c_str());
        return nullptr;
    }
    return &it->second;
}

abr::shared_buffer_config *ConfigureParser::get_shared_buffer_config(std::string shared_buffer_name) {
    auto it = sharedbufferconfigs.find(shared_buffer_name);
    AEG_LOG("%zu shared buffers found\n", sharedbufferconfigs.size());
    for (const auto &pair : sharedbufferconfigs) {
        AEG_LOG("Shared buffer: %s\n", pair.first.c_str());
    }
    if (it == sharedbufferconfigs.end()) {
        AEG_ERROR("%s: shared buffer '%s' does not exist", __func__, shared_buffer_name.c_str());
        return nullptr;
    }
    return &it->second;
}

abr::rtp_config *ConfigureParser::get_rtp_config(std::string rtp_name) {
    auto it = rtpconfigs.find(rtp_name);
    if (it == rtpconfigs.end()) {
        AEG_ERROR("%s: RTP '%s' does not exist", __func__, rtp_name.c_str());
        return nullptr;
    }
    return &it->second;
}

int ConfigureParser::isRTPorSharedBuffer(std::string name) // 0-RTP, 1- shared buffer
{
    if (sharedbufferconfigs.find(name) != sharedbufferconfigs.end())
        return 1;
    if (rtpconfigs.find(name) != rtpconfigs.end())
        return 0;
    return -1;
}

std::shared_ptr<GmioApiImpl> ConfigureParser::get_gmio_api_impl(std::string gmio_name) {
    auto it = gmio_api_impls.find(gmio_name);
    if (it == gmio_api_impls.end() || it->second == nullptr) {
        AEG_ERROR("%s: GMIO API impl for '%s' does not exist", __func__, gmio_name.c_str());
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<ExternalBufferApiImpl> ConfigureParser::get_externalbuffer_api_impl(std::string ebuffer_name) {
    auto it = ebuffer_api_impls.find(ebuffer_name);
    if (it == ebuffer_api_impls.end() || it->second == nullptr) {
        AEG_ERROR("%s: external buffer API impl for '%s' does not exist", __func__, ebuffer_name.c_str());
        return nullptr;
    }
    return it->second;
}

std::shared_ptr<RtpApiImpl> ConfigureParser::get_rtp_api_impl(std::string rtp_name) {
    auto &ret = rtp_api_impls[rtp_name];
    if (ret == nullptr) {
        AEG_ERROR("%s: RTP API impl for '%s' does not exist", __func__, rtp_name.c_str());
        std::abort();
    }
    return ret;
}

std::shared_ptr<SharedBufferApiImpl> ConfigureParser::get_sharedbuffer_api_impl(std::string sharedbuffer_name) {
    auto &ret = sharedbuffer_api_impls[sharedbuffer_name];
    if (ret == nullptr) {
        AEG_ERROR("%s: shared buffer API impl for '%s' does not exist", __func__, sharedbuffer_name.c_str());
        std::abort();
    }
    return ret;
}

void ConfigureParser::Load() {
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

template <typename T> T ConfigureParser::GetConfig(std::string name) {
#ifdef __AIESIM__
    return pt.get<T>(name);
#else
    T ret;
    return ret;
#endif
}

void ConfigureParser::LoadDriverConfig() {
    DriverConfigStaticParser dconfigparser;
    dconfig = dconfigparser.load(partitions);
};

void ConfigureParser::LoadAiecompilerConfig() {
    AiecompilerConfigStaticParser aiecompilerconfigparser;
    aiecompilerconfig = aiecompilerconfigparser.load();
};

void ConfigureParser::throw_if_error(bool err, const char *msg) {
    if (err)
        throw std::runtime_error(msg);
}

void ConfigureParser::LoadGraphConfig() {
    GraphConfigStaticParser grconfigMapparser;
    gconfigs = grconfigMapparser.loadAll(partitions, dconfig.partition_overlay_start_cols[0]);
}

void ConfigureParser::LoadGmios() {
    GMIOsConfigStaticParser gmconfigMapparser;
    gmioconfigs = gmconfigMapparser.loadAll();
}

void ConfigureParser::LoadPlios() {
    PLIOsConfigStaticParser plconfigMapparser;
    plioconfigs = plconfigMapparser.loadAll();
}

void ConfigureParser::LoadExternalbuffers() {
    ExternalbuffersConfigStaticParser ebufferconfigMapparser;
    externalbufferconfigs = ebufferconfigMapparser.loadAll();
}

void ConfigureParser::LoadSharedBuffers() {
    AEG_LOG("LoadSharedBuffers called\n");
    SharedBuffersConfigStaticParser sbufferconfigMapparser;
    sharedbufferconfigs = sbufferconfigMapparser.loadAll();
    AEG_LOG("LoadSharedBuffers exit\n");
}

void ConfigureParser::LoadRtps() {
    RTPsConfigStaticParser rtconfigMapparser;
    rtpconfigs = rtconfigMapparser.loadAll();
}

namespace {
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
} // namespace

aeg_err ConfigureParser::ConfigureGmios(std::shared_ptr<XAieMem> xaiemem) {
    if (!xaiemem) {
        AEG_ERROR("%s: XAieMem backend is null", __func__);
        return aeg_err::null_argument;
    }
    for (auto &item : gmioconfigs) {
        auto gmio_api = std::make_shared<abr::gmio_api>(&item.second);
        if (!gmio_api) {
            AEG_ERROR("%s: failed to allocate GMIO API for '%s'", __func__, item.first.c_str());
            return aeg_err::internal_error;
        }
        aeg_err configure_status = map_err_code(gmio_api->configure());
        if (configure_status != aeg_err::ok) {
            AEG_ERROR("%s: failed to configure GMIO '%s' status=%d", __func__, item.first.c_str(),
                      static_cast<int>(configure_status));
            return configure_status;
        }
        // new gmioapimpl class---
        auto gmio_api_impl = std::make_shared<GmioApiImpl>(gmio_api, item.second, xaiemem, item.first);
        if (!gmio_api_impl) {
            AEG_ERROR("%s: failed to allocate GMIO API impl for '%s'", __func__, item.first.c_str());
            return aeg_err::internal_error;
        }
        gmio_api_impls[item.first] = gmio_api_impl;
    }
    return aeg_err::ok;
}

aeg_err ConfigureParser::ConfigureExternalBuffers(std::shared_ptr<XAieMem> xaiemem) {
    if (!xaiemem) {
        AEG_ERROR("%s: XAieMem backend is null", __func__);
        return aeg_err::null_argument;
    }
    for (auto &item : externalbufferconfigs) {
        AEG_LOG("******Configuring external buffer*******\n");
        AEG_LOG("e buffer name : %s\n", item.first.c_str());
        auto ebuffer_api_impl = std::make_shared<ExternalBufferApiImpl>(item.second, item.first, xaiemem);
        if (!ebuffer_api_impl) {
            AEG_ERROR("%s: failed to allocate external buffer API impl for '%s'", __func__, item.first.c_str());
            return aeg_err::internal_error;
        }
        ebuffer_api_impls[item.first] = ebuffer_api_impl;
    }
    return aeg_err::ok;
}

aeg_err ConfigureParser::ConfigureRtps(std::shared_ptr<abr::graph_api> graphApi) {
    if (!graphApi) {
        AEG_ERROR("%s: graph API backend is null", __func__);
        return aeg_err::null_argument;
    }
    for (auto &item : rtpconfigs) {
        auto rtp_api_impl = std::make_shared<RtpApiImpl>(item.second, graphApi, item.first);
        if (!rtp_api_impl) {
            AEG_ERROR("%s: failed to allocate RTP API impl for '%s'", __func__, item.first.c_str());
            return aeg_err::internal_error;
        }
        rtp_api_impls[item.first] = rtp_api_impl;
    }
    return aeg_err::ok;
}

aeg_err ConfigureParser::ConfigureSharedBuffers(std::shared_ptr<abr::graph_api> graphApi) {
    if (!graphApi) {
        AEG_ERROR("%s: graph API backend is null", __func__);
        return aeg_err::null_argument;
    }
    for (auto &item : sharedbufferconfigs) {
        auto sharedbuffer_api_impl = std::make_shared<SharedBufferApiImpl>(item.second, graphApi, item.first);
        if (!sharedbuffer_api_impl) {
            AEG_ERROR("%s: failed to allocate shared buffer API impl for '%s'", __func__, item.first.c_str());
            return aeg_err::internal_error;
        }
        sharedbuffer_api_impls[item.first] = sharedbuffer_api_impl;
    }
    return aeg_err::ok;
}
