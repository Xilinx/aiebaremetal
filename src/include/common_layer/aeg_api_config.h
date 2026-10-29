/*
Changes made: Added data structures to support external buffer.
-SSW AIE Team, 2024-11-12
*/

/**<!--
 * Copyright (C) 2021 Xilinx, Inc
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
#ifndef AEG_API_CONFIG_H
#define AEG_API_CONFIG_H
#include <cstdint>
#include <string_view>
#include <vector>

namespace abr {

struct driver_config {
    std::uint8_t hw_gen;
    std::uint64_t base_address;
    std::uint8_t column_shift;
    std::uint8_t row_shift;
    std::uint8_t num_columns;
    std::uint8_t num_rows;
    std::uint8_t shim_row;
    std::uint8_t mem_row_start;
    std::uint8_t mem_num_rows;
    std::uint8_t aie_tile_row_start;
    std::uint8_t aie_tile_num_rows;
    std::uint8_t partition_num_cols;
    std::vector<int16_t> partition_overlay_start_cols;

    driver_config() = default;
    driver_config(const driver_config &) = default;
    driver_config(driver_config &&) noexcept = default;
    driver_config &operator=(const driver_config &) & = default;
    driver_config &operator=(driver_config &&) & noexcept = default;
};

struct aiecompiler_options {
    bool broadcast_enable_core;
    std::string_view event_trace;
};

struct graph_config {
    int32_t id = 0;
    std::string_view name;
    std::vector<int16_t> coreColumns;
    std::vector<int16_t> coreRows;
    /// Core iteration memory address
    std::vector<int16_t> iterMemColumns;
    std::vector<int16_t> iterMemRows;
    std::vector<size_t> iterMemAddrs;
    std::vector<std::uint8_t> triggered;
    std::uint32_t broadcast_column = 0U;

    graph_config() = default;
    graph_config(const graph_config &) = default;
    graph_config(graph_config &&) noexcept = default;
    graph_config &operator=(const graph_config &) & = default;
    graph_config &operator=(graph_config &&) & noexcept = default;
};

struct aiecompiler_config {
    bool broadcast_enable_core;
    bool graph_iterator_event;
    bool enable_multi_layer;
};

struct rtp_config {
    enum elementType : uint8_t {
        et_voidty = 0,
        et_int4,
        et_int8,
        et_int16,
        et_int32,
        et_int64,
        et_uint4,
        et_uint8,
        et_uint16,
        et_uint32,
        et_uint64,
        et_cint16,
        et_cint32,
        et_float,
        et_cfloat,
        et_cbfloat16,
        et_double,
        et_bfp16ebs8,
        et_bfp16ebs16,
        et_mx9,
        et_mx4,
        et_mx6,
        et_mx6b,
        et_mxint8,
        et_mxfp4,
        et_mxfp6,
        et_mxfp8,
        et_bfloat16,
        et_float16,
        et_bfloat8,
        et_float8,
        et_float6,
        et_float4,
        et_bool,
        et_int1,
        et_int2,
        et_uint1,
        et_uint2,
    };

    int32_t portId;
    int32_t aliasId;
    std::string_view portName;
    std::string_view aliasName;
    int32_t graphId;
    bool isInput;
    bool isAsync;
    bool isConnect;
    size_t numBytes;
    bool isPL;
    // for graph::update to connected async input RTP, if the connection is within a core, there may not be a lock
    bool hasLock;
    int16_t selectorColumn;
    int16_t selectorRow;
    size_t selectorAddr;
    uint16_t selectorLockId;
    int16_t pingColumn;
    int16_t pingRow;
    size_t pingAddr;
    uint16_t pingLockId;
    int16_t pongColumn;
    int16_t pongRow;
    size_t pongAddr;
    uint16_t pongLockId;
};

struct shared_buffer_config {
    /// Shared buffer instance ID
    int16_t id;
    /// Shared buffer name
    std::string_view name;
    /// Graph id
    int32_t graphId;
    /// Shared buffer size per buffer
    std::size_t numBytes;
    /// Shared buffer column
    int16_t column;
    /// Shared buffer row
    int16_t row;
    /// Shared buffer number of input ports
    int16_t numInputs;
    /// Shared buffer number of output ports
    int16_t numOutputs;
    /// Shared buffer data type
    rtp_config::elementType elemType;
    /// Shared buffer initial_value constraint specified
    bool initialized;
    std::vector<size_t> addr;
    std::vector<int32_t> producerLocks;
    std::vector<int32_t> consumerLocks;

    shared_buffer_config() = default;
    shared_buffer_config(const shared_buffer_config &) = default;
    shared_buffer_config(shared_buffer_config &&) noexcept = default;
    shared_buffer_config &operator=(const shared_buffer_config &) & = default;
    shared_buffer_config &operator=(shared_buffer_config &&) & noexcept = default;
};

struct gmio_config {
    enum gmio_type : uint8_t { gm2aie = 0, aie2gm, gm2pl, pl2gm };

    /// GMIO object id
    int32_t id;
    /// GMIO variable name
    std::string_view name;
    /// GMIO loginal name
    std::string_view logicalName;
    /// GMIO type
    gmio_type type;
    /// Shim tile column to where the GMIO is mapped
    int16_t shimColumn;
    /// Channel number (0-S2MM0,1-S2MM1,2-MM2S0,3-MM2S1).
    int16_t channelNum;
    /// Shim stream switch port id (slave: gm-->me, master: me-->gm)
    int16_t streamId;
    /// For type == gm2aie or type == aie2gm, burstLength is the burst length for the AXI-MM transfer
    /// (4 or 8 or 16 in C_RTS API). The burst length in bytes is burstLength * 16 bytes (128-bit aligned).
    /// For type == gm2pl or type == pl2gm, burstLength is the burst length in bytes.
    int16_t burstLength;
};

struct kernel_config {
    /// Kernel object id
    int32_t id;
    std::vector<int32_t> hierarchicalGraphIds;
    int16_t column;
    int16_t row;
};

struct dma_config {
    /// DMA object
    int16_t column;
    int16_t row;
    std::vector<int32_t> hierarchicalGraphIds;
    std::vector<int32_t> channel;
};

struct plio_config {
    /// PLIO object id
    int32_t id;
    /// PLIO variable name
    std::string_view name;
    /// PLIO loginal name
    std::string_view logicalName;
    /// Shim tile column to where the GMIO is mapped
    int16_t shimColumn;
    /// slave or master. 0:slave, 1:master
    int16_t slaveOrMaster;
    /// Shim stream switch port id
    int16_t streamId;
};

struct trace_unit_config {
    /// tile column
    int16_t column;
    /// tile row
    int16_t row;
    /// core module 0, memory module 1, shim pl module 2
    int16_t module;
    /// packet id
    int16_t packetId;
};

struct shimBDInfo {
    /// BD Id
    uint32_t bd_id = 0;
    /// Buffer Idx (0:ping, 1:pong)
    uint32_t buf_idx = 0;
    /// Offset in 32bit word w.r.t. buffer starting address
    uint32_t offset = 0;
    /// Transaction Size Upper Bound
    uint64_t transaction_size = 0;
};

struct shim_port_config {
    /// Port instance id
    int32_t portId = 0;
    /// Port name
    std::string_view portName = "";
    /// shim column
    int16_t column = 0;
    /// S2MM or MM2S. 0:S2MM, 1:MM2S
    int32_t S2MMOrMM2S = 0;
    /// DMA channel number
    int16_t channel = 0;
    /// Task repetition
    int32_t taskRepetition = 0;
    /// Enable Task Complete Token
    bool enable_task_complete_token = false;
    /// BD Infos (BDs are ordered, get startBDId from first element)
    std::vector<shimBDInfo> shimBDInfos;

    shim_port_config() = default;
    shim_port_config(const shim_port_config &) = default;
    shim_port_config(shim_port_config &&) noexcept = default;
    shim_port_config &operator=(const shim_port_config &) & = default;
    shim_port_config &operator=(shim_port_config &&) & noexcept = default;
};

struct external_buffer_config {
    /// External buffer instance ID
    int16_t id = 0;
    /// External buffer name
    std::string_view name;
    /// Ports
    std::vector<shim_port_config> shimPortConfigs;

    external_buffer_config() = default;
    external_buffer_config(const external_buffer_config &) = default;
    external_buffer_config(external_buffer_config &&) noexcept = default;
    external_buffer_config &operator=(const external_buffer_config &) & = default;
    external_buffer_config &operator=(external_buffer_config &&) & noexcept = default;
};

struct partition_config {
    uint32_t startColumn;
    uint32_t numColumns;
    std::string_view uuid;
    uint32_t aie_pl_intf_id;
    std::string_view name_space;
};

} // namespace abr
#endif // AEG_API_CONFIG_H
