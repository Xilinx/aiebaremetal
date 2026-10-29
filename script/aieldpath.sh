#!/bin/bash
# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#

SCRIPT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")
library_baremetal_path="$SCRIPT_DIR/../example/build/"
export AIE_BAREMETAL_LIB=$SCRIPT_DIR/../example/build/libaiebaremetal.so

export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"

add_to_ld_path() {
    local p="$1"
    if [[ ":$LD_LIBRARY_PATH:" != *":$p:"* ]]; then
        export LD_LIBRARY_PATH="$p:$LD_LIBRARY_PATH"
    fi
}

add_to_ld_path "$library_baremetal_path"

add_to_ld_path "$SCRIPT_DIR/../thirdparty/aielib/aie-rt/driver/src"

if [ -z "${XILINX_VITIS_AIETOOLS:-}" ] && [ -n "${XILINX_VITIS:-}" ] && [ -d "${XILINX_VITIS}/aietools" ]; then
    export XILINX_VITIS_AIETOOLS="${XILINX_VITIS}/aietools"
fi

if [ -n "${XILINX_VITIS_AIETOOLS:-}" ]; then
    add_to_ld_path "$XILINX_VITIS_AIETOOLS/lib/lnx64.o"
fi

source "$SCRIPT_DIR/aieldshim.sh"
