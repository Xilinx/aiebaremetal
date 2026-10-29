<!--- Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc. --->
<!--- SPDX-License-Identifier: Apache-2.0 --->

# src/ipc — AEG PS IPC Boundary

Shared source for the IPC boundary that lets the external PS application run
**outside** the AIE simulator process.

## Overview

This replaces the old **in-process** AIE-simulator runtime with a **two-process,
socket-based (IPC)** architecture.

Previously, the AEG baremetal runtime and the user's graph code were compiled *into*
the simulator's PS library as a patched stub (`ps_i3.aeg.so`) and injected into the
simulator package. Now the real host application runs as a **separate process** and
communicates with the `aiesimulator` over a Unix domain socket. The coupling is
inverted:

- The **simulator side is the server** — it carries a generic IPC server plus a
  per-example generated graph-init shim.
- The **host application is the client** — it runs the user's unmodified `graph.cpp`
  (or `graph_new_api.cpp`) linked against the real AEG runtime.

User graph code no longer needs to be compiled into the simulator's PS wrapper.

## Files

```
src/ipc/
  aeg_ipc_server.cpp        Standalone IPC server — provides ps_main(), linked into
                            ps_i*.so alongside the generated PS wrapper.  Handles
                            all IPC commands via a SystemC work-queue dispatch loop.
                            Graph init is triggered on-demand by the GRAPH_INIT
                            command rather than eagerly at startup.

  aeg_ps_ipc_backend.cpp    Client-side IPC backend — provides ess_*, aeg_ipc_alloc/
                            free/write/read_gm, and aeg_ipc_graph_init_ipc() symbols
                            exported from the external app via -Wl,-E.

  aeg_ps_ipc_backend.h      Declarations for the backend lifecycle functions.

  ps_ipc_protocol.h         Wire protocol: fixed-size request/response structs,
                            command IDs (WRITE32, READ32, ALLOC_GM, GRAPH_INIT, …),
                            status codes, and send/recv helpers.

  ipc_ps_main_decl.h        Minimal stub used as -D__TEST_BENCH__ when building
                            ps_i*.so: just declares ps_main() so the generated
                            wrapper's CallPsMainFunction(ps_main) resolves to
                            aeg_ipc_server.o's implementation.

  aeg_sim_stubs.cpp         3 ADF override stubs compiled into ps_i*.so:
                            setWorkDirectory, initializeConfigurations (no-op),
                            reserveBroadcastchannelForEnablingCores.

  xioutils.cpp / .h         Simulator-side ess_* plumbing (function-pointer dispatch)
                            plus PL address-space helpers (XPl_Write/Read/IOstart).
```

Related files outside this directory:

```
src/aeg_aiebaremetal.cc     AIEGraphSim IPC integration (dlsym wiring, init paths).
src/include/xaiemem.h       Dual-mode (native / IPC) XAieMem.

script/build_sim.sh              Build orchestrator + graph-init generator.
script/build_libxaiengine_sim.sh Builds libxaiengine_sim.so (aie-rt, -D__AIESIM__).
script/runsim.sh / runsim_ipc.sh Launch simulator + client app.
script/ci_sim_example.sh         CI entry point.
```

## Architecture Overview

```
 aiesimulator process                        External PS process (graph.cpp)
 ──────────────────────────────────────────  ──────────────────────────────────────
 ps_i*.so (wrapper + aeg_ipc_server.o)       ipc_app (graph.cpp + aeg_ps_ipc_backend.o)
   │                                           │
   │  ps_main() [aeg_ipc_server.cpp]            │  AbrGraph gr("name")  ← auto-connects
   │    1. Init XAie device                     │  gr.init()
   │    2. Open Unix socket                     │    → GRAPH_INIT("name") ─────────────►
   │    3. Work-queue dispatch loop  ◄──────────┤      server: SetPartIsolationAfterRst,
   │                                            │      broadcastConfigForCoreEnable(),
   │  GRAPH_INIT: aeg_ipc_run_graph_init()      │      {name}_load_elf / {name}_init
   │    direct calls to {graph}_load_elf,        │    → WRITE32 × N          (ConfigureGmios)
   │    {graph}_init via aie_control.cpp         │  gr.malloc(n)         ← ALLOC_GM
   │    linker symbols (name-dispatched,          │                         → XAie_MemAllocate
   │    multi-graph safe, no dlsym)              │  gr.run(1)            ← START_PLIO
   │                                             │  gr.gm2aie_nb(...)    ← WRITE_GM + WRITE32
   │  PSWrite32 / PSRead32 → XTLM AXI-MM         │  gr.gmio_wait(...)    ← READ32 × N
   │  PSWriteGM / PSReadGM → XTLM GM             │  gr.end()             ← WRITE32 × few
   │                                ◄────────────┤  ~AbrGraph()          ← EXIT + disconnect
   │              Unix domain socket             │
   └─────────────────────────────────────────────┘
          AEG_PS_IPC_SOCKET env var
```

### Server (inside the simulator) — `aeg_ipc_server.cpp`

- Provides `ps_main()`, which the generated PS wrapper (`genwrapper_for_ps_i*.cpp`)
  invokes via `CallPsMainFunction`.  The stub header `ipc_ps_main_decl.h` is supplied
  as the make `TEST_BENCH` so the wrapper resolves `ps_main` to the server object
  instead of pulling in the user's `graph.cpp`.
- Opens the Unix domain socket and runs a **SystemC work-queue dispatch loop**:
  socket I/O runs on a detached `std::thread`, but the actual simulator calls are
  marshalled onto the SystemC thread via an `sc_event` plus a mutex-protected work
  queue, because SystemC is **not thread-safe**.  The socket thread enqueues a work
  item and blocks on a per-item condition variable until it is serviced.
- Translates each IPC command into the wrapper's simulator symbols (`PSWrite32 /
  PSRead32 / PSWriteGM / PSReadGM / PSNpiWrite32 / PSNpiRead32 / PSStartPLIO`), which
  route into the simulator's **XTLM AXI-MM / GM transactors**.
- Writes a `/tmp/aeg_ps_ipc_<pid>.ready` file after `listen()` for readiness
  detection.

### Client (host app) — `aeg_ps_ipc_backend.cpp`

- Exports the simulator-facing `ess_*` symbols plus `aeg_ipc_alloc/free/write/read_gm`,
  `aeg_ipc_alloc/free_gm_host`, `aeg_ipc_dev_addr`, `aeg_ipc_graph_init_ipc`, and
  connection lifecycle helpers.  All are exported via `-Wl,-E`.
- Each call becomes a synchronous request/response transaction over the socket.
- Maintains a **host↔device address map**: `aeg_ipc_alloc_gm_host(size)` requests a
  device address from the server (`ALLOC_GM` → `XAie_MemAllocate`), mallocs a
  host-side shadow buffer, and records the mapping.  `aeg_ipc_dev_addr(host)` resolves
  any host pointer (including offsets into an allocation) back to its device address
  via an `upper_bound` range lookup.

### Runtime hook-in — `libaiebaremetal_ipc.so`

Under `__AIESIM__`, the `AIEGraphSim` code in `src/aeg_aiebaremetal.cc` runs
**exclusively** against the out-of-process IPC simulation backend (there is no
in-process native-sim path):

- **Memory** (`src/include/xaiemem.h`): `XAieMem` holds **either** a real
  `XAie_DevInst*` **or** a set of five IPC function pointers
  (alloc/free/dev_addr/write/read).  A boolean `ipc_` selects the path for
  `malloc / free / sync_for_cpu / sync_for_dev / get_dev_addr`.  Under
  `__AIESIM__` the IPC pointers are always used.
- **Wiring** (constructor): under `__AIESIM__` it resolves the backend `aeg_ipc_*`
  symbols via `dlsym(RTLD_DEFAULT, …)`, builds an IPC-backed `XAieMem`, and
  auto-connects the socket from `AEG_PS_IPC_SOCKET`.  If the IPC helpers cannot be
  resolved, construction fails (there is no native fallback).
- **Graph init** (`init()`): sends `GRAPH_INIT` (graph name as payload) via
  `aeg_ipc_graph_init_ipc`.  The sim side then runs `SetPartIsolationAfterRst(0)`,
  `broadcastConfigForCoreEnable()`, and the generated `{graph}_load_elf` /
  `{graph}_init` hooks (see `aeg_ipc_run_graph_init`).
- **Teardown** (destructor): under `__AIESIM__`, resolves and calls
  `aeg_ipc_backend_disconnect`, sending `EXIT` before closing.

## IPC Commands

| Command      | ID   | Direction  | Description |
|--------------|------|------------|-------------|
| PING         | 0x01 | app→sim    | Liveness check |
| WRITE32      | 0x10 | app→sim    | AIE register write → XTLM AXI-MM |
| READ32       | 0x11 | app→sim    | AIE register read |
| NPI_WRITE32  | 0x12 | app→sim    | NPI register write |
| NPI_READ32   | 0x13 | app→sim    | NPI register read |
| WRITE_GM     | 0x20 | app→sim    | Bulk write to AIE global memory |
| READ_GM      | 0x21 | app→sim    | Bulk read from AIE global memory |
| ALLOC_GM     | 0x22 | app→sim    | Allocate GMIO device address (XAie_MemAllocate) |
| FREE_GM      | 0x23 | app→sim    | Release GMIO address |
| START_PLIO   | 0x30 | app→sim    | Start PLIO ports |
| GRAPH_INIT   | 0x40 | app→sim    | Run graph init on sim side via aeg_ipc_run_graph_init() |
| EXIT         | 0xFF | app→sim    | Graceful shutdown (carries app return code) |

Wire format (`ps_ipc_protocol.h`, `#pragma pack(1)`):

```c
request  { u8 cmd; u8 _rsvd[3]; u64 arg1; u32 arg2; }
response { u8 status; u8 _rsvd[7]; u64 value; }
```

Status codes: `OK (0x00)`, `ERR_PROTO (0x01)`, `ERR_IO (0x02)` (the client also uses
`0xFF` internally to signal a connection/receive error).  For `WRITE_GM` and
`GRAPH_INIT`, a variable-length payload (`arg2` bytes) follows the request header; for
`READ_GM`, the payload follows the response.

## Generated File: aeg_ipc_graph_init.cpp

`script/build_sim.sh` generates `aeg_ipc_graph_init.cpp` per-example from
`aie_control_config.json`.  It provides:

- `aeg_ipc_devinst_init()` — device-specific XAie setup (`XAie_SetupConfig` /
  `XAie_SetupPartitionConfig` / `XAie_CfgInitialize`) using the driver config parsed
  from the control JSON.
- `aeg_ipc_run_graph_init(name, workDir)` — name-dispatch to `{graph}_load_elf` /
  `{graph}_init` via direct linker symbols from `aie_control.cpp`.  Works for any
  graph name and multiple graphs simultaneously (no `dlsym` on the sim side).
- `aeg_ipc_alloc_mem()` / `aeg_ipc_free_mem()` — delegates to `XAie_MemAllocate` /
  `XAie_MemFree` from `libaiecompiler_runtime.so` (the Vitis ESS memory pool).

## Build Pipeline

Entry: `example/*/build.sh` → `script/compile.sh` (aiecompiler, produces `Work/`) →
**`script/build_sim.sh`** (orchestrator, called automatically; can also run
standalone).  `build_sim.sh` steps:

1. **Host app `ipc_app`** — compile `graph.cpp` (or `graph_new_api.cpp`) + backend
   with `g++`, link with `-Wl,-E`
   `-Wl,--unresolved-symbols=ignore-in-object-files`, `-lxaiengine -ldl`.  The typed
   API adds `-DAEG_INHERITANCE_BASED_API` and the `Work/ps/c_rts` include path.
2. **`libaiebaremetal_ipc.so`** — recompile the AEG runtime with `-D__AIESIM__
   -D__BAREMETAL_IPC__`, generate per-example metadata via `script/aiemetacpp.py`, and
   link against a fresh simulator build of aie-rt (`libxaiengine_sim.so`, produced by
   `script/build_libxaiengine_sim.sh`).
3. **Server + PS library `ps_i*.so`** — parse graph names + driver config from
   `aie_control_config.json`, **generate `aeg_ipc_graph_init.cpp`**, compile the
   server objects (`aeg_ipc_server.o`, generated init, `xioutils.o`,
   `aeg_sim_stubs.o`) with the **Vitis clang** toolchain, rebuild the wrapper via
   `make TEST_BENCH=ipc_ps_main_decl.h`, and link into `ps_i*.so` against
   `-laiecompiler_runtime -lsystemc -lxtlm`.
4. **`ipc/build_sim.env`** — record artifact paths so `runsim.sh` can locate prebuilt
   artifacts without rebuilding.

## Run Flow

`script/runsim.sh` (identical to `runsim_ipc.sh`):

1. Source `build_sim.env`; verify `ipc_app`, `libaiebaremetal_ipc.so`, and `Work/`.
2. Export `AEG_PS_IPC_SERVER=1` and choose the socket path.
3. Launch `aiesimulator --pkg-dir=./Work … --profile` in the background.
4. Poll for the socket file (60s deadline).
5. Run `ipc_app` with `AIE_BAREMETAL_LIB=<libaiebaremetal_ipc.so>` and the socket path.
6. **Test outcome is determined by the client app** (exit code or "Test Passed"), not
   by the simulator; a nonzero simulator exit only produces a warning.

`script/ci_sim_example.sh` is the CI entry point for the flow.

## Usage

After `source script/settings.sh`, run from any IPC-capable example directory:

```bash
cd example/testkernel

# Legacy API (AbrGraph)
source ./build.sh 2          # compile AIE kernels + build all IPC sim artifacts
runsim.sh                    # run

# Typed API (AEGGraph<>) — two equivalent workflows:
source ./build.sh 2 -new_api # full rebuild including aiecompiler, then build_sim.sh
runsim.sh                    # run

# — or — switch host app only (skips aiecompiler, reuses existing Work/):
source ./build.sh 2          # legacy build to create Work/
runsim.sh --new-api          # rebuilds ipc_app from graph_new_api.cpp, then runs

runsim.sh --log              # add --log to any run for per-command IPC traces
```

To rebuild only the IPC sim artifacts (skipping aiecompiler):

```bash
build_sim.sh                              # from example directory, legacy API
build_sim.sh --runtime_source_file ./src/graph_new_api.cpp -new_api
```

## Environment Variables

| Variable | Side | Purpose |
|----------|------|---------|
| `AEG_PS_IPC_SERVER` | sim | Enables the server (set by run script). |
| `AEG_PS_IPC_SOCKET` | both | Unix socket path. |
| `AIE_BAREMETAL_LIB` | client | Path to `libaiebaremetal_ipc.so`. |
| `AEG_IPC_GRAPH_NAMES` / `AEG_IPC_GRAPH_NAME` | sim | Graph name(s) for server-side init. |
| `AIE_WORK_DIR` | sim | Work directory (default `./Work`). |
| `AEG_IPC_LOG` / `AEG_IPC_LOG_SERVER` | client / sim | CSV IPC trace outputs. |
| `AEG_PS_IPC_VERBOSE` | both | Verbose stderr diagnostics. |

## Migration Note: removed in-process approach

This flow replaces the old in-process stub mechanism, now deleted:

- `sim_stub/` (whole directory: `aeg_wrapper.cpp`, `libaeg_runtime_stub.cpp`,
  `aeg_ioutils.*`, `aeg_aie_control.cpp`, `app_elf_stub.cpp`, `aie_codegen.h`,
  `Makefile`).
- `script/aeg_aiesimulator`, `script/patch_scsim_for_aeg.sh`,
  `script/rebuild_ps_i3_with_aeg.sh`.
- The section of `script/compile.sh` that built `libaeg_runtime.so` + `ps_i3.aeg.so`
  and **patched scsim** into the simulator package.

Previously the runtime was compiled into a patched `ps_i3.aeg.so` and injected into
the simulator; now the runtime lives in a normal process and the simulator carries
only a generic IPC server.
