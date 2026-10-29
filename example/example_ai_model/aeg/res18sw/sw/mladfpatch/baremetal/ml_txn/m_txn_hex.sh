#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Usage: ./bin_to_header.sh input.bin output.h

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <input_binary_file> <output_header_file>"
  exit 1
fi

INPUT_BIN="$1"
OUTPUT_HEADER="$2"

# Check if input binary file exists
if [ ! -f "$INPUT_BIN" ]; then
  echo "Error: File '$INPUT_BIN' not found."
  exit 1
fi

# Inform the user what's happening
echo "Converting '$INPUT_BIN' into header file '$OUTPUT_HEADER'..."

# Use xxd to convert the binary file into a C array format.
# xxd -i produces output like:
#   unsigned char input_bin[] = {
#     0xXX, 0xYY, ...
#   };
#   unsigned int input_bin_len = <length>;
xxd -i "$INPUT_BIN" > "$OUTPUT_HEADER"

echo "Header file '$OUTPUT_HEADER' generated successfully."

