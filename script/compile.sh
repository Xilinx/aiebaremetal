#!/bin/bash
# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Source the environment variables
#AIE_GEN=2
set -e
set -o pipefail
# set -x
CURRDIR=`pwd`

# Check if XILINX_VITIS environment variable is set and both aiecompiler and v++ are available
if [ -z "$XILINX_VITIS" ] || ! which aiecompiler > /dev/null 2>&1 || ! which v++ > /dev/null 2>&1; then
    echo "XILINX_VITIS is not set or aiecompiler/v++ not found. Sourcing envaie2pst50.sh..."
    source envaie2pst50.sh
fi

check_status() {
    status=$1
    cmd_desc=$2
    
    if [ $status -ne 0 ]; then
        echo "Error: ${cmd_desc:-Command} failed with exit code $status" >&2
        exit $status
    fi
    
    return 0
}

# Initialize variables
BAREMETAL_TEST_DIR=$BAREMETAL_DIR/example/
BAREMETAL_SCRIPT_DIR=$BAREMETAL_DIR/script/
ARCH_DIR=$BAREMETAL_DIR/thirdparty/arch/ps/
ARCH_72_DIR=$ARCH_DIR/psv_cortexa72_0/
ARCH_53_DIR=$ARCH_DIR/psv_cortexr5_0/
ARCH_52_DIR=$ARCH_DIR/cortexr52_0/standalone_cortexr52_0/bsp/
ARCH_78_DIR=$ARCH_DIR/cortexa78_0/standalone_cortexa78_0/bsp/

#lwip lib
LWIPLIB=
XTFLWIPALL=./build/libxtflwipall.a


runtime_source_file=""
kernel_folder=`pwd`
default_json_dir=$kernel_folder/Work/ps/c_rts/
control_json=$default_json_dir/aie_control_config.json
partition_json=$default_json_dir/aie_partition.json
aie_version="1"
compiler_options_aie="--disable-multirate-analysis"
compiler_options_vpp="disable-multirate=true"
#                 --include=$BAREMETAL_DIR/thirdparty/arch/ps/psv_cortexa72_0/include/\
include_options="--include=./ --include=$XILINX_VITIS_AIETOOLS/include/aie_api \

                 --include=$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ \
                 --include $BAREMETAL_DIR/src/include --include $BAREMETAL_DIR/src/include/common_layer/ \
                 --include $BAREMETAL_DIR/src/include/common_layer/aegothers"

lwip_include_options=
        
host_compiler_options_aie=""
preproc_options=""
#sysrootinclude="$ARCH_72_DIR/include"
#sysrootlib="$ARCH_72_DIR/lib"
#sysrootlinker="$ARCH_72_DIR/lscript.ld"
hw_qemu=false
bootgen_enable=false
hw_only=false
workfolder=
rpu=false
lwip_enalbe=false
palmyra=false
compiler="aiecompiler"
allgmio=true
PALMYRAAIECOMPILERMOPTION=
PALMYRAHOSTOPTION=
new_api=false
aeg_debug=false

# Print usage
usage() {
    echo "Usage: $0 [--runtime_source_file <path>] [--aie_version <version [1, 2]]> [--kernel_folder <path>] [--compiler_options <compiler optionss and include paths>] [-new_api] [-aeg_debug] [-bootgen] [-rpu]"
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
        --control_json)
            control_json="$2"
            shift 2
            ;;
        --partition_json)
            partition_json="$2"
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
            compiler_options_aie="$2"
            compiler_options_vpp="$2"
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
          --hw_qemu)
            hw_qemu=true
            shift
            ;;
            -bootgen)
            bootgen_enable=true
            shift
            ;;
            -palmyra)
            palmyra=true
            shift
            ;;
            -hw_only)
            hw_only=true
            shift
            ;;
            -custom_pdi)
            custom_pdi=true
            shift
            ;;
            -allgmio)
            allgmio=true
	          shift
            ;;
            -rpu)
            rpu=true
            shift
            ;;
            -new_api)
            new_api=true
            preproc_options="$preproc_options -DAEG_INHERITANCE_BASED_API"
            shift
            ;;
            -aeg_debug|--aeg_debug)
            aeg_debug=true
            preproc_options="$preproc_options -DAEG_DEBUG"
            shift
            ;;
            -lwip)
            lwip_enalbe=true
            LWIPLIB=-lxtflwipall
            shift
            ;;
            --compiler)
            compiler="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

#set the palmyra COMPILE OPTION
if [ $palmyra == true ]; then
    PALMYRAAIECOMPILERMOPTION=" -Xpreproc=-D_PALMYRA_ "
    PALMYRAHOSTOPTION=" -D_PALMYRA_ "
fi

compiler_options_aie+=${PALMYRAAIECOMPILERMOPTION}
host_compiler_options_aie+=${PALMYRAHOSTOPTION}

MAKE_EXTRA_CPPFLAGS=
if [ "$aeg_debug" = true ]; then
    MAKE_EXTRA_CPPFLAGS="EXTRA_CPPFLAGS=-DAEG_DEBUG"
fi

if [[ -n "$control_json" ]]; then
    pushd .
    control_json_dir="$(dirname "$control_json")"
    control_json_file="$(basename "$control_json")"
    mkdir -p $control_json_dir
    cd "$control_json_dir"
    control_json="$(pwd)/$control_json_file"
    popd
fi

if [[ -n "$partition_json" ]]; then
    pushd .
    partition_json_dir="$(dirname "$partition_json")"
    partition_json_file="$(basename "$partition_json")"
    mkdir -p $partition_json_dir
    cd "$partition_json_dir"
    partition_json="$(pwd)/$partition_json_file"
    popd
fi

#custom pdi
if [ $aie_version == 5 ]; then
#   PLATFORM=/proj/xbuilds/2025.1_daily_latest/internal_platforms/vek385_base_202510_1/vek385_base_202510_1.xpfm
#   PLATFORM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/vek385_base_202520_1/vek385_base_202520_1.xpfm
  PLATFORM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/vek385_base/vek385_base.xpfm

elif [ $aie_version == 2 ]; then
  PLATFORM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/xilinx_vek280_base_202520_1/xilinx_vek280_base_202520_1.xpfm

else
  PLATFORM=/proj/xbuilds/2025.2_daily_latest/internal_platforms/xilinx_vck190_base_202520_1/xilinx_vck190_base_202520_1.xpfm
fi

if [[ $compiler == "v++" && -n "$partition_option" ]]; then
    partition_option="${partition_option/--enable-partition=/aie.enable-partition=}"
fi

if [ -z "$sysrootinclude" ]; then
	if [ $rpu == true ]; then
		if [ $aie_version == 5 ]; then
			sysrootinclude="$ARCH_52_DIR/include"
			include_options="$include_options --include $sysrootinclude"
			include_options="$include_options --include $BAREMETAL_DIR/thirdparty/embeddedsw/lib/bsp/standalone/src/arm/cortexr5/armclang/"
		else
			sysrootinclude="$ARCH_53_DIR/include"
			include_options="$include_options --include $sysrootinclude"
		fi
	else
		if [ $aie_version == 5 ]; then
			sysrootinclude="$ARCH_78_DIR/include"
			include_options="$include_options --include $sysrootinclude"
			include_options="$include_options --include $BAREMETAL_DIR/thirdparty/embeddedsw/lib/bsp/standalone/src/arm/ARMv8/64bit/armclang/"
		else
			sysrootinclude="$ARCH_72_DIR/include"
			include_options="$include_options --include $sysrootinclude"
		fi
	fi
fi

if [ -z "$sysrootlib" ]; then
	if [ $rpu == true ]; then
		if [ $aie_version == 5 ]; then
			sysrootlib="$ARCH_52_DIR/lib"
		else
			sysrootlib="$ARCH_53_DIR/lib"
		fi
	else
		if [ $aie_version == 5 ]; then
			sysrootlib="$ARCH_78_DIR/lib"
		else
			sysrootlib="$ARCH_72_DIR/lib"
		fi
	fi
fi

if [ -z "$sysrootlinker" ]; then
	if [ $rpu == true ]; then
		if [ $aie_version == 5 ]; then
			sysrootlinker="$ARCH_52_DIR/lscript.ld"
		else
			sysrootlinker="$ARCH_53_DIR/lscript.ld"
		fi
	else
		if [ $aie_version == 5 ]; then
			sysrootlinker="$ARCH_78_DIR/lscript.ld"
		else
			sysrootlinker="$ARCH_72_DIR/lscript.ld"
		fi
	fi
fi

if [[ -n $workfolder ]]; then

    kernel_folder="$(dirname "${workfolder}")"
   
fi

pushd $kernel_folder
kernel_folder=`pwd`
popd

echo "------------kernel_folder--2---$kernel_folder----------"

pushd .
dir="$(dirname "${runtime_source_file}")"
filename="$(basename "${runtime_source_file}")"
cd $dir
runtime_source_file=`pwd`/$filename
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
if [[ $compiler == "aiecompiler" ]]; then
    echo "compiler_options_aie: ${compiler_options_aie}"
else
    echo "compiler_options_vpp: ${compiler_options_vpp}"
fi
echo "${include_options}"
echo ${preproc_options}
echo "aeg_debug: ${aeg_debug}"
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
    make alib ${MAKE_EXTRA_CPPFLAGS}
    check_status $? "make alib"
    popd
else
    echo "********************Work folder is provided*****************"
fi

# compile the graph
if [ $aie_version == 1 ]; then
	DEVICE_FILE="xcvc1902"     
elif [ $aie_version == 2 ]; then
	DEVICE_FILE="xcve2802"
elif [ $aie_version == 5 ];then
	DEVICE_FILE="xc2ve3858"
else
    echo "Unsupported AIE version: $aie_version"
    exit 1
fi

if [[ -z $workfolder ]]; then
    echo "*******************Compiler being called****************"
    rm -rf $kernel_folder/Work || true
    rm -f $kernel_folder/*.log || true
    rm -f $kernel_folder/libadf.a || true
    rm -f $BAREMETAL_DIR/src/include/generated_graphs.h || true
    rm -f $(dirname $runtime_source_file)/generated_graphs.h || true
    if [[ $compiler == "aiecompiler" ]]; then
        
        metadata_preproc="$preproc_options -D__AEG_METADATA_ONLY__"
        if [[ "$new_api" == true ]]; then
            if [[ $custom_pdi == true ]]; then
                aiecompiler $runtime_source_file --platform $PLATFORM --Xpreproc="$metadata_preproc" $compiler_options_aie "$partition_option" --workdir=$kernel_folder/Work $include_options --target=hw
                check_status $? "aiecompiler $runtime_source_file (metadata generation)"
                v++ -l --platform $PLATFORM   ./libadf.a -t hw --save-temps -g -o new.xsa
            else
                aiecompiler $runtime_source_file --part=$DEVICE_FILE --Xpreproc="$metadata_preproc" $compiler_options_aie "$partition_option" --workdir=$kernel_folder/Work $include_options --target=hw
                check_status $? "aiecompiler $runtime_source_file (metadata generation)"
            fi

            if [ -f "$kernel_folder/Work/ps/c_rts/aie_control_config.json" ]; then
                pushd $kernel_folder/Work/ps/c_rts
                if [ -f "../../arch/aie_partition.json" ]; then
                    python3 $BAREMETAL_SCRIPT_DIR/aiemetacpp.py aie_control_config.json ../../arch/aie_partition.json
                else
                    python3 $BAREMETAL_SCRIPT_DIR/aiemetacpp.py aie_control_config.json
                fi
                generation_status=$?
                popd
                check_status $generation_status "Code generation from metadata"
                cp $kernel_folder/Work/ps/c_rts/generated_graphs.h $BAREMETAL_DIR/src/include/
                check_status $? "Copy generated_graphs.h to baremetal directory"
                cp $kernel_folder/Work/ps/c_rts/generated_graphs.h $(dirname $runtime_source_file)/
                check_status $? "Copy generated_graphs.h to runtime source file directory"
            fi

            if [[ $hw_only == false ]]; then
                if [[ $custom_pdi == true ]]; then
                    aiecompiler $runtime_source_file --platform $PLATFORM --Xpreproc="$preproc_options" $compiler_options_aie "$partition_option" --workdir=$kernel_folder/Work $include_options --target=x86sim
                    check_status $? "aiecompiler $runtime_source_file"
                else
                    aiecompiler $runtime_source_file --part=$DEVICE_FILE --Xpreproc="$preproc_options" $compiler_options_aie "$partition_option" --workdir=$kernel_folder/Work $include_options --target=hw
                    check_status $? "aiecompiler $runtime_source_file"
                fi
            fi
        else
            if [[ $custom_pdi == true ]]; then
                aiecompiler $runtime_source_file --platform $PLATFORM --Xpreproc="$preproc_options" $compiler_options_aie "$partition_option" --workdir=$kernel_folder/Work $include_options
                check_status $? "aiecompiler $runtime_source_file"
                v++ -l --platform $PLATFORM   ./libadf.a -t hw --save-temps -g -o new.xsa
            else
                aiecompiler $runtime_source_file --part=$DEVICE_FILE --Xpreproc="$preproc_options" $compiler_options_aie "$partition_option" --workdir=$kernel_folder/Work $include_options
                check_status $? "aiecompiler $runtime_source_file"
            fi

            if [ -f "$kernel_folder/Work/ps/c_rts/aie_control_config.json" ]; then
                pushd $kernel_folder/Work/ps/c_rts
                if [ -f "../../arch/aie_partition.json" ]; then
                    python3 $BAREMETAL_SCRIPT_DIR/aiemetacpp.py aie_control_config.json ../../arch/aie_partition.json
                else
                    python3 $BAREMETAL_SCRIPT_DIR/aiemetacpp.py aie_control_config.json
                fi
                generation_status=$?
                popd
            fi
        fi

    else
        if [[ -n "$preproc_options" ]]; then
            v++ -c --mode aie --target hw $runtime_source_file --part=$DEVICE_FILE --Xpreproc="$preproc_options" $compiler_options_vpp "$partition_option" --aie.workdir=$kernel_folder/Work $include_options
            check_status $? "v++ $runtime_source_file"
        else
            v++ -c --mode aie --target hw $runtime_source_file --part=$DEVICE_FILE $compiler_options_vpp "$partition_option" --aie.workdir=$kernel_folder/Work $include_options
            check_status $? "v++ $runtime_source_file"
        fi
        
        if [ -f "$kernel_folder/Work/ps/c_rts/aie_control_config.json" ]; then
            pushd $kernel_folder/Work/ps/c_rts
            if [ -f "../../arch/aie_partition.json" ]; then
                python3 $BAREMETAL_SCRIPT_DIR/aiemetacpp.py aie_control_config.json ../../arch/aie_partition.json
            else
                python3 $BAREMETAL_SCRIPT_DIR/aiemetacpp.py aie_control_config.json
            fi
            generation_status=$?
            popd
            
            check_status $generation_status "Code generation from metadata"
            
            cp $kernel_folder/Work/ps/c_rts/generated_graphs.h $BAREMETAL_DIR/src/include/
            check_status $? "Copy generated_graphs.h to baremetal directory"
            cp $kernel_folder/Work/ps/c_rts/generated_graphs.h $(dirname $runtime_source_file)/
            check_status $? "Copy generated_graphs.h to runtime source file directory"
        fi
    fi
    workfolder=$kernel_folder/Work
fi

 pushd .
 echo "The app is located in: $kernel_folder"

cd $BAREMETAL_TEST_DIR

export LD_LIBRARY_PATH=$XILINX_VITIS/aietools/lib/lnx64.o:$LD_LIBRARY_PATH

## compile the simulation library
if [[ $hw_only == false ]]; then
    make ./build/libaiebaremetal.so  AIE_GEN=$aie_version ARCH_INCLUDE=$sysrootinclude AIE_KERNEL_LOC=$kernel_folder AIE_CONTROL_JSON=$control_json AIE_PARTITION_JSON=$partition_json ${MAKE_EXTRA_CPPFLAGS} -j32
    check_status $? "make ./build/libaiebaremetal.so"
fi

AIE=10
AIEML=20
AIE2PS=22
if [ "$aie_version" == "2" ]; then
	__AIE_ARCH__=${AIEML}
	if [[ $rpu == true ]]; then
		BIF=vek280_rpu_boot_image.bif
    elif [[ $custom_pdi == true ]]; then
        BIF=vek280_custom.bif
	else
		BIF=vek280_boot_image.bif
	fi

	BOOTBIN=Vek280BOOT.BIN
	ELF=vek280.elf
elif [ "$aie_version" == "5" ]; then
	__AIE_ARCH__=${AIE2PS}
	if [ $rpu == true ]; then
        if [ $palmyra == true ]; then
		    BIF=Palmyra.bif
        else
		if [ $allgmio == true ]; then
			echo "*************vek385 allgmio*****************"
			BIF=vek385_allgmio.bif
		else
			echo "*************vek385*****************"
			BIF=vek385.bif
		fi
        fi
	else
		 if [ $palmyra == true ]; then
            echo "**********************palmyra**************"
		    BIF=Palmyra.bif
        else
		if [ $allgmio == true ]; then
			echo "*************vek385 allgmio*****************"
			BIF=vek385_allgmio.bif
		else
			echo "*************vek385*****************"
			BIF=vek385.bif
		fi
        fi
	fi

    if [ $palmyra == true ]; then
        BOOTBIN=Palmyra.BIN
        ELF=Palmyra.elf
    else
        BOOTBIN=vek385.BIN
        ELF=vek385.elf
    fi

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

echo "The workfolder is located in: $workfolder"
if [[ -z $workfolder ]]; then
    echo "The workfolder is not exist  in: $workfolder"
     
    if [ $rpu == true ]; then
        make hw_external_rpu  AIE_GEN=$aie_version  AIE_KERNEL_LOC=$kernel_folder ARCH_INCLUDE=$sysrootinclude ${MAKE_EXTRA_CPPFLAGS} -j32
        check_status $? "make hw_external_rpu"
    else
         if [ $aie_version == 5 ]; then
            echo "hw compile for apu kernel_folder is $kernel_folder"
            make hw_external_apu78   AIE_GEN=$aie_version  AIE_KERNEL_LOC=$kernel_folder ARCH_INCLUDE=$sysrootinclude ${MAKE_EXTRA_CPPFLAGS} -j32
            check_status $? "make hw_external_apu78"
         else
            echo "hw compile for apu kernel_folder is $kernel_folder"
            make hw_external_apu   AIE_GEN=$aie_version  AIE_KERNEL_LOC=$kernel_folder ARCH_INCLUDE=$sysrootinclude ${MAKE_EXTRA_CPPFLAGS} -j32
            check_status $? "make hw_external_apu"
         fi
    fi
else
	if [ $rpu == true ]; then
		if [ $aie_version == 5 ]; then
			make hw_external_rpu_r52   AIE_GEN=$aie_version  AIE_COMPILE_WORK=$workfolder ARCH_INCLUDE="$sysrootinclude" AIE_CONTROL_JSON=$control_json AIE_PARTITION_JSON=$partition_json ARCH_RPU_R52_ARC_DIC="$sysrootinclude" ${MAKE_EXTRA_CPPFLAGS} -j32
			check_status $? "make hw_external_rpu"

		else
			make hw_external_rpu   AIE_GEN=$aie_version  AIE_COMPILE_WORK=$workfolder ARCH_INCLUDE="$sysrootinclude" AIE_CONTROL_JSON=$control_json AIE_PARTITION_JSON=$partition_json ARCH_RPU_R53_ARC_DIC="$sysrootinclude" ${MAKE_EXTRA_CPPFLAGS} -j32
			check_status $? "make hw_external_rpu"
		fi
	else
        if [ $aie_version == 5 ]; then
			echo "hw compile for apu A78"
            make hw_external_apu78   AIE_GEN=$aie_version  AIE_COMPILE_WORK=$workfolder ARCH_INCLUDE="$sysrootinclude" AIE_CONTROL_JSON=$control_json AIE_PARTITION_JSON=$partition_json ARCH_APU_A78_ARC_DIC="$sysrootinclude" ${MAKE_EXTRA_CPPFLAGS} -j32
            check_status $? "make hw_external_apu78"
        else 
            echo "hw compile for apu A72"
            make hw_external_apu   AIE_GEN=$aie_version  AIE_COMPILE_WORK=$workfolder ARCH_INCLUDE="$sysrootinclude" AIE_CONTROL_JSON=$control_json AIE_PARTITION_JSON=$partition_json ARCH_APU_A72_ARC_DIC="$sysrootinclude" ${MAKE_EXTRA_CPPFLAGS} -j32
            check_status $? "make hw_external_apu A72"
        fi
    fi     
fi

if [ $lwip_enalbe == true ]; then
    if [ $rpu == true ]; then
        echo "to do not support lwip in rpu yet"
    else
        if [ ! -e "$XTFLWIPALL" ]; then
            make $XTFLWIPALL ${MAKE_EXTRA_CPPFLAGS}
            check_status $? "make $XTFLWIPALL"
        fi
    
        lwip_include_options="-I$BAREMETAL_DIR/thirdparty/xtf/hw/export/hw/sw/hw/mybsp/bspinclude/include	\
		         -I$BAREMETAL_DIR/thirdparty/embeddedsw/ThirdParty/sw_services/lwip220/src/lwip-2.2.0/contrib/ports/xilinx/include/ \
		         -I$BAREMETAL_DIR/thirdparty/embeddedsw/ThirdParty/sw_services/lwip220/src/lwip-2.2.0/src/include	\
		         -I$BAREMETAL_DIR/thirdparty/embeddedsw/XilinxProcessorIPLib/drivers/ttcps/src/	\
		         -I$BAREMETAL_DIR/thirdparty/xtf/app/src/ \
		         -I$BAREMETAL_DIR/src/include/common/net/"
    fi
fi


popd
cp $BAREMETAL_SCRIPT_DIR/$BIF  ./

export BIF
export BOOTBIN
export ELF
#export ARCH_DIR

#build the vek280/a72 app
#source  /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh
#source  /proj/petalinux/2025.1/petalinux-v2025.1_daily_latest/tool/petalinux-v2025.1-final/settings.sh
#source  /proj/petalinux/2025.2/petalinux-v2025.2_daily_latest/tool/petalinux-v2025.2-final/settings.sh
source  /proj/petalinux/2026.1/petalinux-v2026.1_daily_latest/tool/petalinux-v2026.1-final/settings.sh
if [ $rpu == true ]; then
	echo "syslib is $sysrootlib"
	echo "sysinc is $sysrootinclude"
	echo "linker is $sysrootlinker"
	echo "baremetal path is $BAREMETAL_TEST_DIR/build/"
	# _RPU_ used to declare this is RPU, app to support both APU and RPU can use this macro to do RPU specify config
	if [ $aie_version == 5 ]; then
		echo "rpu r52 compile"
        set -x
		armr5-none-eabi-g++ -DARMR5 -D__AIE_ARCH__=${__AIE_ARCH__} ${host_compiler_options_aie} -mcpu=cortex-r52 -mfloat-abi=hard -mfpu=vfpv3-d16 -Wl,-T -Wl,$sysrootlinker -D_RPU_  $preproc_options -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/fal/build/src/include/ -I/$BAREMETAL_DIR/src/include/ -I$sysrootinclude -L$sysrootlib -L$BAREMETAL_TEST_DIR/build/ -o ./${ELF} $runtime_source_file -Wl,--start-group,-laiebaremetalr52,-lxil,-lgcc,-lstdc++,-lxiltimer,-lxilstandalone,-lc,--end-group
        set +x
		check_status $? "armr5-none-eabi-g++ rpu compile"

	else
		echo "rpu r53 compile"
		armr5-none-eabi-g++ -D__AIE_ARCH__=${__AIE_ARCH__} ${host_compiler_options_aie} -mcpu=cortex-r5 -mfloat-abi=hard -mfpu=vfpv3-d16 -Wl,-T -Wl,$sysrootlinker -D_RPU_  $preproc_options -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/fal/build/src/include/ -I/$BAREMETAL_DIR/src/include/ -I$sysrootinclude -L$sysrootlib -L$BAREMETAL_TEST_DIR/build/ -o ./${ELF} $runtime_source_file -Wl,--start-group,-laiebaremetalr53,-lxil,-lgcc,-lc,-lstdc++,--end-group
		check_status $? "armr5-none-eabi-g++ rpu compile"
	fi
else

    if [ $aie_version == 5 ]; then
        echo "apu compile A78"
        aarch64-none-elf-gcc $preproc_options -D__AIE_ARCH__=${__AIE_ARCH__}  ${host_compiler_options_aie} -mcpu=cortex-a78 -Wl,-T -Wl,$sysrootlinker -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/fal/build/src/include/ -I/$BAREMETAL_DIR/src/include -I$sysrootinclude -L$sysrootlib -L$BAREMETAL_TEST_DIR/build/ -o ./${ELF} $runtime_source_file -Wl,--start-group,-laiebaremetala78,-lxil,-lxiltimer,-lxilstandalone,-lgcc,-lc,-lstdc++,--end-group
        check_status $? "aarch64-none-elf-gcc apu compile A78"
    else
        echo "apu compile"
        aarch64-none-elf-gcc $preproc_options -D__AIE_ARCH__=${__AIE_ARCH__}  ${host_compiler_options_aie} -mcpu=cortex-a72 -Wl,-T -Wl,$sysrootlinker $lwip_include_options -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/driver/internal/ -I$BAREMETAL_DIR/thirdparty/aielib/aie-rt/fal/build/src/include/ -I/$BAREMETAL_DIR/src/include -I$sysrootinclude -L$sysrootlib -L$BAREMETAL_TEST_DIR/build/ -o ./${ELF} $runtime_source_file $LWIPLIB -Wl,--start-group,-laiebaremetala72,-lxil,-lgcc,-lc,-lstdc++,--end-group
        check_status $? "aarch64-none-elf-gcc apu compile"
    fi

fi

#generate the boot.bin
if [[ $bootgen_enable == true ]]; then
    if [ ! -d "arch" ]; then
        cp -R $BAREMETAL_DIR/thirdparty/arch ./
    fi

    if [ $aie_version == 5 ]; then 
        bootgen -arch versal_2ve_2vm -image ${BIF} -o ${BOOTBIN} -w
        check_status $? "bootgen versal_2ve_2vm"
    else
        bootgen -arch versal -image ${BIF} -o ${BOOTBIN} -w
        check_status $? "bootgen versal"
    fi
    
    rm -rf ./build
    mkdir ./build
    mv ./${BOOTBIN} ./build/
    rm -f $BIF || true
    rm -rf arch
fi

if [[ $hw_qemu == true ]]; then
    echo "Building QEMU package"
    source $BAREMETAL_SCRIPT_DIR/build_hw_qemu.sh
    check_status $? "qemu.sh"
fi

echo "Compilation script completed."
