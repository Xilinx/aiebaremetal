#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Generate API_COVERAGE.md from an lcov coverage_report.txt file.
#
# Usage:
#     generate_coverage_md.sh <input_coverage_report.txt> <output_API_COVERAGE.md>
#
# Mirrors the previous generate_coverage_md.py: parses the lcov text report,
# extracts the per-file Lines/Functions rates, and writes a markdown table
# with a summary block plus a detailed table.  Uses only POSIX shell + awk
# (no Python dependency) so it can ship as part of the build deliverable.

set -eu

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_coverage_report.txt> <output_API_COVERAGE.md>" >&2
    exit 1
fi

INPUT="$1"
OUTPUT="$2"

if [ ! -f "$INPUT" ]; then
    echo "Error: input file '$INPUT' not found" >&2
    exit 1
fi

awk '
# Extract the first "NN.N%" and the integer that follows from a column.
# Returns "rate|num" or empty string if no match.
function extract_rate_num(col,    r, n, tmp) {
    if (match(col, /[0-9]+\.?[0-9]*%/) == 0) return ""
    r = substr(col, RSTART, RLENGTH - 1)   # drop trailing %
    tmp = substr(col, RSTART + RLENGTH)    # rest of the column after the %
    if (match(tmp, /[0-9]+/) == 0) return ""
    n = substr(tmp, RSTART, RLENGTH)
    return r "|" n
}

function trim(s) {
    sub(/^[ \t]+/, "", s)
    sub(/[ \t]+$/, "", s)
    return s
}

BEGIN {
    n_files = 0
    summary_lines_rate = "N/A"
    summary_lines_num  = "N/A"
    summary_func_rate  = "N/A"
    summary_func_num   = "N/A"
    summary_branch_rate = "N/A"
    summary_branch_num  = "N/A"
}

# Skip header / separator / section header lines.
/^Filename/   { next }
/^=+/         { next }
/^\[/         { next }
/^Reading/    { next }
/^$/          { next }

# Summary line: "                Total:|94.2%  3462|91.6% 321|    -    0"
/Total:/ {
    n = split($0, parts, "|")
    if (n >= 3) {
        ln = extract_rate_num(parts[2])
        if (ln != "") {
            split(ln, a, "|")
            summary_lines_rate = a[1]
            summary_lines_num  = a[2]
        }
        fn = extract_rate_num(parts[3])
        if (fn != "") {
            split(fn, b, "|")
            summary_func_rate = b[1]
            summary_func_num  = b[2]
        }
        if (n >= 4) {
            br = extract_rate_num(parts[4])
            if (br != "") {
                split(br, c, "|")
                summary_branch_rate = c[1]
                summary_branch_num  = c[2]
            }
        }
    }
    next
}

# Per-file lines:  filename | Rate Num | Rate Num | Rate Num
/\|/ {
    n = split($0, parts, "|")
    if (n < 3) next
    fname = trim(parts[1])
    if (fname == "") next
    ln = extract_rate_num(parts[2])
    fn = extract_rate_num(parts[3])
    if (ln == "" || fn == "") next
    split(ln, a, "|")
    split(fn, b, "|")
    n_files++
    file_name[n_files]     = fname
    file_lines_rate[n_files] = a[1]
    file_lines_num[n_files]  = a[2]
    file_func_rate[n_files]  = b[1]
    file_func_num[n_files]   = b[2]
    file_branch_rate[n_files] = "-"
    file_branch_num[n_files]  = "-"
    if (n >= 4) {
        br = extract_rate_num(parts[4])
        if (br != "") {
            split(br, c, "|")
            file_branch_rate[n_files] = c[1]
            file_branch_num[n_files]  = c[2]
        }
    }
}

END {
    print "# API Coverage Report"
    print ""
    print "## Summary"
    print ""
    print "| Metric | Rate | Total |"
    print "|--------|------|-------|"
    print "| **Lines** | " summary_lines_rate "% | " summary_lines_num " |"
    print "| **Functions** | " summary_func_rate "% | " summary_func_num " |"
    print "| **Branches** | " summary_branch_rate "% | " summary_branch_num " |"
    print ""
    print "### GMIO / external buffer (utests_gmioapiimpl only)"
    print ""
    print "Open `build/coverage_report_gmio_headers/index.html` for branch detail on these headers."
    print "Merged `coverage_report/` can show the same line % but lower branch % on other TUs."
    print ""
    print "---"
    print ""
    print "## Detailed Coverage by File"
    print ""
    print "| Filename | Lines Rate | Lines Num | Functions Rate | Functions Num | Branches Rate | Branches Num |"
    print "|----------|------------|-----------|----------------|---------------|---------------|--------------|"
    for (i = 1; i <= n_files; i++) {
        print "| **" file_name[i] "** | " \
              file_lines_rate[i] "% | " \
              file_lines_num[i] " | " \
              file_func_rate[i] "% | " \
              file_func_num[i] " | " \
              file_branch_rate[i] "% | " \
              file_branch_num[i] " |"
    }
    print ""
    print "---"
    print ""
    print "*Generated from lcov coverage analysis*"
}
' "$INPUT" > "$OUTPUT"

echo "Successfully generated $OUTPUT"
