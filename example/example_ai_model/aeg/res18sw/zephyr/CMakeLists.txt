# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Repo root: this app lives at <repo>/example/example_ai_model/aeg/res18sw/zephyr.
get_filename_component(AIEBAREMETAL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../.. ABSOLUTE)
set(AIE_RT_SRC_DIR ${AIEBAREMETAL_SRC_DIR}/thirdparty/aielib/aie-rt)

# Reuse the baremetal host sources in-place; only the Zephyr build glue lives here.
get_filename_component(RES18_SW_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../sw ABSOLUTE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(resnet18)

set(ENV{DEBUG} "true")

# AIE generation comes from build_zephyr.sh (-DAIE_GEN). ResNet18 targets VEK280
# (gen 2 / AIE-ML), so default to 2 here rather than the testkernel's gen-5 default.
if(NOT DEFINED AIE_GEN)
  set(AIE_GEN 2)
endif()
target_compile_definitions(app PUBLIC AIE_GEN=${AIE_GEN})
target_compile_definitions(app PUBLIC __AIEZEPHYR__=1)

# Number of parallel pipeline instances (batch stays 1 per instance). Default 1
# (single-instance); build_zephyr.sh passes -DNUM_INST=10 for the inst10 design
# (baremetal buildbatch110.sh equivalent).
if(NOT DEFINED NUM_INST)
  set(NUM_INST 1)
endif()
target_compile_definitions(app PUBLIC NUM_INST=${NUM_INST})

# Multi-instance data-plumbing path (per-instance compute_graph.resnet18_<i>).
# BATCH_GRAPH is the upstream/baremetal macro name (kept for source parity with
# the shared resnet18_top.cpp); here it just means NUM_INST>1. The AIE Work/ and
# graph metadata must come from the matching design (build_zephyr.sh -inst10).
if(NUM_INST GREATER 1)
  target_compile_definitions(app PUBLIC BATCH_GRAPH)
endif()

# Length of a 'b' benchmark run (build_zephyr.sh -bench_loops N / -bench_seconds N):
# by round count, or by wall clock. The power measurements need minutes of steady
# load because the system controller samples its INA226 rails at best at ~1 Hz.
if(BENCH_LOOPS)
  target_compile_definitions(app PUBLIC BENCH_LOOPS=${BENCH_LOOPS})
endif()
if(BENCH_SECONDS)
  target_compile_definitions(app PUBLIC BENCH_SECONDS=${BENCH_SECONDS})
endif()

include(${AIE_RT_SRC_DIR}/zephyr.cmake)
add_aie_rt_library(app)

include(${AIEBAREMETAL_SRC_DIR}/zephyr.cmake)
add_aiebaremetal_library(app)

# ResNet18 host app, reused from ../sw (no copy).
target_sources(app PRIVATE ${RES18_SW_DIR}/resnet18_top.cpp)

# Zephyr-only: SBSA PL011 console RX bring-up (SYS_INIT). Kept out of the shared
# baremetal sources so it only compiles for the Zephyr build.
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/uart_rx_fixup.c)

# transactionsubmit.h / synset.h and the generated ./mladfpatch/... headers are
# resolved relative to RES18_SW_DIR; netprocess.h lives under the common net dir.
target_include_directories(app PRIVATE
  ${RES18_SW_DIR}
  ${RES18_SW_DIR}/mladfpatch
  ${RES18_SW_DIR}/mladfpatch/baremetal/ml_txn
  ${AIEBAREMETAL_SRC_DIR}/src/include/common/net
)
