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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace abr {

struct driver_config {
    uint8_t hw_gen;
    uint64_t base_address;
    uint8_t column_shift;
    uint8_t row_shift;
    uint8_t num_columns;
    uint8_t num_rows;
    uint8_t shim_row;
    uint8_t mem_row_start;
    uint8_t mem_num_rows;
    uint8_t aie_tile_row_start;
    uint8_t aie_tile_num_rows;
    uint8_t partition_num_cols;
    std::vector<short> partition_overlay_start_cols;
};

struct aiecompiler_options {
    bool broadcast_enable_core;
    std::string event_trace;
};

struct graph_config {
    int id;
    std::string name;
    std::vector<short> coreColumns;
    std::vector<short> coreRows;
    /// Core iteration memory address
    std::vector<short> iterMemColumns;
    std::vector<short> iterMemRows;
    std::vector<size_t> iterMemAddrs;
    std::vector<bool> triggered;
    uint32_t broadcast_column;
};

struct aiecompiler_config {
    bool broadcast_enable_core;
    bool graph_iterator_event;
    bool enable_multi_layer;
};

struct rtp_config {
    enum elementType {
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

    int portId;
    int aliasId;
    std::string portName;
    std::string aliasName;
    int graphId;
    bool isInput;
    bool isAsync;
    bool isConnect;
    size_t numBytes;
    bool isPL;
    // for graph::update to connected async input RTP, if the connection is within a core, there may not be a lock
    bool hasLock;
    short selectorColumn;
    short selectorRow;
    size_t selectorAddr;
    unsigned short selectorLockId;
    short pingColumn;
    short pingRow;
    size_t pingAddr;
    unsigned short pingLockId;
    short pongColumn;
    short pongRow;
    size_t pongAddr;
    unsigned short pongLockId;
};

struct shared_buffer_config {
    /// Shared buffer instance ID
    short id;
    /// Shared buffer name
    std::string name;
    /// Graph id
    int graphId;
    /// Shared buffer size per buffer
    size_t numBytes;
    /// Shared buffer column
    short column;
    /// Shared buffer row
    short row;
    /// Shared buffer number of input ports
    short numInputs;
    /// Shared buffer number of output ports
    short numOutputs;
    /// Shared buffer data type
    rtp_config::elementType elemType;
    /// Shared buffer initial_value constraint specified
    bool initialized;
    std::vector<size_t> addr;
    std::vector<int> producerLocks;
    std::vector<int> consumerLocks;
};

struct gmio_config {
    enum gmio_type { gm2aie, aie2gm, gm2pl, pl2gm };

    /// GMIO object id
    int id;
    /// GMIO variable name
    std::string name;
    /// GMIO loginal name
    std::string logicalName;
    /// GMIO type
    gmio_type type;
    /// Shim tile column to where the GMIO is mapped
    short shimColumn;
    /// Channel number (0-S2MM0,1-S2MM1,2-MM2S0,3-MM2S1).
    short channelNum;
    /// Shim stream switch port id (slave: gm-->me, master: me-->gm)
    short streamId;
    /// For type == gm2aie or type == aie2gm, burstLength is the burst length for the AXI-MM transfer
    /// (4 or 8 or 16 in C_RTS API). The burst length in bytes is burstLength * 16 bytes (128-bit aligned).
    /// For type == gm2pl or type == pl2gm, burstLength is the burst length in bytes.
    short burstLength;
};

struct kernel_config {
    /// Kernel object id
    int id;
    std::vector<int> hierarchicalGraphIds;
    short column;
    short row;
};

struct dma_config {
    /// DMA object
    short column;
    short row;
    std::vector<int> hierarchicalGraphIds;
    std::vector<int> channel;
};

struct plio_config {
    /// PLIO object id
    int id;
    /// PLIO variable name
    std::string name;
    /// PLIO loginal name
    std::string logicalName;
    /// Shim tile column to where the GMIO is mapped
    short shimColumn;
    /// slave or master. 0:slave, 1:master
    short slaveOrMaster;
    /// Shim stream switch port id
    short streamId;
};

struct trace_unit_config {
    /// tile column
    short column;
    /// tile row
    short row;
    /// core module 0, memory module 1, shim pl module 2
    short module;
    /// packet id
    short packetId;
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
    int portId = 0;
    /// Port name
    std::string portName = "";
    /// shim column
    short column = 0;
    // ///shim row : 0 always for now
    // short row;
    /// S2MM or MM2S. 0:S2MM, 1:MM2S
    int S2MMOrMM2S = 0;
    /// DMA channel number
    short channel = 0;
    /// Task repetition
    int taskRepetition = 0;
    /// Enable Task Complete Token
    bool enable_task_complete_token = false;
    /// BD Infos (BDs are ordered, get startBDId from first element)
    std::vector<shimBDInfo> shimBDInfos;
};

struct external_buffer_config {
    /// External buffer instance ID
    short id;
    /// External buffer name
    std::string name;
    /// Ports
    std::vector<shim_port_config> shimPortConfigs;
};

struct partition_config {
    unsigned int startColumn;
    unsigned int numColumns;
    std::string uuid;
    unsigned int aie_pl_intf_id;
    std::string name_space;
};

} // namespace abr
