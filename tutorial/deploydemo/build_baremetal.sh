#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Use the Vitis already in the environment (source settings64.sh first).

_TUTORIAL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "${_TUTORIAL_DIR}/vitis_env.sh" || return 1 2>/dev/null || exit 1

SRCS_CPP="./src/*.cpp  ./src/common_layer/*.cpp ./src/common_layer/abrothers/*.cpp"
BAREMETALINC=./src/include/
BAREMETALINC_COMMON=./src/include/common_layer/
BAREMETALINC_COMMON_OTHER=./src/include/common_layer/aegothers/
cflags="-Wall -Wextra -O0 -g -fPIC -I./tutorial/deploydemo/dependency/psv_cortexa72_0/include/ -I$BAREMETALINC -I$BAREMETALINC_COMMON -I$BAREMETALINC_COMMON_OTHER"
#Default compiler, assembler, archiver
compiler="aarch64-none-elf-gcc"
assembler="aarch64-none-elf-as"
archiver="aarch64-none-elf-ar"
# Print usage
usage() {
    echo "Usage: $0 --compiler <compiler> --assembler <assembler> --archiver <archiver> --cflags <cflags>"
    exit 1
}
while [[ $# -gt 0 ]]; do
    case "$1" in
        -help)
            usage
            ;;
        --compiler)
            compiler="$2"
            shift 2
            ;;
        --assembler)
            assembler="$2"
            shift 2
            ;;
        --archiver)
            archiver="$2"
            shift 2
            ;;
        --cflags)
            cflags="$2"
            shift 2
            ;;

        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done
if ! command -v aarch64-none-elf-gcc &>/dev/null; then
    echo "Error: aarch64-none-elf-gcc not found. Source Vitis settings64.sh first." >&2
    return 1 2>/dev/null || exit 1
fi
pushd ../../
if [ ! -d "build" ]; then
  echo "Build folder not found. Creating it..."
  mkdir build
else
  echo "Build folder already exists."
fi

make clean -f ./Makefile
make release -f ./Makefile  SHELL="/bin/sh" CC=$compiler AS=$assembler AR=$archiver CFLAGS="-O2 -c $cflags -DXAIE_DEBUG -g -Wall -Wextra -DNO_JSON -Dversal -DARMA72_EL3 -fno-tree-loop-distribute-patterns" BAREMETAL_SRCS_DIR="./src"  BAREMETAL_CPP_SRCS=$SRCS_CPP
popd
cp ../../build/libaiebaremetal_api.a ./
