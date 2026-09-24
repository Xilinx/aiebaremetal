# Copyright (C) 2024 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
import json

file_path = '../Work/ps/c_rts/aie_control_config.json'

with open(file_path, 'r') as file:
    data = json.load(file)

aie_metadata = data.get('aie_metadata', {})

driver_config = aie_metadata.get('driver_config', {})
hw_gen = driver_config.get('hw_gen')
print(hw_gen)
