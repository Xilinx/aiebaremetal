# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Repo root: this app lives at <repo>/example/testkernel_zephyr.
get_filename_component(AIEBAREMETAL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
set(AIE_RT_SRC_DIR ${AIEBAREMETAL_SRC_DIR}/thirdparty/aielib/aie-rt)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aie-rt-tests)

set(ENV{DEBUG} "true")

# AIE generation comes from build_zephyr.sh (-DAIE_GEN). Selects HW_GEN/geometry
# in src/include/hw_config.h. Defaults to VEK385 (gen 5 / AIE2PS); VEK280 -> 2 (AIE-ML).
if(NOT DEFINED AIE_GEN)
  set(AIE_GEN 5)
endif()
target_compile_definitions(app PUBLIC AIE_GEN=${AIE_GEN})
target_compile_definitions(app PUBLIC __AIEZEPHYR__=1)

include(${AIE_RT_SRC_DIR}/zephyr.cmake)
add_aie_rt_library(app)

include(${AIEBAREMETAL_SRC_DIR}/zephyr.cmake)
add_aiebaremetal_library(app)

target_sources(app PRIVATE src/graph.cpp)
target_include_directories(app PRIVATE src/)
