# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.

import json
from pathlib import Path
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
                 "partition_num_cols", "partition_overlay_start_cols", "aie_partition_json"]

AIECOMPILEROPTIONS = "aiecompiler_options"
AIECOMPILEROPTIONSLIST = ["broadcast_enable_core",
                          "graph_iterator_event",
                          "enable_multi_layer"]

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

AIE_ENGINE_LIST = ["ai_engine_0"]
PARTITIONCONFIG = [ "startColumn", "numColumns", "partitions"]
PARTITIONCONFIGLIST = ["startColumn", "numColumns", "uuid", "aie_pl_intf_id", "namespace"]

SHAREDBUFFERCONFIG = "SharedBufferConfigs"
SHAREDBUFFERLIST = []
SHAREDBUFFERCONFIGLIST = ["id", "name", "graph_id", "size", "column", "row", "num_inputs", "num_outputs", "initialized", "address", "producer_lock_ids", "consumer_lock_ids"]

EXTERNALBUFFERCONFIG = "ExternalBufferConfigs"
EXTERNALBUFFERLIST = []
EXTERNALBUFFERCONFIGLIST = [ "id","name","shimPortConfigs"]

SHIMPORTCONFIGLIST = [ "portId","portName","direction","shim_column","channel_number",
                        "task_repetition","enable_task_complete_token","shimBDInfos"]

SHIMBDINFOLIST = ["bd_id","buf_idx","offset","transaction_size"]

PLIOCONFIG = "PLIOs"
PLIOLIST = []
PLIOCONFIGLIST = ["id","name", "logical_name","shim_column","slaveOrMaster","stream_id"]

RTPCONFIG = "RTPs"
RTPLIST = []
RTPCONFIGLIST = ["port_id", "alias_id", "port_name", "alias_name", "graph_id", "is_input", "is_asynchronous",  "is_connected",
                "number_of_bytes", "is_PL_RTP", "requires_lock", "selector_column", "selector_row", "selector_address","selector_lock_id",
                "ping_buffer_column", "ping_buffer_row", "ping_buffer_address", "ping_buffer_lock_id", "pong_buffer_column", "pong_buffer_row",
                "pong_buffer_address", "pong_buffer_lock_id"]

VECTORS = ["partition_overlay_start_cols", "core_columns", "core_rows", 
          "iteration_memory_columns", "iteration_memory_rows", "multirate_triggers",
          "shimPortConfigs","shimBDInfos", "partitions", "address", "producer_lock_ids", "consumer_lock_ids"]

ENCODINGTOKEYMAP = {
    "B":["broadcast_enable_core","graph_iterator_event","enable_multi_layer","enable_task_complete_token","hw_gen", "column_shift", "row_shift", 
        "num_columns", "num_rows", "shim_row", "mem_tile_row_start", 
        "mem_tile_num_rows", "aie_tile_row_start", "aie_tile_num_rows",
        "partition_num_cols", "is_input", "is_asynchronous", "is_connected", "is_PL_RTP",
        "requires_lock", "initialized"],
    "i": ["bd_id","buf_idx","offset","task_repetition","direction","portId","id","port_id", "alias_id", "graph_id", "number_of_bytes", "selector_address",
        "ping_buffer_address", "pong_buffer_address", "producer_lock_ids", "consumer_lock_ids",],
    "h": ["partition_overlay_start_cols","core_columns", "core_rows", 
          "iteration_memory_columns", "iteration_memory_rows", "type",
          "shim_column", "channel_number", "stream_id", "burst_length_in_16byte",
          "selector_column", "selector_row", "selector_lock_id","ping_buffer_column",
          "ping_buffer_row", "ping_buffer_lock_id", "pong_buffer_column", "pong_buffer_row",
          "pong_buffer_lock_id","eid", "shim_column","channel_number","slaveOrMaster", "column", "row", 
          "sid", "num_inputs", "num_outputs"],
    "Q": ["transaction_size","base_address", "size", "address"],
    "s": ["portName","name", "logical_name", "port_name", "alias_name","uuid","namespace" ],
    "I": ["iteration_memory_addresses", "startColumn", "numColumns","aie_pl_intf_id"],
    "?": ["multirate_triggers"],
    "P":["shimPortConfigs"],
    "D":["shimBDInfos"],
    "HEX":["aie_pl_intf_id"]
}

NAME_MAX_LEN = 1024
def get_name_len(name: str) -> int:
    nlen = len(name)
    assert nlen <= NAME_MAX_LEN, f"{name} must be less than or equal to {NAME_MAX_LEN} characters"
    return nlen

def convert_if_bool(value):
    if isinstance(value, list):
        return [convert_if_bool(v) for v in value]
    elif isinstance(value, bool):
        return 'true' if value else 'false'
    else:
        return value

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
    if not aiemetadata:
        logging.error(f"Missing '{AIEMETADATA}' in JSON")
        return
    
    driverdata = aiemetadata.get(DRIVERCONFIG)
    if not driverdata:
        logging.error(f"Missing '{DRIVERCONFIG}' in aie_metadata")
        return
    
    partitions = None

    for key in HWCONFIGSLIST:
        if key == "aie_partition_json":
            aie_partition = driverdata.get(key)
            if not aie_partition:
                continue
            aie_engine = aie_partition.get("AIE")
            for aie in AIE_ENGINE_LIST:
                partition_data = aie_engine.get(aie)
                if partition_data:
                    partitions = ParseAIEPartitionConfig(partition_data)
            continue

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

    if partitions is not None:                
        packed_data = struct.pack(format_string, *config_data.values()) + partitions
    else:
        packed_data = struct.pack(format_string, *config_data.values())

    with open("./build/driverconfig.bin", "wb") as binary_file:
        binary_file.write(packed_data)

'''
Code to parse aiecompiler_options from json
returns: list_data, format

'''

def ParseAiecompilerOptions(jobj):

    format_string = "<" #force little endian
    config_data = OrderedDict()

    aiemetadata = jobj.get(AIEMETADATA)
    if not aiemetadata:
        logging.error(f"Missing '{AIEMETADATA}' in JSON")
        return
    
    aiecompilerdata = aiemetadata.get(AIECOMPILEROPTIONS)
    if not aiecompilerdata:
        logging.error(f"Missing '{AIECOMPILEROPTIONS}' in aie_metadata")
        return

    for key in AIECOMPILEROPTIONSLIST:
        value_b = aiecompilerdata.get(key, 0)
        value = 1 if value_b else 0
        format_string += KEYTOENCODINGMAP[key]
        config_data[key] = value

    packed_data = struct.pack(format_string, *config_data.values())
    with open("./build/aiecompilerconfig.bin", "wb") as binary_file:
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
                values = convert_if_bool(graph_config.get(key, [0,1,2,3]))
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
                len_s = get_name_len(values)
                

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
                len_s = get_name_len(values)

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

'''
Code to parse graph config from json
returns: list_data, format

'''

def ParsePliosConfig(jobj):

    format_string = "<" #force little endian
    config_data = OrderedDict()

    aiemetadata = jobj.get(AIEMETADATA)
    pliodata = aiemetadata.get(PLIOCONFIG)

    if pliodata is not None and pliodata != "":
        for plio in PLIOLIST:
            plio_config = pliodata.get(plio)
            if not plio_config:
                break
            for key in PLIOCONFIGLIST:
                if key in VECTORS:
                    values = plio_config.get(key, [0,1,2,3])
                elif key in ENCODINGTOKEYMAP["s"]:
                    value = plio_config.get(key, "")
                else:
                    values = plio_config.get(key, 0)

                key_n = f'{plio}.{key}'
                if type(values) is list:
                    config_data[f'{key_n}.len'] = len(values) #add len of vector
                    format_string+="i"
                    for i, value in enumerate(values):
                        key_n = f'{key_n}.{i}'
                        config_data[key_n] = value
                        format_string+= KEYTOENCODINGMAP[key]
                elif key in ENCODINGTOKEYMAP["s"]:
                    values = str(value)
                    len_s = get_name_len(values)

                    config_data[f'{key_n}.len'] = len(values) #add len of string
                    format_string+="i"

                    config_data[key_n] = values[:len_s].encode('utf-8') #encode string
                    format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
                else:
                    config_data[key_n] = values
                    format_string += KEYTOENCODINGMAP[key]

    packed_data = struct.pack(format_string, *config_data.values())

    with open("./build/pliosconfig.bin", "wb") as binary_file:
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
       
        #print(key, value)
        if key in VECTORS:
            #print("shim bd infos")
            format_string+="i"
            config_data[f'{key}_len'] = len(shim_port_config["shimBDInfos"])
        elif key in ENCODINGTOKEYMAP["s"]:
            values = str(value)
            len_s = get_name_len(values)
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
    #print(format_string)
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
            len_s = get_name_len(values)
            format_string+="I"
            config_data[f'{key}_len'] = len_s

            config_data[key] = values[:len_s].encode('utf-8') #encode string
            format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
        else:
            if key == "id": key = "eid"
            format_string += KEYTOENCODINGMAP[key]
            config_data[key] = value

    #print(format_string)
    #print(config_data.values())
    packed_shim_port_configs = b''.join(pack_shim_port_config(shim_port_config) for shim_port_config in external_buffer_config["shimPortConfigs"])
    return struct.pack(format_string, *config_data.values()) + packed_shim_port_configs
   

def ParseExternalbuffersConfig(jobj):
    aiemetadata = jobj.get(AIEMETADATA)
    externalbufferdata = aiemetadata.get(EXTERNALBUFFERCONFIG)
    packed_data = b""
    if externalbufferdata is not None:
        for i, externalbuffer_config in enumerate(externalbufferdata):
            #print("************External Buffer Config Test****************")
            packed_data += pack_external_buffer_config(externalbuffer_config)

    
    with open("./build/externalbuffersconfig.bin", "wb") as binary_file:
        binary_file.write(packed_data)

def pack_shared_buffer_config(shared_buffer_config, id_num):
    format_string = "<"
    config_data = OrderedDict()

    for key, value in shared_buffer_config.items():
        #print(key, value)
        
        if key in VECTORS:
            format_string += "i"
            l = len(shared_buffer_config[f"{key}"])
            config_data[f'{key}_len'] = l
            format_string += KEYTOENCODINGMAP[key] * l
            config_data[f'{key}'] = shared_buffer_config[f"{key}"]
        elif key in ENCODINGTOKEYMAP["s"]:
            values = str(value)
            len_s = get_name_len(values)
            format_string+="I"
            config_data[f'{key}_len'] = len_s

            config_data[key] = values[:len_s].encode('utf-8') #encode string
            format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
        else:
            if key == "id": key = "sid"
            format_string += KEYTOENCODINGMAP[key]
            config_data[key] = value

    #print(format_string)
    #print(config_data)
    #print("Shared Buffer Config is not implemented yet")
    # exit(0)
    # packed_dma_channels_config = b''.join(pack_dma_channel_config(dma_channel_config) for dma_channel_config in shared_buffer_config["dmaChannels"])
    # return struct.pack(format_string, *config_data.values()) + packed_dma_channels_config
    # Flatten any list values in config_data into the parent list for struct.pack
    flat_values = []
    for v in config_data.values():
        if isinstance(v, list):
            flat_values.extend(v)
        else:
            flat_values.append(v)
    return struct.pack(format_string, *flat_values)

def ParseSharedBuffersConfig(jobj):
    #print("*************Shared Buffer Config Test****************")
    aiemetadata = jobj.get(AIEMETADATA)
    sharedbufferdata = aiemetadata.get(SHAREDBUFFERCONFIG)
    #print(sharedbufferdata)
    id_num = 0
    packed_data = b""
    if sharedbufferdata is not None:
        for i, sharedbuffer_config in enumerate(sharedbufferdata):
           # print("************Packing Shared Buffer Config****************")
            packed_data += pack_shared_buffer_config(sharedbuffer_config, id_num)
            id_num += 1

    
    with open("./build/sharedbuffersconfig.bin", "wb") as binary_file:
        binary_file.write(packed_data)

def ParsePartition(partitions_config):
    format_string = "<"
    config_data = OrderedDict()
    
    for key, value in partitions_config.items():
       
        #print(key, value)
        if key in VECTORS:
            print("Unknown key")
            
        elif key in ENCODINGTOKEYMAP["s"]:
            values = str(value)
            len_s = get_name_len(values)
            format_string+="i"
            config_data[f'{key}_len'] = len_s

            config_data[key] = values[:len_s].encode('utf-8') #encode string
            format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
        
        # elif key in ENCODINGTOKEYMAP["QQ"]:
        #     format_string += KEYTOENCODINGMAP[key]
        #     data = partitions_config.get(key).encode('utf-8')
        #     print(data)
        #     first_long = int.from_bytes(data[:8], byteorder='little', signed=False)
        #     second_long = int.from_bytes(data[8:], byteorder='little', signed=False)

        #     config_data[f'{key}_1'] = first_long
        #     config_data[f'{key}_2'] = second_long

        elif key in ENCODINGTOKEYMAP["HEX"]:
            format_string += "I"
            config_data[key] = int(value, 16)
            #print("****")
            #print(config_data[key])
            #print("****")
        else:
            format_string += KEYTOENCODINGMAP[key]
            config_data[key] = int(value)

    #print(format_string)
    #print(config_data.values())
    return struct.pack(format_string, *config_data.values())

def ParseAIEPartitionConfig(aie_partition):
    format_string = "<"
    config_data = OrderedDict()
    
    for key, value in aie_partition.items():
       
        #print(key, value)
        if key in VECTORS:
            format_string+="i"
            config_data[f'{key}_len'] = len(aie_partition["partitions"])
        elif key in ENCODINGTOKEYMAP["s"]:
            values = str(value)
            len_s = get_name_len(values)
            format_string+="i"
            config_data[f'{key}_len'] = len_s

            config_data[key] = values[:len_s].encode('utf-8') #encode string
            format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
        else:
            format_string += KEYTOENCODINGMAP[key]
            config_data[key] = int(value)

    packed_partition_info = b''.join(ParsePartition(partition_info) for partition_info in aie_partition["partitions"])
    #print(format_string)
    #print(*config_data.values())
    return struct.pack(format_string, *config_data.values()) + packed_partition_info


'''gets all the driver info and saves as binary'''


def ParseRtpsConfig(jobj):

    format_string = "<" #force little endian
    config_data = OrderedDict()

    aiemetadata = jobj.get(AIEMETADATA)
    rtpdata = aiemetadata.get(RTPCONFIG)
    if rtpdata is not None or rtpdata == "":
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
                        len_s = get_name_len(values)

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
    ParseAiecompilerOptions(jobj)
    ParseGraphConfig(jobj)
    ParseGmiosConfig(jobj)
    ParseExternalbuffersConfig(jobj)
    ParseSharedBuffersConfig(jobj)
    ParsePliosConfig(jobj)
    ParseRtpsConfig(jobj)


def check_empty_string(value):
    if isinstance(value, str) and not value:
        return True
    return False

def BIN_Generate(json_file, config_target):

    aiemetadata = json_file.get(AIEMETADATA)
    graphdata = aiemetadata.get(GRAPHSCONFIG)
    gmiodata = aiemetadata.get(GMIOCONFIG)
    pliodata = aiemetadata.get(PLIOCONFIG)
    rtpdata = aiemetadata.get(RTPCONFIG)
    externalbufferdata = aiemetadata.get(EXTERNALBUFFERCONFIG)
    sharedbufferdata = aiemetadata.get(SHAREDBUFFERCONFIG)
    
    if externalbufferdata:
        for i in range(len(externalbufferdata) - 1):
            EXTERNALBUFFERLIST.append(f'ebuffer_{i}')

    for key, value in graphdata.items():
        GRAPHS.append(key)

    for key, value in gmiodata.items():
        GMIOLIST.append(key)

    if  check_empty_string(pliodata) == False:
        for key, value in pliodata.items():
            PLIOLIST.append(key)
    
    if  check_empty_string(rtpdata) == False:
        for key, value in rtpdata.items():
            RTPLIST.append(key)
        
    if config_target == "driverconfig.bin":
        ParseDriverConfig(json_file)
    elif config_target == "graphconfig.bin":
        ParseGraphConfig(json_file)
    elif config_target == "gmiosconfig.bin":
        ParseGmiosConfig(json_file)
    elif config_target == "pliosconfig.bin":
        ParsePliosConfig(json_file)
    elif config_target == "rtpsconfig.bin":
        ParseRtpsConfig(json_file)
    elif config_target == "sharedbufferconfig.bin":
        ParseSharedBufferConfig(json_file)
    elif config_target == "externalbuffersconfig.bin":
        ParseExternalbuffersConfig(json_file)
    elif config_target == "aiecompilerconfig.bin":
        ParseAiecompilerOptions(json_file)
    elif config_target == "all":
        SaveDriverConfigsToBin(json_file)
    else:
        logging.error(f"Unknown configuration file: {config_target}, use all to get all binaries.")


def bin_to_c_array(file_path, var_name):
    """Convert binary file content to C array format."""
    with open(file_path, 'rb') as f:
        data = f.read()

    array_lines = []
    array_lines.append(f"unsigned char {var_name}[] = {{")

    # Format the binary data as comma-separated hex values, 12 per line
    dlen = len(data)
    for i in range(0, dlen, 12):
        line = ', '.join(f'0x{byte:02x}' for byte in data[i:i+12])
        if i + 12 <= dlen - 1:
            array_lines.append(f"  {line},")
        else:
            array_lines.append(f"  {line}")

    array_lines.append("};")
    array_lines.append(f"unsigned int {var_name}_len =  {len(data)};")
    #array_lines.append("\n")
    
    return '\n'.join(array_lines)


def generate_graph_class(json_file, output_file):
    """Generate C++ graph class with typed port members based on metadata."""
    import re
    
    aiemetadata = json_file.get(AIEMETADATA)
    graphdata = aiemetadata.get(GRAPHSCONFIG)
    gmiodata = aiemetadata.get(GMIOCONFIG)
    pliodata = aiemetadata.get(PLIOCONFIG)
    rtpdata = aiemetadata.get(RTPCONFIG)
    externalbufferdata = aiemetadata.get(EXTERNALBUFFERCONFIG)
    sharedbufferdata = aiemetadata.get(SHAREDBUFFERCONFIG)
    
    with open(output_file, 'w') as out_file:
        out_file.write("/**<!--\n")
        out_file.write("* Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.\n")
        out_file.write("* Auto-generated graph class\n")
        out_file.write("* -->*/\n\n")
        out_file.write("#ifndef __GENERATED_GRAPH_CLASS_H__\n")
        out_file.write("#define __GENERATED_GRAPH_CLASS_H__\n\n")
        out_file.write("#include \"aeg_baremetal_api.h\"\n")
        out_file.write("#if defined(__AIESIM__) || defined(__ADF_FRONTEND__)\n")
        out_file.write("#include \"aeg_baremetal_graph_sim_base.h\"\n")
        out_file.write("#endif\n")
        out_file.write("#ifndef AbrGraph\n")
        out_file.write("#define AbrGraph BaremetalGraph\n")
        out_file.write("#endif\n\n")
        # out_file.write("class AIEGraphSim;\n")
        out_file.write("class GmioApiImpl;\n")
        out_file.write("class ExternalBufferApiImpl;\n")
        out_file.write("class RtpApiImpl;\n")
        out_file.write("class SharedBufferApiImpl;\n\n")
        
        out_file.write("namespace aeg {\n")
        for graph_name, graph_config in graphdata.items():
            graph_base_name = graph_config.get("name", graph_name)
            out_file.write(f"struct {graph_base_name} {{}};\n")
        out_file.write("}\n\n")
        out_file.write("template<typename GraphTag>\n")
        out_file.write("struct AEGGraph;\n\n")
        
        graph_decl_macros = []
        for graph_name, graph_config in graphdata.items():
            graph_base_name = graph_config.get("name", graph_name)
            class_name = 'aeg_' + graph_base_name
            
            out_file.write(f"class {class_name} : public AbrGraph {{\n")
            out_file.write("public:\n")
            port_members = []
            
            port_arrays = {}
            
            if gmiodata:
                for gmio_name, gmio_config in gmiodata.items():
                    gmio_logical_name = gmio_config.get("name", gmio_name)
                    gmio_type = gmio_config.get("type", 0)
                    
                    if gmio_logical_name.startswith(graph_base_name + "."):
                        port_member_name = gmio_logical_name.split(".")[-1]
                        
                        array_match = re.match(r'^(\w+)\[(\d+)\]$', port_member_name)
                        if array_match:
                            base_name = array_match.group(1)
                            index = int(array_match.group(2))
                            port_type_str = 'InputGMIO' if gmio_type == 0 else 'OutputGMIO'
                            
                            if base_name not in port_arrays:
                                port_arrays[base_name] = {
                                    'indices': set(),
                                    'type': port_type_str,
                                    'port_type': 'gmio',
                                    'full_names': {}
                                }
                            port_arrays[base_name]['indices'].add(index)
                            port_arrays[base_name]['full_names'][index] = gmio_logical_name
                        else:
                            if gmio_type == 0:  # GM2AIE (input)
                                out_file.write(f"    aeg::detail::InputGMIO {port_member_name};\n")
                                port_members.append((port_member_name, gmio_logical_name, 'gmio'))
                            elif gmio_type == 1:  # AIE2GM (output)
                                out_file.write(f"    aeg::detail::OutputGMIO {port_member_name};\n")
                                port_members.append((port_member_name, gmio_logical_name, 'gmio'))
            
            if pliodata and not check_empty_string(pliodata):
                for plio_name, plio_config in pliodata.items():
                    plio_logical_name = plio_config.get("name", plio_name)
                    slave_or_master = plio_config.get("slaveOrMaster", 0)
                    
                    if plio_logical_name.startswith(graph_base_name + "."):
                        port_member_name = plio_logical_name.split(".")[-1]
                        
                        array_match = re.match(r'^(\w+)\[(\d+)\]$', port_member_name)
                        if array_match:
                            base_name = array_match.group(1)
                            index = int(array_match.group(2))
                            port_type_str = 'InputPLIO' if slave_or_master == 0 else 'OutputPLIO'
                            
                            if base_name not in port_arrays:
                                port_arrays[base_name] = {
                                    'indices': set(),
                                    'type': port_type_str,
                                    'port_type': 'plio',
                                    'full_names': {}
                                }
                            port_arrays[base_name]['indices'].add(index)
                            port_arrays[base_name]['full_names'][index] = plio_logical_name
                        else:
                            if slave_or_master == 0:
                                out_file.write(f"    aeg::detail::InputPLIO {port_member_name};\n")
                                port_members.append((port_member_name, plio_logical_name, 'plio'))
                            else:
                                out_file.write(f"    aeg::detail::OutputPLIO {port_member_name};\n")
                                port_members.append((port_member_name, plio_logical_name, 'plio'))
            
            used_port_names = {pm[0] for pm in port_members}
            rtp_graph_level = {}   # array_key -> {'indices': set(), 'full_names': {}}
            rtp_kernels = {}       # kernel_name -> { port_base -> {'indices': set(), 'full_names': {}} }

            if rtpdata and not check_empty_string(rtpdata):
                for rtp_name, rtp_config in rtpdata.items():
                    rtp_port_name = rtp_config.get("port_name", rtp_name)
                    
                    if rtp_port_name.startswith(graph_base_name + "."):
                        segments = rtp_port_name.split(".")
                        port_member_name = segments[-1]
                        
                        array_match = re.match(r'^(\w+)\[(\d+)\]$', port_member_name)
                        if array_match:
                            base_name = array_match.group(1)
                            index = int(array_match.group(2))
                            if len(segments) > 2:
                                kernel_name = segments[1]
                                if kernel_name not in rtp_kernels:
                                    rtp_kernels[kernel_name] = {}
                                if base_name not in rtp_kernels[kernel_name]:
                                    rtp_kernels[kernel_name][base_name] = {'indices': set(), 'full_names': {}}
                                rtp_kernels[kernel_name][base_name]['indices'].add(index)
                                rtp_kernels[kernel_name][base_name]['full_names'][index] = rtp_port_name
                            else:
                                array_key = base_name
                                if array_key in port_arrays or array_key in used_port_names:
                                    array_key = base_name + "_rtp"
                                if array_key not in rtp_graph_level:
                                    rtp_graph_level[array_key] = {'indices': set(), 'full_names': {}}
                                    used_port_names.add(array_key)
                                rtp_graph_level[array_key]['indices'].add(index)
                                rtp_graph_level[array_key]['full_names'][index] = rtp_port_name
                        else:
                            out_file.write(f"    aeg::detail::RTP {port_member_name};\n")
                            port_members.append((port_member_name, rtp_port_name, 'rtp'))
                            used_port_names.add(port_member_name)
            
            if sharedbufferdata:
                for sb_config in sharedbufferdata:
                    sb_name = sb_config.get("name", "")
                    sb_graph_id = sb_config.get("graph_id", -1)
                    graph_id = graph_config.get("id", -1)
                    
                    if sb_graph_id == graph_id and sb_name.startswith(graph_base_name + "."):
                        port_member_name = sb_name.split(".")[-1]
                        
                        array_match = re.match(r'^(\w+)\[(\d+)\]$', port_member_name)
                        if array_match:
                            base_name = array_match.group(1)
                            index = int(array_match.group(2))
                            
                            if base_name not in port_arrays:
                                port_arrays[base_name] = {
                                    'indices': set(),
                                    'type': 'SharedBuffer',
                                    'port_type': 'shared_buffer',
                                    'full_names': {}
                                }
                            port_arrays[base_name]['indices'].add(index)
                            port_arrays[base_name]['full_names'][index] = sb_name
                        else:
                            out_file.write(f"    aeg::detail::SharedBuffer {port_member_name};\n")
                            port_members.append((port_member_name, sb_name, 'shared_buffer'))
            
            external_buffer_ports = {}  # eb_member_name -> {port_array_name -> {index -> port_full_name}}
            
            if externalbufferdata:
                for eb_config in externalbufferdata:
                    eb_name = eb_config.get("name", "")
                    
                    if eb_name.startswith(graph_base_name + "."):
                        port_member_name = eb_name.split(".")[-1]
                        
                        array_match = re.match(r'^(\w+)\[(\d+)\]$', port_member_name)
                        if array_match:
                            base_name = array_match.group(1)
                            index = int(array_match.group(2))
                            
                            if base_name not in port_arrays:
                                port_arrays[base_name] = {
                                    'indices': set(),
                                    'type': 'ExternalBuffer',
                                    'port_type': 'external_buffer',
                                    'full_names': {}
                                }
                            port_arrays[base_name]['indices'].add(index)
                            port_arrays[base_name]['full_names'][index] = eb_name
                        else:
                            shim_port_configs = eb_config.get("shimPortConfigs", [])
                            port_struct = {}
                            
                            if shim_port_configs:
                                for port_config in shim_port_configs:
                                    port_name = port_config.get("portName", "")
                                    if port_name:
                                        # "out[0]" to "EG.ddrin.out[0]"
                                        port_suffix = port_name.split(".")[-1]
                                        array_match = re.match(r'^(\w+)\[(\d+)\]$', port_suffix)
                                        if array_match:
                                            port_base = array_match.group(1)
                                            port_index = int(array_match.group(2))
                                            
                                            if port_base not in port_struct:
                                                port_struct[port_base] = {}
                                            port_struct[port_base][port_index] = port_name
                            
                            if port_struct:
                                out_file.write(f"    struct {{\n")
                                out_file.write(f"        aeg::detail::ExternalBuffer buffer;\n")
                                
                                for port_array_name, port_indices in port_struct.items():
                                    max_index = max(port_indices.keys()) if port_indices else 0
                                    array_size = max_index + 1
                                    out_file.write(f"        aeg::detail::ExternalBufferPort {port_array_name}[{array_size}];\n")
                                
                                out_file.write(f"        \n")
                                out_file.write(f"        void setAddress(void* ptr) {{ buffer.setAddress(ptr); }}\n")
                                out_file.write(f"        void setupPingPongBuffers(void* const ptr1, void* const ptr2) {{ buffer.setupPingPongBuffers(ptr1, ptr2); }}\n")
                                out_file.write(f"        void gm2aie_nb() {{ buffer.gm2aie_nb(); }}\n")
                                out_file.write(f"        void aie2gm_nb() {{ buffer.aie2gm_nb(); }}\n")
                                out_file.write(f"    }} {port_member_name};\n")
                                
                                external_buffer_ports[port_member_name] = {
                                    'eb_name': eb_name,
                                    'ports': port_struct
                                }
                            else:
                                out_file.write(f"    aeg::detail::ExternalBuffer {port_member_name};\n")
                                port_members.append((port_member_name, eb_name, 'external_buffer'))
            
            port_array_inits = []
            for base_name, array_info in sorted(port_arrays.items()):
                indices = sorted(array_info['indices'])
                array_size = max(indices) + 1 if indices else 0
                port_type_str = array_info['type']
                
                out_file.write(f"    aeg::detail::{port_type_str} {base_name}[{array_size}];\n")
                port_array_inits.append((base_name, array_size, array_info['full_names'], array_info['port_type']))
            
            rtp_graph_inits = []
            for array_key, info in sorted(rtp_graph_level.items()):
                indices = sorted(info['indices'])
                array_size = max(indices) + 1 if indices else 0
                out_file.write(f"    aeg::detail::RTP {array_key}[{array_size}];\n")
                rtp_graph_inits.append((array_key, array_size, info['full_names']))
            
            for kernel_name in sorted(rtp_kernels.keys()):
                kernel_ports = rtp_kernels[kernel_name]
                out_file.write(f"    struct {kernel_name}_t {{\n")
                for port_base, info in sorted(kernel_ports.items()):
                    indices = sorted(info['indices'])
                    array_size = max(indices) + 1 if indices else 0
                    out_file.write(f"        aeg::detail::RTP {port_base}[{array_size}];\n")
                out_file.write(f"    }} {kernel_name};\n")
            
            out_file.write(f"    {class_name}(const std::string& graphName = \"{graph_base_name}\") \n")
            out_file.write(f"        : AbrGraph(graphName)")
            
            _graph_ptr = "this->getGraphImpl()"
            for port_member_name, port_full_name, port_type in port_members:
                if port_type in ['gmio', 'external_buffer', 'rtp', 'shared_buffer']:
                    out_file.write(f",\n          {port_member_name}(\"{port_full_name}\",\n            {_graph_ptr}\n            , nullptr)")
                else:
                    out_file.write(f",\n          {port_member_name}(\"{port_full_name}\",\n            {_graph_ptr}\n            )")
            
            has_body = bool(port_array_inits) or bool(rtp_graph_inits) or bool(rtp_kernels) or bool(external_buffer_ports)
            
            if has_body:
                out_file.write(" {\n")
                
                _gp = "this->getGraphImpl()"
                for base_name, array_size, full_names, port_type in port_array_inits:
                    for idx in range(array_size):
                        full_name = full_names.get(idx, f"{graph_base_name}.{base_name}[{idx}]")
                        if port_type in ['gmio', 'external_buffer', 'rtp', 'shared_buffer']:
                            out_file.write(f"        {base_name}[{idx}] = aeg::detail::{port_arrays[base_name]['type']}(\"{full_name}\",\n            {_gp}\n            , nullptr);\n")
                        else:
                            out_file.write(f"        {base_name}[{idx}] = aeg::detail::{port_arrays[base_name]['type']}(\"{full_name}\",\n            {_gp}\n            );\n")
                
                for array_key, array_size, full_names in rtp_graph_inits:
                    for idx in range(array_size):
                        full_name = full_names.get(idx, f"{graph_base_name}.{array_key}[{idx}]")
                        out_file.write(f"        {array_key}[{idx}] = aeg::detail::RTP(\"{full_name}\",\n            {_gp}\n            , nullptr);\n")
                
                for kernel_name in sorted(rtp_kernels.keys()):
                    for port_base, info in sorted(rtp_kernels[kernel_name].items()):
                        indices = sorted(info['indices'])
                        array_size = max(indices) + 1 if indices else 0
                        full_names = info['full_names']
                        for idx in range(array_size):
                            full_name = full_names.get(idx, f"{graph_base_name}.{kernel_name}.{port_base}[{idx}]")
                            out_file.write(f"        {kernel_name}.{port_base}[{idx}] = aeg::detail::RTP(\"{full_name}\",\n            {_gp}\n            , nullptr);\n")
                
                for eb_member_name, eb_info in external_buffer_ports.items():
                    eb_name = eb_info['eb_name']
                    ports = eb_info['ports']
                    out_file.write(f"        {eb_member_name}.buffer = aeg::detail::ExternalBuffer(\"{eb_name}\",\n            {_gp}\n            , nullptr);\n")
                    for port_array_name, port_indices in ports.items():
                        for port_index, port_full_name in port_indices.items():
                            out_file.write(f"        {eb_member_name}.{port_array_name}[{port_index}] = aeg::detail::ExternalBufferPort(\"{port_full_name}\",\n            {_gp}\n            , nullptr);\n")
                
                out_file.write("    }\n")
            else:
                out_file.write(" {}\n")
            
            out_file.write("};\n\n")
            graph_decl_macros.append((graph_base_name, class_name))
        
        for graph_base_name, class_name in graph_decl_macros:
            out_file.write(f"template<>\n")
            out_file.write(f"struct AEGGraph<aeg::{graph_base_name}> : {class_name} {{\n")
            out_file.write(f"    using {class_name}::{class_name};\n")
            out_file.write("};\n\n")
        
        out_file.write("\n#endif // __GENERATED_GRAPH_CLASS_H__\n")

def write_binary_as_c_arrays(output_file, input_files):
    with open(output_file, 'w') as out_file:
        for file_path in input_files:
            var_name = os.path.basename(file_path).replace('.', '_')
            c_array = bin_to_c_array(file_path, f"_binary_build_{var_name}_start")
            out_file.write(c_array)
            out_file.write("\n")


def replace_in_file(file_path, replacements):
    with open(file_path, 'r') as f:
        content = f.read()

    for old, new in replacements.items():
        content = content.replace(old, new)

    with open(file_path, 'w') as f:
        f.write(content)


# File paths
output_file = 'baremetal_metadata.cpp'
input_files = [
    './build/driverconfig.bin',
    './build/aiecompilerconfig.bin',
    './build/graphconfig.bin',
    './build/gmiosconfig.bin',
    './build/externalbuffersconfig.bin',
    './build/sharedbuffersconfig.bin',
    './build/pliosconfig.bin',
    './build/rtpsconfig.bin'
]

# Convert the binary files to C arrays and write them to the output file
# Perform the replacements to adjust the variable names and sizes
replacements = {
    '_binary_build_driverconfig_bin_start_len': '_binary_build_driverconfig_bin_size',
    '_binary_build_aiecompilerconfig_bin_start_len': '_binary_build_aiecompilerconfig_bin_size',
    '_binary_build_graphconfig_bin_start_len': '_binary_build_graphconfig_bin_size',
    '_binary_build_gmiosconfig_bin_start_len': '_binary_build_gmiosconfig_bin_size',
    '_binary_build_pliosconfig_bin_start_len': '_binary_build_pliosconfig_bin_size',
    '_binary_build_rtpsconfig_bin_start_len': '_binary_build_rtpsconfig_bin_size',
    '_binary_build_externalbuffersconfig_bin_start_len': '_binary_build_externalbuffersconfig_bin_size',
    '_binary_build_sharedbuffersconfig_bin_start_len': '_binary_build_sharedbuffersconfig_bin_size'
}

def merge_json_files(control_json, partition_json):
    """
    Merges the control and partition JSON files into a single one.
    """
    merged_json = control_json.copy()
    if "driver_config" not in merged_json.get("aie_metadata", {}):
        if "aie_metadata" not in merged_json:
            merged_json["aie_metadata"] = {}
        merged_json["aie_metadata"]["driver_config"] = {}
    merged_json["aie_metadata"]["driver_config"]["aie_partition_json"] = partition_json
    return merged_json


def main():
    if len(sys.argv) != 2 and len(sys.argv) != 3:
        logging.error("Usage: python aiemetacpp.py <control json> <partition json>")
        sys.exit(1)

    control_json_file = Path(sys.argv[1])
    if not control_json_file.exists():
        logging.error(f"Control json file {control_json_file} does not exist.")
        sys.exit(1)
    with open(control_json_file, 'r') as f:
        control_json = json.load(f)
        json_file = control_json
        
    
    if len(sys.argv) == 3:
        partition_json_file = Path(sys.argv[2])
        if partition_json_file.exists():
            with open(partition_json_file, 'r') as f:
                partition_json = json.load(f)
            json_file = merge_json_files(control_json, partition_json)

    os.makedirs("./build", exist_ok=True)
    BIN_Generate(json_file, "all")
    write_binary_as_c_arrays(output_file, input_files)
    replace_in_file(output_file, replacements)
    
    graph_class_file = 'generated_graphs.h'
    generate_graph_class(json_file, graph_class_file)
    # logging.info(f"Generated graph class file: {graph_class_file}")

if __name__ == "__main__":
    main()