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
SDT_CHROOT=${AIEBAREMETAL_DIR}/thirdparty/sdtgen-dts
SYSTEM_TOP_DTS=${SDT_CHROOT}/latest/system-top.dts
SDT_GEN_BOARD=versal2-vek385-reva
XSA=/proj/xbuilds/2026.1_daily_latest/internal_platforms/vek385_base_reva/hw/hw.xsa

# --- runtime target (default: VEK385 gen-2, RPU) ---
ZBOARD=versal2_rpu                                       # zephyr board (west -b)
LOPPER_PROC=cortexr52_0                                  # SDT processor for lopper
AIE_OVERLAY=${AIEBAREMETAL_DIR}/script/aiengine.overlay  # AIE devicetree node
DEPLOY=bootgen                                           # bootgen (RPU bif) | tfa (APU PDI+TF-A)
BL31=                                                    # TF-A bl31.elf (APU only)
SYSTEM_TOP_DTS_SET=false

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
            SYSTEM_TOP_DTS_SET=true
            shift 2
            ;;
        -vek280|--vek280)
            # Supplementary target: VEK280 (gen-1 Versal AI Edge, APU/A72, AIE-ML)
            AIE_GEN=2
            RPUFLAG=
            SDT_GEN_BOARD=versal-vek280-reva
            XSA=/proj/xbuilds/2026.1_daily_latest/internal_platforms/vek280_base/hw/hw.xsa
            SDT_CHROOT=${AIEBAREMETAL_DIR}/thirdparty/sdtgen-dts-vek280
            ZBOARD=versal_apu
            LOPPER_PROC=psv_cortexa72_0
            AIE_OVERLAY=${AIEBAREMETAL_DIR}/script/aiengine_vek280.overlay
            # Pack PDI + AIE CDOs + zephyr.elf into one .BIN ("device program"),
            # booting Zephyr at EL3. For the PDI -> TF-A bl31 -> Zephyr flow
            # instead, set DEPLOY=tfa (see deploy_tfa).
            DEPLOY=bootgen_vek280
            BL31=${BL31:-}
            shift
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

# Per-target SDT lives under SDT_CHROOT unless the user pinned --system_top_dts.
if [ "$SYSTEM_TOP_DTS_SET" != true ]; then
    SYSTEM_TOP_DTS=${SDT_CHROOT}/latest/system-top.dts
fi

if [ "$GRAPH_BUILD" = true ]; then
    bash -c "source build.sh ${AIE_GEN} ${BOOTGEN} ${RPUFLAG} ${HW_QEMU} ${NEW_API} ${AEG_DEBUG_FLAG}"
fi

setup_zephyr() {
    ZEPHYR_DIR=${AIEBAREMETAL_DIR}/zephyr_dir
    ${AIEBAREMETAL_DIR}/script/setup_zephyr.sh --target_dir ${ZEPHYR_DIR}
}

build_system_top_dts() {
    if [ ! -d "${SDT_CHROOT}" ]; then
        mkdir -p "${SDT_CHROOT}"
    fi
    if [ ! -d "${AIEBAREMETAL_DIR}/thirdparty/system-device-tree-xlnx" ]; then
         pushd "${AIEBAREMETAL_DIR}/thirdparty/"
         # SDT data lives in AMD-AECG-SSW-PUBLIC/system-device-tree-xlnx (private repo;
         # requires AMD GitHub auth), on the master-next branch.
         git clone https://github.com/AMD-AECG-SSW-PUBLIC/system-device-tree-xlnx.git
         popd # ${AIEBAREMETAL_DIR}/thirdparty/
    fi
    ${AIEBAREMETAL_DIR}/script/gensdt.sh \
        --chroot "${SDT_CHROOT}" \
        --xsa "${XSA}" \
        --branch master-next \
        --repo "${AIEBAREMETAL_DIR}/thirdparty/system-device-tree-xlnx" \
        --origin origin \
        --board ${SDT_GEN_BOARD}
    SYSTEM_TOP_DTS="${SDT_CHROOT}/latest/system-top.dts"

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
        LOPPER_DTC_FLAGS="-b 0 -@" west lopper-command -p ${LOPPER_PROC} -s "$SYSTEM_TOP_DTS" -w .
        # RPU-only: shrink DRAM reg to fit the R52 map. Not needed for versal_apu
        # (2GB sram0 already covers DDR).
        if [ "$ZBOARD" = "versal2_rpu" ]; then
            sed -i "s|reg = <0x0 0x100000 0x0 0x7ff00000>;|reg = <0x0 0x100000 0x0 0x3ff00000>;|" boards/amd/versal2_rpu/versal2_rpu.dts
        fi
    fi # west_update
    west build -b ${ZBOARD} ${CURRDIR} --pristine -- -DDTC_OVERLAY_FILE=${AIE_OVERLAY} -DAIE_GEN=${AIE_GEN}

    # Fail loudly here: without this, a failed west build silently falls through to
    # bootgen, which then errors with a misleading "Unable to open ./zephyr.elf".
    if [ ! -f build/zephyr/zephyr.elf ]; then
        echo "ERROR: west build did not produce build/zephyr/zephyr.elf. See the west/devicetree errors above." >&2
        exit 1
    fi

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
    # BIF references ./arch/...; link the committed BSP/PDI tree into the build dir.
    ln -sfn ${AIEBAREMETAL_DIR}/thirdparty/arch ${CURRDIR}/arch
    mkdir -p ${CURRDIR}/build
    bootgen -arch versal_2ve_2vm -image vek385_rpu_allgmio_zephyr.bif -o build/vek385_zephyr.BIN -w
}

build_bootgen_vek280() {
    # VEK280 APU Zephyr boot image (single .BIN for xsdb "device program"):
    # PLM (PDI) loads the platform + AIE design (aie_cdo_*), then Zephyr @EL3
    # (Zephyr does the EL3->EL1 drop). Reuse the committed BIF, swapping the
    # baremetal partition for zephyr.elf -- same pattern as build_bootgen.
    ZEPHYR_ELF=${ZEPHYR_DIR}/zephyr/build/zephyr/zephyr.elf
    cp ${ZEPHYR_ELF} ${CURRDIR}/zephyr.elf
    cp ${AIEBAREMETAL_DIR}/script/vek280_boot_image.bif ${CURRDIR}/vek280_boot_image_zephyr.bif
    sed -i "s|{ core=a72-0, file=./vek280.elf }|{ core=a72-0, exception_level=el-3, file=./zephyr.elf }|" ${CURRDIR}/vek280_boot_image_zephyr.bif

    source /proj/xbuilds/2026.1_daily_latest/installs/lin64/2026.1/Vitis/settings64.sh
    # BIF references ./arch/...; link the committed BSP/PDI tree into the build dir.
    ln -sfn ${AIEBAREMETAL_DIR}/thirdparty/arch ${CURRDIR}/arch
    mkdir -p ${CURRDIR}/build
    bootgen -arch versal -image vek280_boot_image_zephyr.bif -o build/Vek280_zephyr.BIN -w
}

deploy_tfa() {
    # gen-1 Versal APU boot chain: PDI (PLM) -> TF-A bl31 (EL3) -> Zephyr (EL1).
    # No bootgen image; deploy with xsdb on the board host using PDI + bl31 + elf.
    ZEPHYR_ELF=${ZEPHYR_DIR}/zephyr/build/zephyr/zephyr.elf
    PDI=${AIEBAREMETAL_DIR}/thirdparty/arch/platform/pdi/vek280/allgmio.pdi
    # Prefer the bl31 west builds (PRELOADED_BL33_BASE=SRAM_BASE); a generic bl31
    # points BL33 elsewhere and hangs. Override with BL31=<path> only if set.
    BL31=${BL31:-${ZEPHYR_DIR}/zephyr/build/tfa/versal/release/bl31/bl31.elf}
    if [ ! -f "${BL31}" ]; then
        echo "WARNING: bl31.elf not found at '${BL31}'." >&2
        echo "         It is produced by 'west build' when CONFIG_BUILD_WITH_TFA=y." >&2
    fi
    echo "VEK280 APU Zephyr boot (run these on the board host, e.g. muscaria18):"
    echo
    echo "  # Option A - west runner (needs the venv/west on the board host):"
    echo "  cd ${ZEPHYR_DIR}/zephyr && source ../.venv/bin/activate"
    echo "  west flash --runner xsdb --pdi ${PDI} --bl31 ${BL31}"
    echo
    echo "  # Option B - manual xsdb (fits the existing 'device program' lab flow):"
    echo "  xsdb"
    echo "    connect"
    echo "    targets -set -nocase -filter {name =~ \"*Versal*\"}"
    echo "    device program ${PDI}"
    echo "    # wait for PLM 'Total PLM Boot Time', then:"
    echo "    targets -set -nocase -filter {name =~ \"*Cortex-A72*#0\"}"
    echo "    rst -proc"
    echo "    dow ${BL31}"
    echo "    dow ${ZEPHYR_ELF}"
    echo "    con"
}

if [ -z "$ZEPHYR_DIR" ]; then
    setup_zephyr
fi

if [ ! -f "$SYSTEM_TOP_DTS" ]; then
    build_system_top_dts
fi

build_zephyr

case "$DEPLOY" in
    tfa)            deploy_tfa ;;
    bootgen_vek280) build_bootgen_vek280 ;;
    *)              build_bootgen ;;
esac
