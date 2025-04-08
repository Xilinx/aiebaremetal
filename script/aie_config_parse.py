# Copyright (C) 2025 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may
# not use this file except in compliance with the License. A copy of the
# License is located at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

import json
import os, sys
import struct
import logging
from collections import OrderedDict

logging.basicConfig(level=logging.INFO, format='%(levelname)s: %(message)s')

AIEMETADATA = "aie_metadata"
DRIVERCONFIG = "driver_config"
HWCONFIGSLIST = ["hw_gen", "base_address", "column_shift", "row_shift", 
                 "num_columns", "num_rows", "shim_row", "mem_tile_row_start", 
                 "mem_tile_num_rows", "aie_tile_row_start", "aie_tile_num_rows",
                 "partition_num_cols", "partition_overlay_start_cols"]

GRAPHSCONFIG = "graphs"
#GRAPHS = ["graph0","graph1"]
GRAPHS = []
GRAPHCONFIGLIST = ["id", "name", "core_columns", "core_rows", "iteration_memory_columns",
                   "iteration_memory_rows", "iteration_memory_addresses", "multirate_triggers"]

GMIOCONFIG = "GMIOs"
#GMIOLIST = ['gmio0', 'gmio1','gmio2', 'gmio3', 'gmio4', 'gmio5']
GMIOLIST = []
GMIOCONFIGLIST = ["id", "name", "logical_name", "type", "shim_column", "channel_number", "stream_id",
                  "burst_length_in_16byte"]

EXTERNALBUFFERCONFIG = "ExternalBufferConfigs"
EXTERNALBUFFERLIST = []
EXTERNALBUFFERCONFIGLIST = [ "id","name","shimPortConfigs"]

SHIMPORTCONFIGLIST = [ "portId","portName","direction","shim_column","channel_number",
                        "task_repetition","enable_task_complete_token","shimBDInfos"]

SHIMBDINFOLIST = ["bd_id","buf_idx","offset","transaction_size"]

RTPCONFIG = "RTPs"
RTPLIST = []
RTPCONFIGLIST = ["port_id", "alias_id", "port_name", "alias_name", "graph_id", "is_input", "is_asynchronous",  "is_connected",
                "number_of_bytes", "is_PL_RTP", "requires_lock", "selector_column", "selector_row", "selector_address","selector_lock_id",
                "ping_buffer_column", "ping_buffer_row", "ping_buffer_address", "ping_buffer_lock_id", "pong_buffer_column", "pong_buffer_row",
                "pong_buffer_address", "pong_buffer_lock_id"]

VECTORS = ["partition_overlay_start_cols", "core_columns", "core_rows", 
          "iteration_memory_columns", "iteration_memory_rows", "multirate_triggers",
          "shimPortConfigs","shimBDInfos"]

ENCODINGTOKEYMAP = {
    "B":["enable_task_complete_token","hw_gen", "column_shift", "row_shift", 
        "num_columns", "num_rows", "shim_row", "mem_tile_row_start", 
        "mem_tile_num_rows", "aie_tile_row_start", "aie_tile_num_rows",
        "partition_num_cols", "is_input", "is_asynchronous", "is_connected", "is_PL_RTP",
        "requires_lock", ],
    "i": ["bd_id","buf_idx","offset","task_repetition","direction","portId","id","port_id", "alias_id", "graph_id", "number_of_bytes", "selector_address",
        "ping_buffer_address", "pong_buffer_address"],
    "h": ["partition_overlay_start_cols","core_columns", "core_rows", 
          "iteration_memory_columns", "iteration_memory_rows", "type",
          "shim_column", "channel_number", "stream_id", "burst_length_in_16byte",
          "selector_column", "selector_row", "selector_lock_id","ping_buffer_column",
          "ping_buffer_row", "ping_buffer_lock_id", "pong_buffer_column", "pong_buffer_row",
          "pong_buffer_lock_id","eid", "shim_column","channel_number"],
    "Q": ["transaction_size","base_address"],
    "s": ["portName","name", "logical_name", "port_name", "alias_name", ],
    "I": ["iteration_memory_addresses"],
    "?": ["multirate_triggers"],
    "P":["shimPortConfigs"],
    "D":["shimBDInfos"]
}

def _invert_dict(inp_dict):
    inv_dict = dict()
    for k, v in inp_dict.items():
        for item in v:
            if item not in inv_dict:
                inv_dict[item] = k
    return inv_dict

KEYTOENCODINGMAP = _invert_dict(ENCODINGTOKEYMAP)

'''
Code to parse driver config from json
returns: list_data, format

'''

def ParseDriverConfig(jobj):

    format_string = "<" #force little endian
    config_data = OrderedDict()

    aiemetadata = jobj.get(AIEMETADATA)
    driverdata = aiemetadata.get(DRIVERCONFIG)

    for key in HWCONFIGSLIST:
        if key in VECTORS:
            values = driverdata.get(key, [0,1,2,3])
        else:
            values = driverdata.get(key, 0)

        key_n = f'{DRIVERCONFIG}.{key}'       
        if type(values) is list:
            config_data[f'{key_n}.len'] = len(values) #add len of vector
            format_string+="i"
            for i, value in enumerate(values):
                key_n = f'{key_n}.{i}'
                config_data[key_n] = value
                format_string+= KEYTOENCODINGMAP[key]
        else:
            config_data[key_n] = values
            format_string += KEYTOENCODINGMAP[key]
                    
    packed_data = struct.pack(format_string, *config_data.values())

    with open("./build/driverconfig.bin", "wb") as binary_file:
        binary_file.write(packed_data)

'''
Code to parse graph config from json
returns: list_data, format

'''

def ParseGraphConfig(jobj):

    format_string = "<" #force little endian
    config_data = OrderedDict()

    aiemetadata = jobj.get(AIEMETADATA)
    graphdata = aiemetadata.get(GRAPHSCONFIG)

    for graph in GRAPHS:
        graph_config = graphdata.get(graph)
        for key in GRAPHCONFIGLIST:
            if key in VECTORS:
                values = graph_config.get(key, [0,1,2,3])
            elif key in ENCODINGTOKEYMAP["s"]:
                value = graph_config.get(key, "")
            else:
                values = graph_config.get(key, 0)

            key_n = f'{graph}.{key}'
            if type(values) is list:
                config_data[f'{key_n}.len'] = len(values) #add len of vector
                format_string+="i"
                for i, value in enumerate(values):
                    key_n = f'{key_n}.{i}'
                    config_data[key_n] = value
                    format_string+= KEYTOENCODINGMAP[key]
            elif key in ENCODINGTOKEYMAP["s"]:
                values = str(value)
                len_s = min(len(values), 31)

                config_data[f'{key_n}.len'] = len(values) #add len of string
                format_string+="i"

                config_data[key_n] = values[:len_s].encode('utf-8') #encode string
                format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
            else:
                config_data[key_n] = values
                format_string += KEYTOENCODINGMAP[key]

    packed_data = struct.pack(format_string, *config_data.values())

    with open("./build/graphconfig.bin", "wb") as binary_file:
        binary_file.write(packed_data)

'''
Code to parse graph config from json
returns: list_data, format

'''

def ParseGmiosConfig(jobj):

    format_string = "<" #force little endian
    config_data = OrderedDict()

    aiemetadata = jobj.get(AIEMETADATA)
    gmiodata = aiemetadata.get(GMIOCONFIG)

    for gmio in GMIOLIST:
        gmio_config = gmiodata.get(gmio)
        if not gmio_config:
            break
        for key in GMIOCONFIGLIST:
            if key in VECTORS:
                values = gmio_config.get(key, [0,1,2,3])
            elif key in ENCODINGTOKEYMAP["s"]:
                value = gmio_config.get(key, "")
            else:
                values = gmio_config.get(key, 0)

            key_n = f'{gmio}.{key}'
            if type(values) is list:
                config_data[f'{key_n}.len'] = len(values) #add len of vector
                format_string+="i"
                for i, value in enumerate(values):
                    key_n = f'{key_n}.{i}'
                    config_data[key_n] = value
                    format_string+= KEYTOENCODINGMAP[key]
            elif key in ENCODINGTOKEYMAP["s"]:
                values = str(value)
                len_s = min(len(values), 31)

                config_data[f'{key_n}.len'] = len(values) #add len of string
                format_string+="i"

                config_data[key_n] = values[:len_s].encode('utf-8') #encode string
                format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
            else:
                config_data[key_n] = values
                format_string += KEYTOENCODINGMAP[key]

    packed_data = struct.pack(format_string, *config_data.values())

    with open("./build/gmiosconfig.bin", "wb") as binary_file:
        binary_file.write(packed_data)

'''gets all the driver info and saves as binary'''

'''
Code to parse external buffer config from json
returns: list_data, format

'''

def pack_shim_bd_info(shim_bd_info):
    return struct.pack('<IIIQ', shim_bd_info["bd_id"], shim_bd_info["buf_idx"], shim_bd_info["offset"], shim_bd_info["transaction_size"])

def pack_shim_port_config(shim_port_config):
    format_string = "<"
    config_data = OrderedDict()
    
    for key, value in shim_port_config.items():
       
        print(key, value)
        if key in VECTORS:
            #print("shim bd infos")
            format_string+="i"
            config_data[f'{key}_len'] = len(shim_port_config["shimBDInfos"])
        elif key in ENCODINGTOKEYMAP["s"]:
            values = str(value)
            len_s = min(len(values), 31)
            format_string+="i"
            config_data[f'{key}_len'] = len_s

            config_data[key] = values[:len_s].encode('utf-8') #encode string
            format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
        else:
            if(value == "mm2s"): 
                value = 1
            if(value == "s2mm"): 
                value = 0
            format_string += KEYTOENCODINGMAP[key]
            config_data[key] = value

    packed_shim_bd_info = b''.join(pack_shim_bd_info(shim_bd_info) for shim_bd_info in shim_port_config["shimBDInfos"])
    print(format_string)
    #print(*config_data.values())
    return struct.pack(format_string, *config_data.values()) + packed_shim_bd_info

def pack_external_buffer_config(external_buffer_config):
   
    format_string = "<"
    config_data = OrderedDict()
    
    for key, value in external_buffer_config.items():
        #print(key, value)
        if key in VECTORS:
            #print("shim port configs")
            format_string+="i"
            config_data[f'{key}_len'] = len(external_buffer_config["shimPortConfigs"])
        elif key in ENCODINGTOKEYMAP["s"]:
            values = str(value)
            len_s = min(len(value), 31)
            format_string+="I"
            config_data[f'{key}_len'] = len_s

            config_data[key] = values[:len_s].encode('utf-8') #encode string
            format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
        else:
            if key == "id": key = "eid"
            format_string += KEYTOENCODINGMAP[key]
            config_data[key] = value

    print(format_string)
    print(config_data.values())
    packed_shim_port_configs = b''.join(pack_shim_port_config(shim_port_config) for shim_port_config in external_buffer_config["shimPortConfigs"])
    return struct.pack(format_string, *config_data.values()) + packed_shim_port_configs
   

def ParseExternalbuffersConfig(jobj):
    aiemetadata = jobj.get(AIEMETADATA)
    externalbufferdata = aiemetadata.get(EXTERNALBUFFERCONFIG)
    packed_data = b""
    if externalbufferdata is not None:
        for i, externalbuffer_config in enumerate(externalbufferdata):
            print("************External Buffer Config Test****************")
            packed_data += pack_external_buffer_config(externalbuffer_config)

    
    with open("./build/externalbuffersconfig.bin", "wb") as binary_file:
        binary_file.write(packed_data)


'''gets all the driver info and saves as binary'''


def ParseRtpsConfig(jobj):

    format_string = "<" #force little endian
    config_data = OrderedDict()

    aiemetadata = jobj.get(AIEMETADATA)
    rtpdata = aiemetadata.get(RTPCONFIG)
    if rtpdata is not None:
        for rtp in RTPLIST:
            rtp_config = rtpdata.get(rtp)
            if rtp_config is not None:
                for key in RTPCONFIGLIST:
                    if key in VECTORS:
                        values = rtp_config.get(key, [0,1,2,3])
                    elif key in ENCODINGTOKEYMAP["s"]:
                        value = rtp_config.get(key, "")
                    else:
                        values = rtp_config.get(key, 0)
                    
                    key_n = f'{rtp}.{key}'
                    if type(values) is list:
                        config_data[f'{key_n}.len'] = len(values) #add len of vector
                        format_string+="i"
                        for i, value in enumerate(values):
                            key_n = f'{key_n}.{i}'
                            config_data[key_n] = value
                            format_string+= KEYTOENCODINGMAP[key]
                    elif key in ENCODINGTOKEYMAP["s"]:
                        values = str(value)
                        len_s = min(len(values), 31)

                        config_data[f'{key_n}.len'] = len(values) #add len of string
                        format_string+="i"

                        config_data[key_n] = values[:len_s].encode('utf-8') #encode string
                        format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
                    else:
                        config_data[key_n] = values
                        format_string += KEYTOENCODINGMAP[key]

    packed_data = struct.pack(format_string, *config_data.values())

    with open("./build/rtpsconfig.bin", "wb") as binary_file:
        binary_file.write(packed_data)

def SaveDriverConfigsToBin(jobj):
    ParseDriverConfig(jobj)
    ParseGraphConfig(jobj)
    ParseGmiosConfig(jobj)
    ParseExternalbuffersConfig(jobj)
    ParseRtpsConfig(jobj)
    #ParseExternalbufferConfig(jobj)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        logging.error("Usage: python ./src/script/aie_config_parse.py <config_target> <kernel work folder>")
        sys.exit(1)

    work_folder = sys.argv[2]#os.environ.get('AIE_COMPILE_WORK')
    json_file = f'{work_folder}/ps/c_rts/aie_control_config.json'
    with open(json_file, 'r') as json_file:
        data = json.load(json_file, object_pairs_hook=OrderedDict)
  
    aiemetadata = data.get(AIEMETADATA)
    graphdata = aiemetadata.get(GRAPHSCONFIG)
    gmiodata = aiemetadata.get(GMIOCONFIG)
    rtpdata = aiemetadata.get(RTPCONFIG)
    externalbufferdata = aiemetadata.get(EXTERNALBUFFERCONFIG)
    
    if externalbufferdata:
        for i in range(len(externalbufferdata) - 1):
            EXTERNALBUFFERLIST.append(f'ebuffer_{i}')

    for key, value in graphdata.items():
        GRAPHS.append(key)

    for key, value in gmiodata.items():
        GMIOLIST.append(key)
    
    for key, value in rtpdata.items():
        RTPLIST.append(key)
      
    config_target = sys.argv[1]

    if config_target == "driverconfig.bin":
        ParseDriverConfig(data)
    elif config_target == "graphconfig.bin":
        ParseGraphConfig(data)
    elif config_target == "gmiosconfig.bin":
        ParseGmiosConfig(data)
    elif config_target == "rtpsconfig.bin":
        ParseRtpsConfig(data)
    elif config_target == "externalbuffersconfig.bin":
        ParseExternalbuffersConfig(data)
    elif config_target == "all":
        SaveDriverConfigsToBin(data)
    else:
        logging.error(f"Unknown configuration file: {config_target}, use all to get all binaries.")
