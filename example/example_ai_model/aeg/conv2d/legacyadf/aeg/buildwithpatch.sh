#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ./build.sh 2 -bootgen -rpu
AIE_GEN=2
RPUFLAG=
BOOTGEN=
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


echo ${AIE_GEN}
#set this is is local env
source ../../../../../../script/settings.sh 1
#source ./envaie2pst50.sh
source compile.sh --runtime_source_file ./src/graph.cpp --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
#patch the aie_control.cpp to fix the ooo channel receive packet issue that cause ddrout dma hang
python3 ./dmapatch.py ./Work/ps/c_rts/aie_control.cpp ./aie_new.cpp
cp ./Work/ps/c_rts/aie_control.cpp ./aie_control_bak.cpp
cp ./aie_new.cpp ./Work/ps/c_rts/aie_control.cpp
source ./buildcdo.sh
