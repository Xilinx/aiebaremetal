# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
import re

# Global storage
ddr_names = []
extbuf_ids = []
offsets = []
buffer_descriptors = []

# Helper to insert text after a specific line
def insert_after(lines, index, text):
    lines.insert(index + 1, text + "\n")

# Main patching function
def patch_file(input_file, output_file):
    with open(input_file, 'r') as file:
        lines = file.readlines()
    
    i = 0
    while i < len(lines):
        line = lines[i]
        
        # 1. Find adf::getExternalBufferAddress("<ddrname>", ...)
        ddr_match = re.search(r'adf::getExternalBufferAddress\("(.*?)",\s*\d+\)', line)
        if ddr_match:
            ddr_name = ddr_match.group(1)
            ddr_names.append(ddr_name)
            i += 1
            while i < len(lines):
                line = lines[i]
                
                # 2. Find adf::getExternalBufferAddress(<extbufid>)
                extbuf_match = re.search(r'(\w+)\s*\+\s*adf::getExternalBufferAddress\((.*?)\)', line)
                if extbuf_match:
                    num, extbuf_id = extbuf_match.groups()
                    offsets.append(num)
                    extbuf_ids.append(extbuf_id)
                    i += 1
                    while i < len(lines):
                        line = lines[i]
                        
                        # 3. Find configureBufferDescriptor
                        bd_match = re.search(r'adf::configureBufferDescriptor\s*\(\s*adf::shim_tile\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*\)', line)
                        if bd_match:
                            col, row, bdid, bd = bd_match.groups()
                            buffer_descriptors.append((col, row, bdid, bd))
                            
                            # 4. Insert AddDDRCustomOp after configureBufferDescriptor
                            if extbuf_ids and ddr_names:
                                insert_after(lines, i, f'AddDDRCustomOp(0, {extbuf_ids[-1]}, {offsets[-1]}*4, {col}, {bdid}, "{ddr_names[-1]}", patch_op_code);')
                            break
                        i += 1
                    break
                i += 1
        i += 1
    
    # 7-8: Insert patch_op_code after the first '{' following executeRuntimeControlInternal
    for i, line in enumerate(lines):
        if 'executeRuntimeControlInternal' in line:
            j = i + 1
            while j < len(lines) and '{' not in lines[j]:
                j += 1
            if j < len(lines):
                insert_after(lines, j, '    int32_t patch_op_code = ( 1<<7) + 1;')
            break

    # 9: Add AddDDRCustomOp declaration before executeRuntimeControlInternal
    for i, line in enumerate(lines):
        if 'executeRuntimeControlInternal' in line:
            lines.insert(i, '#include \"aiepatch.h\"\n')
            lines.insert(i+1, 'void AddDDRCustomOp(uint32_t action, uint64_t bufId, uint64_t offsetInBytes, uint8_t shimCol, uint8_t bdId, std::string bufName, int32_t patch_op_code);\n')
            lines.insert(i + 2, '#ifndef __TXN_BIN__\n void AddDDRCustomOp(uint32_t action, uint64_t bufId, uint64_t offsetInBytes, uint8_t shimCol, uint8_t bdId, std::string bufName, int32_t patch_op_code) { return; }\n#endif\n')
            break
    
    # Write patched content to output file
    with open(output_file, 'w') as file:
        file.writelines(lines)

# Usage
patch_file('./Work/ps/c_rts/aie_runtime_control.cpp', 'output.cpp')
