<!--- Copyright (C) 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# PL-Based Softmax

PL-kernel implementation of softmax, for direct comparison against the
AIE-only implementation in `../aie_only/`. Where `aie_only` computes softmax
entirely on an AIE core, this example moves the actual softmax math (exp +
sum + divide) into a PL (HLS) kernel, using the AIE core only as the minimal
required participant in the graph (aiebaremetal needs a non-empty `libadf.a`).

Both examples use the exact same math (`exp(x) / sum(exp(x))` per 32-element
tile, no max-subtraction) over the same input data, so their outputs are
directly comparable.

## Data flow

```
DDR (input) -> [mm2s PL] -> [AIE passthrough @ (2,0)] -> [softmax_pl PL] -> [s2mm PL] -> DDR (output)
```

* **mm2s / s2mm** (`pl_kernels/{mm2s,s2mm}.cpp`) — reused verbatim from
  `../../example_kernelplio/pl_kernels/`. Generic 32-bit-word DDR<->stream
  movers; type-agnostic (the payload here is `float`, bit-cast at the
  `softmax_pl` boundary).
* **AIE passthrough** (`src/passthrough.cc`) — a single AIE tile that copies
  its input window to its output window unchanged. Exists only because
  aiebaremetal requires at least one AIE core in the graph; it does no
  softmax math.
* **softmax_pl** (`pl_kernels/softmax_pl.cpp`) — the actual softmax kernel,
  synthesized via Vitis HLS / `v++`. Pure AXI-Stream in/out (no `m_axi`
  pointer arg — data arrives from/leaves to `mm2s`/`s2mm` over streams, not
  DDR directly).

Data is moved in `NUM_TILES` (8) chunks of `TILE_LEN` (32) elements each
(`src/common.h`), matching `aie_only`'s per-tile transfer loop so host-side
`aeg::pl_kernel` `start()`/`wait()` calls are exercised once per tile.

## HW/SW Architecture

### 1. Interfaces

| Interface | Standard | Width | Where used |
|---|---|---|---|
| `m_axi` (memory-mapped) | AXI4 | 32-bit data (`ap_int<32>* mem`), `bundle=gmem` | `mm2s`/`s2mm` ↔ DDR (via NoC) |
| `axis` (stream) | AXI4-Stream, `ap_axis<32,0,0,0>` w/ `TLAST` | 32-bit | `mm2s` → PLIO → AIE passthrough → PLIO → `softmax_pl` → `s2mm` (all data-path hops) |
| `s_axilite` (control) | AXI4-Lite | 32-bit registers | PS host (`aeg::pl_kernel`) → each PL kernel's control/status/argument registers |
| PLIO | AIE-array-to-PL streaming bridge (`plio_32_bits`) | 32-bit | The two hops that specifically cross the AIE↔PL boundary: `mm2s.s → ai_engine_0.plioin1` and `ai_engine_0.plioout1 → softmax_pl.in` (see `pl_kernels/system.cfg`) |

Every PL kernel exposes both an `axis` data port and an `s_axilite` control
port (`return`, plus `size`/`mem` scalar args) — this is what `aeg::pl_kernel`
drives via `set_arg(offset, value)`/`start()`/`wait()` (see "Host control"
above).

### 2. Communication

- **Data plane**: one-directional AXI4-Stream chain, `mm2s` → AIE → `softmax_pl`
  → `s2mm`, all 32-bit-word streams (`float` bit-cast at the `softmax_pl`
  boundary — see `pl_kernels/softmax_pl.cpp`'s `bits_to_float`/`float_to_bits`).
  Each kernel asserts `TLAST` on the final beat it produces so the PLIO
  bridge (and any downstream AXI4-Stream-compliant consumer) can detect
  end-of-window and re-arm correctly.
- **Control plane**: independent from the data plane. The PS issues
  `s_axilite` register writes/reads to each kernel's own control base
  address (`MM2S_BASE`/`SOFTMAX_PL_BASE`/`S2MM_BASE`, from `xparameters.h`)
  — there is no shared control bus contention between kernels; each
  `aeg::pl_kernel` instance owns one kernel's register file exclusively.
- **Synchronization**: `AP_CTRL_HS` (`start`/`done`/`idle` bits at control
  offset `0x0`) per kernel. The host `start()`s all three kernels, then
  `wait()`s only on `s2mm` (the last kernel in the chain) — `mm2s` and
  `softmax_pl` are back-pressured by the stream `TREADY` handshake, so
  `s2mm` completing implies the whole chain has drained.

### 3. HW / Clock frequencies

| Clock domain | VEK280 (`xcve2802`, AIE-ML) | VCK190 (`xcvc1902`, AIE) | VEK385 (`2VE3858`, AIE2P) |
|---|---|---|---|
| AIE array core clock (`FMAX`) | **1250 MHz** — confirmed directly from this repo's own build (`Work/graph.aiecompile_summary`: `"frequency": 1250`) | 1250 MHz default (1000 MHz on early ES silicon) [AMD Adaptive Support] | 1250 MHz @ 0.80V best speed grade (1000/1050 MHz at lower voltage/speed corners) — [DS1021 Table 1] |
| PL kernel clock (v++ link default, no explicit `--kernel_frequency` set) | **312.5 MHz** — confirmed from this repo's own post-route Vivado timing report (`pl_kernels/_x/.../impl_1_route_report_timing_summary_0.rpt`, net `clk_wizard_0_clk_out1_o2`) | one of the same discrete AIE-PLL divisors (150/178.57/208.33/250/312.5/416.67/625/1250 MHz) — 312.5 MHz is a common default | **312.496875 MHz** — confirmed from AMD's own published `vek385_base` platform info (`UG1702`, Clock Index 1, the platform's *default* clock) — independently corroborates the VEK280 number measured in this repo |
| AIE↔PL interface clock (`FMAX_AI_PL`, async crossing) | not separately published for VE2802 in DS958; architecturally identical mechanism to VEK385 (async FIFO, see below) | not separately published for VC1902 | **625 MHz @ 0.80V** (525/500 MHz lower corners) — [DS1021 Table 1] |

All three platforms use the same mechanism at the AIE-PL interface: an
**asynchronous clock-domain crossing** with FIFOs for width conversion and
back-pressure (unchanged across AIE generations — see AMD's internal
Vail/AIE2 Architecture Specification §2.2.3.3). Concretely for this design:
the AIE array runs far faster (1.25 GHz) than the PL kernels it talks to
(312.5 MHz by default here) — the PL side, not the AIE side, is the
bottleneck for this particular example's data-plane throughput.

### 4. Bandwidth (DDR, PL, AIE)

These are **published/derived ceilings**, not per-run measurements — see
"A note on these numbers" below for how each was obtained, and the
"Correctness check" section's perf numbers for this design's actual
end-to-end wall-clock timing (which is dominated by one-shot kernel-start
latency on a 256-element buffer, not sustained throughput).

| Path | VEK280 (`VE2802`) | VCK190 (`VC1902`) | VEK385 (`2VE3858`) |
|---|---|---|---|
| DDR/LPDDR peak (device-level, all controllers) | 75 GB/s (LPDDR4, shared with PL & VDU) — [AMD internal Vail/AIE2 ArchSpec Table 2-1] | 102.4 GB/s (DDR4, 4× DDRMC) — [UG1079 worked example] | 170 GB/s (LPDDR5X) — [AMD Versal AI Edge Gen 2 product page] |
| AIE array ↔ PL aggregate, PL→AIE direction | 1164.8 GB/s (best speed grade) down to 896.0 GB/s (worst) — [DS958 Table 2, device `VE2802`] | ~1.25 TB/s example calc at 1 GHz AIE clock, 39 PL-interface columns — [UG1079 worked example] | 1080.0 GB/s (best) down to 864.0 GB/s (worst) — [DS1021 Table 2, device `2VE3858`] |
| **This design's PLIO data-plane** (1 stream each direction, 32-bit, `PIPELINE II=1`) | **≈1.25 GB/s theoretical** (312.5 MHz × 4 bytes/cycle) — PL-clock-limited, not AIE-clock-limited | same formula, PL-clock-dependent | same formula, PL-clock-dependent |
| GMIO per-tile ceiling (relevant to `aie_only`, not this example — different DMA path) | AIE-side DMA: 2 × 4 GB/s = 8 GB/s; NoC-side: 16 GB/s (128-bit aggregated) — [UG1079 Table 2] | same table (generation-independent numbers in UG1079's example) | same shape, device-specific numbers not yet pulled |

**A note on these numbers**: the "aggregate" AIE↔PL rows are the *whole device's*
theoretical ceiling across every PL-interface column (e.g. all 39 columns on
a VC1902) — not what a single design like this one can achieve. This
design's actual achievable rate is the **PLIO data-plane** row: it only
opens 2 PLIO channels (1 in, 1 out), each 32-bit wide, clocked by the PL
kernel clock (312.5 MHz measured on VEK280), giving ≈1.25 GB/s per direction
as a theoretical per-channel ceiling — three to four orders of magnitude
below the device's aggregate PL-interface bandwidth, which is expected: this
example is intentionally minimal (2 PLIO channels), not bandwidth-scaled.
Sources: AMD's public DS958 (`ds958-versal-ai-edge`)/DS1021
(`ds1021-versal-ai-edge-gen2`) "AI Engine Switching Characteristics", UG1079
"Performance Comparison Between AI Engine/PL and AI Engine/NoC Interfaces",
AMD's internal Vail/AIE2 Architecture Specification v1.7 §"Table 2-1" and
§3.4.10, and this repo's own build artifacts (`Work/graph.aiecompile_summary`,
`pl_kernels/_x/reports/link/imp/impl_1_route_report_timing_summary_0.rpt`)
for the two directly-measured numbers.

### 5. Data flow: PL → AIE and back

```
DDR (input)                                                          DDR (output)
    │  m_axi (AXI4-MM, 32-bit, via NoC)                                   ▲
    ▼                                                                     │ m_axi
┌─────────┐  axis   ┌──────────────┐  axis   ┌──────────┐  axis   ┌──────────┐
│  mm2s   │────────▶│   AIE tile   │────────▶│softmax_pl│────────▶│  s2mm    │
│  (PL)   │  PLIO   │  passthrough │  PLIO   │   (PL)   │ (PL-PL, │  (PL)    │
│         │ bridge  │   @ (2,0)    │ bridge  │          │no PLIO) │          │
└─────────┘         └──────────────┘         └──────────┘         └──────────┘
     ▲                                                                  │
     │ s_axilite (control, from PS/aeg::pl_kernel)                      │
     └──────────────────────────────────────────────────────────────────┘
```

- **PL → AIE**: `mm2s` reads `TOTAL_LEN` (256) 32-bit words from DDR over
  `m_axi`, pushes them one word/cycle (`PIPELINE II=1`) onto its `axis`
  output port. v++ inserts a PLIO bridge (async clock-domain crossing, PL
  clock → AIE clock) between `mm2s.s` and `ai_engine_0.plioin1`
  (`system.cfg`). The AIE-side `input_plio` (`graph.h`) feeds the
  `passthrough` kernel at `adf::tile(2,0)` in fixed `TILE_LEN` (32)-element
  windows — 8 windows per host run (`NUM_TILES`).
- **AIE → PL (reverse)**: `passthrough`'s output window goes to
  `output_plio`, back across a second PLIO bridge (AIE clock → PL clock)
  into `softmax_pl.in`. `softmax_pl` loops over all 8 tiles internally in
  HLS (one HLS invocation covers the whole 256-element buffer — see the
  "Restart-limitation" design note in this repo's project history for why),
  computing an independent `exp(x)/sum(exp(x))` per `TILE_LEN`-element
  tile, then streams results PL-to-PL directly into `s2mm` (no PLIO
  bridge needed here — both ends are PL kernels on the same PL clock).
  `s2mm` writes the result back to DDR over its own `m_axi` port.

## Host control (`aeg::pl_kernel`)

Three `aeg::pl_kernel` instances — `mm2s_kernel`, `softmax_kernel`,
`s2mm_kernel` — control the PL kernels from `src/graph_new_api.cpp`, per the
unified PL API added. `softmax_kernel` takes only a `size`
arg (no `MEM_OFFSET` — it never touches DDR directly):

```cpp
aeg::pl_kernel mm2s_kernel(MM2S_BASE);
aeg::pl_kernel softmax_kernel(SOFTMAX_PL_BASE);
aeg::pl_kernel s2mm_kernel(S2MM_BASE);

mm2s_kernel.set_arg(MEM_OFFSET, in);
mm2s_kernel.set_arg(MEM_SIZE_OFFSET, len);
softmax_kernel.set_arg(SOFTMAX_SIZE_OFFSET, len);
s2mm_kernel.set_arg(MEM_OFFSET, out);
s2mm_kernel.set_arg(MEM_SIZE_OFFSET, len);
mm2s_kernel.start();
softmax_kernel.start();
s2mm_kernel.start();
...
s2mm_kernel.wait();
```

The s_axilite register offsets are confirmed from the HLS-generated driver
headers under `pl_kernels/_x/.../drivers/*/src/x*_hw.h` after running
`pl_kernels/build.sh` — see the comments in `src/graph_new_api.cpp`. Notably
`softmax_pl`'s `size` lands at `0x10` (not `0x1C` like `mm2s`/`s2mm`), since
it has no `m_axi` pointer argument occupying that slot first.

## Compilation Flow

### 1. Compile flow (overview)

```
buildall.sh <AIE_GEN>
  │
  ├─▶ make compile AIE_GEN=<n>          (aiecompiler: src/graph_new_api.cpp → Work/, libadf.a)
  │
  ├─▶ make hw AIE_KERNEL_LOC=. AIE_GEN=<n>
  │     ├─▶ builds libaiebaremetala72.a/a78.a (cross-compiled AEG runtime + graph glue)
  │     └─▶ aiemetacpp.py → ../../generated_graphs.h  (typed graph metadata for AEG_INHERITANCE_BASED_API)
  │
  └─▶ script/builda72.sh <n>  (or builda78.sh for AIE_GEN=5/VEK385)
        ├─▶ pl_kernels/build.sh <n>            [PL synthesis — see §2]
        ├─▶ vitis -s platform_baremetal.py     [BSP — see §3]
        ├─▶ aarch64-none-elf-gcc  (host ELF link — see §3)
        └─▶ bootgen -arch versal[_2ve_2vm]     → build/Vek280SoftmaxPLBOOT.BIN
```

Two independent toolchains run in this pipeline and only meet at the very
end (§3): **`aiecompiler`** (AIE graph → `libadf.a`) and **`v++`/Vitis HLS**
(PL kernel C++ → `.xo` → linked `.xsa`). Everything upstream of `bootgen` can
be — and in this repo's build scripts is — rebuilt independently and
incrementally (`pl_kernels/build.sh` is skipped entirely on a `builda72.sh`
re-run if `new.xsa` already exists, since it's the slowest step, ~20+ min).

### 2. Component compilation

| Component | Source | Tool / command | Output | Approx. time |
|---|---|---|---|---|
| AIE graph | `src/graph_new_api.cpp`, `src/passthrough.{h,cc}`, `src/graph.h` | `aiecompiler` (via `make compile`) | `Work/`, `libadf.a` | seconds |
| AIE runtime glue | generated + AEG sources | `aarch64-none-elf-g++` (via `make hw`) | `libaiebaremetala72.a` / `a78.a` | ~1 min |
| `mm2s` / `s2mm` PL kernels | `pl_kernels/{mm2s,s2mm}.cpp` (HLS C++, reused verbatim from `example_kernelplio`) | `v++ -c -k <name> --platform <xpfm>` (Vitis HLS under the hood) | `<name>.xo` | ~1-2 min each |
| `softmax_pl` PL kernel | `pl_kernels/softmax_pl.cpp` (HLS C++, `#include common.h` for `TILE_LEN`/`NUM_TILES`) | `v++ -c -k softmax_pl --platform <xpfm>` | `softmax_pl.xo` | ~2-3 min |
| PL kernel link (Vivado synth+impl+bitgen) | 3× `.xo` + `system.cfg` + `../libadf.a` | `v++ -l --config system.cfg` | `new.xsa` (wraps `vitis_design_wrapper.pdi`) | ~20-24 min (dominant cost) |
| Platform BSP | `new.xsa` | `vitis -s platform_baremetal.py` | standalone-domain BSP (headers, `libxil.a`, device tree) | ~2-3 min |
| Host application | `src/graph_new_api.cpp` (same TU as the AIE compile, gated by `__ADF_FRONTEND__`/`AEG_INHERITANCE_BASED_API`), `../../generated_graphs.h` | `aarch64-none-elf-gcc -mcpu=cortex-a72` (or `-a78`), linking `libaiebaremetala72.a` + BSP `libxil.a` | `script/vek280.elf` | ~1-2 min |
| Boot image | `.elf` + `new.xsa`'s PDI + BIF | `bootgen -arch versal` (`-arch versal_2ve_2vm` for a78/VEK385) | `build/Vek280SoftmaxPLBOOT.BIN` | seconds |

Note that `src/graph_new_api.cpp` is compiled **twice**, by two different
compilers, for two different purposes: once by `aiecompiler` under
`__ADF_FRONTEND__` (emits only the ADF graph declaration, `myGraph gradf3`)
and once by `aarch64-none-elf-gcc` as the actual host `main()` (the
`aeg::pl_kernel` control code) — the `#ifdef __ADF_FRONTEND__` guard at the
top of the file is what makes one source serve both roles cleanly.

### 3. Compilation integration

The four build outputs below are independent artifacts (different tools,
different clock domains, different address spaces) that only get tied
together at the final integration step, `bootgen`:

- **`libadf.a`** (AIE graph binary) is linked directly into the host `.elf`
  by `aarch64-none-elf-gcc` — this is how the ARM host ends up able to call
  `gr.init()`/`gr.run()`/`gr.wait()` against the AIE array at runtime.
- **`new.xsa`**'s PDI (`vitis_design_wrapper.pdi`, from `v++ -l`) carries the
  actual PL bitstream (the 3 synthesized+placed+routed HLS kernels) plus the
  Vivado-built platform shell (NoC, clocking, PS-PL interconnect). It does
  **not** get linked into the `.elf` — it's a separate binary blob that
  `bootgen` packages alongside the `.elf` in the final `BOOT.BIN`, per the
  layout declared in `vek280_boot_image.bif`/`vek385.bif`.
- **`generated_graphs.h`** (from `aiemetacpp.py`, reading `libadf.a`'s
  metadata) is what lets the host `.elf`'s `AEGGraph<aeg::gradf3>` template
  instantiation see the *same* typed graph structure the AIE side compiled,
  without hand-maintained duplication — `-DAEG_INHERITANCE_BASED_API` is
  what switches `aiebaremetal.h` to consume this generated header. This is
  the one file that literally connects the two compiler toolchains'
  understanding of "what the graph looks like".
- **The BSP** (from `platform_baremetal.py`, reading
  `new.xsa`) supplies the standalone-domain headers (`xparameters.h` — where
  `MM2S_BASE`/`SOFTMAX_PL_BASE`/`S2MM_BASE` in `graph_new_api.cpp` come
  from) and `libxil.a` (low-level driver/BSP glue) that the host `.elf` also
  links against — this is how the ARM host's `aeg::pl_kernel` register
  writes end up targeting addresses that actually match what Vivado placed
  the 3 PL kernels' control interfaces at in `new.xsa`.
- **`bootgen`** is the true integration point: it takes the host `.elf`,
  `new.xsa`'s PDI, and the BIF's partition ordering, and produces one
  `BOOT.BIN` the board's boot ROM loads as a single unit — PMC configures
  the PL (from the PDI) and releases the A72/A78 reset to start executing
  the `.elf`, all driven by one boot image.

## v++ Kernel Compile & PLIO Wiring

The canonical, generalizable walkthrough for "how do I use v++ to compile a
PL kernel, and how do I go through PLIO to connect it to an AIE kernel?"
already lives in **`../../example_kernelplio/README.md`**'s "Step 2: Build
Flow of `BOOT.BIN`" section (`v++ -c`/`v++ -l`, `system.cfg`'s `nk=`/
`stream_connect=` directives, `xparameters.h` base addresses, `bootgen`
packaging — with official UG1393 references). This example's mechanics are
identical; not repeated here to avoid the two docs drifting apart.

What's specific to *this* example, beyond what `example_kernelplio` shows
(which only has 2 kernels and 2 PLIO hops, no PL-to-PL leg):

- **A third kernel and a third `nk=`/`sc=` pair** — `softmax_pl` sits
  between the two PLIO hops:
  ```ini
  nk=softmax_pl:1:softmax_pl
  sc=ai_engine_0.plioout1:softmax_pl.in
  sc=softmax_pl.out:s2mm.s
  ```
- **One `sc=` line with no `ai_engine_0.*` endpoint** —
  `sc=softmax_pl.out:s2mm.s` connects two PL kernels directly. v++ only
  inserts a PLIO bridge (clock-domain-crossing FIFO) when one side of an
  `sc=` line is an AIE port; a PL-to-PL line like this one gets a plain
  same-clock-domain stream connection instead, since there's no clock
  domain to cross (see "HW/SW Architecture" §5 above for the full data-flow
  picture this wiring implements).

### Quick recipe: adding a new PL kernel to either example

1. Write the kernel (HLS C++, `axis` + `s_axilite` pragmas — see
   `pl_kernels/softmax_pl.cpp` or `example_kernelplio`'s `mm2s.cpp` for a
   template).
2. Add one `v++ -c -k <name>` line to `pl_kernels/build.sh`.
3. If it needs to talk to the AIE array, add/reuse a named `input_plio`/
   `output_plio` in the graph (`src/graph.h`) — the name string is the
   literal token `system.cfg` will reference.
4. Add one `nk=` line and the `sc=` line(s) that connect it in
   `system.cfg`.
5. Re-run `pl_kernels/build.sh` (or the whole `buildall.sh`).
6. Add an `aeg::pl_kernel` instance on the host side (see "Host control"
   above) if the new kernel needs runtime `start()`/`wait()` control.

## Build

Mirrors `../../example_kernelplio/`'s build pipeline exactly (see that
example's README for the full explanation of each step).

### Simulation (AIE graph only; PL kernels are not exercised)

```bash
$ source ../../../script/settings.sh 1
$ make compile AIE_GEN=2
$ make sim
```

### Hardware (BOOT.BIN)

```bash
# From this directory:
$ ./buildall.sh 2
```

`AIE_GEN`: 1 = vck190, 2 = vek280, 5 = vek385. The host always builds from
`src/graph_new_api.cpp` (the `aeg::pl_kernel`-based path). The earlier legacy
`src/graph.cpp` (direct register pokes, pre-dating the unified PL API) has
been removed now that `graph_new_api.cpp` is HW-validated on all three
platforms.

Output: `build/Vek280SoftmaxPLBOOT.BIN` (or `Vck190SoftmaxPLBOOT.BIN` /
`Vek385SoftmaxPLBOOT.BIN`), i.e. `example/example_softmax/pl_only/build/`.

### Vitis release

This example does not pin a Vitis release of its own. It uses whichever one
the shared `script/settings.sh` puts on the path (currently **2026.2**) and
reads the release back out of `XILINX_VITIS` to pick the matching platform, so
the toolchain and the `.xpfm` can never come from different releases. To build
against another release, point `settings.sh` at it; nothing here needs editing.

The platforms used are the un-tagged `vek280_base` / `vck190_base` /
`vek385_base`. The release-tagged equivalents (`xilinx_vek280_base_202620_1`
and friends) carry the same XSA but are deprecated in 2026.2 and will not ship
after it. The un-tagged directories first appear in 2026.1, so this example
needs 2026.1 or newer.

Each build echoes what it resolved to:

```
pl_kernels: Vitis 2026.2, platform .../internal_platforms/vek280_base/vek280_base.xpfm
```

`pl_kernels/.built_for_aie_gen` records the `AIE_GEN` and release the cached
`new.xsa` was built for, so switching either one triggers a rebuild instead of
silently reusing an incompatible bitstream.

The BSP comes from the Vitis Python flow (`script/platform_baremetal.py`), not
the older XSCT `generate-platform.sh`, which stopped working in 2026.1 when
AMD removed XSCT.

## Correctness check

`src/graph_new_api.cpp` computes a CPU-side reference (`exp(x)/sum(exp(x))`
per tile, identical to `softmax_pl.cpp`) after `gr.wait()`/`gr.end()` return,
compares it against the PL-kernel output with a `1e-4` absolute tolerance,
and returns a real exit code — see `../aie_only/src/graph.cpp` for the same
pattern and its rationale (keeping the check strictly post-timing so it can't
affect a future `aie_only` vs `pl_only` performance comparison).
