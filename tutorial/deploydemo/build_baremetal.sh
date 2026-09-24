# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
SRCS_CPP="./src/*.cpp  ./src/common_layer/*.cpp ./src/common_layer/abrothers/*.cpp"
BAREMETALINC=./src/include/
BAREMETALINC_COMMON=./src/include/common_layer/
BAREMETALINC_COMMON_OTHER=./src/include/common_layer/aegothers/
cflags="-Wall -Wextra -O0 -g -fPIC -I./tutorial/deploydemo/dependency/psv_cortexa72_0/include/ -I$BAREMETALINC -I$BAREMETALINC_COMMON -I$BAREMETALINC_COMMON_OTHER"
PETALINUX_PATH="/proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/"
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
source "$PETALINUX_PATH/settings.sh"
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
	          
