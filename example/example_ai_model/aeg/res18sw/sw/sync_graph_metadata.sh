#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Install the graph metadata belonging to ONE design: the graph-class header
# (generated_graphs.h) and the config registry (baremetal_metadata.cpp, the blob
# get_graph_config() looks up by name).
#
# Why this exists: compile.sh runs this codegen only when called WITHOUT
# --workfolder, and every resnet18 driver passes --workfolder, so the installed
# copies are whatever the previously built design left behind. The single- and
# 10-instance designs both register a graph named 'compute_graph', so a mismatch
# is not caught by name: the host boots, then the control transaction addresses
# instances the design never configured and the first inference hangs in a DMA
# mask-poll (seen as ~19.5 ms/round becoming an indefinite stall at NUM_INST=10).
# Hence every design switch must reinstall these, which is why switching between
# build.sh and buildbatch110.sh used to require a clean build.
#
# Usage: ./sync_graph_metadata.sh <design Work dir>

set -e
WORK=$1
if [ -z "${WORK}" ] || [ ! -d "${WORK}" ]; then
    echo "usage: $0 <design Work dir>" >&2
    exit 1
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
AIEBM_DIR=$(cd "${SCRIPT_DIR}/../../../../../" && pwd)
CRTS=${WORK}/ps/c_rts

if [ ! -f "${CRTS}/aie_control_config.json" ]; then
    echo "ERROR: ${CRTS}/aie_control_config.json missing; compile the design first" >&2
    exit 1
fi

pushd "${CRTS}" > /dev/null
if [ -f ../../arch/aie_partition.json ]; then
    python3 "${AIEBM_DIR}/script/aiemetacpp.py" aie_control_config.json ../../arch/aie_partition.json
else
    python3 "${AIEBM_DIR}/script/aiemetacpp.py" aie_control_config.json
fi
popd > /dev/null

mkdir -p "${AIEBM_DIR}/example/build"
cp "${CRTS}/generated_graphs.h"     "${AIEBM_DIR}/src/include/"
cp "${CRTS}/generated_graphs.h"     "${SCRIPT_DIR}/"
cp "${CRTS}/baremetal_metadata.cpp" "${AIEBM_DIR}/example/build/"

echo "[sync_graph_metadata] installed from ${WORK}"
echo "[sync_graph_metadata] baremetal_metadata.cpp: $(stat -c%s "${AIEBM_DIR}/example/build/baremetal_metadata.cpp") bytes"
