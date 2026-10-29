#!/usr/bin/env python3
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0
"""Measure VEK280 rail power for an aiebaremetal application, via the system
controller. See README.md in this directory for the full walkthrough.

The board's 20 instrumented rails are read by INA226 sensors on the ZU4 system
controller, and systest exposes the SC's sc_app utility through its
`sysctl_sendcmd` verb -- so power can be sampled from a dev host with no
physical board access:

    systest <board>  ->  sysctl_sendcmd "sc_app -c getpower -t VCCINT"

Each sc_app round trip costs on the order of a second, so the app under test has
to hold a steady load for minutes; see "Making your app measurable" in README.md.

Typical use:

    # idle baseline, board powered but unconfigured
    ./measure_power.py --label idle --snapshots 5

    # configured idle, then a sustained run sampled under load
    ./measure_power.py --label myapp --program ~/images/myapp.BIN \
                       --console --wait-for 'b - benchmark' --temps \
                       --idle-snapshots 3 --send b --settle 20 --snapshots 20 \
                       --wait-summary 260 --csv myapp.csv

Throughput is scraped from the app's own console output; --rate-regex adapts that
to whatever your app prints. Requires pexpect and /proj/systest/bin/systest.

When the board is already yours -- a booted PetaLinux you do not want to lose,
for instance -- taking it a second time is not possible, so sample from the
session you have:

    # print the commands, paste them into your systest shell, save the output
    ./measure_power.py --temps --emit-sweeps 3          # before starting the load
    ./measure_power.py --temps --emit-sweeps 8          # under load

    ./measure_power.py --parse session.txt --label pl_resnet18 \
                       --idle-sweeps 3 --rate 365 --csv pl_resnet18.csv

--parse and --emit-sweeps need neither pexpect nor a board.
"""

import argparse
import collections
import csv
import math
import os
import re
import statistics
import subprocess
import sys
import threading
import time

try:
    import pexpect
except ImportError:
    # Usually not a missing package but a shadowed interpreter: sourcing the
    # Vitis settings or the Zephyr .venv puts a python3 without pexpect ahead of
    # the system one on PATH, and the shebang follows PATH. Only the session
    # modes need it, so defer the complaint: --emit-sweeps and --parse work
    # without it.
    pexpect = None


def require_pexpect():
    if pexpect is None:
        sys.exit(f"ERROR: no pexpect for {sys.executable}\n"
                 f"       try:  /usr/bin/python3 {sys.argv[0]} ...\n"
                 f"       or:   {sys.executable} -m pip install --user pexpect")

# Rails that carry the ResNet18 work: AIE-ML array and PL (VCCINT), NoC/SoC,
# A72 (VCC_PSFP), PMC, block RAM, aux supplies, and LPDDR4. The remaining
# rails on this board (MGT/SFP/FMC) are idle for this demo; use --rails all to
# include them.
DEFAULT_RAILS = [
    "VCCINT",
    "VCC_SOC",
    "VCC_PSFP",
    "VCC_PSLP_CPM5",
    "VCC_PMC",
    "VCC_RAM_VCCINT_GT",
    "VCCAUX",
    "VCCAUX_PMC",
    "VCC1V1_LP4",
]

SYSTEST_BIN = "/proj/systest/bin/systest"
CLUSTER_PING = "/proj/systest/bin/cluster-ping"

# Above this per-rail standard deviation, single readings are sampling a load
# that switches faster than sc_app can read it, and the mean needs an error bar.
SPREAD_NOTE_W = 0.5

# How to read the app's own throughput off the console. The default matches the
# resnet18 demo ("throughput        : 515.3 img/s"); --rate-regex covers apps
# that print something else. Whatever it captures is treated as work per second,
# which is what turns watts into energy per unit of work.
RATE_REGEX = r'throughput\s*:\s*([\d.]+)\s*img/s'


def clean_env():
    """os.environ without the dynamic-loader overrides.

    Sourcing the Vitis settings puts its own libcrypto.so.3 (from
    Vitis/<ver>/lib/lnx64.o) on LD_LIBRARY_PATH, and /bin/ssh then dies with
    "OpenSSL version mismatch" and EOF before any prompt appears. Since the build
    shell is usually also the measuring shell, drop the overrides for the helper
    tools rather than requiring a pristine shell.
    """
    env = dict(os.environ)
    for var in ("LD_LIBRARY_PATH", "LD_PRELOAD"):
        env.pop(var, None)
    return env


def farm_host(board, attempts=3):
    """Map a board to the host its console lives on (vek280-4 -> muscaria18).

    Retried: cluster-ping goes through the board-farm web API, which intermittently
    fails, and losing the console silently turns a load measurement into an idle one.
    """
    board_type = board.rsplit("-", 1)[0]
    for attempt in range(attempts):
        try:
            out = subprocess.run([CLUSTER_PING, board_type], capture_output=True,
                                 text=True, timeout=120, env=clean_env()).stdout
        except subprocess.SubprocessError:
            out = ""
        for line in out.splitlines():
            parts = line.strip().lstrip("\r").split()
            if len(parts) >= 2 and parts[0] == board:
                return parts[1]
        if attempt + 1 < attempts:
            print(f"[console] cluster-ping gave no host for {board}, retrying",
                  flush=True)
            time.sleep(10)
    return None


class ConsoleChannel:
    """com0 console through the farm host, drained by a background thread.

    ssh to the farm host is only permitted while an LSF job holds the board, so
    this must be opened *after* the systest session reaches its prompt -- the
    session is the credential, no ssh key setup is involved.
    """

    def __init__(self, host, board, path, timeout=180):
        self.path = path
        self.prompt = r'\[' + re.escape(board) + r'\]\s*Systest[#>]'
        self.fh = open(path, "w")
        self.child = pexpect.spawn(
            f"ssh -o StrictHostKeyChecking=no -o ServerAliveInterval=20 {host}",
            encoding="utf-8", timeout=timeout, env=clean_env())
        # Farm hosts hand out a tcsh '%' prompt; accept the usual shells too.
        try:
            self.child.expect(r'[$#>%]\s*$', timeout=60)
        except pexpect.EOF:
            raise RuntimeError(
                f"ssh to {host} closed before a prompt appeared:\n"
                f"  {(self.child.before or '').strip()}") from None
        self.child.sendline("/opt/systest/common/bin/systest-client")
        self.child.expect(self.prompt, timeout=timeout)
        self.child.sendline("connect com0")
        self.child.expect(r'Connecting to device com0.*escape', timeout=timeout)
        self.t0 = time.time()
        self.stop = threading.Event()
        self.thread = threading.Thread(target=self._drain, daemon=True)
        self.thread.start()

    def _drain(self):
        """Timestamp each console line, so output can be correlated with what
        the measurement was doing (programming, leaving xsdb, sampling)."""
        buf = ""
        while not self.stop.is_set():
            try:
                buf += self.child.read_nonblocking(size=4096, timeout=1)
            except pexpect.TIMEOUT:
                continue
            except Exception:
                break
            *complete, buf = re.split(r'\r?\n', buf)
            for line in complete:
                line = line.strip()
                if line:
                    self.fh.write(f"[{time.time() - self.t0:8.1f}] {line}\n")
            self.fh.flush()

    def send(self, text):
        """Type into com0 (e.g. 'b' to start the benchmark from the menu)."""
        self.mark(f"sending {text!r}")
        self.child.sendline(text)

    def mark(self, text):
        self.fh.write(f"[{time.time() - self.t0:8.1f}] --- {text} ---\n")
        self.fh.flush()

    def lines(self):
        self.fh.flush()
        if not os.path.exists(self.path):
            return []
        with open(self.path, errors="replace") as fh:
            return [l.rstrip() for l in fh if l.strip()]

    def wait_for(self, pattern, timeout=300):
        """Block until the console shows pattern; returns False on timeout."""
        deadline = time.time() + timeout
        rx = re.compile(pattern)
        while time.time() < deadline:
            if any(rx.search(l) for l in self.lines()):
                return True
            time.sleep(1)
        return False

    def bench_summary(self, rate_regex=RATE_REGEX):
        """Pull the app's own rate (and, if present, its per-round detail) out of
        the console. Only the rate is required; the rest is resnet18's summary
        format and is simply absent for other apps."""
        text = "\n".join(self.lines())
        out = {}
        for key, pat in (("rate", rate_regex),
                         ("us_per_round", r'per round\s+mean\s*:\s*([\d.]+)\s*us'),
                         ("us_per_image", r'per image \(avg\)\s*:\s*([\d.]+)\s*us'),
                         ("gops", r'compute \(INT8\)\s*:\s*([\d.]+)\s*GOPS')):
            m = re.search(pat, text)
            if m:
                out[key] = float(m.group(1))
        return out

    def close(self):
        self.stop.set()
        self.thread.join(timeout=5)
        self.fh.flush()
        try:
            self.child.close(force=True)
        except Exception:
            pass


class BoardSession:
    """A systest session on one board, with sc_app reachable via sysctl_sendcmd."""

    def __init__(self, board, logpath, dispatch_timeout=300):
        self.board = board
        self.prompt = r'\[' + re.escape(board) + r'\]\s*Systest[#>]'
        self.log = open(logpath, "w")
        self.child = pexpect.spawn(f"{SYSTEST_BIN} {board}", encoding="utf-8",
                                   timeout=dispatch_timeout)
        self.child.logfile_read = self.log
        # LSF dispatch + power-on + SC boot all happen before the first prompt.
        self.child.expect(self.prompt, timeout=dispatch_timeout)

    def systest(self, cmd, timeout=180):
        self.child.sendline(cmd)
        self.child.expect(self.prompt, timeout=timeout)
        return self.child.before or ""

    def sc_app(self, args, timeout=120):
        return self.systest(f'sysctl_sendcmd "sc_app {args}"', timeout=timeout)

    def list_rails(self):
        out = self.sc_app("-c listpower")
        return [l.strip() for l in out.splitlines()
                if re.fullmatch(r'[A-Z0-9_]+', l.strip())]

    def get_power(self, rail):
        """Return (volts, amps, watts) for one rail, or None if sc_app failed."""
        out = self.sc_app(f"-c getpower -t {rail}")
        vals = {}
        for key, pat in (("v", r'Voltage\(V\):\s*([\d.\-]+)'),
                         ("a", r'Current\(A\):\s*([\d.\-]+)'),
                         ("w", r'Power\(W\):\s*([\d.\-]+)')):
            m = re.search(pat, out)
            if m:
                vals[key] = float(m.group(1))
        if len(vals) != 3:
            return None
        return vals["v"], vals["a"], vals["w"]

    def get_temp(self, target="Versal"):
        """Device temperature in C, or None. Worth recording with every snapshot:
        VCCINT is largely leakage at this array size, and leakage climbs with
        temperature, so two runs of the same design read differently depending on
        what ran before them (`sc_app -c listtemp` offers just this one target)."""
        out = self.sc_app(f"-c gettemp -t {target}")
        m = re.search(r'([-+]?\d+\.\d+)', out)
        return float(m.group(1)) if m else None

    def program(self, boot_bin, hold=0.0, console=None):
        """Load a BOOT.BIN, then leave xsdb so sysctl_sendcmd works again.

        The ResNet18 BOOT.BIN carries the A72 partition, so the app starts on
        its own; no dow/con is needed. Leaving xsdb only disconnects the debug
        session -- the processor keeps running, which the app's menu appearing
        on com0 afterwards confirms.
        """
        self.child.sendline("xsdb")
        self.child.expect(r'xsdb%', timeout=60)
        self.child.sendline("conn")
        self.child.expect(r'xsdb%', timeout=120)
        self.child.sendline("tar 1")
        self.child.expect(r'xsdb%', timeout=60)
        print(f"[program] device program {boot_bin}", flush=True)
        self.child.sendline(f"device program {boot_bin}")
        self.child.expect(r'xsdb%', timeout=600)
        out = self.child.before or ""
        if re.search(r'error|fail', out, re.I):
            print("[program] WARNING: xsdb reported a problem:", flush=True)
            print(out, flush=True)
        if console:
            console.mark("programmed, still attached in xsdb")
        if hold:
            print(f"[program] holding in xsdb for {hold:.0f} s", flush=True)
            time.sleep(hold)
        if console:
            console.mark("leaving xsdb")
        self.child.sendline("exit")
        self.child.expect(self.prompt, timeout=120)
        print("[program] done; back at systest prompt", flush=True)

    def close(self, power_off=True):
        try:
            if power_off:
                self.systest("power 0", timeout=180)
            self.child.sendline("exit")
            self.child.expect([pexpect.EOF, self.prompt], timeout=120)
        except Exception:
            pass
        finally:
            self.log.flush()
            try:
                self.child.close()
            except Exception:
                pass


def summarize(samples, rails, label):
    """Print per-rail mean/sd/min/max watts plus the total, and return the means.

    A load that bursts faster than sc_app can read leaves each sample at an
    arbitrary phase of the waveform, so the spread matters as much as the mean:
    the mean is only trustworthy to about sd/sqrt(n), reported below the table
    for any rail that swings by more than SPREAD_NOTE_W."""
    print(f"\n===== {label}: {len(samples)} snapshot(s) =====")
    print(f"{'rail':<20}{'V':>9}{'A':>10}{'W mean':>10}{'W sd':>8}{'W min':>9}{'W max':>9}")
    means, noisy = {}, []
    for rail in rails:
        watts = [s[rail][2] for s in samples if s.get(rail)]
        if not watts:
            print(f"{rail:<20}{'(no reading)':>47}")
            continue
        volts = statistics.mean(s[rail][0] for s in samples if s.get(rail))
        amps = statistics.mean(s[rail][1] for s in samples if s.get(rail))
        means[rail] = statistics.mean(watts)
        sd = statistics.stdev(watts) if len(watts) > 1 else 0.0
        if sd > SPREAD_NOTE_W:
            noisy.append((rail, sd / math.sqrt(len(watts))))
        print(f"{rail:<20}{volts:>9.4f}{amps:>10.4f}"
              f"{means[rail]:>10.4f}{sd:>8.4f}{min(watts):>9.4f}{max(watts):>9.4f}")
    print(f"{'TOTAL (listed)':<20}{'':>19}{sum(means.values()):>10.4f}")
    for rail, se in noisy:
        print(f"  {rail} mean is +/- {se:.2f} W (1 sigma) at n={len(samples)}")
    return means


def emit_sweeps(rails, count, temps, oneline=False, pack=False):
    """Print sc_app commands to paste into a systest session you already hold.

    Nothing outside that session can reach sc_app: sysctl_sendcmd is a verb of
    the systest shell, not a program on PATH. When the board is already yours --
    holding a booted PetaLinux, say -- a second `systest <board>` would queue in
    LSF behind your own reservation, so sampling has to happen in the session
    that exists. Feed the output of this back through --parse.

    The systest prompt consumes only the first line of a multi-line paste, so
    --oneline packs a whole sweep into one command. The rail being read is then
    no longer visible in what the prompt echoes, hence the interleaved `echo`
    markers: they put the same text --parse keys on into the output stream.

    A sweep of every rail takes ~10 s, which aliases badly against a load that
    swings on a millisecond scale: single readings then land anywhere between
    the load's trough and its peak, and even one sc_app call can report a power
    register that disagrees with its own voltage times current. --pack instead
    puts all `count` readings of a short rail list into one command, sampling
    fast enough for the spread to describe the waveform rather than hide it."""
    if pack:
        parts = []
        for _ in range(count):
            for rail in rails:
                parts += [f"echo getpower -t {rail}", f"sc_app -c getpower -t {rail}"]
        if temps:
            parts += ["echo gettemp -t Versal", "sc_app -c gettemp -t Versal"]
        print(f'sysctl_sendcmd "{"; ".join(parts)}"')
        return
    for _ in range(count):
        if not oneline:
            for rail in rails:
                print(f'sysctl_sendcmd "sc_app -c getpower -t {rail}"')
            if temps:
                print('sysctl_sendcmd "sc_app -c gettemp -t Versal"')
            continue
        parts = []
        for rail in rails:
            parts += [f"echo getpower -t {rail}", f"sc_app -c getpower -t {rail}"]
        if temps:
            parts += ["echo gettemp -t Versal", "sc_app -c gettemp -t Versal"]
        print(f'sysctl_sendcmd "{"; ".join(parts)}"')


def parse_session_log(path, rails):
    """Recover sweeps from captured systest output.

    Each reading is identified by the echoed command rather than the values, so
    interleaved console noise is harmless. A rail repeating means a new sweep
    started, which is also how sweeps are counted when --temps was not used."""
    rail_re = re.compile(r'getpower\s+-t\s+([A-Za-z0-9_]+)')
    temp_re = re.compile(r'gettemp\s+-t\s+(\S+)')
    val_res = (("v", re.compile(r'Voltage\(V\):\s*([\d.\-]+)')),
               ("a", re.compile(r'Current\(A\):\s*([\d.\-]+)')),
               ("w", re.compile(r'Power\(W\):\s*([\d.\-]+)')))
    number_re = re.compile(r'([-+]?\d+\.\d+)')

    sweeps, temperatures = [], []
    cur, pending, rail, want_temp = {}, {}, None, False
    with open(path, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            m = rail_re.search(line)
            if m:
                rail, pending, want_temp = m.group(1), {}, False
                continue
            if temp_re.search(line):
                rail, want_temp = None, True
                continue
            if want_temp:
                m = number_re.search(line)
                if m:
                    temperatures.append((len(sweeps), float(m.group(1))))
                    want_temp = False
                continue
            if rail is None:
                continue
            for key, pat in val_res:
                m = pat.search(line)
                if m:
                    pending[key] = float(m.group(1))
            if len(pending) == 3:
                if rail in cur:
                    sweeps.append(cur)
                    cur = {}
                cur[rail] = (pending["v"], pending["a"], pending["w"])
                rail, pending = None, {}
    if cur:
        sweeps.append(cur)

    missing = [r for r in rails if not any(r in s for s in sweeps)]
    if missing:
        print(f"[parse] no readings for: {' '.join(missing)}", flush=True)
    return sweeps, temperatures


def write_csv(path, run_id, label, groups, temps=None):
    """`temps` maps (phase, snapshot index) to degrees C. Keep it populated: VCCINT
    on a large array is mostly leakage, so a reading is only comparable against
    another at the same temperature, and a summary range across runs cannot
    reconstruct which run was where."""
    temps = temps or {}
    fresh = not os.path.exists(path) or os.path.getsize(path) == 0
    with open(path, "a", newline="") as fh:
        w = csv.writer(fh)
        if fresh:
            w.writerow(["run", "label", "phase", "snapshot", "rail",
                        "volts", "amps", "watts", "temp_c"])
        for phase, group in groups:
            for n, snap in enumerate(group):
                temp = temps.get((phase, n), "")
                for rail, (v, a, watt) in snap.items():
                    w.writerow([run_id, label, phase, n, rail, v, a, watt, temp])
    print(f"[csv] appended to {path}", flush=True)


def run_parse(args, rails):
    sweeps, temperatures = parse_session_log(args.parse, rails)
    if not sweeps:
        print(f"ERROR: no sc_app readings found in {args.parse}", file=sys.stderr)
        return 1
    idle, load = sweeps[:args.idle_sweeps], sweeps[args.idle_sweeps:]
    if not load:
        print(f"ERROR: --idle-sweeps {args.idle_sweeps} consumed all "
              f"{len(sweeps)} sweep(s)", file=sys.stderr)
        return 1
    print(f"[parse] {len(sweeps)} sweep(s) from {args.parse}: "
          f"{len(idle)} idle, {len(load)} load", flush=True)

    idle_means = summarize(idle, rails, f"{args.label} (idle)") if idle else None
    means = summarize(load, rails, f"{args.label} (load)")
    total_w = sum(means.values())
    if idle_means:
        print(f"\n{'load - idle delta':<20}{'':>19}"
              f"{total_w - sum(idle_means.values()):>10.4f}")
    for tag, lo, hi in (("idle", 0, len(idle)), ("load", len(idle), len(sweeps))):
        vals = [t for i, t in temperatures if lo <= i < hi]
        if vals:
            print(f"{'temperature (' + tag + ')':<20}{'':>19}"
                  f"{statistics.mean(vals):>10.1f} C"
                  f"  (min {min(vals):.1f}, max {max(vals):.1f})")
    if args.rate:
        print(f"\nrate (given)          : {args.rate:.1f} /s")
        print(f"energy per unit       : {1000.0 * total_w / args.rate:.2f} mJ "
              f"(listed rails, total power)")
        if idle_means:
            delta = total_w - sum(idle_means.values())
            print(f"  above idle          : {1000.0 * delta / args.rate:.2f} mJ")
    if args.csv:
        per_snapshot = {}
        for i, t in temperatures:
            if i < len(idle):
                per_snapshot[("idle", i)] = t
            else:
                per_snapshot[("load", i - len(idle))] = t
        write_csv(args.csv, time.strftime("%Y%m%d-%H%M%S"), args.label,
                  (("idle", idle), ("load", load)), per_snapshot)
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--board", default="vek280-4")
    ap.add_argument("--label", default="run", help="name for the summary and CSV rows")
    ap.add_argument("--program", metavar="BOOT.BIN",
                    help="program this image before sampling (omit for an idle baseline)")
    ap.add_argument("--rails", default=",".join(DEFAULT_RAILS),
                    help="comma-separated rail list, or 'all'")
    ap.add_argument("--snapshots", type=int, default=5)
    ap.add_argument("--interval", type=float, default=0.0,
                    help="extra seconds between snapshots (sc_app round trips already pace this)")
    ap.add_argument("--settle", type=float, default=0.0,
                    help="seconds to wait after programming, for the load to reach steady state")
    ap.add_argument("--xsdb-hold", type=float, default=0.0,
                    help="seconds to stay attached in xsdb after programming (diagnostic: shows "
                         "whether leaving xsdb disturbs the running app)")
    ap.add_argument("--csv", help="append per-snapshot readings here")
    ap.add_argument("--console", action="store_true",
                    help="also capture com0, to confirm the load is running and read its rate")
    ap.add_argument("--farm-host", help="console host (default: looked up with cluster-ping)")
    ap.add_argument("--send", action="append", metavar="KEY",
                    help="type KEY into com0 after settling (menu-driven images); repeatable")
    ap.add_argument("--send-gap", type=float, default=3.0,
                    help="seconds between --send keys")
    ap.add_argument("--idle-snapshots", type=int, default=0,
                    help="snapshots to take before --send, as a configured-but-idle baseline")
    ap.add_argument("--wait-for", metavar="REGEX",
                    help="wait for this console pattern before sampling/sending (e.g. the menu)")
    ap.add_argument("--wait-timeout", type=float, default=300.0)
    ap.add_argument("--wait-summary", type=float, default=0.0,
                    help="after load sampling, wait up to this many seconds for the "
                         "app to print its rate (needed when the run outlasts sampling)")
    ap.add_argument("--rate-regex", default=RATE_REGEX, metavar="REGEX",
                    help="one capture group giving the app's work/second, used for "
                         f"energy per unit of work (default: {RATE_REGEX})")
    ap.add_argument("--temps", action="store_true",
                    help="also read device temperature with each snapshot (adds "
                         "~0.6 s per snapshot; VCCINT leakage tracks temperature)")
    ap.add_argument("--keep-power", action="store_true",
                    help="leave the board powered at exit (default powers it off)")
    ap.add_argument("--emit-sweeps", type=int, metavar="N",
                    help="print N sweeps of sc_app commands to paste into a "
                         "systest session you already hold, instead of taking "
                         "the board; capture the output and feed it to --parse")
    ap.add_argument("--oneline", action="store_true",
                    help="with --emit-sweeps, one command per sweep instead of "
                         "one per rail (the systest prompt takes only the first "
                         "line of a paste)")
    ap.add_argument("--pack", action="store_true",
                    help="with --emit-sweeps, put every sweep in one command, to "
                         "sample a bursty load faster than one sweep per ~10 s "
                         "(use with a short --rails list)")
    ap.add_argument("--parse", metavar="FILE",
                    help="summarize captured systest output instead of sampling")
    ap.add_argument("--idle-sweeps", type=int, default=0,
                    help="with --parse, treat the first N sweeps as the idle baseline")
    ap.add_argument("--rate", type=float,
                    help="with --parse, work per second, for energy per unit")
    args = ap.parse_args()

    rails = DEFAULT_RAILS if args.rails == "all" else args.rails.split(",")
    if args.rails == "all" and (args.emit_sweeps or args.parse):
        # 'all' is resolved with `sc_app -c listpower`, which needs a session.
        print("[rails] 'all' needs a session; using the default list", flush=True)
    if args.emit_sweeps:
        emit_sweeps(rails, args.emit_sweeps, args.temps, args.oneline or args.pack, args.pack)
        return 0
    if args.parse:
        return run_parse(args, rails)
    require_pexpect()

    t_start = time.time()
    session = BoardSession(args.board, f"/tmp/measure_power_{args.label}.log")
    print(f"[systest] session up on {args.board}", flush=True)
    console = None
    try:
        board = session.sc_app("-c board").strip().splitlines()[-1].strip()
        print(f"[sc_app] board reports: {board}", flush=True)

        rails = session.list_rails() if args.rails == "all" else args.rails.split(",")
        print(f"[sc_app] sampling {len(rails)} rail(s): {' '.join(rails)}", flush=True)

        if args.console:
            host = args.farm_host or farm_host(args.board)
            if not host:
                print(f"[console] could not resolve a host for {args.board}", flush=True)
                if args.send:
                    # Without com0 the menu key never arrives, so the "load" phase
                    # would quietly measure an idle board. Refuse instead.
                    print("ERROR: --send needs the console; pass --farm-host to "
                          "skip the cluster-ping lookup", file=sys.stderr)
                    return 1
            else:
                console_log = f"/tmp/console_{args.label}.log"
                console = ConsoleChannel(host, args.board, console_log)
                print(f"[console] com0 via {host}, logging to {console_log}", flush=True)

        if args.program:
            session.program(args.program, hold=args.xsdb_hold, console=console)

        temps = collections.defaultdict(list)

        def take(count, tag):
            """Sample every rail `count` times; the first snapshot after a state
            change reads low (INA226 averaging), so callers discard it."""
            out = []
            for n in range(count):
                snap, t0 = {}, time.time()
                for rail in rails:
                    reading = session.get_power(rail)
                    if reading:
                        snap[rail] = reading
                out.append(snap)
                temp = session.get_temp() if args.temps else None
                if temp is not None:
                    temps[tag].append(temp)
                print(f"[{tag} {n + 1}/{count}] {len(snap)}/{len(rails)} rails in "
                      f"{time.time() - t0:.1f} s, "
                      f"total {sum(v[2] for v in snap.values()):.3f} W"
                      + (f", {temp:.1f} C" if temp is not None else ""), flush=True)
                if args.interval:
                    time.sleep(args.interval)
            return out

        # Configured-but-idle baseline: the array is loaded and clocked, but the
        # app is sitting at its menu. Isolates the cost of the inference itself.
        idle_samples = []
        if args.idle_snapshots:
            if console and args.wait_for:
                if console.wait_for(args.wait_for, timeout=args.wait_timeout):
                    print(f"[console] saw {args.wait_for!r}", flush=True)
                else:
                    print(f"[console] timed out waiting for {args.wait_for!r}", flush=True)
            if console:
                console.mark("idle sampling")
            idle_samples = take(args.idle_snapshots, "idle")

        if console and args.send:
            for key in args.send:
                console.send(key)
                time.sleep(args.send_gap)
        if args.settle:
            print(f"[wait] {args.settle:.0f} s for steady state", flush=True)
            time.sleep(args.settle)
        if console:
            console.mark("load sampling")

        samples = take(args.snapshots, "load")

        # A run long enough to sample is usually still going when sampling ends;
        # its summary carries the rate needed for energy per unit of work.
        if console and args.wait_summary:
            print(f"[wait] up to {args.wait_summary:.0f} s for the app's rate line",
                  flush=True)
            # Wait for the rate line itself, not a summary header: the header
            # lands a beat earlier and parsing then races the console drain.
            if not console.wait_for(args.rate_regex, timeout=args.wait_summary):
                print("[console] no line matched --rate-regex", flush=True)

        idle_means = None
        if len(idle_samples) > 1:
            idle_means = summarize(idle_samples[1:], rails, f"{args.label} (configured, idle)")
        means = summarize(samples, rails, f"{args.label} (load)")

        total_w = sum(means.values())
        if idle_means:
            print(f"\n{'load - idle delta':<20}{'':>19}"
                  f"{total_w - sum(idle_means.values()):>10.4f}")
        for tag, values in temps.items():
            print(f"{'temperature (' + tag + ')':<20}{'':>19}"
                  f"{statistics.mean(values):>10.1f} C"
                  f"  (min {min(values):.1f}, max {max(values):.1f})")

        if console:
            rate = console.bench_summary(args.rate_regex).get("rate")
            if rate:
                print(f"\nrate from com0        : {rate:.1f} /s")
                print(f"energy per unit       : {1000.0 * total_w / rate:.2f} mJ "
                      f"(listed rails, total power)")
                if idle_means:
                    delta = total_w - sum(idle_means.values())
                    print(f"  above configured idle: {1000.0 * delta / rate:.2f} mJ")
            print("\nlast console lines:")
            for line in console.lines()[-10:]:
                print(f"  {line}")

        if args.csv:
            # A run stamp keeps repeat runs of the same label apart; without it
            # the snapshot indices collide and naive sums double-count.
            per_snapshot = {(tag, n): t
                            for tag, values in temps.items()
                            for n, t in enumerate(values)}
            write_csv(args.csv, time.strftime("%Y%m%d-%H%M%S", time.localtime(t_start)),
                      args.label, (("idle", idle_samples), ("load", samples)),
                      per_snapshot)
    except pexpect.TIMEOUT as e:
        print(f"TIMEOUT: {e}", file=sys.stderr)
        return 1
    except RuntimeError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1
    finally:
        if console:
            console.close()
        session.close(power_off=not args.keep_power)
    return 0


if __name__ == "__main__":
    sys.exit(main())
