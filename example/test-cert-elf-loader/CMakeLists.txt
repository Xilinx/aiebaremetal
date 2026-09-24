# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.0)

project(test_cert_elf_loader VERSION 0.0 LANGUAGES CXX)

add_executable(test_cert_elf_loader src/main.cc ../src/cert_elf_loader.cc)

target_include_directories(test_cert_elf_loader PRIVATE include ../include ../thirdparty/ELFIO/elfio/ ../thirdparty/ELFIO/)

target_link_libraries(test_cert_elf_loader PRIVATE elf)
