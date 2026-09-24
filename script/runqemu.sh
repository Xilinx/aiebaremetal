#!/usr/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

print_usage() {
	echo "Usage: $0"
	echo ""
	echo "Mandatory environment variables:"
	echo "  BAREMETAL_TEST_DIR - Path to baremetal test directory, eg: example/testaie/kernel"
	echo "  AIE_GEN           - AIE generation (1, 2, or 5)"
	echo "  ELF               - Path to ELF file, eg: vek385.elf"
	echo "  BOOT_BIN          - Path to boot.bin file, eg: build/vek385.BIN"
	echo ""
	echo "Optional environment variables:"
	echo "  AMD_EDF_SDK                     - Path to AMD EDF SDK environment setup script"
	echo "  RUNQEMU_DIR                     - QEMU run directory (default: \$BAREMETAL_DIR/runqemu_dir_aiegen\$AIE_GEN)"
	echo "  SYNC_RUNQEMU_ARTIFACTS          - Sync QEMU artifacts (default: 1)"
	echo "  UPDATE_BOOT_BIN                 - Update boot.bin in QEMU directory (default: 1)"
	echo "  RUN_QEMU                        - Run QEMU after setup (default: 1)"
	echo "  VERSAL_ARTIFACTS_DIR            - Path to Versal artifacts (AIE_GEN=5 only)"
	echo "  EDF_LINUX_DISK_IMAGE_DIR        - Path to EDF Linux disk image (AIE_GEN=5 only)"
	echo "  BOOT_VERSAL_QEMUBOOT_CONF       - Boot qemuboot.conf filename (AIE_GEN=5 only)"
	echo "  EDF_LINUX_DISK_IMAGE_QEMUBOOT_CONF - EDF Linux disk image qemuboot.conf filename (AIE_GEN=5 only)"
	echo "  BOOT_VERSAL_BIN                 - Boot bin filename (AIE_GEN=5 only)"
}

if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
	print_usage
	exit 0
fi

if [ -z "$BAREMETAL_TEST_DIR" ]; then
	echo "Error: BAREMETAL_TEST_DIR is not set" >&2
	echo "" >&2
	print_usage >&2
	exit 1
else
	if [ ! -d "$BAREMETAL_TEST_DIR" ]; then
		echo "Error: BAREMETAL_TEST_DIR does not exist: $BAREMETAL_TEST_DIR" >&2
		exit 1
	fi
	echo "BAREMETAL_TEST_DIR=$BAREMETAL_TEST_DIR"
fi

if [ -z "$AIE_GEN" ]; then
	echo "Error: AIE_GEN is not set" >&2
	echo "" >&2
	print_usage >&2
	exit 1
else
	echo "AIE_GEN=$AIE_GEN"
fi

if [ -z "$ELF" ]; then
	echo "Error: ELF is not set" >&2
	echo "" >&2
	print_usage >&2
	exit 1
else
	if [ ! -f "$ELF" ]; then
		echo "Error: ELF does not exist: $ELF" >&2
		exit 1
	fi
	echo "ELF=$ELF"
fi

if [ -z "$BOOT_BIN" ]; then
	echo "Error: BOOT_BIN is not set" >&2
	echo "" >&2
	print_usage >&2
	exit 1
else
	if [ ! -f "$BOOT_BIN" ]; then
		echo "Error: BOOT_BIN does not exist: $BOOT_BIN" >&2
		exit 1
	fi
	echo "BOOT_BIN=$BOOT_BIN"
fi

SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "SCRIPTS_DIR=$SCRIPTS_DIR"

BAREMETAL_DIR="$(realpath "$SCRIPTS_DIR/../")"
echo "BAREMETAL_DIR=$BAREMETAL_DIR"


RUNQEMU_DIR="${RUNQEMU_DIR:-$BAREMETAL_DIR/runqemu_dir_aiegen$AIE_GEN}"
mkdir -p "$RUNQEMU_DIR"
RUNQEMU_DIR="$(realpath "$RUNQEMU_DIR")"
echo "RUNQEMU_DIR=$RUNQEMU_DIR"

if [ -z "$AMD_EDF_SDK" ]; then
	AMD_EDF_SDK_DIR=$(ls -1d /opt/amd-edf/26.06+development*/ 2>/dev/null | sort | tail -n 1 | sed 's:/$::')
	if [ -z "$AMD_EDF_SDK_DIR" ]; then
		echo "Error: No directories found in /opt/amd-edf/" >&2
		echo "Please install by running /proj/yocto/qemu_delivery/2026.1/qemu_regression_latest/linux_sdk/x86_64-qemu-xilinx-nativesdk-standalone-26.06+development-S03160534.sh"
		exit 1
	fi
fi
AMD_EDF_SDK="${AMD_EDF_SDK:-$AMD_EDF_SDK_DIR/environment-setup-x86_64-amdedfsdk-linux}"
echo "AMD_EDF_SDK=$AMD_EDF_SDK"
source "$AMD_EDF_SDK"

case "$AIE_GEN" in
	1)
		echo "AIE_GEN 1 selected"
		echo "WIP. exiting"
		exit 0
		;;
	2)
		echo "AIE_GEN 2 selected"
		echo "WIP. exiting"
		exit 0
		;;
	5)
		echo "AIE_GEN 5 selected"
		VERSAL_ARTIFACTS_DIR="${VERSAL_ARTIFACTS_DIR:-/proj/yocto/edf/2026.1/stable/Yocto_edf_2026.1_latest/artifacts/versal-2ve-2vm-vek385-revb-multidomain_edf-ospi}"
		if [ ! -d "$VERSAL_ARTIFACTS_DIR" ]; then
			echo "Error: VERSAL_ARTIFACTS_DIR does not exist: $VERSAL_ARTIFACTS_DIR" >&2
			exit 1
		fi
		echo "VERSAL_ARTIFACTS_DIR=$VERSAL_ARTIFACTS_DIR"
		EDF_LINUX_DISK_IMAGE_DIR="${EDF_LINUX_DISK_IMAGE_DIR:-/proj/yocto/edf/2026.1/stable/Yocto_edf_2026.1_latest/artifacts/amd-cortexa78-mali-common_edf-linux-disk-image}"
		if [ ! -d "$EDF_LINUX_DISK_IMAGE_DIR" ]; then
			echo "Error: EDF_LINUX_DISK_IMAGE_DIR does not exist: $EDF_LINUX_DISK_IMAGE_DIR" >&2
			exit 1
		fi
		echo "EDF_LINUX_DISK_IMAGE_DIR=$EDF_LINUX_DISK_IMAGE_DIR"
		BOOT_VERSAL_QEMUBOOT_CONF="${BOOT_VERSAL_QEMUBOOT_CONF:-BOOT-versal-2ve-2vm-vek385-revb-multidomain.qemuboot.conf}"
		echo "BOOT_VERSAL_QEMUBOOT_CONF=$BOOT_VERSAL_QEMUBOOT_CONF"
		EDF_LINUX_DISK_IMAGE_QEMUBOOT_CONF="${EDF_LINUX_DISK_IMAGE_QEMUBOOT_CONF:-edf-linux-disk-image-amd-cortexa78-mali-common.rootfs.qemuboot.conf}"
		echo "EDF_LINUX_DISK_IMAGE_QEMUBOOT_CONF=$EDF_LINUX_DISK_IMAGE_QEMUBOOT_CONF"
		BOOT_VERSAL_BIN="${BOOT_VERSAL_BIN:-BOOT-versal-2ve-2vm-vek385-revb-multidomain.bin}"
		;;
	*)
		echo "Error: Unsupported AIE_GEN value: $AIE_GEN" >&2
		exit 1
		;;
esac

SYNC_RUNQEMU_ARTIFACTS="${SYNC_RUNQEMU_ARTIFACTS:-1}"
echo "SYNC_RUNQEMU_ARTIFACTS=$SYNC_RUNQEMU_ARTIFACTS"
if [ "$SYNC_RUNQEMU_ARTIFACTS" -ne 0 ]; then
	pushd "$RUNQEMU_DIR"
	rsync -arl "${VERSAL_ARTIFACTS_DIR}/" ./ &
	pid1=$!
	rsync -arl "${EDF_LINUX_DISK_IMAGE_DIR}/" ./ &
	pid2=$!

	echo "Waiting for rsync processes to complete... (PID1: $pid1, PID2: $pid2)"
	wait $pid1 $pid2
	echo "Both rsync processes completed successfully."

	if [ ! -f "$BOOT_VERSAL_QEMUBOOT_CONF" ]; then
		echo "Error: BOOT_VERSAL_QEMUBOOT_CONF does not exist: $BOOT_VERSAL_QEMUBOOT_CONF" >&2
		exit 1
	fi
	if [ ! -f "$EDF_LINUX_DISK_IMAGE_QEMUBOOT_CONF" ]; then
		echo "Error: EDF_LINUX_DISK_IMAGE_QEMUBOOT_CONF does not exist: $EDF_LINUX_DISK_IMAGE_QEMUBOOT_CONF" >&2
		exit 1
	fi

	qemuboot-tool \
		load "$BOOT_VERSAL_QEMUBOOT_CONF" \
		remove image_link_name \
		remove image_name \
		merge "$EDF_LINUX_DISK_IMAGE_QEMUBOOT_CONF" \
			> combined.qemuboot.conf
	sed -i 's|-serial null -serial null -serial null -serial mon:stdio|-serial null -serial null -serial mon:stdio -serial unix:serial4.sock,server,nowait|' combined.qemuboot.conf

	popd #RUNQEMU_DIR
fi

#placeholder for different method of bootmode. NOP for now
#BUILD_BOOTGEN=0
#if [ "$BUILD_BOOTGEN" -ne 0 ]; then
#	rm -rf $RUNQEMU_DIR/boot.bin-extracted-bm
#	cp $RUNQEMU_DIR/boot.bin-extracted $RUNQEMU_DIR/boot.bin-extracted-bm -r
#	cp bootgen-aiebaremetal.bif $RUNQEMU_DIR/boot.bin-extracted-bm/
#
#	pushd "$RUNQEMU_DIR"
#
#	pushd boot.bin-extracted-bm
#	cp $ELF ./vek385.elf
#	cp /mnt/1tb/scratch/mini-office/git/aiebaremetal/qemu-sim-vek385/example/testaie/kernel/aie.merged.cdo.bin ./
#	bootgen -image bootgen-aiebaremetal.bif -arch versal_2ve_2vm -dump bh -w -o boot.bin -log
#	popd #boot.bin-extracted-bm
#
#	#cp boot.bin-extracted-bm/boot.bin BOOT-versal-2ve-2vm-vek385-revb-multidomain.bin
#	cp boot.bin-extracted-bm/boot.bin BOOT-versal-2ve-2vm-vek385-revb-multidomain.bin
#	dd if=boot.bin-extracted-bm/boot.bin of=qemu-ospi.bin conv=notrunc
#
#	popd #RUNQEMU_DIR
#fi

if [ -z "$UPDATE_BOOT_BIN" ]; then UPDATE_BOOT_BIN=1; fi
echo "UPDATE_BOOT_BIN=$UPDATE_BOOT_BIN"
if [ "$UPDATE_BOOT_BIN" -ne 0 ]; then
	pushd "$RUNQEMU_DIR"
	if [ ! -f "$BOOT_VERSAL_BIN" ]; then
		echo "Error: BOOT_VERSAL_BIN does not exist: $BOOT_VERSAL_BIN" >&2
		exit 1
	fi
	cp "$BOOT_BIN" "$BOOT_VERSAL_BIN"
	dd if="$BOOT_BIN" of=qemu-ospi.bin conv=notrunc

	popd #RUNQEMU_DIR
fi

if [ -z "$RUN_QEMU" ]; then RUN_QEMU=1; fi
echo "RUN_QEMU=$RUN_QEMU"
if [ "$RUN_QEMU" -ne 0 ]; then
	pushd "$RUNQEMU_DIR"
	echo "Running QEMU with combined.qemuboot.conf..."
	echo "Default serial3 is selected for this terminal"
	echo "To connect to serial4, cd into $RUNQEMU_DIR"
	echo "and run: socat - UNIX-CONNECT:serial4.sock or nc -U serial4.sock"
	echo "Ctrl + a then x to exit QEMU"
	echo "Press any key to start..."
	read -n 1 -s -r
	runqemu combined.qemuboot.conf nographic slirp
	popd #RUNQEMU_DIR
fi
