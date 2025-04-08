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
                 "partition_num_cols", "partition_overlay_start_cols", "aie_partition_json"]

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
          "shimPortConfigs","shimBDInfos", "partitions"]

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
          "pong_buffer_lock_id","eid", "shim_column","channel_number","slaveOrMaster"],
    "Q": ["transaction_size","base_address"],
    "s": ["portName","name", "logical_name", "port_name", "alias_name","uuid","namespace" ],
    "I": ["iteration_memory_addresses", "startColumn", "numColumns","aie_pl_intf_id"],
    "?": ["multirate_triggers"],
    "P":["shimPortConfigs"],
    "D":["shimBDInfos"],
    "HEX":["aie_pl_intf_id"]
}

def convert_if_int(value):
    """
    Converts the value to an integer if it's a string representing an integer,
    otherwise returns the original value.
    """

    if isinstance(value, str) and value.isdigit():
        return int(value)
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

    aiemetadata = convert_if_int(jobj.get(AIEMETADATA))
    driverdata = convert_if_int(aiemetadata.get(DRIVERCONFIG))
    partitions = None

    for key in HWCONFIGSLIST:
        if key == "aie_partition_json":
            aie_partition = convert_if_int(driverdata.get(key))
            if not aie_partition:
                continue
            aie_engine = convert_if_int(aie_partition.get("AIE"))
            for aie in AIE_ENGINE_LIST:
                partition_data = convert_if_int(aie_engine.get(aie))
                if partition_data:
                    partitions = ParseAIEPartitionConfig(partition_data)
            continue

        if key in VECTORS:
            values = driverdata.get(key, [0,1,2,3])
        else:
            values = convert_if_int(driverdata.get(key, 0))

        key_n = f'{DRIVERCONFIG}.{key}'       
        if type(values) is list:
            config_data[f'{key_n}.len'] = len(values) #add len of vector
            format_string+="i"
            for i, value in enumerate(values):
                key_n = f'{key_n}.{i}'
                config_data[key_n] = convert_if_int(value)
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
Code to parse graph config from json
returns: list_data, format

'''

def ParseGraphConfig(jobj):

    format_string = "<" #force little endian
    config_data = OrderedDict()

    aiemetadata = convert_if_int(jobj.get(AIEMETADATA))
    graphdata = convert_if_int(aiemetadata.get(GRAPHSCONFIG))

    for graph in GRAPHS:
        graph_config = convert_if_int(graphdata.get(graph))
        for key in GRAPHCONFIGLIST:
            if key in VECTORS:
                values = graph_config.get(key, [0,1,2,3])
            elif key in ENCODINGTOKEYMAP["s"]:
                value = graph_config.get(key, "")
            else:
                values = convert_if_int(graph_config.get(key, 0))

            key_n = f'{graph}.{key}'
            if type(values) is list:
                config_data[f'{key_n}.len'] = len(values) #add len of vector
                format_string+="i"
                for i, value in enumerate(values):
                    key_n = f'{key_n}.{i}'
                    config_data[key_n] = convert_if_int(value)
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

    aiemetadata = convert_if_int(jobj.get(AIEMETADATA))
    gmiodata = convert_if_int(aiemetadata.get(GMIOCONFIG))

    for gmio in GMIOLIST:
        gmio_config = convert_if_int(gmiodata.get(gmio))
        if not gmio_config:
            break
        for key in GMIOCONFIGLIST:
            if key in VECTORS:
                values = gmio_config.get(key, [0,1,2,3])
            elif key in ENCODINGTOKEYMAP["s"]:
                value = gmio_config.get(key, "")
            else:
                values = convert_if_int(gmio_config.get(key, 0))

            key_n = f'{gmio}.{key}'
            if type(values) is list:
                config_data[f'{key_n}.len'] = len(values) #add len of vector
                format_string+="i"
                for i, value in enumerate(values):
                    key_n = f'{key_n}.{i}'
                    config_data[key_n] = convert_if_int(value)
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

'''
Code to parse graph config from json
returns: list_data, format

'''

def ParsePliosConfig(jobj):

    format_string = "<" #force little endian
    config_data = OrderedDict()

    aiemetadata = convert_if_int(jobj.get(AIEMETADATA))
    pliodata = convert_if_int(aiemetadata.get(PLIOCONFIG))

    if pliodata is not None and pliodata != "":
        for plio in PLIOLIST:
            plio_config = convert_if_int(pliodata.get(plio))
            if not plio_config:
                break
            for key in PLIOCONFIGLIST:
                if key in VECTORS:
                    values = plio_config.get(key, [0,1,2,3])
                elif key in ENCODINGTOKEYMAP["s"]:
                    value = plio_config.get(key, "")
                else:
                    values = convert_if_int(plio_config.get(key, 0))

                key_n = f'{plio}.{key}'
                if type(values) is list:
                    config_data[f'{key_n}.len'] = len(values) #add len of vector
                    format_string+="i"
                    for i, value in enumerate(values):
                        key_n = f'{key_n}.{i}'
                        config_data[key_n] = convert_if_int(value)
                        format_string+= KEYTOENCODINGMAP[key]
                elif key in ENCODINGTOKEYMAP["s"]:
                    values = str(value)
                    len_s = min(len(values), 31)

                    config_data[f'{key_n}.len'] = len(values) #add len of string
                    format_string+="i"

                    config_data[key_n] = values[:len_s].encode('utf-8') #encode string
                    format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
                else:
                    config_data[key_n] = convert_if_int(values)
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
    aiemetadata = convert_if_int(jobj.get(AIEMETADATA))
    externalbufferdata = convert_if_int(aiemetadata.get(EXTERNALBUFFERCONFIG))
    packed_data = b""
    if externalbufferdata is not None:
        for i, externalbuffer_config in enumerate(externalbufferdata):
            print("************External Buffer Config Test****************")
            packed_data += pack_external_buffer_config(externalbuffer_config)

    
    with open("./build/externalbuffersconfig.bin", "wb") as binary_file:
        binary_file.write(packed_data)

def ParsePartition(partitions_config):
    format_string = "<"
    config_data = OrderedDict()
    
    for key, value in partitions_config.items():
       
        print(key, value)
        if key in VECTORS:
            print("Unknown key")
            
        elif key in ENCODINGTOKEYMAP["s"]:
            values = str(value)
            len_s = min(len(values), 31)
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
            print("****")
            print(config_data[key])
            print("****")
        else:
            format_string += KEYTOENCODINGMAP[key]
            config_data[key] = int(value)

    print(format_string)
    print(config_data.values())
    return struct.pack(format_string, *config_data.values())

def ParseAIEPartitionConfig(aie_partition):
    format_string = "<"
    config_data = OrderedDict()
    
    for key, value in aie_partition.items():
       
        print(key, value)
        if key in VECTORS:
            format_string+="i"
            config_data[f'{key}_len'] = len(aie_partition["partitions"])
        elif key in ENCODINGTOKEYMAP["s"]:
            values = str(value)
            len_s = min(len(values), 31)
            format_string+="i"
            config_data[f'{key}_len'] = len_s

            config_data[key] = values[:len_s].encode('utf-8') #encode string
            format_string += f'{len_s}{KEYTOENCODINGMAP[key]}'
        else:
            format_string += KEYTOENCODINGMAP[key]
            config_data[key] = int(value)

    packed_partition_info = b''.join(ParsePartition(partition_info) for partition_info in aie_partition["partitions"])
    print(format_string)
    #print(*config_data.values())
    return struct.pack(format_string, *config_data.values()) + packed_partition_info


'''gets all the driver info and saves as binary'''


def ParseRtpsConfig(jobj):

    format_string = "<" #force little endian
    config_data = OrderedDict()

    aiemetadata = convert_if_int(jobj.get(AIEMETADATA))
    rtpdata = convert_if_int(aiemetadata.get(RTPCONFIG))
    if rtpdata is not None or rtpdata == "":
        for rtp in RTPLIST:
            rtp_config = convert_if_int(rtpdata.get(rtp))
            if rtp_config is not None:
                for key in RTPCONFIGLIST:
                    if key in VECTORS:
                        values = rtp_config.get(key, [0,1,2,3])
                    elif key in ENCODINGTOKEYMAP["s"]:
                        value = rtp_config.get(key, "")
                    else:
                        values = convert_if_int(rtp_config.get(key, 0))
                    
                    key_n = f'{rtp}.{key}'
                    if type(values) is list:
                        config_data[f'{key_n}.len'] = len(values) #add len of vector
                        format_string+="i"
                        for i, value in enumerate(values):
                            key_n = f'{key_n}.{i}'
                            config_data[key_n] = convert_if_int(value)
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
    ParsePliosConfig(jobj)
    ParseRtpsConfig(jobj)


def check_empty_string(value):
    if isinstance(value, str) and not value:
        return True
    return False

def BIN_Generate(json_file, config_target):
    #json_file = f'{work_folder}/ps/c_rts/aie_control_config.json'

    data = ""
    with open(json_file, 'r') as json_file:
        data = json.load(json_file, object_pairs_hook=OrderedDict)

    aiemetadata = convert_if_int(data.get(AIEMETADATA))
    graphdata = convert_if_int(aiemetadata.get(GRAPHSCONFIG))
    gmiodata = convert_if_int(aiemetadata.get(GMIOCONFIG))
    pliodata = convert_if_int(aiemetadata.get(PLIOCONFIG))
    rtpdata = convert_if_int(aiemetadata.get(RTPCONFIG))
    externalbufferdata = convert_if_int(aiemetadata.get(EXTERNALBUFFERCONFIG))
    
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
        ParseDriverConfig(data)
    elif config_target == "graphconfig.bin":
        ParseGraphConfig(data)
    elif config_target == "gmiosconfig.bin":
        ParseGmiosConfig(data)
    elif config_target == "pliosconfig.bin":
        ParsePliosConfig(data)
    elif config_target == "rtpsconfig.bin":
        ParseRtpsConfig(data)
    elif config_target == "externalbuffersconfig.bin":
        ParseExternalbuffersConfig(data)
    elif config_target == "all":
        SaveDriverConfigsToBin(data)
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


def write_binary_as_c_arrays(output_file, input_files):
    with open(output_file, 'w') as out_file:
        for file_path in input_files:
            # Extract the file name for variable naming
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
    './build/graphconfig.bin',
    './build/gmiosconfig.bin',
    './build/externalbuffersconfig.bin',
    './build/pliosconfig.bin',
    './build/rtpsconfig.bin'
]

# Convert the binary files to C arrays and write them to the output file
# Perform the replacements to adjust the variable names and sizes
replacements = {
    '_binary_build_driverconfig_bin_start_len': '_binary_build_driverconfig_bin_size',
    '_binary_build_graphconfig_bin_start_len': '_binary_build_graphconfig_bin_size',
    '_binary_build_gmiosconfig_bin_start_len': '_binary_build_gmiosconfig_bin_size',
    '_binary_build_pliosconfig_bin_start_len': '_binary_build_pliosconfig_bin_size',
    '_binary_build_rtpsconfig_bin_start_len': '_binary_build_rtpsconfig_bin_size',
    '_binary_build_externalbuffersconfig_bin_start_len': '_binary_build_externalbuffersconfig_bin_size'
}


if __name__ == "__main__":
    if len(sys.argv) != 2:
        logging.error("Usage: python aiemetacpp.py <kernel work folder>")
        sys.exit(1)

    #work_folder = sys.argv[1]#os.environ.get('AIE_COMPILE_WORK')
    json_file = sys.argv[1]
    os.makedirs("./build", exist_ok=True)
    BIN_Generate(json_file, "all")
    write_binary_as_c_arrays(output_file, input_files)
    replace_in_file(output_file, replacements)

