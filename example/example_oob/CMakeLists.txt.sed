# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(AIE_RT_SRC_DIR @AIE_RT_SRC_DIR@)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aie-rt-tests)

set(ENV{DEBUG} "true")

target_compile_definitions(app PUBLIC AIE_GEN=5)
target_compile_definitions(app PUBLIC __AIE_ARCH__=22)
target_compile_definitions(app PUBLIC ARMR5=1)
target_compile_definitions(app PUBLIC DEVICE=50)
target_compile_definitions(app PUBLIC __AIEZEPHYR__=1)
target_compile_definitions(app PUBLIC _RPU_=1)
target_compile_definitions(app PUBLIC _APURPU_=1)
#target_compile_definitions(app PUBLIC __AIEBAREMETAL__=1)

#include(/mnt/mini-office-git/zephyrproject/zephyr/lib/aie-rt/aie-rt/zephyr.cmake)

#include(/mnt/raid5/git/aie-rt/zephyrissue-aie/zephyr.cmake)
include(@AIE_RT_SRC_DIR@/zephyr.cmake)
add_aie_rt_library(app)

set(AIEBAREMETAL_SRC_DIR @AIEBAREMETAL_SRC_DIR@)
include(@AIEBAREMETAL_SRC_DIR@/zephyr.cmake)
add_aiebaremetal_library(app)
#add_subdirectory(/mnt/mini-office-git/zephyrproject/zephyr/lib/aie-rt/aie-rt aie-rt)
#add_library(aie-rt STATIC IMPORTED)
#set_target_properties(aie-rt PROPERTIES IMPORTED_LOCATION
#  /mnt/mini-office-git/zephyrproject/zephyr/lib/aie-rt/build/libaie-rt.a)


#zephyr_library_include_directories(
#  /mnt/mini-office-git/zephyrproject/zephyr/lib/aie-rt/aie-rt/
#)
target_sources(app PRIVATE src/graph.cpp)
target_include_directories(app PRIVATE src/)

#target_link_libraries(app PRIVATE aie-rt)
