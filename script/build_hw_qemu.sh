#!/usr/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
set -x

if [ -z "${PLATFORM:-}" ]; then
	echo "Error: PLATFORM environment variable is not set."
	exit 1
fi

if [ -z "${BAREMETAL_SCRIPT_DIR:-}" ]; then
	echo "Error: BAREMETAL_SCRIPT_DIR environment variable is not set."
	exit 1
fi

if [ -z "${ELF:-}" ]; then
	echo "Error: ELF environment variable is not set."
	exit 1
fi

SYSTEM_XSIM_CFG=$BAREMETAL_SCRIPT_DIR/system_xsim.cfg
echo "Using SYSTEM_XSIM_CFG: $SYSTEM_XSIM_CFG"

v++ -g -l --platform "$PLATFORM"  libadf.a --vivado.prop fileset.sim_1.xsim.simulate.runtime=500us \
	-t hw_emu --save-temps --verbose --config "$SYSTEM_XSIM_CFG" -o aie_base_graph.xsa

v++ -p -t hw_emu -f "$PLATFORM"  \
	--package.domain aiengine \
	--package.boot_mode sd \
	--package.image_format=fat32 \
	--package.ps_elf $ELF,a72-0 aie_base_graph.xsa libadf.a

