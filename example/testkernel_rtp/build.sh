#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Usage: source ./build.sh [options]
# Options:
#   <1|2|5>                 AIE generation (default: 1)
#   -bootgen                Enable bootgen
#   -rpu                    Build for RPU
#   qemu                    Package for QEMU/hw_emu
#   -new_api                Use new inheritance-based API
set -e
AIE_GEN=1
RPUFLAG=
BOOTGEN=
HW_QEMU=
new_api=false

while [ $# -gt 0 ]; do
	case "$1" in
		-bootgen)
			BOOTGEN="-bootgen"
			shift
			;;
		-rpu)
			RPUFLAG="-rpu"
			shift
			;;
		qemu)
			HW_QEMU=--hw_qemu
			shift
			;;
		-new_api)
			new_api=true
			shift
			;;
		*)
			if [[ $1 =~ ^[0-9]+$ ]]; then
				AIE_GEN=$1
				shift
			else
				echo "Unknown option: $1" >&2
				exit 1
			fi
			;;
	esac
done

if [[ "$new_api" == true ]]; then
	NEW_API_FLAG=-new_api
	SOURCE_FILE=./src/graph_new_api.cpp
else
	NEW_API_FLAG=
	SOURCE_FILE=./src/graph.cpp
fi

echo ${AIE_GEN}
#set this is is local env
source ../../script/settings.sh 1
if [ $AIE_GEN == 5 ]; then
	source compile.sh ${NEW_API_FLAG} --runtime_source_file ${SOURCE_FILE} --partition_option "--enable-partition=0:12:pr0" --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN} ${HW_QEMU}
else
	source compile.sh ${NEW_API_FLAG} --runtime_source_file ${SOURCE_FILE} --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN} ${HW_QEMU}
fi

