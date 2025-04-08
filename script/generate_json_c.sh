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

CUR_DIR=`pwd`
echo "current directory"
echo $CUR_DIR
# Initialize variables

work_folder=""
output_file="baremetal_metadata.cpp"

# Print usage
usage() {
    echo "Usage: $0 --work_folder <path> --output_file <file> "
}

# Parse arguments
echo $#

if [ $# -lt 2 ]; then
	usage
	return
fi

while [[ $# -gt 0 ]]; do
    case "$1" in
        -help)
            usage
            ;;
        --work_folder)
            work_folder="$2"
            shift 2
            ;;
				--output_file)
						output_file="$2"
						shift 2
						;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

pushd .
cd $work_folder
work_folder=`pwd`
popd
echo "*******************JSON******************"
source /proj/petalinux/2023.2/petalinux-v2023.2_daily_latest/tool/petalinux-v2023.2-final/settings.sh

python3 ../script/aie_config_parse.py driverconfig.bin $work_folder
python3 ../script/aie_config_parse.py graphconfig.bin $work_folder
python3 ../script/aie_config_parse.py gmiosconfig.bin $work_folder
python3 ../script/aie_config_parse.py rtpsconfig.bin $work_folder
python3 ../script/aie_config_parse.py externalbuffersconfig.bin $work_folder

if [ -f $output_file ]; then
   rm $output_file
fi

xxd -i ./build/driverconfig.bin >> $output_file
xxd -i ./build/graphconfig.bin >> $output_file
xxd -i ./build/gmiosconfig.bin >> $output_file
xxd -i ./build/rtpsconfig.bin >> $output_file
xxd -i ./build/externalbuffersconfig.bin >> $output_file

sed -i -e 's/__build_driverconfig_bin/_binary_build_driverconfig_bin_start/g' $output_file
sed -i -e 's/__build_graphconfig_bin/_binary_build_graphconfig_bin_start/g' $output_file
sed -i -e 's/__build_gmiosconfig_bin/_binary_build_gmiosconfig_bin_start/g' $output_file
sed -i -e 's/__build_rtpsconfig_bin/_binary_build_rtpsconfig_bin_start/g' $output_file
sed -i -e 's/__build_externalbuffersconfig_bin/_binary_build_externalbuffersconfig_bin_start/g' $output_file


sed -i -e 's/unsigned int _binary_build_driverconfig_bin_start_len =/unsigned int _binary_build_driverconfig_bin_size = /g' $output_file
sed -i -e 's/unsigned int _binary_build_graphconfig_bin_start_len =/unsigned int _binary_build_graphconfig_bin_size = /g' $output_file
sed -i -e 's/unsigned int _binary_build_gmiosconfig_bin_start_len =/unsigned int _binary_build_gmiosconfig_bin_size = /g' $output_file
sed -i -e 's/unsigned int _binary_build_rtpsconfig_bin_start_len =/unsigned int _binary_build_rtpsconfig_bin_size = /g' $output_file
sed -i -e 's/unsigned int _binary_build_externalbuffersconfig_bin_start_len =/unsigned int _binary_build_externalbuffersconfig_bin_size = /g' $output_file



echo "Script completed."




