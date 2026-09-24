#!/usr/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
set -e
set -x

BUILD_TEST1=1
BUILD_TEST2=1
CLEAN=1
AIE_GEN=5
LINK_FLOW=1
PLATFORM_FLOW=1
BUILD_COMBINED=1
BOOTGEN_MULTI=1

while [[ "$#" -gt 0 ]]; do
	case "$1" in
		-h|--help)
			echo "Usage: $0 --aie_gen <1|2|5> [--clean] [--build_test1 <0|1>] [--build_test2 <0|1>] [--link_flow <0|1>] [--platform_flow <0|1>]"
			exit 0
			;;
		--aie_gen)
			if [ -z "$2" ]; then
				echo "Error: --aie_gen requires a value."
				exit 1
			fi
			AIE_GEN=$2
			shift
			shift
			;;
		--clean)
			CLEAN=1
			shift
			;;
		--build_test1)
			if [ -z "$2" ]; then
				echo "Error: --build_test1 requires a value (0 or 1)."
				exit 1
			fi
			BUILD_TEST1=$2
			shift
			shift
			;;
		--build_test2)
			if [ -z "$2" ]; then
				echo "Error: --build_test2 requires a value (0 or 1)."
				exit 1
			fi
			BUILD_TEST2=$2
			shift
			shift
			;;
		--link_flow)
			if [ -z "$2" ]; then
				echo "Error: --link_flow requires a value (0 or 1)."
				exit 1
			fi
			LINK_FLOW=$2
			shift
			shift
			;;
		--platform_flow)
			if [ -z "$2" ]; then
				echo "Error: --platform_flow requires a value (0 or 1)."
				exit 1
			fi
			PLATFORM_FLOW=$2
			shift
			shift
			;;
		--build_combined)
			if [ -z "$2" ]; then
				echo "Error: --build_combined requires a value (0 or 1)."
				exit 1
			fi
			BUILD_COMBINED=$2
			shift
			shift
			;;
		--bootgen)
			if [ -z "$2" ]; then
				echo "Error: --bootgen requires a value (0 or 1)."
				exit 1
			fi
			BOOTGEN_MULTI=$2
			shift
			shift
			;;
		*)
			echo "Unknown option: $1"
			exit 1
			;;
	esac
done

case "$AIE_GEN" in
	1)
		XPFM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/xilinx_vck190_base_202520_1/xilinx_vck190_base_202520_1.xpfm
		BOOTGEN_BIF_FILE=./vck190_boot_image_combined.bif
		VERSAL=versal
		;;
	2)
		XPFM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/xilinx_vek280_base_202520_1/xilinx_vek280_base_202520_1.xpfm
		BOOTGEN_BIF_FILE=./vek280_boot_image_combined.bif
		VERSAL=versal
		;;
	5)
		XPFM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/vek385_base/vek385_base.xpfm
		BOOTGEN_BIF_FILE=./vek385_boot_image_combined.bif
		VERSAL=versal_2ve_2vm
		;;
	*)
		echo "Error: Unsupported AIE_GEN value '$AIE_GEN'. Supported values are 1, 2, or 5."
		exit 1
		;;
esac

echo "AIE_GEN is set to: $AIE_GEN"

if [ "$CLEAN" == "1" ]; then
	echo "Cleaning build directories..."
	pushd ../
	make clean
	popd
fi

if [ "$BUILD_TEST1" == "1" ]; then
	pushd ./test_partition1
	source build.sh "${AIE_GEN}"
	popd
fi

if [ "$BUILD_TEST2" == "1" ]; then
	pushd ./test_partition2
	source build.sh "${AIE_GEN}"
	popd
fi

if [ "$LINK_FLOW" == "1" ]; then
	v++ -l --platform "$XPFM" test_partition1/libadf.a test_partition2/libadf.a --save-temps
fi

if [ "$PLATFORM_FLOW" == "1" ]; then
	v++ -p -s -f a.xsa test_partition1/libadf.a test_partition2/libadf.a
fi

if [ "$BUILD_COMBINED" == "1" ]; then
	source ./build.sh "${AIE_GEN}"
fi

if [ "$BOOTGEN_MULTI" == "1" ]; then
	bootgen -arch $VERSAL -image "$BOOTGEN_BIF_FILE" -o i BOOT_COMBINED.BIN -w
fi
