#!/usr/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
set -e

if [ -z "${PLATFORM}" ] || [ ! -e "$PLATFORM" ]; then
	echo "Error: PLATFORM is not set or does not exist: PLATFORM=${PLATFORM}"
	exit 1
fi

if [ -z "${BAREMETAL_SCRIPT_DIR}" ] || [ ! -d "$BAREMETAL_SCRIPT_DIR" ]; then
	echo "Error: BAREMETAL_SCRIPT_DIR is not set or is not a directory: BAREMETAL_SCRIPT_DIR=${BAREMETAL_SCRIPT_DIR}"
	exit 1
fi

if [ -z "${ELF}" ] || [ ! -e "$ELF" ]; then
	echo "Error: ELF is not set or does not exist: ELF=${ELF}"
	exit 1
fi

if [ -z "${AIE_GEN}" ]; then
	echo "Error: AIE_GEN is not set"
	exit 1
fi

SYSTEM_XSIM_CFG=$BAREMETAL_SCRIPT_DIR/system_xsim.cfg

build_hw_emu_gen1_2() {
	v++ -g -l --platform "$PLATFORM" libadf.a \
		--vivado.prop fileset.sim_1.xsim.simulate.runtime=100ms \
		-t hw_emu --save-temps --verbose --config "$SYSTEM_XSIM_CFG" -o aie_base_graph.xsa

	v++ -p -t hw_emu -f "$PLATFORM" \
		--package.domain aiengine \
		--package.os baremetal \
		--package.boot_mode sd \
		--package.image_format=fat32 \
		--save-temps \
		--package.ps_elf "$ELF",a72-0 aie_base_graph.xsa libadf.a
}

build_hw_emu_gen5() {
	GLOBAL_INI=$BAREMETAL_SCRIPT_DIR/global.ini
	if [ ! -f "$GLOBAL_INI" ]; then
		echo "Error: global.ini file not found at $GLOBAL_INI"
		exit 1
	fi
	v++ -g -l --platform "$PLATFORM" libadf.a \
		--vivado.prop fileset.sim_1.xsim.simulate.runtime=100ms \
		-t hw_emu --save-temps --verbose --config "$SYSTEM_XSIM_CFG" -o aie_base_graph.xsa

	v++ -p -t hw_emu -f aie_base_graph.xsa libadf.a \
		--package.domain aiengine \
		--package.os baremetal \
		--package.boot_mode sd \
		--package.image_format=fat32 \
		--save-temps \
		--package.ps_elf "$ELF",a78-0
}

case "$AIE_GEN" in
	1|2) build_hw_emu_gen1_2 ;;
	5)   build_hw_emu_gen5 ;;
	*)   echo "Error: Unknown AIE_GEN value: $AIE_GEN"; exit 1 ;;
esac

if [ ! -f "$BAREMETAL_SCRIPT_DIR/qemu_hw_emu_common.sh" ]; then
	echo "Error: qemu_hw_emu_common.sh not found" >&2
	exit 1
fi
source "$BAREMETAL_SCRIPT_DIR/qemu_hw_emu_common.sh"
patch_baremetal_launch_hw_emu "$PWD" "$PLATFORM"
