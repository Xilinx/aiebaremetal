# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

#!/usr/bin/env bash 
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
python3 $SCRIPT_DIR/../../../script/aiemetacpp.py $SCRIPT_DIR/aie/Work/ps/c_rts/aie_control_config.json

if [ -f $SCRIPT_DIR/generated_graphs.h ]; then
    mv $SCRIPT_DIR/generated_graphs.h $SCRIPT_DIR/../
fi