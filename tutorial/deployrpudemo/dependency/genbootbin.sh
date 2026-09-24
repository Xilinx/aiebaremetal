# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIF=$SCRIPT_DIR/vek280_rpu_boot_image.bif
ARCH=$SCRIPT_DIR/../../../thirdparty/arch/
pushd $SCRIPT_DIR
ln -s $ARCH ./arch
bootgen -arch versal -image ${BIF} -o boot.BIN -w
popd
