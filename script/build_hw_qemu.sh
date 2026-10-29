#!/usr/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
set -e

if [ -z "${PLATFORM}" ] || [ ! -e "$PLATFORM" ]; then
	echo "Error: PLATFORM is not set or does not exist: PLATFORM=${PLATFORM}"
	exit 1
else
	echo "Using PLATFORM: $PLATFORM"
fi

if [ -z "${BAREMETAL_SCRIPT_DIR}" ] || [ ! -d "$BAREMETAL_SCRIPT_DIR" ]; then
	echo "Error: BAREMETAL_SCRIPT_DIR is not set or is not a directory: BAREMETAL_SCRIPT_DIR=${BAREMETAL_SCRIPT_DIR}"
	exit 1
else
	echo "Using BAREMETAL_SCRIPT_DIR: $BAREMETAL_SCRIPT_DIR"
fi

if [ -z "${ELF}" ] || [ ! -e "$ELF" ]; then
	echo "Error: ELF is not set or does not exist: ELF=${ELF}"
	exit 1
else
	echo "Using ELF: $ELF"
fi

if [ -z "${AIE_GEN}" ]; then
	echo "Error: AIE_GEN is not set"
	exit 1
else
	echo "Using AIE_GEN: $AIE_GEN"
fi

build_hw_emu_gen1_2() {
	SYSTEM_XSIM_CFG=$BAREMETAL_SCRIPT_DIR/system_xsim.cfg
	echo "Using SYSTEM_XSIM_CFG: $SYSTEM_XSIM_CFG"

	v++ -g -l --platform "$PLATFORM"  libadf.a --vivado.prop fileset.sim_1.xsim.simulate.runtime=500us \
		-t hw_emu --save-temps --verbose --config "$SYSTEM_XSIM_CFG" -o aie_base_graph.xsa

	v++ -p -t hw_emu -f "$PLATFORM"  \
		--package.domain aiengine \
		--package.os baremetal \
		--package.boot_mode sd \
		--package.image_format=fat32 \
		--save-temps \
		--package.ps_elf $ELF,a72-0 aie_base_graph.xsa libadf.a
}

build_hw_emu_gen5() {
	GLOBAL_INI=$BAREMETAL_SCRIPT_DIR/global.ini
	if [ ! -f "$GLOBAL_INI" ]; then
		echo "Error: global.ini file not found at $GLOBAL_INI"
		exit 1
	fi
	PACKAGE_LINK_INI=$BAREMETAL_SCRIPT_DIR/package_link.ini
	SYSTEM_XSIM_CFG=$BAREMETAL_SCRIPT_DIR/system_xsim.cfg
	echo "Building for AIE_GEN 5 (Versal AI Core Gen 5)"
	v++ -g -l --platform "$PLATFORM"  libadf.a --vivado.prop fileset.sim_1.xsim.simulate.runtime=500us \
		-t hw_emu --save-temps --verbose --config "$SYSTEM_XSIM_CFG" -o aie_base_graph.xsa

	v++ -p -t hw_emu -f aie_base_graph.xsa libadf.a \
		--package.domain aiengine \
		--package.os baremetal \
		--package.boot_mode sd \
		--package.image_format=fat32 \
		--save-temps \
		--package.ps_elf $ELF,a78-0
}

QEMU_COMMON="$BAREMETAL_SCRIPT_DIR/qemu_hw_emu_common.sh"
patch_launch_if_needed() {
	if [ -f "$QEMU_COMMON" ]; then
		source "$QEMU_COMMON"
		patch_baremetal_launch_hw_emu "$PWD" "$PLATFORM"
	else
		echo "Warning: $QEMU_COMMON not found; launch_hw_emu.sh left unchanged"
	fi
}

case "$AIE_GEN" in
	1|2)
		build_hw_emu_gen1_2
		patch_launch_if_needed
		;;
	5)
		build_hw_emu_gen5
		patch_launch_if_needed
		;;
	*)
		echo "Error: Unknown AIE_GEN value: $AIE_GEN"
		exit 1
		;;
esac

