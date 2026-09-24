# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

SCRIPT_DIR=$(dirname "$(readlink -f "$BASH_SOURCE")")

mkdir -p $SCRIPT_DIR/../../thirdparty/aielib/include

make -C $SCRIPT_DIR/../ alib
make -C $SCRIPT_DIR/../../thirdparty/aielib/aie-rt/fal/src/ include

cp -rf $SCRIPT_DIR/../../thirdparty/aielib/aie-rt/driver/internal/* $SCRIPT_DIR/../../thirdparty/aielib/include
cp $SCRIPT_DIR/../../thirdparty/arch/ps/psv_cortexa72_0/include/*.h $SCRIPT_DIR/../../thirdparty/aielib/include/
