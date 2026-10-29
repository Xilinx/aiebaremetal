#!/usr/bin/env bash
# Copyright (C) 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

qemu_log_stage() {
  local n="$1"
  local total="$2"
  local title="$3"
  echo ""
  echo "================================================================================"
  echo "[STAGE $n/$total] $title"
  echo "================================================================================"
}

qemu_log_hint() {
  echo "  [hint] $*"
}

resolve_qemu_bin() {
  local vitis_root="${1:-${XILINX_VITIS:-}}"
  local candidate="" prefix=""

  if [[ -z "$vitis_root" ]]; then
    echo "[qemu_hw_emu] Error: XILINX_VITIS is not set" >&2
    return 1
  fi

  for prefix in \
    "$vitis_root/data/emulation" \
    "$vitis_root/data/emulation/qemu_plnx" \
    "${XILINX_PETALINUX:-/nonexistent}/components/yocto/buildtools_extended"; do
    for candidate in \
      "$prefix/qemu/comp/qemu_edf/sysroots/x86_64-amdedfsdk-linux/usr/bin/qemu-system-aarch64" \
      "$prefix/qemu/comp/qemu/sysroots/x86_64-petalinux-linux/usr/bin/qemu-system-aarch64" \
      "$prefix/sysroots/x86_64-petalinux-linux/usr/bin/qemu-system-aarch64"; do
      if [[ -x "$candidate" ]]; then
        export PATH="$(dirname "$candidate"):$PATH"
        echo "$candidate"
        return 0
      fi
    done
  done

  if command -v qemu-system-aarch64 >/dev/null 2>&1; then
    command -v qemu-system-aarch64
    return 0
  fi

  return 1
}

require_qemu_bin() {
  local vitis_root="${1:-${XILINX_VITIS:-}}"
  local qemu_bin qemu_comp_path pmc_bin qemu_settings
  if ! qemu_bin="$(resolve_qemu_bin "$vitis_root")"; then
    echo "[qemu_hw_emu] Error: qemu-system-aarch64 not found under $vitis_root" >&2
    return 1
  fi

  qemu_comp_path="${qemu_bin%/comp/*}"
  pmc_bin="$qemu_comp_path/comp/qemu_edf/sysroots/x86_64-amdedfsdk-linux/usr/bin/qemu-system-microblazeel"
  qemu_settings="$qemu_comp_path/comp/qemu_edf/environment-setup-x86_64-amdedfsdk-linux"
  if [[ ! -x "$pmc_bin" || ! -f "$qemu_settings" ]]; then
    echo "[qemu_hw_emu] Error: incomplete QEMU installation under $qemu_comp_path" >&2
    return 1
  fi

  QEMU_BIN="$qemu_bin"
  QEMU_COMP_PATH="$qemu_comp_path"
  export QEMU_BIN QEMU_COMP_PATH
  export PATH="$(dirname "$qemu_bin"):$PATH"
  echo "  QEMU resolved: $qemu_bin"
  echo "  QEMU/PMC root: $QEMU_COMP_PATH"
}

find_baremetal_pmc_dtb() {
  local work_dir="$1"
  local sim_dir="$2"
  local dts_dir="$work_dir/_x/package/qemu_dts_files"
  local built="$dts_dir/LATEST/MULTI_ARCH/board-versal-pmc-virt.dtb"

  if [[ -f "$sim_dir/board-versal-pmc-virt.dtb" ]]; then
    echo "$sim_dir/board-versal-pmc-virt.dtb"
    return 0
  fi
  if [[ ! -f "$built" && -f "$dts_dir/Makefile" ]]; then
    make -C "$dts_dir" "LATEST/MULTI_ARCH/board-versal-pmc-virt.dtb" >/dev/null 2>&1 || true
  fi
  if [[ -f "$built" ]]; then
    echo "$built"
    return 0
  fi
  return 1
}

# v++ 2026.2 generates an EDF (Linux guest) launch script even for
# --package.os baremetal, and no longer emits the BOOT.BIN / sd_card.img / PMC
# device tree that 2025.2 produced. Rebuild that flow here: the design PLM boots
# the packaged PDI from the emulated SD card, so it applies the PMC and AIE CDOs
# and starts the A72 itself, exactly as on hardware. Without this the AIE comes
# up unconfigured and any graph operation stalls.
patch_baremetal_launch_hw_emu() {
  local work_dir="${1:?work_dir required}"
  local platform_xpfm="${2:?platform .xpfm required}"
  local sim_dir="$work_dir/sim/behav_waveform/xsim"
  local ps_dtb pmc_dtb pdi_image sd_image platform_name candidate
  local qemu_bin qemu_comp_path qemu_bindir pmc_bin

  if [[ ! -f "$work_dir/launch_hw_emu.sh" ]]; then
    echo "[qemu_hw_emu] Error: $work_dir/launch_hw_emu.sh not found" >&2
    return 1
  fi

  if ! grep -q '\-enable-edf' "$work_dir/launch_hw_emu.sh"; then
    echo "[qemu_hw_emu] launch_hw_emu.sh already baremetal (no -enable-edf); leaving unchanged"
    return 0
  fi

  qemu_comp_path="${QEMU_COMP_PATH:-}"
  if [[ -z "$qemu_comp_path" ]]; then
    if ! qemu_bin="$(resolve_qemu_bin "${XILINX_VITIS:-}")"; then
      echo "[qemu_hw_emu] Error: unable to resolve QEMU for launch script" >&2
      return 1
    fi
    qemu_comp_path="${qemu_bin%/comp/*}"
  fi
  qemu_bindir="$qemu_comp_path/comp/qemu_edf/sysroots/x86_64-amdedfsdk-linux/usr/bin"
  pmc_bin="$qemu_bindir/qemu-system-microblazeel"
  if [[ ! -x "$qemu_bindir/qemu-system-aarch64" || ! -x "$pmc_bin" ]]; then
    echo "[qemu_hw_emu] Error: incomplete QEMU/PMC binaries under $qemu_comp_path" >&2
    return 1
  fi

  for candidate in "$sim_dir"/board-versal-*-ps-cosim*.dtb; do
    if [[ -f "$candidate" ]]; then
      ps_dtb="$candidate"
      break
    fi
  done

  # v++ 2026.2 still builds a design-specific PMC device tree but no longer
  # copies it next to the PS one, so fall back to the package temp directory
  # (kept by --save-temps). The stock device tree in the Vitis install is not a
  # substitute: it lacks the design DDR mapping, and the A72 never starts.
  pmc_dtb="$(find_baremetal_pmc_dtb "$work_dir" "$sim_dir")"

  for candidate in "$work_dir/a.pdi" "$work_dir"/*.pdi; do
    if [[ -f "$candidate" ]]; then
      pdi_image="$candidate"
      break
    fi
  done

  if [[ -z "${ps_dtb:-}" || -z "${pdi_image:-}" ]]; then
    echo "[qemu_hw_emu] Error: baremetal hw_emu artifacts are incomplete" >&2
    return 1
  fi
  if [[ -z "${pmc_dtb:-}" ]]; then
    echo "[qemu_hw_emu] Error: PMC device tree not found; package with --save-temps" >&2
    return 1
  fi
  for candidate in a_bh.bin pmc_cdo.bin plm.bin; do
    if [[ ! -f "$work_dir/$candidate" ]]; then
      echo "[qemu_hw_emu] Error: boot file $candidate missing under $work_dir" >&2
      return 1
    fi
  done
  if ! command -v mkfatimg >/dev/null 2>&1; then
    echo "[qemu_hw_emu] Error: mkfatimg not found; source Vitis settings first" >&2
    return 1
  fi

  sd_image="$work_dir/sd_card.img"
  rm -rf "$work_dir/sd_card"
  mkdir -p "$work_dir/sd_card"
  cp "$pdi_image" "$work_dir/sd_card/BOOT.BIN"
  if ! mkfatimg "$work_dir/sd_card" "$sd_image" 500000 >/dev/null; then
    echo "[qemu_hw_emu] Error: could not create $sd_image" >&2
    return 1
  fi
  # QEMU 8.2 rejects non-power-of-two SD capacities. Extending the image keeps
  # the FAT filesystem and BOOT.BIN intact while presenting a valid 512 MiB card.
  truncate -s 512M "$sd_image"

  platform_name="$(basename "$platform_xpfm" .xpfm)"

  # boot mode 5 = SD1, matching the FAT32 image holding BOOT.BIN.
  cat >"$work_dir/qemu_args.txt" <<EOF
-M
arm-generic-fdt
-display
none
-chardev
stdio,id=serial2,logfile=qemu_output.log,mux=on,signal=off
-mon
serial2
-sync-quantum
"1000000000"
-boot
mode=5
EOF

  # The PLM boots the PDI itself, so no ELF is preloaded and the A72 stays in
  # reset until the PLM releases it.
  cat >"$work_dir/pmc_args.txt" <<EOF
-M
microblaze-fdt
-display
none
-device
loader,file=$work_dir/a_bh.bin,addr=0xf201e000,force-raw=on
-device
loader,file=$work_dir/pmc_cdo.bin,addr=0xF2000000,force-raw=on
-device
loader,file=$work_dir/plm.bin,addr=0xF0200000,force-raw=on
-device
loader,addr=0xf0200000,cpu-num=1
-device
loader,addr=0xf0000000,data=0xba020004,data-len=4
-device
loader,addr=0xf0000004,data=0xb800fffc,data-len=4
-device
loader,addr=0xF1110624,data=0x0,data-len=4
-device
loader,addr=0xF1110620,data=0x1,data-len=4
EOF

  cat >"$work_dir/launch_hw_emu.sh" <<EOF
#!/bin/bash
export QEMU_COMP_PATH="$qemu_comp_path"
export PATH="$qemu_bindir:\$PATH"
export LAUNCH_EMULATOR_INVOKE_DIR="\$PWD"
cd "\$(dirname "\$(readlink -f "\$0")")" || exit 1
launch_emulator -device-family versal \\
  -qemu-args-file "\$PWD/qemu_args.txt" \\
  -pmc-args-file "\$PWD/pmc_args.txt" \\
  -qemu-dtb "$ps_dtb" \\
  -pmc-dtb "$pmc_dtb" \\
  -aie-sim-config "\$PWD/cfg/aie.sim.config.txt" \\
  -noc-memory-config "\$PWD/sim/behav_waveform/xsim/noc_memory_config.txt" \\
  -pl-sim-dir "\$PWD/sim/behav_waveform/xsim" \\
  -sd-card-image "$sd_image" \\
  -target hw_emu \\
  -platform-name $platform_name \\
  "\$@"
EOF
  chmod +x "$work_dir/launch_hw_emu.sh"
  echo "[qemu_hw_emu] Patched launch_hw_emu.sh for baremetal SD boot of $(basename "$pdi_image")"
}

run_hw_emu_launch_test() {
  local work_dir="${1:?work_dir required}"
  local log_tail="${2:-}"
  local pass_string="${PASS_STRING:-Test Passed}"
  local timeout="${QEMU_TIMEOUT:-600}"
  local quiet="${JENKINS_QEMU_QUIET:-}"
  local qemu_log="${TMPDIR:-/tmp}/qemu_hw_emu_$$.log"
  local early_pass=false success=false tail_pid=""

  if [[ ! -f "$work_dir/launch_hw_emu.sh" ]]; then
    echo "[qemu_hw_emu] Error: $work_dir/launch_hw_emu.sh not found" >&2
    return 1
  fi

  qemu_log_stage 4 5 "Emulator — start hw_emu (launch_hw_emu.sh)"
  qemu_log_hint "FAIL AREA: QEMU/xsim/PMC never ready, wrong boot files, or hang before serial output."
  qemu_log_hint "Wraps ./launch_hw_emu.sh with timeout ${timeout}s; log: $qemu_log"
  [[ -n "$quiet" && "$quiet" != "0" ]] && echo "  Quiet mode: output mainly in $qemu_log"

  setsid bash -c "cd '$work_dir' && exec timeout '$timeout' ./launch_hw_emu.sh 2>&1" >"$qemu_log" 2>&1 &
  local emu_pid=$!
  local emu_pgid
  emu_pgid="$(ps -o pgid= -p "$emu_pid" 2>/dev/null | tr -d ' ')"
  [[ -z "$emu_pgid" ]] && emu_pgid="$emu_pid"
  echo "  Emulator started (PID $emu_pid, PGID $emu_pgid)"

  if [[ -z "$quiet" || "$quiet" == "0" ]]; then
    tail -n +1 -f "$qemu_log" 2>/dev/null &
    tail_pid=$!
  fi

  stop_emulator() {
    kill -TERM "-$emu_pgid" 2>/dev/null || true
    for _ in 1 2 3 4 5 6; do
      kill -0 "-$emu_pgid" 2>/dev/null || break
      sleep 0.5
    done
    kill -KILL "-$emu_pgid" 2>/dev/null || true
    [[ -n "$tail_pid" ]] && kill "$tail_pid" 2>/dev/null || true
  }

  local poll_start poll_end last_status elapsed
  poll_start="$(date +%s)"
  poll_end=$((poll_start + timeout + 120))
  last_status=0
  while [[ "$(date +%s)" -lt "$poll_end" ]]; do
    sleep 10
    if [[ -f "$qemu_log" ]] && grep -q "$pass_string" "$qemu_log" 2>/dev/null; then
      echo "  ✓ '$pass_string' seen in $qemu_log — stopping emulator pipeline."
      early_pass=true
      break
    fi
    if [[ -f "$work_dir/qemu_output.log" ]] && grep -q "$pass_string" "$work_dir/qemu_output.log" 2>/dev/null; then
      echo "  ✓ '$pass_string' seen in qemu_output.log — stopping emulator pipeline."
      early_pass=true
      break
    fi
    if ! kill -0 "-$emu_pgid" 2>/dev/null; then
      echo "  Emulator (PGID $emu_pgid) exited before pass detection."
      break
    fi
    elapsed=$(( $(date +%s) - poll_start ))
    if [[ -n "$quiet" && "$quiet" != "0" ]]; then
      if [[ $((elapsed - last_status)) -ge 60 ]]; then
        echo "  ... still running ${elapsed}s — waiting for '$pass_string' (log: $qemu_log)"
        last_status=$elapsed
      fi
    elif [[ $((elapsed - last_status)) -ge 120 ]]; then
      echo "  ... still running ${elapsed}s — waiting for '$pass_string' or pipeline exit"
      last_status=$elapsed
    fi
  done

  stop_emulator
  wait "$emu_pid" 2>/dev/null || true

  if [[ -f "$work_dir/qemu_output.log" ]]; then
    cat "$work_dir/qemu_output.log" >>"$qemu_log"
  fi

  qemu_log_stage 5 5 "Verify — search for '$pass_string'"
  if [[ "$early_pass" == true ]]; then
    success=true
  elif [[ -f "$qemu_log" ]] && grep -q "$pass_string" "$qemu_log" 2>/dev/null; then
    success=true
  elif [[ -f "$work_dir/qemu_output.log" ]] && grep -q "$pass_string" "$work_dir/qemu_output.log" 2>/dev/null; then
    success=true
  fi

  if [[ -n "$log_tail" && -f "$qemu_log" ]]; then
    tail -n 200 "$qemu_log" >"$log_tail" 2>/dev/null || true
    echo "  Wrote tail log: $log_tail"
  fi

  if [[ "$success" == true ]]; then
    rm -f "$qemu_log"
    return 0
  fi

  echo "[qemu_hw_emu] FAIL: '$pass_string' not found in hw_emu output" >&2
  if [[ -f "$qemu_log" ]]; then
    echo "[qemu_hw_emu] Last 100 lines of $qemu_log:" >&2
    tail -n 100 "$qemu_log" >&2 || true
  fi
  return 1
}
