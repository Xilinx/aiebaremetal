# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
bash -c 'export SCRIPT_DIR=$(dirname "$(realpath "$0")") && export AIE_COMPILE_WORK="$SCRIPT_DIR/Work/" && echo "$AIE_COMPILE_WORK" && pushd ../ && source ./aieldpath.sh && popd && source envaie20231.sh && aiesimulator --profile --disable-unused-mem-tiles=false --pkg-dir=./Work'
