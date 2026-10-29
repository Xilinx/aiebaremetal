#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Use the Vitis already in the environment (source settings64.sh first).

_TUTORIAL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "${_TUTORIAL_DIR}/vitis_env.sh" || exit 1

pushd dependency/aie/
make compile
popd

pushd dependency/
source ./gensdtbsp.sh
source ./genmetacpp.sh
source ./genbootbin.sh
popd

source ./buildmeta.sh
source ./buildmain.sh
source ./buildsdtbspelf.sh
