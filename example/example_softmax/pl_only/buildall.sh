#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

PL_ONLY_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

fail() {
	echo "ERROR: $*" >&2
	if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
		return 1
	fi
	exit 1
}

abort_buildall() {
	fail "$@"
	[[ "${BASH_SOURCE[0]}" != "${0}" ]] && return 1
	exit 1
}

AIE_GEN=1
hw_emu=false

# Parse args:
#   <1|2|5>     positional AIE generation (default: 1)
#   qemu        build PL kernels with -t hw_emu (generates launch_hw_emu.sh)
while [[ $# -gt 0 ]]; do
	case "$1" in
		qemu)
			hw_emu=true
			shift
			;;
		*)
			if [[ $1 =~ ^[0-9]+$ ]]; then
				AIE_GEN=$1
				shift
			else
				fail "Unknown option: $1"
			fi
			;;
	esac
done

source "${PL_ONLY_DIR}/../../../script/settings.sh" 1

if [[ -z "${XILINX_VITIS:-}" ]]; then
	fail "XILINX_VITIS is not set"
fi
export XILINX_VITIS_AIETOOLS="${XILINX_VITIS}/aietools"

export PL_TARGET="hw"
if [[ "$hw_emu" == true ]]; then
	export PL_TARGET="hw_emu"
	echo "hw_emu mode: PL kernels will be built with -t hw_emu (generates launch_hw_emu.sh)"
fi

if ! (
	cd "${PL_ONLY_DIR}" || abort_buildall "cannot cd to ${PL_ONLY_DIR}"
	make compile "AIE_GEN=$AIE_GEN"
); then
	abort_buildall "make compile failed (AIE graph / Work/ outputs missing)"
fi

LIBADF="${PL_ONLY_DIR}/libadf.a"
if [ ! -f "${LIBADF}" ]; then
	abort_buildall "Expected ${LIBADF} after AIE compile — check aiecompiler logs"
fi

CONTROL_JSON="${PL_ONLY_DIR}/Work/ps/c_rts/aie_control_config.json"
if [ ! -f "${CONTROL_JSON}" ]; then
	abort_buildall "Expected ${CONTROL_JSON} after AIE compile — check aiecompiler logs"
fi

SCSIM_CONFIG="${PL_ONLY_DIR}/Work/config/scsim_config.json"
if [ ! -f "${SCSIM_CONFIG}" ]; then
	abort_buildall "Expected ${SCSIM_CONFIG} after AIE compile — check aiecompiler logs"
fi

EXAMPLE_DIR="${PL_ONLY_DIR}/../.."
pushd "${EXAMPLE_DIR}" || fail "cannot cd to ${EXAMPLE_DIR}"
if [ "$AIE_GEN" == "5" ]; then
	echo "********************A78*************************"
	make hw_external_apu78 "AIE_KERNEL_LOC=${PL_ONLY_DIR}" "AIE_GEN=$AIE_GEN" -j32 || abort_buildall "make hw_external_apu78 failed"
else
	echo "********************A72*************************"
	make hw "AIE_KERNEL_LOC=${PL_ONLY_DIR}" "AIE_GEN=$AIE_GEN" -j32 || abort_buildall "make hw failed"
fi

make build/libaiebaremetal.so "AIE_KERNEL_LOC=${PL_ONLY_DIR}" "AIE_GEN=$AIE_GEN" -j32 || abort_buildall "make build/libaiebaremetal.so failed"
popd

pushd "${PL_ONLY_DIR}/script" || fail "cannot cd to ${PL_ONLY_DIR}/script"
if [ "$AIE_GEN" == "5" ]; then
	source ./builda78.sh "$AIE_GEN" || abort_buildall "builda78.sh failed"
else
	source ./builda72.sh "$AIE_GEN" || abort_buildall "builda72.sh failed"
fi

popd
