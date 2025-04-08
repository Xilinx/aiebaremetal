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

import json

file_path = '../Work/ps/c_rts/aie_control_config.json'

with open(file_path, 'r') as file:
    data = json.load(file)

aie_metadata = data.get('aie_metadata', {})

driver_config = aie_metadata.get('driver_config', {})
hw_gen = driver_config.get('hw_gen')
print(hw_gen)
