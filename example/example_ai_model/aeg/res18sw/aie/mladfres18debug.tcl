#!/usr/bin/env tclsh
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# This script reads the core status for cores defined by:
#   - Columns 0 to 6
#   - Rows 3 to 6
#
# The register address for each core is computed as:
#   0x20000000000 + (col << 25) + (row << 20) + 0x00032004
#
# The register read is performed using the command:
#   mrd -force <address>
#
# Update 'mrd_path' to the full path of the mrd command if necessary.
package require json

# Set the full path to your mrd command here.
set mrd_path "/usr/local/bin/mrd"

# Function to read a register using the mrd command.
proc read_register {addr} {
    # Format the address as a hexadecimal string.
    set addr_hex [format "0x%X" $addr]
    # Execute the mrd command using the full path and capture the output.
    set output [mrd -force $addr_hex]
    # Trim any leading/trailing whitespace from the output.
    return [string trim $output]
}

# Loop over columns (0 to 6) and rows (3 to 6)
for {set col 0} {$col <= 6} {incr col} {
    for {set row 3} {$row <= 6} {incr row} {
        # Compute the register address using the given formula.
        set addr [expr {0x20000000000 + ($col << 25) + ($row << 20) + 0x00032004}]
        # Read the status using the mrd command.
        set status [read_register $addr]
        # Output the column, row, and the status value.
        puts "Col: $col, Row: $row, Status: $status"
    }
}
