#!/bin/bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Usage: source ./build.sh [options]
# Options:
#   <1|2|5>                 AIE generation (default: 2)
#   -bootgen                Enable bootgen (produces BOOT.BIN)
#   -rpu                    Build for RPU
#   -new_api                Use new typed AEG API
#   -perf                   Enable [PERF] phase timers (see script/perf_report.py)

set -e
CURRDIR=$PWD

AIE_GEN=2
RPUFLAG=
new_api=false
bootgen_enable=false
perf_enable=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -bootgen)
            bootgen_enable=true
            shift
            ;;
        -rpu)
            RPUFLAG="-rpu"
            shift
            ;;
        -new_api)
            new_api=true
            shift
            ;;
        -perf)
            perf_enable=true
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

echo "AIE_GEN=${AIE_GEN}, new_api=${new_api}, bootgen=${bootgen_enable}, perf=${perf_enable}"
source $CURRDIR/../../script/settings.sh 1

if [ ! -f "$CURRDIR/src/golden.data" ] || [ ! -f "$CURRDIR/src/input.data" ]; then
    unzip -o $CURRDIR/src/input_golden.zip -d $CURRDIR
    mv -f $CURRDIR/input.data $CURRDIR/src/
    mv -f $CURRDIR/golden.data $CURRDIR/src/
fi

if [ ! -d "$CURRDIR/dsp_lib" ]; then
    echo "DSP library not found. Run 'bash download_dsplib.sh' first." >&2
    exit 1
fi

DSP_LIB=$CURRDIR/dsp_lib/dsp
INCLUDE_OPTS="--include=$CURRDIR/src \
    --include=$XILINX_VITIS_AIETOOLS/include/aie_api \
    --include=$DSP_LIB/L1/include/aie \
    --include=$DSP_LIB/L1/include/hw \
    --include=$DSP_LIB/L1/src/aie \
    --include=$DSP_LIB/L1/src/hw \
    --include=$DSP_LIB/L1/tests/aie/inc \
    --include=$DSP_LIB/L1/tests/aie/src \
    --include=$DSP_LIB/L2/include/aie \
    --include=$DSP_LIB/L2/include/hw \
    --include=$DSP_LIB/L2/tests/aie/common/inc \
    --include=$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/include/ \
    --include=$BAREMETAL_DIR/include \
    --include=$BAREMETAL_DIR/src/include \
    --include=$BAREMETAL_DIR/src/include/common_layer/ \
    --include=$BAREMETAL_DIR/src/include/common_layer/aegothers"

PREPROC_OPTS="-DDATA_TYPE_ROW=cint16 -DTWIDDLE_TYPE=cint16 -DDATA_TYPE_COL=cint32 -DUUT_GRAPH=fft_ifft_dit_1ch_graph"

# -DPERF enables the [PERF] phase timers in src/perf.h. It reaches the host
# compile via compile.sh's --preproc_options; the AIE-side compiles ignore it
# because the timers live in the host-only branch of the graph sources.
if [[ "$perf_enable" == true ]]; then
    PREPROC_OPTS="$PREPROC_OPTS -DPERF"
fi

COMPILER_OPTS="--disable-multirate-analysis"

# compile.sh handles: aiecompiler, metadata gen, baremetal lib, host ELF link.
# We intentionally do NOT pass -bootgen here because CDO patching must happen
# between compilation and bootgen.
if [[ "$new_api" == true ]]; then
    $CURRDIR/../../script/compile.sh \
        --runtime_source_file ./src/graph_new_api.cpp \
        --aie_version ${AIE_GEN} \
        --include_options "$INCLUDE_OPTS" \
        --preproc_options "$PREPROC_OPTS" \
        --compiler_options "$COMPILER_OPTS" \
        ${RPUFLAG} \
        -new_api
else
    $CURRDIR/../../script/compile.sh \
        --runtime_source_file ./src/graph.cpp \
        --aie_version ${AIE_GEN} \
        --include_options "$INCLUDE_OPTS" \
        --preproc_options "$PREPROC_OPTS" \
        --compiler_options "$COMPILER_OPTS" \
        ${RPUFLAG}
fi
compile_status=$?
if [ $compile_status -ne 0 ]; then
    echo "Error: compile.sh failed with status $compile_status" >&2
    exit $compile_status
fi

# ---------------------------------------------------------------------------
# Patch CDO: trim RangeFFT output buffers from 2048 to 1024 samples.
# The real-input 2048-pt FFT output has conjugate symmetry, so the second
# half is redundant. The aiecompiler generates BDs for full 2048-sample
# windows; this patches the shim DMA BD size fields from 0x800 to 0x400
# across all 4 instances.
# ---------------------------------------------------------------------------
CDO_INIT=$CURRDIR/Work/ps/cdo/aie_cdo_init.bin
if [ -f "$CDO_INIT" ]; then
    echo "=== Patching CDO for conjugate symmetry buffer trimming ==="
    CDOUTIL=${XILINX_VITIS}/bin/cdoutil

    $CDOUTIL -device xcve2802 -output-source -output-file ${CDO_INIT}.src.txt $CDO_INIT

    sed -i 's/write 0x2000431d040 0x6000800/write 0x2000431d040 0x6000400/g' ${CDO_INIT}.src.txt
    sed -i 's/write 0x2000431d060 0x2000800/write 0x2000431d060 0x2000400/g' ${CDO_INIT}.src.txt
    sed -i 's/write 0x2000831d040 0x6000800/write 0x2000831d040 0x6000400/g' ${CDO_INIT}.src.txt
    sed -i 's/write 0x2000831d060 0x2000800/write 0x2000831d060 0x2000400/g' ${CDO_INIT}.src.txt
    sed -i 's/write 0x2000c31d040 0x6000800/write 0x2000c31d040 0x6000400/g' ${CDO_INIT}.src.txt
    sed -i 's/write 0x2000c31d060 0x2000800/write 0x2000c31d060 0x2000400/g' ${CDO_INIT}.src.txt
    sed -i 's/write 0x2001031d040 0x6000800/write 0x2001031d040 0x6000400/g' ${CDO_INIT}.src.txt
    sed -i 's/write 0x2001031d060 0x2000800/write 0x2001031d060 0x2000400/g' ${CDO_INIT}.src.txt

    patch_count=$(grep -c '0x[26]000400' ${CDO_INIT}.src.txt || true)
    if [ "$patch_count" -lt 8 ]; then
        echo "WARNING: CDO patch applied only $patch_count/8 expected replacements." >&2
        echo "Buffer size values may be incorrect. Check CDO source manually." >&2
    else
        echo "CDO patch verified: $patch_count replacements found."
    fi

    $CDOUTIL -output-binary-le -output-file ${CDO_INIT}.patched ${CDO_INIT}.src.txt
    mv ${CDO_INIT} ${CDO_INIT}.orig
    mv ${CDO_INIT}.patched ${CDO_INIT}
    rm -f ${CDO_INIT}.src.txt
    echo "CDO patching complete. Original saved as aie_cdo_init.bin.orig"

    if [ -f "$CURRDIR/libadf.a" ]; then
        echo "Updating libadf.a with patched CDO..."
        AR=$XILINX_VITIS/aietools/tps/lnx64/gcc/bin/ar
        OBJCOPY=$XILINX_VITIS/aietools/tps/lnx64/gcc/bin/objcopy
        pushd $CURRDIR > /dev/null
        $AR x libadf.a
        $OBJCOPY --update-section ".aie.cdo.init.bin"="$CDO_INIT" sw.o
        $AR r libadf.a hw.o sw.o cfg.o
        rm -f hw.o sw.o cfg.o
        popd > /dev/null
    fi
else
    echo "WARNING: CDO init file not found at $CDO_INIT, skipping patch." >&2
fi

# ---------------------------------------------------------------------------
# Generate BOOT.BIN (after CDO patching so the patched CDO is included)
# ---------------------------------------------------------------------------
if [[ "$bootgen_enable" == true ]]; then
    echo "=== Generating BOOT.BIN ==="
    cd $CURRDIR

    if [ ! -d "arch" ]; then
        cp -R $BAREMETAL_DIR/thirdparty/arch ./
    fi

    BIF=vek280_boot_image.bif
    BOOTBIN=Vek280BOOT.BIN
    if [[ "$RPUFLAG" == "-rpu" ]]; then
        BIF=vek280_rpu_boot_image.bif
    fi

    if [ "$AIE_GEN" == "2" ]; then
        bootgen -arch versal -image ${BIF} -o ${BOOTBIN} -w
    elif [ "$AIE_GEN" == "5" ]; then
        bootgen -arch versal_2ve_2vm -image ${BIF} -o ${BOOTBIN} -w
    else
        bootgen -arch versal -image ${BIF} -o ${BOOTBIN} -w
    fi

    rm -rf ./build
    mkdir ./build
    mv ./${BOOTBIN} ./build/
    rm -f $BIF || true
    rm -rf arch

    echo "BOOT.BIN generated: build/${BOOTBIN}"
fi

if [[ "$new_api" == true ]]; then
    build_sim.sh --runtime_source_file ./src/graph_new_api.cpp -new_api
else
    build_sim.sh --runtime_source_file ./src/graph.cpp
fi

echo "Build completed successfully."
echo "  AIE_GEN=${AIE_GEN}, new_api=${new_api}, bootgen=${bootgen_enable}, perf=${perf_enable}"
if [[ "$perf_enable" == true ]]; then
    echo "  Perf timers enabled. Capture the UART log, then run:"
    echo "    python3 script/perf_report.py <legacy.log> <new_api.log>"
fi
