#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ./build.sh 2 -bootgen -rpu
AIE_GEN=1
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
source ../../script/settings.sh 1
compile.sh --runtime_source_file ./graph.cpp --workfolder  ./_x/package/ --control_json ./_x/package/aie_control_config.json --partition_json ./_x/package/aie_partition.json --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN} -hw_only
# test_partition1 Workfolder
#compile_merged.sh --runtime_source_file ./graph.cpp --workfolder   test_partition2/Work/ps/c_rts --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}
#compile.sh --runtime_source_file ./graph.cpp  --aie_version ${AIE_GEN} ${RPUFLAG} ${BOOTGEN}

