# Copyright (C) 2025 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"). You may
# not use this file except in compliance with the License. A copy of the
# License is located at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

export ROOT_DIR=$ARCH_DIR/kernels/
export XILINX_VITIS_AIETOOLS=$XILINX_VITIS/aietools
export INCLUDE_PATH="-I $XILINX_VITIS_AIETOOLS/include -I /proj/xbuilds/SWIP/2023.1_0507_1903/installs/lin64/Vitis/2023.1/aietools/include/aie_api -I /proj/xbuilds/SWIP/2023.1_0507_1903/installs/lin64/Vitis/2023.1/aietools/include/drivers/aiengine -I $CARDANO_AIE_ARCH_MODEL_DIR/runtime_cxx/libcxx-lite/include -I $CARDANO_AIE_ARCH_MODEL_DIR/runtime_cxx/libs/libcxx-9.0.0/include-lite -I $CARDANO_AIE_ARCH_MODEL_DIR/runtime/include"

# Function to skip compilation if output file exists
skip_if_exists() {
    if [ -f "$1" ]; then
        echo "Skipping, as $1 already exists."
    else
        eval $2
    fi
}

generate_symbol_name() {
    local file_path="$1"
    local file_name="$2"
    
    # Ensure no trailing slash on path, then append file name
    local full_path="${file_path%/}/${file_name}"
    
    # Replace all slashes with underscores, prepend with _binary, and convert hyphens to underscores
    local symbol_name="_binary_$(echo $full_path | sed 's/\//_/g' | sed 's/-/_/g')"

    echo "$symbol_name"
}

case $1 in
    compile)
        case $2 in
            aie)
            export CARDANO_AIE_ARCH_MODEL_DIR="$XILINX_VITIS_AIETOOLS/data/versal_prod/lib"
            echo "Compiling Kernel for AIE..."   
            mkdir -p "$ROOT_DIR/output/aie"

            skip_if_exists "$ROOT_DIR/NeighborTileLoopback/build/D" && \
            xchesscc +f +s -p me -P "$CARDANO_AIE_ARCH_MODEL_DIR" +P 4 +Wllvm,-O2,-fno-jump-tables,-fno-discard-value-names,-mllvm,-chess-collapse-struct-types-during-linking=0,-Xclang,-chess-only-info-critical-passes -D__AIENGINE__ -D__AIE_ARCH__=10 -D__LOCK_FENCE_MODE__=0 -DAIE_OPTION_SCALAR_FLOAT_ON_VECTOR -DAIE2_FP32_EMULATION_ACCURACY_FAST $INCLUDE_PATH -I"$ROOT_DIR/NeighborTileLoopback/TheHouseOfCommons/" -o "$ROOT_DIR/NeighborTileLoopback/build/D_orig.ll" "$ROOT_DIR/NeighborTileLoopback/Tile_D/D_wrapper.cc" -L"$ROOT_DIR/build" && \
            $XILINX_VITIS_AIETOOLS/lnx64.o/tools/clang/bin/opt -S -load-pass-plugin="$XILINX_VITIS_AIETOOLS/lib/lnx64.o/libLLVMXLOpt.so" -passes=xlopt "$ROOT_DIR/NeighborTileLoopback/build/D_orig.ll" -o "$ROOT_DIR/NeighborTileLoopback/build/D.ll" && \
            $XILINX_VITIS_AIETOOLS/lnx64.o/tools/clang/bin/opt -S "$ROOT_DIR/NeighborTileLoopback/build/D.ll" -o "$ROOT_DIR/NeighborTileLoopback/build/D.ll" && \
            xchessmk -C Release_LLVM -P "${CARDANO_AIE_ARCH_MODEL_DIR}" +P 4 -DDEPLOYMENT_ELF=1 -D__LOCK_FENCE_MODE__=0 -DAIE_OPTION_SCALAR_FLOAT_ON_VECTOR -DAIE2_FP32_EMULATION_ACCURACY_FAST +o "$ROOT_DIR/NeighborTileLoopback/build/" "$ROOT_DIR/NeighborTileLoopback/Tile_D/D_aie.prx" && \
            armr5-none-eabi-ld -EL -r -b binary -o "$ROOT_DIR/output/aie/D.o" "$ROOT_DIR/NeighborTileLoopback/build/D"
            obj_file="$ROOT_DIR/output/aie/D.o"
            nm "$obj_file" | while read -r line ; do
                # Extract the symbol name
                symbol=$(echo "$line" | awk '{print $3}')
                
                # Directly replace the complex symbol names with your standardized names
                if echo "$symbol" | grep -q '_end$'; then
                    armr5-none-eabi-objcopy --redefine-sym "$symbol"=_binary_D_end "$obj_file"
                elif echo "$symbol" | grep -q '_start$'; then
                    armr5-none-eabi-objcopy --redefine-sym "$symbol"=_binary_D_start "$obj_file"
                elif echo "$symbol" | grep -q '_size$'; then
                    armr5-none-eabi-objcopy --redefine-sym "$symbol"=_binary_D_size "$obj_file"
                fi
            done
            ;;     
            aie-ml)
                export CARDANO_AIE_ARCH_MODEL_DIR="$XILINX_VITIS_AIETOOLS/data/aie_ml/lib"
                echo "Compiling Kernel for AIE-ML..."
                mkdir -p "$ROOT_DIR/output/aieml"

                xchesscc -aiearch aie-ml +f +s -p me -P "$CARDANO_AIE_ARCH_MODEL_DIR" +P 4 +Wllvm,-O2,-fno-jump-tables,-fno-discard-value-names,-mllvm,-chess-collapse-struct-types-during-linking=0,-Xclang,-chess-only-info-critical-passes -D__AIENGINE__ -D__AIE_ARCH__=20 -D__LOCK_FENCE_MODE__=0 -DAIE_OPTION_SCALAR_FLOAT_ON_VECTOR -DAIE2_FP32_EMULATION_ACCURACY_FAST $INCLUDE_PATH -I"$ROOT_DIR/NeighborTileLoopback/TheHouseOfCommons/" -o "$ROOT_DIR/NeighborTileLoopback/build/D_orig.ll" "$ROOT_DIR/NeighborTileLoopback/Tile_D/D_wrapper.cc" -L"$ROOT_DIR/build" && \
                $XILINX_VITIS_AIETOOLS/lnx64.o/tools/clang/bin/opt -S -load-pass-plugin=$XILINX_VITIS_AIETOOLS/lib/lnx64.o/libLLVMXLOpt.so -passes=xlopt "$ROOT_DIR/NeighborTileLoopback/build/D_orig.ll" -o "$ROOT_DIR/NeighborTileLoopback/build/D.ll" && \
                $XILINX_VITIS_AIETOOLS/lnx64.o/tools/clang/bin/opt -S "$ROOT_DIR/NeighborTileLoopback/build/D.ll" -o "$ROOT_DIR/NeighborTileLoopback/build/D.ll" && \
                xchessmk -aiearch aie-ml -C Release_LLVM -P "$CARDANO_AIE_ARCH_MODEL_DIR" +P 4 -DDEPLOYMENT_ELF=1 -D__LOCK_FENCE_MODE__=0 -DAIE_OPTION_SCALAR_FLOAT_ON_VECTOR -DAIE2_FP32_EMULATION_ACCURACY_FAST +o "$ROOT_DIR/NeighborTileLoopback/build/" "$ROOT_DIR/NeighborTileLoopback/Tile_D/D.prx" && \
                aarch64-none-elf-ld -EL -r -b binary -o "$ROOT_DIR/output/aieml/D.o" "$ROOT_DIR/NeighborTileLoopback/build/D"
                
                obj_file="$ROOT_DIR/output/aieml/D.o"
                nm "$obj_file" | while read -r line ; do
                    # Extract the symbol name
                    symbol=$(echo "$line" | awk '{print $3}')
                    
                    # Directly replace the complex symbol names with your standardized names
                    if echo "$symbol" | grep -q '_end$'; then
                        aarch64-none-elf-objcopy --redefine-sym "$symbol"=_binary_D_end "$obj_file"
                    elif echo "$symbol" | grep -q '_start$'; then
                        aarch64-none-elf-objcopy --redefine-sym "$symbol"=_binary_D_start "$obj_file"
                    elif echo "$symbol" | grep -q '_size$'; then
                        aarch64-none-elf-objcopy --redefine-sym "$symbol"=_binary_D_size "$obj_file"
                    fi
                done
                ;;
            *)
            echo "Invalid 3rd argument. Usage [compile aie|compile aie-ml|clean]"
            exit 1
            ;;
        esac
        ;;
    clean)

        echo "Cleaning up NeighborTileLoopback..."
        rm -rf $ROOT_DIR/NeighborTileLoopback/build/*
        ;;

    *)
    echo "Invalid 2nd argument. Usage [compile aie|compile aie-ml|clean]"
    exit 1
    ;;
esac
