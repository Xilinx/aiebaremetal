# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
import os

def process_file(file_path):
    with open(file_path, 'r') as file:
        # Convert each hex line into uint32 format if necessary
        data = []
        for line in file:
            hex_value = int(line.strip(), 16)
            if hex_value > 0x7FFFFFFF:  # if it exceeds int32 range
                data.append(f'(uint32_t)0x{line.strip()}')
            else:
                data.append(f'0x{line.strip()}')
    return data

def get_variable_name(file_path):
    # Extract file name without extension for variable name
    return os.path.splitext(os.path.basename(file_path))[0]

def generate_cpp_header(files, output_file):
    with open(output_file, 'w') as header:
        header.write("#ifndef HEX_DATA_H\n#define HEX_DATA_H\n\n")
        for file_path in files:
            var_name = get_variable_name(file_path)
            data = process_file(file_path)
            header.write(f"const uint32_t {var_name}[] = {{\n")
            header.write(",\n".join(data))
            header.write("\n};\n\n")
        header.write("#endif // HEX_DATA_H\n")

def main():
    # Replace with your file names
    input_files = ["../../aie/data/ifm32.txt", "../../aie/data/ofm32_ref.txt", "../../aie/data/wts32.txt"]
    output_file = "hex_data.h"
    generate_cpp_header(input_files, output_file)
    print(f"Header file '{output_file}' generated successfully.")

if __name__ == "__main__":
    main()
