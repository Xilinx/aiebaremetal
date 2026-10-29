#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
CMD_NAME="aiecompiler"

CMD_PATH=$(which "$CMD_NAME" 2>/dev/null)
if [ -z "$CMD_PATH" ]; then
    echo "Error: '$CMD_NAME' not found"
    exit 1
fi
echo "'$CMD_NAME' location: $CMD_PATH"

TWO_LEVEL_UP=$(dirname "$(dirname "$CMD_PATH")")
echo "cdodriver folder: $TWO_LEVEL_UP"
NEW_LIB_PATH="${TWO_LEVEL_UP}/lib/lnx64.o"

if [ -z "$LD_LIBRARY_PATH" ]; then
    export LD_LIBRARY_PATH="$NEW_LIB_PATH"
else
    export LD_LIBRARY_PATH="$NEW_LIB_PATH:$LD_LIBRARY_PATH"
fi

echo "update LD_LIBRARY_PATH: $LD_LIBRARY_PATH"

pushd ./Work/ps/cdo/
#export LD_LIBRARY_PATH=/proj/xbuilds/SWIP/9999.0_0204_1941/installs/lin64/Vitis/2025.1/aietools/lib/lnx64.o:$LD_LIBARARY_PATH
make
popd
