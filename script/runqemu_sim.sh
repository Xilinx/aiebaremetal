#!/usr/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
set -x
if [ -z "$BAREMETAL_TEST_DIR" ]; then
	echo "Error: BAREMETAL_TEST_DIR is not set" >&2
	echo "" >&2
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

if [ "$AIE_GEN" -neq 5 ]; then
	echo "This script is only intended for AIE_GEN=5 (Versal AI Core Gen 5) platforms" >&2
	exit 1
fi

SCRIPTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "SCRIPTS_DIR=$SCRIPTS_DIR"

BAREMETAL_DIR="$(realpath "$SCRIPTS_DIR/../")"
echo "BAREMETAL_DIR=$BAREMETAL_DIR"

if [ ! -f "$SCRIPTS_DIR/xsim_machine_path.sh" ]; then
	echo "Error: xsim_machine_path.sh script not found in $SCRIPTS_DIR" >&2
	exit 1
fi

source "$SCRIPTS_DIR/xsim_machine_path.sh"

if [ ! -f "$SCRIPTS_DIR/env_simulate.sh" ]; then
	echo "Error: env_simulate.sh script not found in $SCRIPTS_DIR" >&2
	exit 1
fi

source "$SCRIPTS_DIR/env_simulate.sh"

export AIESIM_CONFIG="$BAREMETAL_TEST_DIR/cfg/aie.sim.config.txt"
echo "AIESIM_CONFIG=$AIESIM_CONFIG"
if [ ! -f "$AIESIM_CONFIG" ]; then
	echo "Error: AIESIM_CONFIG file not found at $AIESIM_CONFIG" >&2
	exit 1
fi

export NOCSIM_MULTI_DRAM_FILE="$BAREMETAL_TEST_DIR/sim/behav_waveform/xsim/noc_memory_config.txt"
echo "NOCSIM_MULTI_DRAM_FILE=$NOCSIM_MULTI_DRAM_FILE"
if [ ! -f "$NOCSIM_MULTI_DRAM_FILE" ]; then
	echo "Error: NOCSIM_MULTI_DRAM_FILE not found at $NOCSIM_MULTI_DRAM_FILE" >&2
	exit 1
fi

export EMULATION_RUN_DIR="$BAREMETAL_TEST_DIR/sim/behav_waveform/xsim"
echo "EMULATION_RUN_DIR=$EMULATION_RUN_DIR"
if [ ! -d "$EMULATION_RUN_DIR" ]; then
	echo "Error: EMULATION_RUN_DIR does not exist: $EMULATION_RUN_DIR" >&2
	exit 1
fi

export VITIS_WAVEFORM="$BAREMETAL_TEST_DIR/sim/behav_waveform/xsim/dr_behav.wcfg"
echo "VITIS_WAVEFORM=$VITIS_WAVEFORM"
if [ ! -f "$VITIS_WAVEFORM" ]; then
	echo "Error: VITIS_WAVEFORM file not found at $VITIS_WAVEFORM" >&2
	exit 1
fi

export AIE_ENABLE_UNUSED_TILES=true
export AIE_SIM_LOG_DEBUG=true

echo "COSIM_MACHINE_PATH=$COSIM_MACHINE_PATH"
pushd "$EMULATION_RUN_DIR"
#./run.sh
./simulate.sh
popd
