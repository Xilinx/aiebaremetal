# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
import re
import sys

import re

def get_core_num_per_col():
    """
    Parse the config.h file in the current folder and return the integer value
    of CORE_NUM_PER_COL if defined as '#define CORE_NUM_PER_COL <number>'.
    Returns None if not found or not parseable.
    """
    pattern = re.compile(r'^#\s*define\s+CORE_NUM_PER_COL\s+(\d+)')
    try:
        with open('./src/config.h', 'r') as f:
            for line in f:
                match = pattern.match(line.strip())
                if match:
                    var = int(match.group(1))
                    print("find CORE_NUM_PER_COL value is {}".format(var))
                    return int(match.group(1))
    except FileNotFoundError:
        print("config.h not found in current directory.")
    return 2 #define is 2 if not found

def parse_ddr_map(lines):
    """
    Parse lines to find 'EG.ddrout[i].in[0]' and the corresponding
    'XAie_DmaWriteBd(..., number)', then build a dictionary { i: number }.
    """
    ddr_map = {}
    ddrout_pattern = re.compile(r'EG\.ddrout\[(\d+)\]\.in\[0\]')
    dmawrite_pattern = re.compile(r'XAie_DmaWriteBd\s*\(.*?,\s*(\d+)\s*\);')
    current_ddr_i = None

    for line in lines:
        match_ddr = ddrout_pattern.search(line)
        if match_ddr:
            current_ddr_i = int(match_ddr.group(1))

        match_bd = dmawrite_pattern.search(line)
        if match_bd and current_ddr_i is not None:
            number_val = int(match_bd.group(1))
            ddr_map[current_ddr_i] = number_val
            current_ddr_i = None

    return ddr_map

def patch_lines(lines, ddr_map):
    """
    1. After finding 'EG.ddrout[i].in[0]', comment out the original
       XAie_DmaSetMultiDimAddr line, then add a new line with '(112*7*9*8 - 1)*4'.
    2. Handle 'EG.conv[i][j].out[0]' logic.
    """
    core_per_col = get_core_num_per_col()
    ddrout_pattern = re.compile(r'EG\.ddrout\[(\d+)\]\.in\[0\]')
    multidimaddr_pattern = re.compile(
        r'(XAie_DmaSetMultiDimAddr\s*\((.*?,){3})(\s*\d+)(\))'
    )
    conv_pattern = re.compile(r'EG\.conv\[(\d+)\]\[(\d+)\]\.out\[0\]')
    setpkt_pattern = re.compile(r'XAie_DmaSetPkt\s*\(')

    new_lines = []
    line_index = 0
    total_lines = len(lines)

    while line_index < total_lines:
        line = lines[line_index]
        ddr_match = ddrout_pattern.search(line)

        if ddr_match:
            i_val = int(ddr_match.group(1))
            new_lines.append(line)
            line_index += 1

            while line_index < total_lines:
                next_line = lines[line_index]
                multidimaddr_match = multidimaddr_pattern.search(next_line)
                if multidimaddr_match:
                    # Comment out original line
                    new_lines.append("// " + next_line.rstrip('\n') + "\n")
                    replaced_line = re.sub(
                        #r'(XAie_DmaSetMultiDimAddr\s*\((.*?,){3})(\s*\d+)(\))',
                        #r'\1 (112*7*9*8 - 1)*4\4',
                        r'(XAie_DmaSetMultiDimAddr\s*\((.*?,){3})(\s*\d+)(\))',
                        fr'\1 (112*7*9*{core_per_col} - 1)*4\4',
                        next_line
                    )
                    new_lines.append(replaced_line)
                    line_index += 1
                    break

                new_lines.append(next_line)
                line_index += 1
            continue

        conv_match = conv_pattern.search(line)
        if conv_match:
            i_val = int(conv_match.group(1))
            new_lines.append(line)
            line_index += 1

            while line_index < total_lines:
                next_line = lines[line_index]
                pkt_match = setpkt_pattern.search(next_line)
                new_lines.append(next_line)

                if pkt_match:
                    if i_val in ddr_map:
                        number_val = ddr_map[i_val]
                        out_of_order_line = f"\tXAie_DmaSetOutofOrderBdId(&DmaInst, {number_val});\n"
                        new_lines.append(out_of_order_line)
                    line_index += 1
                    break

                line_index += 1
            continue

        new_lines.append(line)
        line_index += 1

    return new_lines

def main():
    
    if len(sys.argv) < 3:
        print("Usage: patch_file.py <input_file> <output_file>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    with open(input_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    ddr_map = parse_ddr_map(lines)
    print("DMA port mapping:", ddr_map)

    new_lines = patch_lines(lines, ddr_map)

    with open(output_file, 'w', encoding='utf-8') as f:
        f.writelines(new_lines)

    print(f"Patched file saved to {output_file}")

if __name__ == "__main__":
    main()

def test():
    sample_lines = [
        "driverStatus |= XAie_DmaSetMultiDimAddr(&DmaInst, &aie2DmaTensor, 0, 86016);\n",
        "EG.ddrout[0].in[0]\n",
        "EG.conv[0][0].out[0]\n",
        "XAie_DmaSetPkt(&DmaInst);\n"
    ]
    result = patch_lines(sample_lines, {})
    print("\n".join(result))
