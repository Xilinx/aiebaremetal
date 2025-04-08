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


export GLOBAL_DIR="$(dirname "$(realpath "$0")")"
elf_path="$GLOBAL_DIR/adftoaeg"
deps_path="$GLOBAL_DIR/thirdparty/adftoaeg_deps"

if [[ -f "$elf_path" ]] && [[ -d "$deps_path" ]]  ; then
    export GLOBAL_DIR
    export MY_DEPS_PATH=""$GLOBAL_DIR"/thirdparty/adftoaeg_deps"
    echo "Setup is done."
else
    echo "The executable adftoaeg and/or its dependencies thirdparty/adftoaeg_deps are not found in the current working directory."
fi
