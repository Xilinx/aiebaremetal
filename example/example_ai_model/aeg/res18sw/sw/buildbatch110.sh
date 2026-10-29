#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#source ./buildbatch110.sh -bootgen
source /proj/xbuilds/2024.2_INT_qualified_latest/installs/lin64/Vitis/2024.2/settings64.sh
AIECOMPILERPATH=$(which aiecompiler)
AIETOOLPATH=$(dirname "$(dirname "$AIECOMPILERPATH")")
export XILINX_VITIS_AIETOOLS=$AIETOOLPATH
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
export MLLIB_ROOT=$(cd "$SCRIPT_DIR/../../../mllib" && pwd)
AIE_GEN=2
RPUFLAG=
BOOTGEN=
AIEFOLDER=$(pwd)/../aie/batch1_inst10/
AIEWORKFOLDER=$AIEFOLDER/Work/
# Own GRAPH_PARAMS outright (do not append inherited shell state). Appending
# `${GRAPH_PARAMS}` let stale values leak across builds sourced into the same
# bash session; set it explicitly so inst10 config is deterministic.
# EXTRA_DEFINES adds host-app -D flags on top, e.g.
#   EXTRA_DEFINES="-DBENCH_SECONDS=300" source ./buildbatch110.sh -bootgen
# for the long-running 'b' benchmark the power measurements sample.
GRAPH_PARAMS="-DNUM_INST=10 -DBATCH_GRAPH ${EXTRA_DEFINES}"
echo "GRAPH_PARAMS: ${GRAPH_PARAMS}"
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
	make compile NUM_INST=10
	popd
fi

rm -f ./Work
ln -s $AIEWORKFOLDER $(pwd)/

# Install THIS design's graph metadata. compile.sh skips that codegen whenever
# --workfolder is passed (which the call below does), so without this the host
# links against whatever design was built last -- and since the 1- and
# 10-instance designs both register 'compute_graph', the mismatch is silent:
# the app boots, then the first inference hangs in a DMA mask-poll on instances
# the design never configured. This is why switching between build.sh and
# buildbatch110.sh used to need a clean build.
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
# Consumed by script/compile.sh's host_compiler_options_aie. Mirrors build.sh.
export HOST_OPT_AIE="-Os"
compile.sh --workfolder "$AIEWORKFOLDER" --runtime_source_file $(pwd)/resnet18_top.cpp --aie_version ${AIE_GEN} -hw_only ${RPUFLAG} -bootgen -lwip --preproc_options "${GRAPH_PARAMS}" --compiler_options "--enable-multi-layer --multi-layer-ipu-txn-bin-sim --include=../../common/conv2d_4x4/L1/include/common/ --include=../../common/conv2d_4x4/L2/include/common/ --include=/proj/xbuilds/2024.1_INT_0522_2023/installs/lin64/Vitis/2024.1/aietools/include/aie_api/ --include=/proj/xbuilds/2024.1_INT_0522_2023/installs/lin64/Vitis/2024.1/aietools/include/drivers/aiengine/"
