#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

set -e
set -x

# Default values
AIE_GEN=2
BOOTGEN=
RPU=

if [ $# -gt 0 ]; then
	AIE_GEN="$1"
	shift
fi
	# Parse arguments
while [[ "$#" -gt 0 ]]; do
	case "$1" in
		--bootgen)
			BOOTGEN=1
			shift
			;;
		--rpu)
			RPU=1
			shift
			;;
		-h|--help)
			echo "Usage: $0 [AIE_GEN] [--bootgen] [--rpu]"
			echo "  AIE_GEN: AIE generation version (e.g., 1)"
			echo "  --bootgen: Enable bootgen"
			echo "  --rpu: Enable RPU mode"
			exit 0
			;;
		*)
			echo "Unknown option: $1"
			exit 1
			;;
	esac
done

echo "AIE_GEN=${AIE_GEN}, BOOTGEN=${BOOTGEN}, RPU=${RPU}"

if [ -n "$RPU" ]; then
	export AIE_PLATFORM="versal_rpu"
	case $AIE_GEN in
		1)
			ARCH_DIR=../../thirdparty/arch/ps/cortexr52_0/standalone_cortexr52_0/bsp/
			MCPU=cortex-r52
			LIB_AIEBAREMETAL_NAME=aiebaremetalr52
			;;
		2)
			ARCH_DIR=../../thirdparty/arch/ps/psv_cortexr5_0/
			MCPU=cortex-r53
			LIB_AIEBAREMETAL_NAME=aiebaremetalr53
			;;
		5)
			ARCH_DIR=../../thirdparty/arch/ps/psv_cortexr5_0/
			MCPU=cortex-r53
			LIB_AIEBAREMETAL_NAME=aiebaremetalr53
			;;
		*)
			echo "Unsupported AIE_GEN: $AIE_GEN"
			exit 1
		;;
	esac
else
	case $AIE_GEN in
		1)
			ARCH_DIR=../../thirdparty/arch/ps/psv_cortexa72_0/
			MCPU=cortex-a72
			LIB_AIEBAREMETAL_NAME=aiebaremetala72
			;;
		2)
			ARCH_DIR=../../thirdparty/arch/ps/psv_cortexa72_0/
			MCPU=cortex-a72
			LIB_AIEBAREMETAL_NAME=aiebaremetala72
			;;
		5)
			ARCH_DIR=../../thirdparty/arch/ps/cortexa78_0/standalone_cortexa78_0/bsp/
			MCPU=cortex-a78
			LIB_AIEBAREMETAL_NAME=aiebaremetala78

			;;
		*)
			echo "Unsupported AIE_GEN: $AIE_GEN"
			exit 1
		;;
	esac
fi


source $PWD/../../script/settings.sh 1

pushd ../
make ./build/libxtflwipall.a
popd

if [ ! -e ../build/lib${LIB_AIEBAREMETAL_NAME}.a ]; then
	echo "File ../build/libaiebaremetala72.a not found!"
	echo "Building AIE baremetal library..."
	pushd ../testkernel/
	source build.sh ${AIE_GEN} ${RPU}
	popd
fi

CC=aarch64-none-elf-gcc AR=aarch64-none-elf-ar make ARCH_DIR=${ARCH_DIR} MCPU=${MCPU} LIB_AIEBAREMETAL_NAME=${LIB_AIEBAREMETAL_NAME}
