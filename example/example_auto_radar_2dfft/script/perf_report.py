#!/usr/bin/env python3
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
"""Turn radar 2D-FFT UART logs into a performance report.

Build the application with `-perf` so the host emits [PERF] lines, capture the
UART output to a file, then run this script on one or two logs:

    python3 script/perf_report.py new_api.log
    python3 script/perf_report.py legacy.log new_api.log
    python3 script/perf_report.py --csv report.csv legacy.log new_api.log

With two logs the report includes a before-vs-after delta column.
"""

import argparse
import csv
import re
import sys
from pathlib import Path

# Workload constants, mirroring the transfer sizes in graph_new_api.cpp.
# Each of the 4 instances moves 64*4*8*24*2048 bytes in and the same out.
INSTANCES = 4
BYTES_IN_PER_INSTANCE = 64 * 4 * 8 * 24 * 2048
BYTES_OUT_PER_INSTANCE = 2048 * 64 * 4 * 8 * 24
SAMPLES_PER_INSTANCE = 2048 * 64 * 8 * 24

TOTAL_BYTES = INSTANCES * (BYTES_IN_PER_INSTANCE + BYTES_OUT_PER_INSTANCE)
TOTAL_SAMPLES = INSTANCES * SAMPLES_PER_INSTANCE

# Phase that represents actual AIE execution plus data movement.
THROUGHPUT_PHASE = "run_output_wait"

# Preferred display order; any unrecognised phase is appended in log order.
PHASE_ORDER = [
    "init",
    "dma_config",
    "malloc",
    "preprocess",
    "gm2aie",
    "run_output_wait",
    "golden_check",
    "total",
]

HEADER_RE = re.compile(r"\[PERF\]\s+api=(\S+)\s+iters=(\d+)")
PHASE_RE = re.compile(r"\[PERF\]\s+([A-Za-z_][A-Za-z0-9_]*)\s*:\s*([0-9]+\.?[0-9]*)\s*ms")
PASS_RE = re.compile(r"Golden Check Passed")
FAIL_RE = re.compile(r"Golden Check Failed")


class Run:
    def __init__(self, path):
        self.path = Path(path)
        self.api = None
        self.iters = None
        self.phases = {}
        self.golden = "not reported"

    @property
    def label(self):
        return self.api or self.path.stem

    def throughput_gbps(self):
        ms = self.phases.get(THROUGHPUT_PHASE)
        if not ms:
            return None
        return TOTAL_BYTES / (ms / 1000.0) / 1e9

    def msps(self):
        ms = self.phases.get(THROUGHPUT_PHASE)
        if not ms:
            return None
        return TOTAL_SAMPLES / (ms / 1000.0) / 1e6


def parse_log(path):
    run = Run(path)
    try:
        text = Path(path).read_text(errors="replace")
    except OSError as exc:
        sys.exit(f"error: cannot read {path}: {exc}")

    for line in text.splitlines():
        header = HEADER_RE.search(line)
        if header:
            run.api, run.iters = header.group(1), int(header.group(2))
            continue
        phase = PHASE_RE.search(line)
        if phase:
            run.phases[phase.group(1)] = float(phase.group(2))
            continue
        if PASS_RE.search(line):
            run.golden = "PASSED"
        elif FAIL_RE.search(line):
            run.golden = "FAILED"

    if not run.phases:
        sys.exit(
            f"error: no [PERF] lines in {path}. "
            "Was the image built with 'source build.sh 2 -new_api -perf'?"
        )
    return run


def ordered_phases(runs):
    seen = []
    for name in PHASE_ORDER:
        if any(name in r.phases for r in runs):
            seen.append(name)
    for run in runs:
        for name in run.phases:
            if name not in seen:
                seen.append(name)
    return seen


def render(runs):
    phases = ordered_phases(runs)
    lines = []

    lines.append("# Radar 2D-FFT performance report")
    lines.append("")
    for run in runs:
        lines.append(
            f"- `{run.label}` from `{run.path.name}` "
            f"(iterations: {run.iters if run.iters is not None else 'unknown'}, "
            f"golden check: {run.golden})"
        )
    lines.append("")

    if len(runs) == 2:
        a, b = runs
        lines.append(f"| Phase | {a.label} (ms) | {b.label} (ms) | Delta (ms) |")
        lines.append("|---|---:|---:|---:|")
        for name in phases:
            va, vb = a.phases.get(name), b.phases.get(name)
            if va is None or vb is None:
                lines.append(
                    f"| `{name}` | {fmt(va)} | {fmt(vb)} | n/a |"
                )
            else:
                lines.append(
                    f"| `{name}` | {va:.2f} | {vb:.2f} | {vb - va:+.2f} |"
                )
    else:
        run = runs[0]
        lines.append(f"| Phase | {run.label} (ms) |")
        lines.append("|---|---:|")
        for name in phases:
            lines.append(f"| `{name}` | {fmt(run.phases.get(name))} |")

    lines.append("")
    lines.append(f"## Throughput over `{THROUGHPUT_PHASE}`")
    lines.append("")
    lines.append(
        f"Moving {TOTAL_BYTES / 1e9:.3f} GB "
        f"({TOTAL_SAMPLES / 1e6:.1f} M samples) across {INSTANCES} instances, "
        "counting input and output together."
    )
    lines.append("")
    lines.append("| Run | Throughput (GB/s) | Rate (MSPS) |")
    lines.append("|---|---:|---:|")
    for run in runs:
        gbps, msps = run.throughput_gbps(), run.msps()
        lines.append(
            f"| {run.label} | "
            f"{'n/a' if gbps is None else f'{gbps:.2f}'} | "
            f"{'n/a' if msps is None else f'{msps:.1f}'} |"
        )
    lines.append("")
    return "\n".join(lines)


def fmt(value):
    return "n/a" if value is None else f"{value:.2f}"


def write_csv(path, runs):
    phases = ordered_phases(runs)
    with open(path, "w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["phase"] + [r.label for r in runs])
        for name in phases:
            writer.writerow([name] + [fmt(r.phases.get(name)) for r in runs])
        writer.writerow(
            ["throughput_gbps"]
            + [fmt(r.throughput_gbps()) for r in runs]
        )
        writer.writerow(["rate_msps"] + [fmt(r.msps()) for r in runs])


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("logs", nargs="+", metavar="LOG",
                        help="UART log(s); pass two to get a before-vs-after delta")
    parser.add_argument("--csv", metavar="FILE", help="also write the table as CSV")
    parser.add_argument("-o", "--output", metavar="FILE",
                        help="write the markdown report to FILE instead of stdout")
    args = parser.parse_args()

    if len(args.logs) > 2:
        sys.exit("error: pass at most two logs (baseline and new)")

    runs = [parse_log(path) for path in args.logs]
    report = render(runs)

    if args.output:
        Path(args.output).write_text(report + "\n")
        print(f"Wrote {args.output}")
    else:
        print(report)

    if args.csv:
        write_csv(args.csv, runs)
        print(f"Wrote {args.csv}")


if __name__ == "__main__":
    main()
