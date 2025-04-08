#!/bin/bash
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