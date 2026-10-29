#!/usr/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

set -x

# Vitis ships an OpenSSL whose CA store is elsewhere, so wget/git can't verify
# github.com. Nothing here needs Vitis.
unset LD_LIBRARY_PATH LD_PRELOAD

# zephyr-amd master-next (2026.2) requires SDK >= 0.17.4; older SDKs fail to
# build arch/arm64 (versal_apu) with a parse error on __weak.
ZEPHYR_SDK_VERSION=0.17.4

TARGET_DIR=

while [[ $# -gt 0 ]]; do
    case "$1" in
        -target_dir|--target_dir)
            TARGET_DIR="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [ -z "$TARGET_DIR" ]; then
    echo "Error: Target directory not specified. Use -target_dir or --target_dir to set it." >&2
    exit 1
fi

if [ ! -d "$TARGET_DIR" ]; then
    mkdir -p "$TARGET_DIR"
fi

echo "###########################################################################################"
echo "Make sure you install dependencies for zephyr: https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html#install-dependencies"
echo "###########################################################################################"

# pushd failures are fatal: unwinding past the intended popd once left
# `west init` creating .venv/.west in the caller's directory.
pushd "$TARGET_DIR" || exit 1
    SDK_TARBALL=zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-x86_64.tar.xz
    if [ ! -d "zephyr-sdk/zephyr-sdk-${ZEPHYR_SDK_VERSION}" ]; then
        mkdir -p zephyr-sdk
        pushd zephyr-sdk || exit 1
            if ! wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/${SDK_TARBALL}; then
                echo "ERROR: could not download ${SDK_TARBALL}; check access to github.com." >&2
                exit 1
            fi
            tar -xf ${SDK_TARBALL} || exit 1
            pushd zephyr-sdk-${ZEPHYR_SDK_VERSION} || exit 1
                yes | HOSTTYPE=x86_64 ./setup.sh
            popd # zephyr-sdk-${ZEPHYR_SDK_VERSION}
            rm ${SDK_TARBALL}
        popd # zephyr-sdk
    fi
    # Point at the versioned directory, not its parent: a workspace set up
    # before the 0.17.4 bump still has zephyr-sdk-0.16.8 sitting next to it.
    export ZEPHYR_SDK_INSTALL_DIR=$(realpath zephyr-sdk/zephyr-sdk-${ZEPHYR_SDK_VERSION})
    if [ ! -d ".venv" ]; then
        python3 -m venv .venv
    fi
    sed -i "/^export ZEPHYR_SDK_INSTALL_DIR=/d" .venv/bin/activate
    echo "export ZEPHYR_SDK_INSTALL_DIR=${ZEPHYR_SDK_INSTALL_DIR}" >> .venv/bin/activate
    source .venv/bin/activate

    pip install --upgrade pip
    pip     install west
    if [ ! -d .west ]; then
        west init ./ || exit 1
    fi
    if [ ! -d .west ] || [ ! -d zephyr ]; then
        echo "ERROR: no west workspace in $(pwd) after 'west init'." >&2
        exit 1
    fi

    pushd zephyr || exit 1
        git remote add xilinx https://github.com/AMD-AECG-SSW-PUBLIC/zephyr-amd.git
        git remote update
        git checkout xilinx/master-next
    popd # zephyr

    west update
    west zephyr-export
    pip install -r zephyr/scripts/requirements.txt

    pushd zephyr || exit 1
        west lopper-install
        # lopper-install is a one-shot `pip install` of the lopper checkout and
        # is not refreshed by `west update`, so the venv can keep an old lopper
        # after zephyr moves. That mismatch shows up much later as a devicetree
        # binding error (e.g. lopper 1.4.2 emitting xlnx,bus-width, which the
        # current xlnx,zynqmp-dma binding rejects), so fail here instead.
        LOPPER_EXPECTED=$(cat ../lopper/lopper/VERSION)
        LOPPER_INSTALLED=$(lopper --version 2>/dev/null)
        if [ "$LOPPER_EXPECTED" != "$LOPPER_INSTALLED" ]; then
            echo "Error: lopper ${LOPPER_INSTALLED:-<not installed>} is installed but zephyr expects ${LOPPER_EXPECTED}." >&2
            echo "       Re-run 'west lopper-install' from ${TARGET_DIR}/zephyr with the venv active." >&2
            exit 1
        fi
    popd # zephyr
    deactivate
popd # "$TARGET_DIR"
