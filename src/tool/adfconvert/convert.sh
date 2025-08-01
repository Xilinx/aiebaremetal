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
#!/usr/bin/env bash

aiecompiler_path=$(which aiecompiler)

usage() {
  echo "Usage: $0 <input> -out <output>"
  echo "Converts the input to the new format and stores it in the output."
  echo
  echo "Options:"
  echo "  --help        Display this help message"
  echo "  -out          Specifies output file. Default is <"inputName".new.cc>"
  echo "  -rpu          Sets rpu to true during conversion"
  echo "  -compile      Converts to new format then compiles with compile.sh"
  echo "  -aie_version      Needed with compile flag"
  echo "  -I            Add a custom include path"
}

if [ -z "$aiecompiler_path" ]; then
    echo -e "\e[31mError: aiecompiler not found in your PATH.\e[0m"
    echo
    exit 1
fi

if [ -z "$1" ]; then
    usage
    exit 1
fi

pushd $XILINX_VITIS > /dev/null
first_adf_path=$(find -name "adf.h" -print -quit)
popd > /dev/null

if [ -z "$first_adf_path" ]; then
    echo -e "\e[31mError: adf.h not found in your PATH.\e[0m"
    echo
    exit 1
fi

ADF_INCLUDE_PATH="${XILINX_VITIS}/$(dirname "$first_adf_path")"
echo "ADF_INCLUDE_PATH: $ADF_INCLUDE_PATH"

compile_flag=false
rpu_flag=false
aiegen_flag=""
out_file=""
declare -a include_dirs

input="$1"

if [[ "$input" = "--help" || "$input" = "-h" || "$input" = "-help" ]]; then
    usage
    exit 1 
fi

if [ ! -e "$input" ]; then
    echo -e "\e[31mError: No such file in path. \e[0m"
    echo
    exit 1
fi

dir=$(dirname "$input") 
filename=$(basename "$input")
base="${filename%%.*}"   
suffix="${filename#*.}"  
out_name="${base}.new.${suffix}"
work_folder=
shift  

while [[ $# -gt 0 ]]; do
    case "$1" in
        -out)
            if [ -n "$2" ] && [[ "$2" != -* ]]; then
                out_file="$2"  
                shift  
            fi
            ;;
        -compile)
            compile_flag=true
            ;;
        -aie_version)
            if [ -n "$2" ] && [[ "$2" != -* ]]; then
                aiegen_flag="$2"  
                echo "aiegen_flag: $aiegen_flag"
                shift  
            else
                echo -e "\e[31mError: -aie_version requires a value.\e[0m"
                echo
                exit 1
            fi
            ;;
        -I*)
            include_dirs+=("${1#-I}")
            ;;
        -rpu)
            rpu_flag=true
            ;;
        --work-folder)
            if [ -n "$2" ] && [[ "$2" != -* ]]; then
                work_folder="$2"
                shift
            else
                echo -e "\e[31mError: --work-folder requires a value.\e[0m"
                echo
                exit 1
            fi
            ;;
        *)
            echo -e "\e[31mError: Unknown option: $1\e[0m"
            echo "Supported flags are -rpu, -compile with -aie_version, and --work-folder."
            echo
            exit 1
            ;;
    esac
    shift 
done

if [ "$out_file" = "" ]; then
    output="${dir}/${out_name}"
else
    output="$out_file"
fi

include_args=""
for dir in "${include_dirs[@]}"; do
    include_args="$include_args -I$dir"
done

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ "$compile_flag" = true ]; then

    compile_script_path="${script_dir}/thirdparty/aiebaremetal/script/compile.sh"
    if [ -z "$compile_script_path" ] || [ ! -f "$compile_script_path" ]; then
        echo -e "\e[31mError: compile.sh not found in your PATH.\e[0m"
        echo
        exit 1
    else
        if [ -n "$aiegen_flag" ]; then
            if [ "$out_file" = "" ]; then
                if [ "$rpu_flag" = true ]; then
                    "$LD_SO" --library-path "${LIB_PATH}:${LIB_BASE_PATH}" ""$GLOBAL_DIR"/adftoaeg" "$input" -- -I"$ADF_INCLUDE_PATH" $include_args -isystem "${CLANG_INCLUDE_PATH}" 
                else
                    "$LD_SO" --library-path "${LIB_PATH}:${LIB_BASE_PATH}" ""$GLOBAL_DIR"/adftoaeg" "$input" -- -I"$ADF_INCLUDE_PATH" $include_args -isystem "${CLANG_INCLUDE_PATH}"
                fi
            else
                if [ "$rpu_flag" = true ]; then
                    "$LD_SO" --library-path "${LIB_PATH}:${LIB_BASE_PATH}" ""$GLOBAL_DIR"/adftoaeg" "$input" -out $out_file -- -I"$ADF_INCLUDE_PATH" $include_args -isystem "${CLANG_INCLUDE_PATH}" 
                else
                    "$LD_SO" --library-path "${LIB_PATH}:${LIB_BASE_PATH}" ""$GLOBAL_DIR"/adftoaeg" "$input" -out $out_file -- -I"$ADF_INCLUDE_PATH" $include_args -isystem "${CLANG_INCLUDE_PATH}"
                fi
            fi
						if [ "$rpu_flag" = true ]; then
            	echo "compile.sh" -hw_only --runtime_source_file "$output" --aie_version $aiegen_flag -bootgen -rpu
            	"$compile_script_path" -hw_only --runtime_source_file "$output" --aie_version $aiegen_flag -bootgen -rpu
						else
            	echo "compile.sh" -hw_only --runtime_source_file "$output" --aie_version $aiegen_flag -bootgen
            	"$compile_script_path" -hw_only --runtime_source_file "$output" --aie_version $aiegen_flag -bootgen
						fi
            echo
        else
            echo -e "\e[31mError: Please set -aie_version flag for compile mode.\e[0m"
            echo
            exit 1
        fi
    fi

else
    if [ "$out_file" = "" ]; then 
        if [ "$rpu_flag" = true ]; then
            "$LD_SO" --library-path "${LIB_PATH}:${LIB_BASE_PATH}" ""$GLOBAL_DIR"/adftoaeg" "$input" -- -I"$ADF_INCLUDE_PATH" $include_args -isystem "${CLANG_INCLUDE_PATH}" 
        else
            "$LD_SO" --library-path "${LIB_PATH}:${LIB_BASE_PATH}" ""$GLOBAL_DIR"/adftoaeg" "$input" -- -I"$ADF_INCLUDE_PATH" $include_args -isystem "${CLANG_INCLUDE_PATH}"
        fi
    else
        if [ "$rpu_flag" = true ]; then
            "$LD_SO" --library-path "${LIB_PATH}:${LIB_BASE_PATH}" ""$GLOBAL_DIR"/adftoaeg" "$input" -out $out_file -- -I"$ADF_INCLUDE_PATH" $include_args -isystem "${CLANG_INCLUDE_PATH}" 
        else
            "$LD_SO" --library-path "${LIB_PATH}:${LIB_BASE_PATH}" ""$GLOBAL_DIR"/adftoaeg" "$input" -out $out_file -- -I"$ADF_INCLUDE_PATH" $include_args -isystem "${CLANG_INCLUDE_PATH}"
        fi
    fi
fi

echo "Converted '$input' to '$output'"