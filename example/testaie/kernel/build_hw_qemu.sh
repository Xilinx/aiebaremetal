#!/usr/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

export PLATFORM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/vek385_base/vek385_base.xpfm
#export PLATFORM=/proj/xbuilds/2026.1_daily_latest/internal_platforms/vek385_base/vek385_base.xpfm
export BAREMETAL_SCRIPT_DIR=$(realpath ../../../script)
export ELF=$(realpath vek385.elf)
export AIE_GEN=5

source /proj/xbuilds/2025.2_daily_latest/installs/lin64/2025.2/Vitis/settings64.sh
../../../script/build_hw_qemu.sh
