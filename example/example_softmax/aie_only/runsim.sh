# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
bash -c 'export SCRIPT_DIR=$(dirname "$(realpath "$0")") && export AIE_COMPILE_WORK="$SCRIPT_DIR/Work/" && echo "$AIE_COMPILE_WORK" && cd ../../ && source ./aieldpath.sh && cd ./testUT/softmax && source envaie2pst50.sh && aiesimulator --profile --disable-unused-mem-tiles=false --enable-uc-uart-to-stdout=true --pkg-dir=./Work'
#bash -c "cd ../;source ./aieldpath.sh; cd ./testkernel/; source envaie2pst50.sh;aiesimulator --profile --disable-unused-mem-tiles=false --pkg-dir=./Work"
