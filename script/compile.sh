#!/bin/bash
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

# Source the environment variables
#AIE_GEN=2
CUR_DIR=`pwd`
echo "current directory"
echo $CUR_DIR
# Initialize variables
BAREMETAL_TEST_DIR=$BAREMETAL_DIR/example/
BAREMETAL_SCRIPT_DIR=$BAREMETAL_DIR/script/
ARCH_DIR=$BAREMETAL_DIR/thirdparty/arch/ps/
ARCH_72_DIR=$ARCH_DIR/psv_cortexa72_0/
ARCH_53_DIR=$ARCH_DIR/psv_cortexr5_0/

runtime_source_file=""
json_file=$kernel_folder/Work/ps/c_rts/aie_control_config.json
aie_version="1"
kernel_folder=`pwd`
compiler_options="--disable-multirate-analysis"
include_options="--include=./ --include=$XILINX_VITIS_AIETOOLS/include/aie_api \
                 --include=$BAREMETAL_DIR/thirdparty/arch/ps/psv_cortexa72_0/include/\
                 --include=$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ \
                 --include $BAREMETAL_DIR/include --include $BAREMETAL_DIR/include/common_layer/ \
                 --include $BAREMETAL_DIR/include/common_layer/abrothers"
preproc_options=""
#sysrootinclude="$ARCH_72_DIR/include"
#sysrootlib="$ARCH_72_DIR/lib"
#sysrootlinker="$ARCH_72_DIR/lscript.ld"
bootgen_enable=false
hw_only=false
workfolder=
rpu=false

# Print usage
usage() {
    echo "Usage: $0 --runtime_source_file <path> [--aie_version <version [1, 2]]> [--kernel_folder <path>] [--compiler_options <compiler optionss and include paths>] [-bootgen] [-rpu]"
    exit 1
}

# Parse arguments

if [ $# -eq 0 ]; then
	usage
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        -help)
            usage
            ;;
        --runtime_source_file)
            runtime_source_file="$2"
            shift 2
            ;;
        --json_file)
            json_file="$2"
            shift 2
            ;;
        --aie_version)
            aie_version="$2"
            shift 2
            ;;
        --kernel_folder)
            kernel_folder="$2"
            shift 2
            ;;
        --compiler_options)
            compiler_options="$2"
            shift 2
            ;;
        --partition_option)
            partition_option="$2"
            shift 2
            ;;
         --include_options)
            include_options="$2"
            shift 2
            ;;
          --preproc_options)
            preproc_options="$2"
            shift 2
            ;;
           --sysrootinclude)
            sysrootinclude="$2"
            shift 2
            ;;
          --sysrootlib)
            sysrootlib="$2"
            shift 2
            ;;
          --sysrootlinker)
            sysrootlinker="$2"
            shift 2
            ;;
          --workfolder)
            workfolder="$2"
            shift 2
            ;;
            -bootgen)
            bootgen_enable=true
            shift
            ;;
            -hw_only)
            hw_only=true
            shift
            ;;
            -rpu)
            rpu=true
            shift
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done


if [ -z "$sysrootinclude" ]; then
		if [ $rpu == true ]; then
			sysrootinclude="$ARCH_53_DIR/include"
		else
			sysrootinclude="$ARCH_72_DIR/include"
		fi
fi
if [ -z "$sysrootlib" ]; then
		if [ $rpu == true ]; then
			sysrootlib="$ARCH_53_DIR/lib"
		else
			sysrootlib="$ARCH_72_DIR/lib"
		fi
fi
if [ -z "$sysrootlinker" ]; then
		if [ $rpu == true ]; then
			sysrootlinker="$ARCH_53_DIR/lscript.ld"
		else
			sysrootlinker="$ARCH_72_DIR/lscript.ld"
		fi
fi

pushd $kernel_folder
kernel_folder=`pwd`
popd

pushd .
dir="$(dirname "${runtime_source_file}")"
filename="$(basename "${runtime_source_file}")"
cd $dir
runtime_source_file=`pwd`/$filename
popd 

pushd .
dir="$(dirname "${json_file}")"
filename="$(basename "${json_file}")"
cd $dir
json_file=`pwd`/$filename
popd 

pushd .
cd $sysrootinclude
sysrootinclude=`pwd`
popd

pushd .
cd $sysrootlib
sysrootlib=`pwd`
popd

if [ -z "$workfolder" ]; then
    echo "************empty work folder************"
else
    pushd .
    cd $workfolder
    workfolder=`pwd`
    popd
fi

pushd .
linker_dir="$(dirname "${sysrootlinker}")"
linker_file="$(basename "${sysrootlinker}")"
cd $linker_dir
sysrootlinker=`pwd`/$linker_file
popd 

echo "Before"
echo ${aie_version}
echo ${runtime_source_file}
echo ${kernel_folder}
echo ${compiler_options}
echo ${include_options}
echo ${preproc_options}
echo ${sysrootinclude}
echo ${sysrootlinker}
echo workfolder is  ${workfolder}
echo "After"

# Need to find a way to check versions
# HW_GEN=$(python3 vercheck.py)

# if [ ${aie_version} != ${HW_GEN}  ]; then
# 	echo "HW gen is ${HW_GEN} but request to build AIE_GEN ${aie_version}, info mismatch"
# 	return
# fi

# Compile AIE driver
if [[ -z $workfolder ]]; then
    echo "*****************Work folder is not provided, enter "$BAREMETAL_TEST_DIR "****************"
    pushd $BAREMETAL_TEST_DIR
    make alib
    popd
else
    echo "********************Work folder is provided*****************"
fi

# compile the graph
if [ $aie_version == 1 ]; then
	DEVICE_FILE="xcvc1902"     
elif [ $aie_version == 2 ]; then
	DEVICE_FILE="xcvc2802"     
elif [ $aie_version == 5 ];then
	DEVICE_FILE="xcve3858"     
else
    echo "Unsupported AIE version: $aie_version"
    exit 1
fi

if [[ -z $workfolder ]]; then
    echo "*******************Compiler being called****************"
    rm -rf $kernel_folder/Work
    rm $kernel_folder/*.log
    rm $kernel_folder/libadf.a
    #source $BAREMETAL_DIR/adfrcompile.sh $runtime_source_file --part=$DEVICE_FILE -Xpreproc="$preproc_options" $compiler_options --workdir=$kernel_folder/Work $include_options
    aiecompiler   $runtime_source_file --part=$DEVICE_FILE -Xpreproc="$preproc_options" $compiler_options "$partition_option" --workdir=$kernel_folder/Work $include_options
   
else
    echo "*******************Skipping Compiler call ****************************"
 
fi

 pushd .
 echo "The app is located in: $kernel_folder"

cd $BAREMETAL_TEST_DIR

## compile the simulation library
if [[ $hw_only == false ]]; then
    make ./build/libaiebaremetal.so  AIE_GEN=$aie_version  AIE_KERNEL_LOC=$kernel_folder AIE_CONTROL_JSON=$kernel_folder/Work/ps/c_rts/aie_control_config.json -j32
fi
#source /proj/xbuilds/HEAD_qualified_latest/installs/lin64/Vitis/HEAD/settings64.sh

AIE=10
AIEML=20
if [ "$aie_version" == "2" ]; then
	__AIE_ARCH__=${AIEML}
	if [ $rpu == true ]; then
		BIF=vek280_rpu_boot_image.bif
	else
		BIF=vek280_boot_image.bif
	fi

	BOOTBIN=Vek280BOOT.BIN
	ELF=vek280.elf
else
	__AIE_ARCH__=${AIE}
	if [ $rpu == true ]; then
		BIF=vck190_rpu_boot_image.bif
	else
		BIF=vck190_boot_image.bif
	fi
	BOOTBIN=Vck190BOOT.BIN
	ELF=vck190.elf
fi

if [[ -z $workfolder ]]; then
    if [ $rpu == true ]; then
        make hw_external_rpu   AIE_GEN=$aie_version  AIE_KERNEL_LOC=$kernel_folder ARCH_APU_A53_ARC_DIC=$sysrootinclude  -j32
    
    else
				 echo "hw compile for apu kernel_folder is $kernel_folder"
         make hw_external_apu   AIE_GEN=$aie_version  AIE_KERNEL_LOC=$kernel_folder ARCH_APU_A72_ARC_DIC=$sysrootinclude  -j32    
    fi
else
    if [ $rpu == true ]; then
        make hw_external_rpu   AIE_GEN=$aie_version  AIE_COMPILE_WORK=$workfolder AIE_CONTROL_JSON=$json_file ARCH_APU_A53_ARC_DIC="$sysrootinclude"  -j32
    else
				 echo "hw compile for apu 2"
         make hw_external_apu   AIE_GEN=$aie_version  AIE_COMPILE_WORK=$workfolder AIE_CONTROL_JSON=$json_file ARCH_APU_A72_ARC_DIC="$sysrootinclude"  -j32
    fi     
fi
popd
cp $BAREMETAL_SCRIPT_DIR/$BIF  ./

export BIF
export BOOTBIN
export ELF
#export ARCH_DIR

#build the vek280/a72 app
source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh
if [ $rpu == true ]; then
		echo "rpu compile"
		echo "syslib is $sysrootlib"
		echo "sysinc is $sysrootinclude"
		echo "linker is $sysrootlinker"
		echo "baremetal path is $BAREMETAL_TEST_DIR/build/"
		# _RPU_ used to declare this is RPU, app to support both APU and RPU can use this macro to do RPU specify config
    armr5-none-eabi-g++ -D__AIE_ARCH__=${__AIE_ARCH__} -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -Wl,-T -Wl,$sysrootlinker -D_RPU_ -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/fal/build/src/include/ -I/$BAREMETAL_DIR/include/ -I$sysrootinclude -L$sysrootlib -L$BAREMETAL_TEST_DIR/build/ -o ./${ELF} $runtime_source_file -Wl,--start-group,-laiebaremetalr53,-lxil,-lgcc,-lc,-lstdc++,--end-group
else
		echo "apu compile"
    aarch64-none-elf-gcc -D__AIE_ARCH__=${__AIE_ARCH__} -mcpu=cortex-a72 -Wl,-T -Wl,$sysrootlinker -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/fal/build/src/include/ -I/$BAREMETAL_DIR/include -I$sysrootinclude -L$sysrootlib -L$BAREMETAL_TEST_DIR/build/ -o ./${ELF} $runtime_source_file -Wl,--start-group,-laiebaremetala72,-lxil,-lgcc,-lc,-lstdc++,--end-group
fi

#generate the boot.bin
if [[ $bootgen_enable == true ]]; then
    if [ ! -d "arch" ]; then
        cp -R $BAREMETAL_DIR/thirdparty/arch ./
    fi
    /proj/xbuilds/2022.2_daily_latest/installs/lin64/Vitis/2022.2/bin/bootgen -arch versal -image ${BIF} -o ${BOOTBIN} -w
    
    rm -rf ./build
    mkdir ./build
    mv ./${BOOTBIN} ./build/
    rm $BIF
    #rm -rf arch
fi

echo "Compilation script completed."




