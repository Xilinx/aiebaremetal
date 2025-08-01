# Copyright (C) 2025 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may
# not use this file except in compliance with the License. A copy of the
# License is located at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

cmake_minimum_required(VERSION 3.5)
project(adftoaeg)


set(CMAKE_BUILD_TYPE Debug)
#set the llvm location where you installed
set(LLVM_INSTALL_DIR /usr/local/)

set(CMAKE_PREFIX_PATH "${LLVM_INSTALL_DIR}/bin/" ${CMAKE_PREFIX_PATH})
find_program(LLVM_CONFIG llvm-config REQUIRED)

message(STATUS "Found LLVM ${LLVM_CONFIG}")

execute_process(
  COMMAND ${LLVM_CONFIG} --includedir
  OUTPUT_VARIABLE LLVM_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${LLVM_CONFIG} --cxxflags
  OUTPUT_VARIABLE LLVM_DEFINITIONS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_PREFIX_PATH "${LLVM_INSTALL_DIR}/lib/cmake/clang" ${CMAKE_PREFIX_PATH})
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found Clang ${Clang_PACKAGE_VERSION}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

include_directories(${Clang_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD 17)  
set(SOURCE_FILES ./src/adftoaeg.cc)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/..)

add_executable(adftoaeg ${SOURCE_FILES})

target_link_libraries(adftoaeg 
    PRIVATE
    clangTooling
)


