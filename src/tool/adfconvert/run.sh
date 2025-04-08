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

if [ -z "$aiecompiler_path" ]; then
    echo "Error: aiecompiler not found in your PATH."
    exit 1
fi

aietools_path=$(echo "$aiecompiler_path" | awk -F'/aietools/' '{print $1}')
first_adf_path=$(find "$aietools_path" -name adf.h -print -quit)

if [ -z "$aiecompiler_path" ]; then
    echo "Error: adf.h not found in your PATH."
    exit 
else
    echo "Found adf.h in "$first_adf_path"" 
fi


first_adf_path=$(find "$aietools_path" -name adf.h -print -quit)

ADF_INCLUDE_PATH=$(dirname "$first_adf_path")

""$MY_DEPS_PATH"/ld-linux-x86-64.so.2" --library-path ""$MY_DEPS_PATH/shared_libs"" ""$GLOBAL_DIR"/adftoaeg" "$@" -- -I"$ADF_INCLUDE_PATH" -isystem ""$MY_DEPS_PATH"/clang_headers/" 
 
