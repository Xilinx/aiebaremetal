#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#SCRIPT_DIR=$(dirname "$(realpath "$0")")
SCRIPT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")
library_path="$SCRIPT_DIR/../thirdparty/aielib/aie-rt/driver/src/"
library_baremetal_path="$SCRIPT_DIR/../example/build/"
export AIE_BAREMETAL_LIB=$SCRIPT_DIR/../example/build/libaiebaremetal.so
# Check if library_path already exists in LD_LIBRARY_PATH
if [[ ":$LD_LIBRARY_PATH:" == *":$library_path:"* ]]; then
    echo "Library path already exists in LD_LIBRARY_PATH."
else
    # Append library_path to LD_LIBRARY_PATH
    export LD_LIBRARY_PATH="$library_path:$LD_LIBRARY_PATH"
    echo "Library path added to LD_LIBRARY_PATH."
fi
# Check if library_path already exists in LD_LIBRARY_PATH
if [[ ":$LD_LIBRARY_PATH:" == *":$library_baremetal_path:"* ]]; then
    echo "Library path already exists in LD_LIBRARY_PATH."
else
    # Append library_path to LD_LIBRARY_PATH
    export LD_LIBRARY_PATH="$library_baremetal_path:$LD_LIBRARY_PATH"
    echo "Library path added to LD_LIBRARY_PATH."
fi