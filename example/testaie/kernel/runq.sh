#!/usr/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

#export BAREMETAL_TEST_DIR="$(realpath "$(dirname "${BASH_SOURCE[0]}")/..")"
export AIE_GEN=5
export BAREMETAL_DIR="$(realpath "$BAREMETAL_TEST_DIR/../../../")"
export ELF="$(realpath vek385.elf)"
export BOOT_BIN="$(realpath build/vek385.BIN)"

../../../script/runqemu.sh
