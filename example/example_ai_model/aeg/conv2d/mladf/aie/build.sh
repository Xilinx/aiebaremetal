# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
!/usr/bin/bash
source /proj/xbuilds/2024.1_INT_qualified_latest/installs/lin64/Vitis/2024.1/settings64.sh
AIECOMPILERPATH=$(which aiecompiler)
export XILINX_VITIS_AIETOOLS=$(dirname "$(dirname "$AIECOMPILERPATH")")
make compile
