#!/usr/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

set -x

CURRDIR=$PWD
AIE_GEN=5
RPUFLAG=-rpu
BOOTGEN=-bootgen
NEW_API=
AEG_DEBUG_FLAG=
HW_QEMU=
ZEPHYR_DIR=
GRAPH_BUILD=true
WEST_UPDATE=true
AIEBAREMETAL_DIR=$(realpath ../../)
AIE_RT_DIR=${AIEBAREMETAL_DIR}/thirdparty/aielib/aie-rt/
SYSTEM_TOP_DTS=${AIEBAREMETAL_DIR}/thirdparty/sdtgen-dts/latest/system-top.dts
SDT_GEN_BOARD=versal2-vek385-reva
XSA=/proj/xbuilds/2026.1_daily_latest/internal_platforms/vek385_base_reva/hw/hw.xsa

echo "Current directory: $CURRDIR"
echo "aiebaremetal directory: $AIEBAREMETAL_DIR"

while [[ $# -gt 0 ]]; do
    case "$1" in
        -bootgen|--bootgen)
            BOOTGEN="-bootgen"
            shift
            ;;
        -rpu|--rpu)
            RPUFLAG="-rpu"
            shift
            ;;
        -hw_qemu|--hw_qemu)
            HW_QEMU="--hw_qemu"
            shift
            ;;
        -new_api|--new_api)
            NEW_API=-new_api
            shift
            ;;
        -aeg_debug|--aeg_debug)
            AEG_DEBUG_FLAG="-aeg_debug"
            shift
            ;;
        -zephyr_dir|--zephyr_dir)
            ZEPHYR_DIR="$2"
            shift 2
            ;;
        -graph_build|--graph_build)
            GRAPH_BUILD=true
            shift
            ;;
        -west_update|--west_update)
            WEST_UPDATE=true
            shift
            ;;
        -system_top_dts|--system_top_dts)
            SYSTEM_TOP_DTS="$2"
            shift 2
            ;;
        *)
            if [[ $1 =~ ^[0-9]+$ ]]; then
                AIE_GEN=$1
                shift
            else
                echo "Unknown option: $1" >&2
                exit 1
            fi
            ;;
    esac
done

if [ "$GRAPH_BUILD" = true ]; then
    bash -c "source build.sh ${AIE_GEN} ${BOOTGEN} ${RPUFLAG} ${HW_QEMU} ${NEW_API} ${AEG_DEBUG_FLAG}"
fi

setup_zephyr() {
    ZEPHYR_DIR=${AIEBAREMETAL_DIR}/zephyr_dir
    ${AIEBAREMETAL_DIR}/script/setup_zephyr.sh --target_dir ${ZEPHYR_DIR}
}

build_system_top_dts() {
    if [ ! -d "${AIEBAREMETAL_DIR}/thirdparty/sdtgen-dts" ]; then
        mkdir -p "${AIEBAREMETAL_DIR}/thirdparty/sdtgen-dts"
    fi
    if [ ! -d "${AIEBAREMETAL_DIR}/thirdparty/system-device-tree-xlnx" ]; then
         pushd "${AIEBAREMETAL_DIR}/thirdparty/"
         git clone https://github.com/AMD-AECG-SSW-PUBLIC/system-device-tree-xlnx.git
         popd # ${AIEBAREMETAL_DIR}/thirdparty/
    fi
    ${AIEBAREMETAL_DIR}/script/gensdt.sh \
        --chroot "${AIEBAREMETAL_DIR}/thirdparty/sdtgen-dts" \
        --xsa "${XSA}" \
        --branch master-next \
        --repo "${AIEBAREMETAL_DIR}/thirdparty/system-device-tree-xlnx" \
        --origin origin \
        --board ${SDT_GEN_BOARD}
    SYSTEM_TOP_DTS="${AIEBAREMETAL_DIR}/thirdparty/sdtgen-dts/latest/system-top.dts"

}

build_zephyr() {
    if [ -z "$ZEPHYR_DIR" ]; then
        echo "Error: Zephyr directory not specified. Use -zephyr_dir or --zephyr_dir to set it." >&2
        exit 1
    fi

    if [ ! -d "$ZEPHYR_DIR" ]; then
        echo "Error: Zephyr directory '$ZEPHYR_DIR' does not exist." >&2
        exit 1
    fi

    pushd "$ZEPHYR_DIR"

    source .venv/bin/activate
    pushd zephyr
    rm build -rf
    if [ "$WEST_UPDATE" = true ]; then
        west update
        west lopper-install
        LOPPER_DTC_FLAGS="-b 0 -@" west lopper-command -p cortexr52_0 -s "$SYSTEM_TOP_DTS" -w .
        sed -i "s|reg = <0x0 0x100000 0x0 0x7ff00000>;|reg = <0x0 0x100000 0x0 0x3ff00000>;|" boards/amd/versal2_rpu/versal2_rpu.dts
    fi # west_update

    sed -e "s|@AIE_RT_SRC_DIR@|${AIE_RT_DIR}|g" -e "s|@AIEBAREMETAL_SRC_DIR@|${AIEBAREMETAL_DIR}|g" ${CURRDIR}/CMakeLists.txt.sed > ${CURRDIR}/CMakeLists.txt

    west build -b versal2_rpu ${CURRDIR} --pristine -- -DDTC_OVERLAY_FILE=${AIEBAREMETAL_DIR}/script/aiengine.overlay

    popd # zephyr
    deactivate

    popd # "$ZEPHYR_DIR"

    echo "Zephyr build completed successfully."

}

build_bootgen() {
    ZEPHYR_ELF=${ZEPHYR_DIR}/zephyr/build/zephyr/zephyr.elf
    cp ${ZEPHYR_ELF} ${CURRDIR}/zephyr.elf
    cp ${AIEBAREMETAL_DIR}/script/vek385_rpu_allgmio.bif ${CURRDIR}/vek385_rpu_allgmio_zephyr.bif

    sed -i "s|file=./vek385.elf|file=./zephyr.elf|g" ${CURRDIR}/vek385_rpu_allgmio_zephyr.bif
    source /proj/xbuilds/2026.1_daily_latest/installs/lin64/2026.1/Vitis/settings64.sh
    bootgen -arch versal_2ve_2vm -image vek385_rpu_allgmio_zephyr.bif -o build/vek385_zephyr.BIN -w
}

if [ -z "$ZEPHYR_DIR" ]; then
    setup_zephyr
fi

if [ ! -f "$SYSTEM_TOP_DTS" ]; then
    build_system_top_dts
fi

build_zephyr

build_bootgen
