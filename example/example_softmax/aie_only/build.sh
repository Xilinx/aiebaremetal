#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ./build.sh 2 -bootgen -rpu
#source ./build.sh 2 -bootgen "" -custom_pdi   # production-platform-locked PDI (S-prod boards)
AIE_GEN=1
RPUFLAG=
BOOTGEN=
CUSTOMPDIFLAG=
if [ $# -gt 0 ]; then
	AIE_GEN=$1
fi
if [ $# -gt 1 ]; then
	BOOTGEN=$2
	#-bootgen
fi
if [ $# -gt 2 ]; then
	RPUFLAG=$3
	#-rpu
fi
if [ $# -gt 3 ]; then
	CUSTOMPDIFLAG=$4
	#-custom_pdi -- links against the real production platform (full Vivado
	#synth+impl+bitgen, ~20+ min) so the PDI's IDCODE matches production
	#(S-prod) silicon. Without it, the PDI is generic/unlocked and fails to
	#program S-prod boards (PLM Error Major 0x326 Minor 0x14).
fi


echo ${AIE_GEN}
#set this is is local env
source ../../../script/settings.sh 1
compile.sh --runtime_source_file ./src/graph.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN} ${CUSTOMPDIFLAG}
