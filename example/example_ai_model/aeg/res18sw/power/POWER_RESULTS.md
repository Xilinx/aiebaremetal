<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# ResNet18 on VEK280: power and energy, baremetal vs Zephyr

Measured 2026-07-29/30 on **vek280-5** (farm host chanterelle19), Vitis
2024.2_INT_1113_1001, aie-rt `main-next` cf9c933d, and re-measured 2026-08-05 on
the same board with per-snapshot temperature logging.
Rails are read by the system
controller's INA226 monitors via `systest` → `sysctl_sendcmd "sc_app -c getpower"`,
so no physical access is needed. The method is a worked example of
[`tutorial/powermeasure`](../../../../../tutorial/powermeasure/README.md), which
documents the procedure for any aiebaremetal app; "Method" below records what was
specific to these runs.

## Headline

Ten instances buy **1.40× throughput for 1.8× power**, so per-image energy is
*worse* than a single instance, not better. The round is dominated by serial A72
control-transaction time in both configurations (19.4 ms for 10 images vs 2.71 ms
for 1), so the array spends most of its time idle while its static power scales
with the number of instances. Energy efficiency here is bounded by the control
path, not by the compute.

**The runtime does not affect power.** Compared in the same VCCINT state (see
"VCCINT two-state offset"), baremetal and Zephyr draw the same watts at both
instance counts, rail for rail. What differs is speed: at
a single instance baremetal is **5.0% faster** (2708.5 vs 2842.5 µs/round),
consistent with the June timing results, and that alone accounts for its lower
energy per image.

## Results

Nine instrumented rails, 20 load snapshots per run, 300 s benchmark
(`-DBENCH_SECONDS=300`, per-transaction UART output suppressed inside the timed
region so the round is not serial-port bound).

| configuration | µs/round | img/s | ms/image | idle W | load W | delta W | mJ/img total | mJ/img above idle | idle °C | load °C |
|---|---|---|---|---|---|---|---|---|---|---|
| baremetal, NUM_INST=1 | 2708.5 | 369.2 | 2.71 | 7.01 | 8.62 | 1.61 | 23.3 | 4.34 | 27.1 (26.7–27.5) | 32.2 (29.8–33.5) |
| Zephyr, NUM_INST=1 | 2842.5 | 351.8 | 2.84 | 7.06 | 8.64 | 1.58 | 24.6 | 4.49 | 27.5 (27.1–28.1) | 32.5 (30.4–34.1) |
| baremetal, NUM_INST=10 | 19405.5 | 515.3 | 1.94 | 12.67 | 15.22 | 2.55 | 29.5 | 4.96 | 30.6 (30.0–31.4) | 37.8 (34.2–40.0) |
| Zephyr, NUM_INST=10 | 19472.1 | 513.6 | 1.95 | 12.67 | 15.40 | 2.73 | 30.0 | 5.31 | 30.1 (29.2–31.0) | 37.2 (34.0–39.2) |

"idle" is *configured* idle: the design is loaded and clocked with the app sitting
at its menu. Every row comes from a run that started on a cold board, which
matters: see "VCCINT two-state offset". The Zephyr N=1 row was re-measured on
2026-07-30 because the original read 1.8 W high on VCCINT alone; its timing was
unaffected (2684.0 vs 2684.4 µs/round in ctrl_TXN).

The temperature columns give the mean and range across a run's snapshots, and
come from the 2026-08-05 re-measurement, the first one taken after the script
started writing a temperature with every CSV row. That session reproduced every throughput
figure above to within 0.2% and the baremetal N=1 power row to within 0.05 W;
its other three rows landed in the high VCCINT state and are not repeated here.
Note that the die is still warming while a run is sampled — the load column
climbs ~4 °C across the 20 snapshots — so a single load temperature is a mean,
not a plateau.

Every number here comes from the measurement script's own output, which also
reports per-rail min/max, temperatures, and the throughput the app itself
printed. Those raw run logs are not kept in the repo; "Method" below is enough to
regenerate them.

Per-rail load power (W), single instance vs ten, baremetal:

| rail | inst1 | inst10 | what it feeds |
|---|---|---|---|
| VCCINT | 1.57 | 8.41 | AIE-ML array + PL — the only rail that scales with instances |
| VCC_SOC | 3.44 | 3.34 | NoC, DDR↔array traffic |
| VCC1V1_LP4 | 1.36 | 1.18 | LPDDR4 |
| VCCAUX | 1.16 | 1.17 | auxiliary 1.5 V |
| VCC_PSFP | 0.53 | 0.54 | A72 full-power domain |
| VCC_PSLP_CPM5 | 0.29 | 0.30 | low-power domain |
| VCC_PMC | 0.13 | 0.13 | platform management controller |
| VCCAUX_PMC | 0.11 | 0.11 | PMC auxiliary |
| VCC_RAM_VCCINT_GT | 0.03 | 0.03 | block RAM / GT |

Zephyr at one instance matches the baremetal column rail for rail once both runs
are in the same VCCINT state — 1.57 W either way, every other rail within 20 mW —
so the runtime does not change what the hardware draws. A mixed-state pair looks
like a 1.6 W runtime difference and is the easiest way to misread these numbers.

The remaining 11 rails on this board (MGT, SFP, FMC, HDIO) are unused by this
demo; measured across all 20 they contribute ~0.18 W of static draw that does not
respond to load, and are excluded. Unconfigured, powered-on board: 0.71 W
(vek280-4). Note vek280-5 comes up with something already configured and reads
2.77 W before programming, so its pre-program reading is not an idle baseline.

## Interpretation

**The marginal cost of an inference is the same either way.** Energy above
configured idle is 4.3–4.5 mJ/image at one instance and 4.9–5.3 mJ/image at ten:
the compute costs roughly what it costs. What differs is the fixed cost being
amortized — configured idle is 7.0 W for the single-instance overlay and 12.7 W
for the 10-instance one, a 5.6 W increase in static/clock power for a 1.40×
throughput gain. That ratio is why total energy per image rises.

**The bottleneck is the serial control path, not the array.** Both configurations
report `phase gr.run() = 0` with `ctrl_TXN()` accounting for 94–99.7% of the
round: the A72 replays the
control transaction serially, and at ten instances that transaction is ~7× longer
(19.4 ms vs 2.71 ms) while covering 10× the images. Overlapping or pipelining
control with compute is where both latency and energy per image would improve;
adding instances alone cannot fix it.

**Runtime choice does not change power, only speed.** Same-state runs of the two
runtimes draw the same watts at one instance (8.62 vs 8.64 W under load), so the
whole energy difference there is baremetal's 5% timing advantage: 23.3 vs
24.6 mJ/image. That advantage is ~134 µs/round, which matches the June finding
that Zephyr pays an extra per-round input-buffer copy — visible in the benchmark's
own phase line, where the work outside `ctrl_TXN()` is ~159 µs/round on Zephyr
against ~7 µs on baremetal. At ten instances both power and timing are within the
run-to-run spread.

## VCCINT two-state offset (measurement caveat)

Absolute totals carry a 1.6–2.3 W offset that lives **entirely in VCCINT**; every
other rail repeats within tens of milliwatts and throughput is unchanged to
within 0.2%. Configured idle for a given image is bimodal, and the two states are
far enough apart to read off directly:

| configuration | low state, VCCINT idle | high state, VCCINT idle |
|---|---|---|
| NUM_INST=1 | 0.61–0.77 W | 1.91–2.40 W |
| NUM_INST=10 | 6.31–6.39 W | 8.06–8.67 W |

Both runtimes and both instance counts have been recorded in both states — the
published Zephyr N=1 row is a low-state run (VCCINT 1.57 W under load) and the
same image read 2.12–3.34 W in other runs — so this is a property of the board or
of the programming event, not of the design or the runtime.

The 2026-08-05 re-measurement, which logs die temperature with every snapshot,
excludes the explanations that were available when this was first seen:

- **Not temperature.** Baremetal N=1 at 27.1 °C idle read 0.65 W while Zephyr N=1
  at 27.5 °C read 2.28 W in the same session: 1.6 W apart at the same
  temperature. The thermal-history explanation previously offered here is
  withdrawn. Leakage does track temperature, but over the 26–31 °C spread these
  runs actually cover it cannot produce 2 W.
- **Not the image.** The surviving July BOOT.BIN files, re-run today, reproduce
  today's values rather than July's (Zephyr N=1 idle 2.27 W, baremetal N=10 idle
  8.34 W), so neither rebuild nor build drift is the variable.
- **Not run position.** The low state has been recorded on the first run of a
  session and on the fifth, the high state on the second; July's own rerun landed
  low on its fourth run, and a 2026-07-30 session held the low state for six
  consecutive runs before flipping.
- **Not rest duration.** Twelve minutes powered off before programming still
  produced the high state (2.12 W at 27.2 °C idle).

Device temperature (`sc_app -c gettemp -t Versal`, logged with every snapshot)
rises from ~26–31 °C at idle to ~30–40 °C under load, and is essentially the same
in both states — the high state runs ~0.5 °C warmer, which is the consequence of
the extra 2 W, not its cause. What sets the state is still unidentified;
residual configuration or clocking state that survives a `power 0`/`power 1`
cycle remains the most plausible candidate.

**Consequence for reading these numbers:** load−idle deltas and throughput are
stable to a few percent across sessions and across both states, and are what
should be compared between configurations. Before quoting an absolute total,
check the configured-idle VCCINT against the table above to see which state the
run landed in; a run in the high state is 2 W off and reprogramming is the only
known way to try for the low one.

## Building the images

A measurement image is an ordinary image with one extra compile flag: `b` is
bounded by wall clock (`BENCH_SECONDS`) instead of round count (`BENCH_LOOPS`,
default 100). Everything else — the `l`/`b`/`n`/`q` menu, the network demo, the
throughput summary — is unchanged, so a measurement image is still usable for a
normal run; it just makes `b` take 300 s.

Regular builds are exactly the commands documented in §4 and §5 of the app
[`README.MD`](../README.MD), unchanged by this work:

```
cd sw      && source ./build.sh -bootgen          # baremetal, NUM_INST=1
cd sw      && source ./buildbatch110.sh -bootgen  # baremetal, NUM_INST=10
cd zephyr  && ./build_zephyr.sh                   # Zephyr,    NUM_INST=1
cd zephyr  && ./build_zephyr.sh -inst10           # Zephyr,    NUM_INST=10
```

All four measurement images at once, copied out under names that record what
they are (each driver overwrites the same output path):

```
cd power && ./build_measurement_images.sh     # → ~/power_images/t300_{bm,zephyr}_inst{1,10}.BIN
SECS=600 OUT=/tmp/imgs ./build_measurement_images.sh   # different length / location
```

One configuration at a time, if that is all that is needed:

```
cd sw     && EXTRA_DEFINES="-DBENCH_SECONDS=300" bash -c 'source ./build.sh -bootgen'
cd sw     && EXTRA_DEFINES="-DBENCH_SECONDS=300" bash -c 'source ./buildbatch110.sh -bootgen'
cd zephyr && ./build_zephyr.sh -bench_seconds 300
cd zephyr && ./build_zephyr.sh -inst10 -bench_seconds 300
```

`EXTRA_DEFINES` passes flags to the host compile only; `-bench_loops N` /
`-bench_seconds N` are the Zephyr equivalents. Source each baremetal driver in
its own shell (`bash -c`) as shown: `build.sh` and `buildbatch110.sh` share
`GRAPH_PARAMS`, so sourcing both into one interactive shell leaks
`-DNUM_INST=10` into the single-instance build. Switching instance counts no
longer needs a clean build (see "Prerequisite" below).

## Method

```
cd tutorial/powermeasure
/usr/bin/python3 ./measure_power.py --board vek280-5 --label bm_inst10 \
    --program ~/power_images/t300_bm_inst10.BIN \
    --console --wait-for 'b - benchmark' --temps \
    --idle-snapshots 3 --send b --settle 20 --snapshots 20 \
    --wait-summary 260 --csv power_final.csv
```

Environment traps (a shadowed `python3`, and Vitis breaking `/bin/ssh`) are
covered in the tutorial's troubleshooting section.

The script takes the board with `systest` (the same LSF reservation as a manual
session, which is also what authorizes ssh to the farm host for com0), programs
the image over xsdb, leaves xsdb so `sysctl_sendcmd` is usable again, samples the
configured-idle baseline at the menu, types `b`, then samples the load plateau and
reads the app's own throughput summary from the console so power and throughput
come from the same run.

Three details that matter for correctness:

- **A 9-rail snapshot takes ~5–7 s**, so the load must be a plateau lasting
  minutes. Hence `-DBENCH_SECONDS=300`, which bounds the benchmark by wall clock
  instead of round count and so covers both instance counts with one recipe.
- **The first snapshot after any state change reads low** (INA226 averaging has
  not caught up — an early idle run read 0.28 W then 0.93 W twice), so the first
  sample of each phase is discarded.
- **Per-transaction UART output is suppressed during the timed region**
  (`txn_silent`). At 115200 baud one line per round costs ~2.4 ms against a
  2.71 ms round, which would otherwise make the single-instance benchmark measure
  the serial port.
