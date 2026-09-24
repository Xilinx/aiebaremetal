###############################################################################
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
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
###############################################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd $SCRIPT_DIR

pushd ../../

mkdir -p thirdparty
pushd thirdparty
if [ ! -d "aiebaremetal" ]; then
    git clone --recursive <aiebaremetal_repo_url> aiebaremetal
else
    echo "Directory 'thirdparty/aiebaremetal' already exists. Skipping clone."
fi
popd

source setup.sh
if [ ! -d "build" ]; then
    mkdir build
fi
cd build
cmake ../
make
cd ..
source thirdparty/aiebaremetal/script/settings.sh 1

popd
../../convert.sh ./graph.cpp -compile -aie_version 2 --work-folder ./Work -out ./graph_new.cpp 2>&1 | tee cmd_output.txt

if grep -q "error: " cmd_output.txt; then
    echo "Test Failure"
    echo
    echo "Errors found:"
    grep "error: " cmd_output.txt
else
    echo "Test Success"
fi
