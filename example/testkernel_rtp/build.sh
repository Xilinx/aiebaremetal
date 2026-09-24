#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ./build.sh 2 -bootgen -rpu
# For QEMU: source ./build.sh 2 qemu
set -e
AIE_GEN=1
RPUFLAG=
BOOTGEN=
HW_QEMU=
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
if [ $# -gt 1 ]; then
	BOOTGEN=$2
	# Support "build.sh 2 qemu" for QEMU/hw_emu package
	if [ "$2" = "qemu" ]; then
		BOOTGEN=
		HW_QEMU=--hw_qemu
	fi
fi
if [ $# -gt 2 ] && [ "$2" != "qemu" ]; then
	RPUFLAG=$3
	#-rpu
fi

echo ${AIE_GEN}
#set this is is local env
source ../../script/settings.sh 1
if [ $AIE_GEN == 5 ]; then
	source compile.sh --runtime_source_file ./src/graph.cpp --partition_option "--enable-partition=0:12:pr0" --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN} ${HW_QEMU}
else
	source compile.sh --runtime_source_file ./src/graph.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN} ${HW_QEMU}
fi

