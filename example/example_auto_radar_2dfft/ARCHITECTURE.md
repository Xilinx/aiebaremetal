<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# Radar 2D-FFT Application Architecture

## Overview

This application computes range and velocity from radar chirp data using a two-stage FFT pipeline on AMD Versal AI Engine (AIE-ML). Four parallel instances process 32 RX channels (8 per instance) to meet aggregate throughput of ~2 GSPS.

## Data Flow Diagram

```
                                 ×4 parallel instances (i = 0..3)

  DDR                    AIE Tile              Mem Tile 1           Shim Tile
  ────                   ────────              ──────────           ─────────
                    ┌──────────────┐      ┌──────────────┐
  gm_in[i] ──MM2S──►  RangeFFT    ├─────►│ memtile1_buf ├──S2MM──► DDR
  (input      DMA   │  2048-pt    │window│  partial     │stream    (3D BD:
   chirps)          │  cint16     │ 2048 │  transpose   │          Step/Wrap
                    └──────────────┘      └──────────────┘          for transpose)
                                                                        │
  ┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄┄ DDR bounce (entire matrix must complete) ┄┄┄┄┄┄┄┄┤
                                                                        │
  DDR                Mem Tile 2            AIE Tile                 Shim Tile
  ────               ──────────            ────────                 ─────────
                ┌──────────────┐     ┌──────────────┐
  DDR ──MM2S───►│ memtile2_buf ├────►│ cint16_to_   │
  (linear  DMA  │  final       │win  │ cint32       │
   read)        │  transpose   │ 256 │ (type conv)  │
                └──────────────┘     └──────┬───────┘
                                            │ window 256 × cint32
                                     ┌──────▼───────┐
                                     │ VelocityFFT  │
                                     │  64-pt       ├──S2MM──► DDR
                                     │  cint32      │          gm_out[i]
                                     └──────────────┘          (output)
```

## Design Parameters

| Parameter | Value |
|---|---|
| Range FFT point size | 2048 (cint16 in/out) |
| Velocity FFT point size | 64 (cint32 in/out) |
| Samples per chirp (L) | 2048 (trimmed to 1024 due to conjugate symmetry) |
| TX channels | 24 |
| RX channels | 32 total (8 per instance × 4 instances) |
| Groups (G) | 64 |
| AIE clock | 1 GHz |
| Parallel instances | 4 |
| AIE tiles used | 8 (2 per instance: RangeFFT on one, cint16_to_cint32 + VelocityFFT co-located on another) |
| Mem tiles used | 8 (2 per instance: memtile1 and memtile2) |
| Shim tiles used | 8 columns (4 for primary GMIO, 4 for intermediate DDR bounce) |

## Three Kernels

**RangeFFT** -- Vitis DSP Library `fft_ifft_dit_1ch_graph<cint16, cint16, 2048>`. Computes range profile from each chirp. Output is conjugate symmetric, so only the first 1024 points are passed downstream (halving the data).

**cint16_to_cint32** -- Custom kernel (`kernel.cc`). Converts cint16 range data to cint32 for the velocity FFT which requires higher precision. Co-located with VelocityFFT on the same AIE tile (combined runtime ratio = 0.6).

**VelocityFFT** -- Vitis DSP Library `fft_ifft_dit_1ch_graph<cint32, cint16, 64>`. Computes Doppler/velocity profile across the 64 groups. This FFT operates on the transposed data, where the groups dimension has been moved to the innermost position.

## Why the DDR Bounce?

The VelocityFFT needs to compute 64-point FFTs across the **groups dimension**. The RangeFFT processes one group at a time, so we need all 64 groups' worth of data before we can start the velocity stage. The full matrix per instance is:

```
G × TX × L = 64 × 24 × 1024 = 1,572,864 cint16 samples = ~6 MB
```

This exceeds the 512 KB mem tile capacity, so the data is bounced through DDR between the two stages. The transpose (reordering the groups dimension from outermost to innermost) is split across three stages:

1. **Mem tile 1**: Partial transpose via tiling parameters -- rearranges `[G][TX*RX][L]` column-by-column
2. **Shim DMA S2MM**: Writes to DDR with 3D buffer descriptors (Step0=1, Wrap0=96, Step1=24576, Wrap1=512, Step2=96) that scatter data into transposed positions
3. **Mem tile 2**: Final transpose via tiling parameters -- produces `[L][TX*RX][G]` ordering for VelocityFFT

The shim DMA MM2S (DDR → mem tile 2) reads linearly since the data is already transposed in DDR.

## GMIO Port Map

| Ports | Shim Columns | Direction | Purpose | DMA Config |
|---|---|---|---|---|
| `gm_in[0..3]` | 14, 15, 22, 23 | DDR → AIE | Primary input (chirp data) | Runtime-managed (GMIO API) |
| `gm_out[0..3]` | 14, 15, 22, 23 | AIE → DDR | Final output (velocity data) | Runtime-managed (GMIO API) |
| `output_aiegm[0..3]` | 2, 3, 6, 7 | AIE → DDR | Intermediate: memtile1 → DDR (3D transpose) | Manual (`shim_config1-4`) |
| `input_gmaie[0..3]` | 2, 3, 6, 7 | DDR → AIE | Intermediate: DDR → memtile2 (linear read) | Manual (`shim_config1-4`) |

The intermediate ports (`output_aiegm`, `input_gmaie`) use ping-pong buffering (BD0/BD1 for S2MM, BD4/BD5 for MM2S) with lock synchronization, all manually configured via `XAie_Write32` register writes in the current code.

## Input Preprocessing

Before feeding data to the AIE, the host performs **chirp rotation** -- rotating each chirp left by 4 samples. This replaces the HLS `mm2s` PL kernel from the upstream IPSP design, which performed this rotation in hardware. In the baremetal port, this is done in software (C++ loop in the host code).
