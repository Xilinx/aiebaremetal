#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
AIE_GEN=5

# Parse args:
#   <1|2|5>     positional AIE generation (default: 5)
while [[ $# -gt 0 ]]; do
	case "$1" in
		*)
			if [[ $1 =~ ^[0-9]+$ ]]; then
				AIE_GEN=$1
				shift
			else
				echo "Unknown option: $1" >&2
				return 1 2>/dev/null || exit 1
			fi
			;;
	esac
done
#clear the parameter number to avoid enclose script inherit the param and get confuse
set --

echo "AIE_GEN=${AIE_GEN}"

HW_GEN=$(python3 vercheck.py 2>/dev/null) || true

if [[ -z "${HW_GEN}" ]]; then
	echo "WARNING: vercheck.py did not produce hw_gen (need ../Work/ps/c_rts/aie_control_config.json from a successful aiecompiler run)." >&2
elif [[ "${AIE_GEN}" != "${HW_GEN}" ]]; then
	echo "HW gen is ${HW_GEN} but request to build AIE_GEN ${AIE_GEN}, info mismatch" >&2
	return 1
fi

export ARCH_DIR=`pwd`/../../../../thirdparty/arch/ps/
export ARCH_78_DIR=$ARCH_DIR/cortexa78_0/

BIF=vek385.bif
BOOTBIN=Vek385SoftmaxPLBOOT.BIN
ELF=vek385.elf

# compile plio kernels and link the platform
pushd .
cd ../pl_kernels
source ./build.sh ${AIE_GEN} || {
	popd
	return 1
}
popd

if [ ! -f "../pl_kernels/new.xsa" ]; then
	echo "ERROR: Missing ../pl_kernels/new.xsa after pl_kernels build." >&2
	return 1
fi

# generate platform
export PFM_NAME="pfm_baremetal"
vitis -s platform_baremetal.py --xsa ../pl_kernels/new.xsa || return 1

export SW_DOMAIN=./workspace/platform_baremetal/cortexa78_0/standalone_cortexa78_0/bsp
if [ ! -f "${SW_DOMAIN}/include/xil_cache.h" ]; then
	echo "ERROR: Missing ${SW_DOMAIN}/include/xil_cache.h after platform generation." >&2
	return 1
fi

# This board's physical console cable is wired to PS UART1 (0xf1930000), but
# platform_baremetal.py always generates a BSP whose STDIN/STDOUT point at PS
# UART0 (0xf1920000) regardless of which UART is physically connected on this
# board. outbyte()/inbyte() (bsp/libsrc/uartpsv/src/xuartpsv_hw.c) bake
# STDIN/STDOUT_BASEADDRESS in as an immediate at compile time, so patching the
# header alone does nothing until that one object is recompiled and updated
# into libxil.a. Must run after platform_baremetal.py (which wipes and
# regenerates ./workspace from scratch every time) and before the host ELF
# link below, which is what actually consumes libxil.a.
CONSOLE_UART1_ADDR=0xf1930000
# Vitis relocates the BSP's generated trees between releases, so find every
# copy of bspconfig.h the platform build produced instead of listing paths.
for cfg in $(find ./workspace -type f -name bspconfig.h); do
	sed -i -E "s/#define (STDIN|STDOUT)_BASEADDRESS 0x[0-9a-fA-F]+/#define \1_BASEADDRESS ${CONSOLE_UART1_ADDR}/" "$cfg"
done

UARTPSV_HW_SRC="${SW_DOMAIN}/libsrc/uartpsv/src/xuartpsv_hw.c"
LIBXIL="${SW_DOMAIN}/lib/libxil.a"
if [ -f "$UARTPSV_HW_SRC" ] && [ -f "$LIBXIL" ]; then
	AARCH64_GCC="$XILINX_VITIS/gnu/aarch64/lin/aarch64-none/bin/aarch64-none-elf-gcc"
	# ar replaces a member by basename, so build the object outside the BSP
	# tree; where Vitis keeps its CMake artifacts then stops mattering.
	UARTPSV_TMP="$(mktemp -d)"
	"$AARCH64_GCC" \
		-isystem "${SW_DOMAIN}/include" \
		-DSDT -DARMA78_EL3 \
		-specs="${SW_DOMAIN}/Xilinx.spec" \
		-c -O2 -g -Wall -Wextra -fno-tree-loop-distribute-patterns \
		-o "${UARTPSV_TMP}/xuartpsv_hw.c.obj" "$UARTPSV_HW_SRC" \
		|| { rm -rf "$UARTPSV_TMP"; return 1; }
	ar r "$LIBXIL" "${UARTPSV_TMP}/xuartpsv_hw.c.obj" \
		|| { rm -rf "$UARTPSV_TMP"; return 1; }
	rm -rf "$UARTPSV_TMP"
	echo "Patched libxil.a: outbyte()/inbyte() now target PS UART1 (${CONSOLE_UART1_ADDR}) to match this board's console wiring."
else
	echo "WARNING: could not apply VEK385 console-UART1 patch (missing $UARTPSV_HW_SRC or $LIBXIL)." >&2
fi
# Host source: src/graph_new_api.cpp (typed AEG API). AEG_INHERITANCE_BASED_API
# makes aiebaremetal.h pull in generated_graphs.h.
HOST_SRC=../src/graph_new_api.cpp
HOST_DEFS=-DAEG_INHERITANCE_BASED_API
# aiemetacpp.py wrote generated_graphs.h into example/ during `make hw`;
# stage it on the baremetal include path so aiebaremetal.h finds it.
GENERATED_GRAPHS_SRC=../../../generated_graphs.h
if [ ! -f "$GENERATED_GRAPHS_SRC" ]; then
	echo "ERROR: $GENERATED_GRAPHS_SRC not found - 'make hw' must run before builda78.sh." >&2
	return 1 2>/dev/null || exit 1
fi
cp "$GENERATED_GRAPHS_SRC" ../../../../src/include/generated_graphs.h

aarch64-none-elf-gcc $HOST_DEFS -mcpu=cortex-a78 -Wl,-T -Wl,$ARCH_78_DIR//standalone_cortexa78_0/bsp/lscript.ld -D__AIE_ARCH__=22 -I../../../../thirdparty/aielib/aie-rt/driver/internal/ -I../../../../src/include/ -I$XILINX_VITIS/aietools/include/ -I$SW_DOMAIN/include -L$SW_DOMAIN/lib -L../../../build/ -o ./${ELF} $HOST_SRC -Wl,--start-group,-laiebaremetala78,-lxil,-lxiltimer,-lxilstandalone,-lgcc,-lc,-lstdc++,-lm,--end-group
if [ ! -f "./${ELF}" ]; then
	echo "ERROR: Failed to build ${ELF}." >&2
	return 1
fi

#generate the boot.bin
if [ ! -f "../pl_kernels/_x/link/vivado/vpl/prj/prj.runs/impl_1/vitis_design_wrapper_boot.pdi" ]; then
	echo "ERROR: Missing linked PDI at ../pl_kernels/_x/link/vivado/vpl/prj/prj.runs/impl_1/vitis_design_wrapper_boot.pdi." >&2
	return 1
fi

BOOTGEN_BIN="${XILINX_VITIS}/bin/bootgen"
if [ ! -x "${BOOTGEN_BIN}" ]; then
	BOOTGEN_BIN="/proj/xbuilds/2025.1_daily_latest/installs/lin64/2025.1/Vitis/bin/bootgen"
fi
if [ ! -x "${BOOTGEN_BIN}" ]; then
	echo "ERROR: bootgen executable not found." >&2
	return 1
fi

"${BOOTGEN_BIN}" -arch versal_2ve_2vm -image ${BIF} -o ${BOOTBIN} -w || return 1
if [ ! -f "./${BOOTBIN}" ]; then
	echo "ERROR: Failed to generate ${BOOTBIN}." >&2
	return 1
fi
mkdir -p ../build
mv ./${BOOTBIN} ../build/
