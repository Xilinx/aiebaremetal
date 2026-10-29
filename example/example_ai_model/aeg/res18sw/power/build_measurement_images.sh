#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# Build the four images behind POWER_RESULTS.md: baremetal and Zephyr, at
# NUM_INST=1 and NUM_INST=10, each with 'b' bounded by wall clock instead of
# round count (-DBENCH_SECONDS=300). A fixed-length plateau is what makes the
# rails measurable: one 9-rail snapshot costs 5-7 s through the system controller.
#
# Each driver overwrites the SAME output path, so images are copied out under
# names that record what they are -- the .BIN itself is not self-describing.
#
# Every build installs its own design's graph metadata (sw/sync_graph_metadata.sh),
# so switching instance counts no longer needs a clean build. The Zephyr builds
# still pass -no_graph_build to reuse the design the baremetal twin just compiled,
# which is why each pair is ordered baremetal first.
set -x
RES18=$(cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd)
AIEBM=$(cd "${RES18}/../../../../" && pwd)
OUT=${OUT:-$HOME/power_images}
SECS=${SECS:-300}
ZFLAGS="-no_graph_build -no_west_update -zephyr_dir ${AIEBM}/zephyr_dir"

export CCACHE_DIR=${CCACHE_DIR:-$HOME/.ccache}
export ZEPHYR_TOOLCHAIN_VARIANT=${ZEPHYR_TOOLCHAIN_VARIANT:-zephyr}
mkdir -p ${OUT}

fail() { echo "BUILD_FAILED: $1"; exit 1; }

# Each baremetal driver is sourced in its OWN shell: build.sh and buildbatch110.sh
# share GRAPH_PARAMS, and sourcing both into one shell leaks -DNUM_INST=10 into the
# single-instance build.
cd ${RES18}/sw || fail cd
EXTRA_DEFINES="-DBENCH_SECONDS=${SECS}" bash -c 'source ./buildbatch110.sh -bootgen' || fail bm10
cp build/Vek280BOOT.BIN ${OUT}/t${SECS}_bm_inst10.BIN || fail bm10_cp

cd ${RES18}/zephyr || fail cd
./build_zephyr.sh -inst10 -bench_seconds ${SECS} ${ZFLAGS} || fail z10
cp build/Vek280_zephyr.BIN ${OUT}/t${SECS}_zephyr_inst10.BIN || fail z10_cp

cd ${RES18}/sw || fail cd
EXTRA_DEFINES="-DBENCH_SECONDS=${SECS}" bash -c 'source ./build.sh -bootgen' || fail bm1
cp build/Vek280BOOT.BIN ${OUT}/t${SECS}_bm_inst1.BIN || fail bm1_cp

cd ${RES18}/zephyr || fail cd
./build_zephyr.sh -bench_seconds ${SECS} ${ZFLAGS} || fail z1
cp build/Vek280_zephyr.BIN ${OUT}/t${SECS}_zephyr_inst1.BIN || fail z1_cp

ls -l ${OUT}/t${SECS}_*
echo ALL_MEASUREMENT_IMAGES_BUILT
