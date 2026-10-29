#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ./build.sh 2 -bootgen -rpu
source /proj/xbuilds/2024.2_INT_qualified_latest/installs/lin64/Vitis/2024.2/settings64.sh
AIECOMPILERPATH=$(which aiecompiler)
AIETOOLPATH=$(dirname "$(dirname "$AIECOMPILERPATH")")
export XILINX_VITIS_AIETOOLS=$AIETOOLPATH
AIE_GEN=2
RPUFLAG=
BOOTGEN=
# Single-instance build: own GRAPH_PARAMS outright. Do NOT inherit it from the
# shell, otherwise a prior `source ./buildbatch110.sh` in the same bash session
# leaks `-DNUM_INST=10 -DBATCH_GRAPH` into this single build, compiling the host
# for 10 instances against the 1-instance aie/Work graph (host/graph mismatch).
# EXTRA_DEFINES adds host-app -D flags on top, e.g.
#   EXTRA_DEFINES="-DBENCH_SECONDS=300" source ./build.sh -bootgen
# for the long-running 'b' benchmark the power measurements sample.
GRAPH_PARAMS="${EXTRA_DEFINES}"
echo "GRAPH_PARAMS: ${GRAPH_PARAMS}"
AIEFOLDER=$(pwd)/../aie/
AIEWORKFOLDER=$AIEFOLDER/Work/
if [ $# -gt 0 ]; then
	BOOTGEN=$1
	#-bootgen
fi
if [ $# -gt 1 ]; then
	RPUFLAG=$2
	#-rpu
fi

echo ${AIE_GEN}

if [ ! -d "$AIEWORKFOLDER" ]; then
	pushd $AIEFOLDER
	make compile
	popd
fi

# Always (re)point the sw/Work symlink at the single-instance AIE Work,
# even when AIEWORKFOLDER already exists from an earlier build. Without
# this the symlink may still point at aie/batch1_inst10/Work/ (from a
# prior `buildbatch110.sh` run), causing the mladfpatch ml_txn build to
# fail with "aie_control.cpp: No such file or directory". Mirrors the
# unconditional symlink setup in buildbatch110.sh.
rm -f ./Work
ln -s $AIEWORKFOLDER $(pwd)/

# Install THIS design's graph metadata; see sync_graph_metadata.sh. compile.sh
# skips that codegen whenever --workfolder is passed (as below), so without this
# the host links against whatever design was built last. Both designs register a
# graph named 'compute_graph', so the mismatch is silent at init and shows up as
# the first inference hanging in a DMA mask-poll.
if ! bash ./sync_graph_metadata.sh "$AIEWORKFOLDER"; then
	echo "ERROR: graph metadata sync failed; aborting build" >&2
	return 1 2>/dev/null || exit 1
fi

pushd ./mladfpatch/baremetal/ml_txn/
source ./build.sh
cd ../
source ./buildappbootbin.sh
popd
pushd ./mladfpatch
python3 ./dataconvert.py 
popd
 
source ../../../../../script/settings.sh 1
# Build the baremetal host app with -Os to match the Zephyr app's size
# optimization (CONFIG_SIZE_OPTIMIZATIONS) for an apples-to-apples comparison.
# Consumed by script/compile.sh's host_compiler_options_aie.
export HOST_OPT_AIE="-Os"
compile.sh --workfolder "$AIEWORKFOLDER" --runtime_source_file $(pwd)/resnet18_top.cpp --aie_version ${AIE_GEN} -hw_only ${RPUFLAG} ${BOOTGEN} -lwip --preproc_options "${GRAPH_PARAMS}" --compiler_options "--enable-multi-layer --multi-layer-ipu-txn-bin-sim --include=../../common/conv2d_4x4/L1/include/common/ --include=../../common/conv2d_4x4/L2/include/common/ --include=/proj/xbuilds/2024.1_INT_0522_2023/installs/lin64/Vitis/2024.1/aietools/include/aie_api/ --include=/proj/xbuilds/2024.1_INT_0522_2023/installs/lin64/Vitis/2024.1/aietools/include/drivers/aiengine/"
