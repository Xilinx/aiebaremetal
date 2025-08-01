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
#!/usr/bin/env bash


GLOBAL_DIR=$(dirname "$(realpath "${BASH_SOURCE[0]}")")
elf_path="$GLOBAL_DIR/adftoaeg"
run_path="$GLOBAL_DIR/convert.sh"

if [[ -z "${XILINX_VITIS}" ]]; then
    source /proj/xbuilds/HEAD_qualified_latest/installs/lin64/HEAD/Vitis/settings64.sh
fi
echo "XILINX_VITIS: ${XILINX_VITIS}"

LIB_PATH="${XILINX_VITIS}/gnu/aarch64/lin/aarch64-none/x86_64-oesdk-linux/usr/lib"
LIB_BASE_PATH="${XILINX_VITIS}/gnu/aarch64/lin/aarch64-none/x86_64-oesdk-linux/lib"
LD_SO="${LIB_BASE_PATH}/ld-linux-x86-64.so.2"
CLANG_INCLUDE_PATH="${XILINX_VITIS}/gnu/aarch64/lin/aarch64-none/x86_64-oesdk-linux/usr/lib/aarch64-xilinx-elf/gcc/aarch64-xilinx-elf/13.3.0/include"

if [[ -f "$elf_path" ]] && [[ -d "$LIB_PATH" ]] && [[ -d "$LIB_BASE_PATH" ]] && [[ -f "$LD_SO" ]] ; then
    LIB_PATH="${LIB_PATH}/"
    LIB_BASE_PATH="${LIB_BASE_PATH}/"
    CLANG_INCLUDE_PATH="${CLANG_INCLUDE_PATH}/"
    export LIB_PATH 
    export LIB_BASE_PATH
    export LD_SO
    export CLANG_INCLUDE_PATH
    export GLOBAL_DIR

    echo "Executable and its dependencies found."
else
    echo "Error: Unable to find adftoaeg executable and/or its dependencies."
fi

if [[ -f "$run_path" ]] ; then 
    alias convert.sh="$GLOBAL_DIR/convert.sh"
    echo "Command created"
else
    echo "Command not created; run.sh not in the current working directory"
fi
