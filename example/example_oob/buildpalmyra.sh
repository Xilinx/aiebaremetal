#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ./build.sh 2 -bootgen -rpu --workfolder ./Work/
RPUFLAG=
BOOTGEN=
if [ $# -gt 0 ]; then
	BOOTGEN=$1
	#-bootgen
fi
if [ $# -gt 1 ]; then
	RPUFLAG=$2
	#-rpu
fi


#set this is is local env
source ../../script/settings.sh 1
compile.sh --runtime_source_file ./src/graph.cpp --partition_option "--enable-partition=0:12:pr0" -palmyra --aie_version 5 ${RPUFLAG} ${BOOTGEN}
#compile.sh --runtime_source_file ./src/graph.cpp --aie_version 1 -bootgen --workfolder ./Work/
#compile.sh --runtime_source_file ./src/graph.cpp --partition_option "--enable-partition=0:12:pr0" --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}

