#!/bin/bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
#
# The aiecompiler-generated graph.out runs under a custom dynamic linker whose
# rpath only covers /usr/lib64:/lib64, so on multiarch distros (where the
# compression libs live in /usr/lib/<triplet>) it cannot find libz/libzstd.
# Expose ONLY those libs through a private directory of symlinks rather than
# putting the whole system lib dir on LD_LIBRARY_PATH -- the latter would
# shadow libc/libstdc++ for downstream tools (e.g. the relocated PetaLinux
# sh/make, which need a newer glibc than the host).
#
# Source this from every entry point that invokes aiecompiler (aieldpath.sh,
# compile.sh, prod_flow.sh). Safe to source repeatedly and safe on RHEL, where
# the probe finds no multiarch dir and the shim is never created.

_aeg_ldshim_setup() {
    local shim_dir sys_dir lib
    shim_dir="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")/.ldshim"
    for sys_dir in "/usr/lib/$(uname -m)-linux-gnu" "/usr/lib/x86_64-linux-gnu" "/lib/x86_64-linux-gnu"; do
        if [ -e "$sys_dir/libz.so.1" ]; then
            mkdir -p "$shim_dir" 2>/dev/null || return 0
            for lib in libz.so.1 libz.so libzstd.so.1 libzstd.so liblzma.so.5 liblzma.so; do
                if [ -e "$sys_dir/$lib" ]; then
                    ln -sf "$sys_dir/$lib" "$shim_dir/$lib" 2>/dev/null || true
                fi
            done
            case ":${LD_LIBRARY_PATH:-}:" in
                *":$shim_dir:"*) ;;
                *) export LD_LIBRARY_PATH="$shim_dir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" ;;
            esac
            return 0
        fi
    done
    return 0
}

_aeg_ldshim_setup
unset -f _aeg_ldshim_setup
