#!/usr/bin/env bash
# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
# Strip specific line ranges from an lcov .info coverage tracefile.
#
# Usage:
#     exclude_lines_from_coverage.sh <coverage.info> <basename:start-end> [...]
#
# Each exclusion is "<source-file-basename>:<start>-<end>" (inclusive).
# Example:
#     exclude_lines_from_coverage.sh coverage.info.cleaned \
#         aeg_aiebaremetal.cc:2002-2088
#
# Removes DA:, BRDA:, FN:, FNDA: records for matching lines in records whose
# SF: path endswith the given basename, then recomputes the LF/LH/BRF/BRH/
# FNF/FNH summary counters.  Rewrites the file in-place.
#
# Why post-process instead of LCOV_EXCL_START markers?
#   Modifying production (UUT) source just to satisfy coverage tooling is
#   undesirable.  This script keeps the exclusion configuration in the build.

set -eu

if [ "$#" -lt 2 ]; then
    sed -n '2,/^$/p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
fi

INFO_FILE="$1"
shift

if [ ! -f "$INFO_FILE" ]; then
    echo "exclude_lines_from_coverage.sh: ERROR: '$INFO_FILE' not found" >&2
    exit 1
fi

# Build a pipe-separated list of "basename:start:end" for awk consumption.
RANGES=""
for spec in "$@"; do
    case "$spec" in
        *:*-*) ;;
        *) echo "Bad exclusion spec '$spec'. Expected 'name:start-end'." >&2 ; exit 1 ;;
    esac
    name="${spec%%:*}"
    rng="${spec#*:}"
    start="${rng%%-*}"
    end="${rng##*-}"
    if [ "$end" -lt "$start" ]; then
        echo "Bad range in '$spec': end < start" >&2
        exit 1
    fi
    if [ -z "$RANGES" ]; then
        RANGES="${name}:${start}:${end}"
    else
        RANGES="${RANGES}|${name}:${start}:${end}"
    fi
done

TMP_FILE="${INFO_FILE}.tmp.$$"

awk -v RANGES="$RANGES" '
function in_excluded(line_no, basename,    n, i, parts, sub_parts, n_sub) {
    n = split(RANGES, parts, "|")
    for (i = 1; i <= n; i++) {
        n_sub = split(parts[i], sub_parts, ":")
        if (n_sub != 3) continue
        if (basename ~ sub_parts[1] "$") {
            if (line_no + 0 >= sub_parts[2] + 0 && line_no + 0 <= sub_parts[3] + 0)
                return 1
        }
    }
    return 0
}

# Reset record-local state at the start of each SF: block.
function reset_record() {
    rec_idx = 0
    delete rec
    delete removed_fn_names
    cur_sf = ""
}

BEGIN { reset_record() }

/^SF:/ {
    cur_sf = substr($0, 4)
    rec[++rec_idx] = $0
    next
}

/^DA:/ {
    payload = substr($0, 4)
    ln = payload + 0
    if (cur_sf != "" && in_excluded(ln, cur_sf)) next
    rec[++rec_idx] = $0
    next
}

/^BRDA:/ {
    payload = substr($0, 6)
    ln = payload + 0
    if (cur_sf != "" && in_excluded(ln, cur_sf)) next
    rec[++rec_idx] = $0
    next
}

/^FN:/ {
    payload = substr($0, 4)
    n = split(payload, parts, ",")
    ln = parts[1] + 0
    if (cur_sf != "" && in_excluded(ln, cur_sf) && n >= 2) {
        removed_fn_names[parts[2]] = 1
        next
    }
    rec[++rec_idx] = $0
    next
}

/^FNDA:/ {
    payload = substr($0, 6)
    n = split(payload, parts, ",")
    if (n >= 2 && (parts[2] in removed_fn_names)) next
    rec[++rec_idx] = $0
    next
}

/^end_of_record/ {
    # Recompute the LF/LH/BRF/BRH/FNF/FNH counters from the remaining lines.
    lf = 0; lh = 0; brf = 0; brh = 0; fnf = 0; fnh = 0
    for (i = 1; i <= rec_idx; i++) {
        line = rec[i]
        if (line ~ /^DA:/) {
            lf++
            p = substr(line, 4)
            split(p, q, ",")
            if (q[2] + 0 > 0) lh++
        } else if (line ~ /^BRDA:/) {
            brf++
            p = substr(line, 6)
            n2 = split(p, q, ",")
            if (n2 == 4 && q[4] != "-" && q[4] + 0 > 0) brh++
        } else if (line ~ /^FN:/) {
            fnf++
        } else if (line ~ /^FNDA:/) {
            p = substr(line, 6)
            split(p, q, ",")
            if (q[1] + 0 > 0) fnh++
        }
    }
    # Flush the record, overwriting any existing summary counters.
    for (i = 1; i <= rec_idx; i++) {
        line = rec[i]
        if (line ~ /^LF:/) print "LF:" lf
        else if (line ~ /^LH:/) print "LH:" lh
        else if (line ~ /^BRF:/) print "BRF:" brf
        else if (line ~ /^BRH:/) print "BRH:" brh
        else if (line ~ /^FNF:/) print "FNF:" fnf
        else if (line ~ /^FNH:/) print "FNH:" fnh
        else print line
    }
    print "end_of_record"
    reset_record()
    next
}

{ rec[++rec_idx] = $0 }
' "$INFO_FILE" > "$TMP_FILE"

mv "$TMP_FILE" "$INFO_FILE"
echo "exclude_lines_from_coverage.sh: filtered $(basename "$INFO_FILE") ($*)"
