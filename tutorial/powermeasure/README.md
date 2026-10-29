<!--- Copyright (C) 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Measuring power and energy of an aiebaremetal application (VEK280)

How to find out what your AIE application costs in watts, and how many joules it
spends per unit of work, without physical access to the board.

The procedure is app-agnostic: anything that boots as a `BOOT.BIN` on a VEK280
can be measured this way. The worked example is the ResNet-18 demo in
[`example/example_ai_model/aeg/res18sw`](../../example/example_ai_model/aeg/res18sw),
whose results are written up in
[`power/POWER_RESULTS.md`](../../example/example_ai_model/aeg/res18sw/power/POWER_RESULTS.md).

- [1. What is being measured](#1-what-is-being-measured)
- [2. Measuring by hand](#2-measuring-by-hand)
- [3. Making your app measurable](#3-making-your-app-measurable)
  - [3.1 When you cannot add a benchmark mode](#31-when-you-cannot-add-a-benchmark-mode)
- [4. Measuring with the script](#4-measuring-with-the-script)
  - [4.1 When the board is already yours](#41-when-the-board-is-already-yours)
  - [4.2 When the load bursts faster than a sweep](#42-when-the-load-bursts-faster-than-a-sweep)
- [5. Turning readings into numbers you can defend](#5-turning-readings-into-numbers-you-can-defend)
- [6. Worked example: ResNet-18](#6-worked-example-resnet-18)
- [7. Troubleshooting](#7-troubleshooting)

## 1. What is being measured

A VEK280 carries **INA226 current/voltage monitors on 20 power rails**. They are
owned by the board's **system controller (SC)** — a ZU4 that runs independently
of the Versal device you are programming — and read out by a utility on the SC
called **`sc_app`**. Because the SC is a separate chip, sampling power does not
perturb the application under test.

You do not need to be in the lab. `systest`, the board-farm tool, forwards a
command line to the SC with its `sysctl_sendcmd` verb:

```
systest <board>  ->  sysctl_sendcmd "sc_app -c getpower -t VCCINT"
```

The rails that matter for an AIE application on this board:

| rail | what it feeds |
|---|---|
| `VCCINT` | AIE-ML array and PL — the rail that responds to your design |
| `VCC_SOC` | NoC, DDR ↔ array traffic |
| `VCC_PSFP` | A72 full-power domain (your host code) |
| `VCC_PSLP_CPM5` | low-power domain |
| `VCC_PMC` / `VCCAUX_PMC` | platform management controller |
| `VCC_RAM_VCCINT_GT` | block RAM / GT |
| `VCCAUX` | auxiliary 1.5 V |
| `VCC1V1_LP4` | LPDDR4 |

The other 11 rails (MGT, SFP, FMC, HDIO) are unused by a typical AIE design and
contribute a fixed ~0.18 W; `sc_app -c listpower` shows the full list.

Two properties of this measurement path shape everything below:

- **A snapshot is slow.** Each `sc_app` round trip costs 0.6–1 s depending on farm
  and SC load, so reading nine rails takes 6–10 s. Your application must hold a
  steady load for *minutes*.
- **Readings are averaged.** The first sample taken after any state change reads
  low, because the INA226 averaging window has not caught up.

## 2. Measuring by hand

Worth doing once, to see where the numbers come from. Everything here is a
`systest` command; `sc_app` runs on the SC, not on the Versal.

```bash
/proj/systest/bin/systest vek280-4        # reserves the board via LSF
```

```
[vek280-4] Systest# power 0
[vek280-4] Systest# power 1
[vek280-4] Systest# sysctl_sendcmd "sc_app -c board"
[vek280-4] Systest# sysctl_sendcmd "sc_app -c listpower"
[vek280-4] Systest# sysctl_sendcmd "sc_app -c getpower -t VCCINT"
Voltage(V):     0.8062
Current(A):     0.9620
Power(W):       0.7935
[vek280-4] Systest# sysctl_sendcmd "sc_app -c gettemp -t Versal"
```

That is the **unconfigured idle** baseline: powered, nothing programmed (~0.7 W
total on a clean board). Now program your image and read the same rail again:

```
[vek280-4] Systest# xsdb
xsdb% conn
xsdb% tar 1
xsdb% device program /home/<user>/myapp.BIN
xsdb% exit
[vek280-4] Systest# sysctl_sendcmd "sc_app -c getpower -t VCCINT"
```

`exit` matters: `sysctl_sendcmd` is a systest command, and while you sit at the
`xsdb%` prompt it is not available. Leaving xsdb only drops the debug
attachment — the A72 keeps running your application.

To watch the console at the same time, open a second terminal on the host that
owns the board's serial port (`/proj/systest/bin/cluster-ping vek280` tells you
which host):

```bash
ssh <farm host>
/opt/systest/common/bin/systest-client
[vek280-4] Systest# connect com0
```

Doing a full run this way means typing ~40 `getpower` commands per phase, which
is why the rest of this tutorial uses the script.

## 3. Making your app measurable

This is the part that is about *your application*, and it is the step most
easily skipped. A one-shot run that finishes in milliseconds cannot be measured
by a sampler that needs 6–10 s per snapshot: you would sample an idle board.

Your app needs a mode that **holds a steady load for several minutes**. Three
requirements, all of which cost real accuracy if ignored:

1. **Bound the run by wall clock, not iteration count.** Then one recipe covers
   fast and slow configurations alike. Five minutes is a good default: it gives
   ~20 snapshots after the load has settled.
2. **Silence per-iteration console output inside the timed region.** At 115200
   baud a single line costs ~2.4 ms. The ResNet-18 round is 2.71 ms, so leaving
   one `printf` in the loop makes the benchmark measure the UART instead of the
   design.
3. **Print the achieved rate at the end.** Power alone is not efficiency. Energy
   per unit of work needs a rate measured *in the same run*, under the same
   conditions.

A minimal sustained-load loop, in whatever passes for your app's main:

```c
    #define BENCH_SECONDS 300
    #define TS_HZ         100000000ULL          /* pinned timestamp rate      */
    #define TS_PER_US     (TS_HZ / 1000000ULL)

    XTime t0, t1;
    unsigned long rounds = 0;
    quiet = true;                               /* silence per-round printfs  */
    XTime_GetTime(&t0);
    for (;;) {
        run_one_iteration();                    /* your unit of work          */
        rounds++;
        XTime_GetTime(&t1);
        if ((double)(uint64_t)(t1 - t0) >= (double)BENCH_SECONDS * TS_HZ)
            break;
    }
    quiet = false;

    double us = (double)(uint64_t)(t1 - t0) / TS_PER_US;
    printf("throughput        : %.1f img/s\n", 1.0e6 * rounds / us);
```

The last line is what the sampler scrapes. Keep the format or pass your own
pattern with `--rate-regex`; the units are yours, the script just needs one
number meaning "work per second".

Drive the mode from a menu key (the sampler can type it) rather than running it
automatically at boot, so the same image still serves for normal use and gives
you a configured-idle baseline while it waits at the menu.

ResNet-18 implements exactly this: `sw/resnet18_top.cpp` gains `-DBENCH_SECONDS=N`
for the wall-clock bound and a `txn_silent` flag for requirement 2, both reached
through the existing `b` menu key.

### 3.1 When you cannot add a benchmark mode

For an application you did not write — one running under an OS on the APU, say,
where the array is driven through a runtime instead of your own loop — the same
three requirements apply, but you satisfy them by choosing the right entry point
rather than by editing code.

Look for a flag that repeats the work **inside one process**, after any setup: a
`--repeat`, `--iterations` or `--loop` option. What matters is that the timed
region contains only the work itself. A wrapper script that re-launches the
application per iteration measures process startup and device configuration, and
one that validates or compares results between iterations measures the CPU doing
that comparison — in both cases the array idles for most of the wall clock and the
measured power collapses towards idle. Prepare inputs before the loop for the same
reason: decoding or resizing per iteration is host work inside your measurement.

Two cheap checks tell you whether the load is real, before you trust any number:

- **Read the rail signature.** If the CPU rail is up while the compute rail only
  occasionally rises above its idle band, you are measuring the host, not the
  design. A genuine load raises the compute rail in most samples.
- **Compare the reported rate against the run's wall time.** If the application
  prints a per-iteration time, iterations x time should account for nearly all of
  the elapsed time. A large shortfall is host overhead you have not eliminated.

Loads that burst on a millisecond scale also need §4.2's sampling technique, since
a single slow sweep cannot average them.

## 4. Measuring with the script

[`measure_power.py`](./measure_power.py) automates one full measurement:
it reserves the board, programs the image, samples a configured-idle baseline at
the menu, starts the load, samples the plateau, and reads the app's own rate off
the console so power and throughput come from the same run.

```bash
/usr/bin/python3 ./measure_power.py --board vek280-5 --label myapp \
    --program /home/<user>/images/myapp.BIN \
    --console --wait-for 'b - benchmark' --temps \
    --idle-snapshots 3 --send b --settle 20 --snapshots 20 \
    --wait-summary 260 --csv myapp.csv
```

What the important flags do:

| flag | why |
|---|---|
| `--program` | image to load over xsdb (the script leaves xsdb afterwards) |
| `--console` | open com0, needed both to start the load and to read the rate |
| `--wait-for` | console pattern that means "app is up" (typically its menu) |
| `--idle-snapshots 3` | configured-idle baseline before the load starts |
| `--send b` | key that starts your sustained mode |
| `--settle 20` | let the load reach steady state before sampling |
| `--snapshots 20` | load samples; 20 × ~6 s ≈ 2 min of the 5 min run |
| `--wait-summary` | wait for the rate line after sampling ends |
| `--temps` | log device temperature with each snapshot (see §5) |
| `--rate-regex` | if your app doesn't print `throughput : N img/s` |
| `--keep-power` | leave the board on at exit (default is `power 0`) |

Use `/usr/bin/python3` explicitly. Sourcing the Vitis settings or a Zephyr
`.venv` puts a different `python3` first on PATH, and the shebang follows PATH.
(The script strips `LD_LIBRARY_PATH` for its ssh calls, because the Vitis
`lib/lnx64.o` ships an incompatible `libcrypto.so.3` that otherwise kills
`/bin/ssh` with "OpenSSL version mismatch".)

Four outputs, only one of which you name:

- **stdout** — progress per snapshot, then per-rail tables for idle and load, the
  load − idle delta, temperatures, and energy per unit of work. Not saved
  anywhere: pipe it through `tee`.
- **`--csv` path** — one row per rail per snapshot, appended, with a `run`
  timestamp column so repeat runs stay separable. Note it keeps *every* snapshot,
  including the first of each phase that the printed summary discards.
- **`/tmp/console_<label>.log`** — timestamped com0 capture, including your app's
  summary. Check this first when the rate comes back empty.
- **`/tmp/measure_power_<label>.log`** — the raw systest/xsdb transcript.

Both `/tmp` logs are overwritten by the next run with the same `--label`, so use
distinct labels per configuration.

### 4.1 When the board is already yours

The script above reserves the board itself, which does not work when you are
already holding it — a booted PetaLinux you would rather not lose, for example.
A second `systest <board>` queues in LSF behind your own reservation, and
`sysctl_sendcmd` is a verb of the systest shell, not a program on PATH, so
nothing outside that session can reach `sc_app`.

For that case, generate the commands, paste them into the session you have, and
parse the captured output afterwards:

```bash
# 1. before starting the load, print an idle sweep and paste it into your session
/usr/bin/python3 ./measure_power.py --temps --oneline --emit-sweeps 3

# 2. start your sustained load, wait for steady state, then paste this
/usr/bin/python3 ./measure_power.py --temps --oneline --emit-sweeps 8

# 3. save the terminal text (idle sweeps first, load sweeps after) and summarize
/usr/bin/python3 ./measure_power.py --parse session.txt --label myapp \
    --idle-sweeps 3 --rate 365 --csv myapp.csv
```

`--oneline` matters because the systest prompt executes only the first line of a
multi-line paste; it packs each sweep into a single `sysctl_sendcmd`, so a
three-sweep baseline is three pastes rather than thirty. It relies on the system
controller running the string as a shell command, which one cheap check confirms:

```
sysctl_sendcmd "echo hello; sc_app -c board"
```

If both `hello` and the board name come back, use `--oneline`. If only the first
part runs, drop it and paste the per-rail lines individually — or cut the rail
list to the ones that actually move under load
(`--rails VCCINT,VCC_SOC,VCC_PSFP,VCC1V1_LP4`), which makes a sweep five pastes
instead of ten.

### 4.2 When the load bursts faster than a sweep

A sweep of nine rails takes about 10 s. If the load switches on a millisecond
scale — an inference loop, say, where each batch alternates compute and data
movement — every reading lands at an arbitrary phase of that waveform, and a
handful of sweeps cannot average it. The tell is inside a single `sc_app` call:
it reads voltage, current and power registers at different instants, so a bursty
load makes the reported watts disagree with the same call's V x I.

`--pack` answers that by putting every reading of a short rail list into one
command, sampling as fast as the system controller will answer:

```bash
# ~12 reads of VCCINT plus a temperature, in one paste, repeated during the load
/usr/bin/python3 ./measure_power.py --temps --rails VCCINT --pack --emit-sweeps 12
```

Take the same bursts with the load stopped for a matched baseline. `--parse`
counts a repeated rail as a new sweep, so this needs no special handling, and it
prints a standard deviation with an error bar on the mean (`sd/sqrt(n)`) for any
rail that swings more than 0.5 W. Read the spread, not just the mean: a bimodal
histogram means you are seeing a duty cycle, and dividing the mean's rise by the
busy-versus-idle gap estimates what fraction of the time the array really works.

Because individual readings can be internally inconsistent, sanity-check the mean
before quoting it: `--parse` prints mean voltage, mean current and mean power per
rail, and mean V x mean A should land within a few percent of the mean W. It does
even when single samples disagree badly, since the register skew averages out. If
the two means diverge, the capture is too short rather than merely noisy.

`--parse` identifies each reading by its echoed command rather than by position,
so console noise mixed into the capture is harmless. Sweeps are counted by rails
repeating, `--idle-sweeps` splits the phases, and `--rate` supplies the
throughput that the console scrape would otherwise provide. Output and CSV format
match the automated path, so both kinds of run land in one spreadsheet. Neither
`--emit-sweeps` nor `--parse` needs pexpect or a board.

## 5. Turning readings into numbers you can defend

Getting watts out is easy; getting watts that mean something takes five rules.
Each of these was learned by getting it wrong first.

**Expect VCCINT to be bistable, and record the state you got.** In the ResNet-18
work the *same image* read 3.34 W on VCCINT in some runs and 1.57 W in others,
with every other rail unchanged and throughput identical to within 0.03% — a
1.8 W error, larger than most effects you are trying to measure. Per-snapshot
temperature logging later showed this is *not* thermal: two runs 1.6 W apart were
both at 27 °C, and the image, the run's position in the session and the rest
before it were all excluded as well. Start cold and power the board down between
configurations anyway (leakage is real, just smaller than this), log temperature
with `--temps`, and record each run's configured-idle VCCINT so you can tell
which state a number came from before you compare it with another.

**Baseline against configured idle, not against nothing.** "Unconfigured idle"
(~0.7 W) tells you about the board; "configured idle" — design loaded and clocked,
app waiting at its menu — tells you what your design costs before it does any
work. The difference between configured idle and load is the marginal cost of the
work itself.

**Discard the first snapshot of each phase.** INA226 averaging lags a state
change; an early idle run read 0.28 W, then 0.93 W twice. The script's printed
summary already drops it; if you post-process the CSV yourself, filter
`snapshot > 0`.

**Compare deltas across sessions, absolutes only within one.** Load − idle and
throughput reproduce to a few percent, across sessions and across both VCCINT
states. Absolute totals only survive comparison between runs that started equally
cold *and* landed in the same state.

**Take the rate from the same run.** Energy per unit of work is
`total_watts / rate`. A rate copied from a different session silently mixes
thermal and clock conditions.

## 6. Worked example: ResNet-18

Build a measurement image (300 s benchmark instead of the default 100 rounds),
then measure it:

```bash
cd example/example_ai_model/aeg/res18sw/sw
EXTRA_DEFINES="-DBENCH_SECONDS=300" bash -c 'source ./build.sh -bootgen'
cp build/Vek280BOOT.BIN ~/power_images/t300_bm_inst1.BIN

cd ../../../../../tutorial/powermeasure
/usr/bin/python3 ./measure_power.py --board vek280-5 --label bm_inst1 \
    --program ~/power_images/t300_bm_inst1.BIN \
    --console --wait-for 'b - benchmark' --temps \
    --idle-snapshots 3 --send b --settle 20 --snapshots 20 \
    --wait-summary 260 --csv bm_inst1.csv
```

Roughly ten minutes later:

```
===== bm_inst1 (load): 20 snapshot(s) =====
rail                        V         A    W mean    W min    W max
VCCINT                 0.8066    1.9455    1.5748   0.9766   1.9533
VCC_SOC                0.8080    4.2446    3.4334   3.3953   3.4525
...
TOTAL (listed)                             8.6133

load - idle delta                          1.6039
temperature (idle)                           26.4 C  (min 26.0, max 26.9)
temperature (load)                           31.5 C  (min 29.3, max 32.9)
rate from com0        : 369.2 /s
energy per unit       : 23.33 mJ (listed rails, total power)
  above configured idle: 4.34 mJ
```

The `W min` of 0.9766 on VCCINT is the first load snapshot, taken before the
INA226 average caught up — the artifact §5 warns about, visible in real data.

Read that as: the single-instance design costs 7.0 W just to sit configured, the
inference work adds 1.6 W on top, and each image costs 23.3 mJ in total or
4.3 mJ marginally.

`example/example_ai_model/aeg/res18sw/power/build_measurement_images.sh` builds
all four ResNet-18 configurations (baremetal and Zephyr, one and ten instances)
this way, and `POWER_RESULTS.md` compares them. The headline finding is a good
illustration of why marginal and total energy both matter: ten instances raise
throughput 1.40× but power 1.8×, so energy *per image* gets worse, because the
static cost of a bigger overlay is amortized over a throughput gain that the
serial control path caps.

## 7. Troubleshooting

| symptom | cause and fix |
|---|---|
| `ModuleNotFoundError: No module named 'pexpect'` | shadowed interpreter; run `/usr/bin/python3 ./measure_power.py` |
| ssh EOF, `OpenSSL version mismatch. Built against 30000020` | Vitis `libcrypto` on `LD_LIBRARY_PATH`; fixed in-script, so update your copy |
| `pexpect.TIMEOUT` with `<<Waiting for dispatch ...>>` | LSF has not granted the board yet; retry when it frees up |
| `[console] could not resolve a host` | `cluster-ping` API hiccup; pass `--farm-host <host>` |
| Load phase looks identical to idle | the `--send` key never started anything: check `/tmp/console_<label>.log` and `--wait-for` |
| `no line matched --rate-regex` | app did not print its rate within `--wait-summary`, or the pattern doesn't match its wording |
| Total reads ~0.3 W on the first snapshot | INA226 averaging; that is why the first sample per phase is discarded |
| Every `sc_app` call times out mid-run | session stuck at `xsdb%` after a failed `device program` |
| Same config reads ~2 W higher than yesterday, on VCCINT only | the bistable VCCINT state (§5); check configured-idle VCCINT, reprogram and re-measure — cooling alone does not fix it |
