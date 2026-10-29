# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
import os
import re

def binary_to_cpp_header(file_list, output_header):
    """
    Converts a list of binary files into a single C++ header file.

    :param file_list: List of binary file paths.
    :param output_header: Path to the output C++ header file.
    """
    with open(output_header, 'w') as header_file:
        # Write header guards
        guard_name = os.path.basename(output_header).replace('.', '_').upper()
        header_file.write(f"#ifndef {guard_name}\n")
        header_file.write(f"#define {guard_name}\n\n")

        for binary_path in file_list:
            # Extract variable name from file name
            variable_name = os.path.splitext(os.path.basename(binary_path))[0]
            variable_name = re.sub(r'\.', '', variable_name)
            try:
                # Read binary content
                with open(binary_path, 'rb') as binary_file:
                    binary_data = binary_file.read()

                # Write C++ array definition
                header_file.write(f"// Data from {binary_path}\n")
                header_file.write(f"unsigned char {variable_name}[] = {{\n")
                
                # Write data as comma-separated hex bytes
                for i, byte in enumerate(binary_data):
                    header_file.write(f"0x{byte:02x}, ")
                    if (i + 1) % 12 == 0:  # Wrap lines every 12 bytes
                        header_file.write("\n")
                
                # Close array definition
                header_file.write("\n};\n\n")
                header_file.write(f"size_t {variable_name}_size = sizeof({variable_name});\n\n")

            except Exception as e:
                print(f"Error processing {binary_path}: {e}")

        # Close header guard
        header_file.write(f"#endif // {guard_name}\n")


# Example usage
binary_files = ["./ml_txn/ml_txn.bin.aig", "./data/ifm32.txt", "./data/ofm32.txt","./data/wts32.txt"]  # Replace with your binary file paths
output_header_file = "binary_data.h"
binary_to_cpp_header(binary_files, output_header_file)

print(f"C++ header file generated: {output_header_file}")

