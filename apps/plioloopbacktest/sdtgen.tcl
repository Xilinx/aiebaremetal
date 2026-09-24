# Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
# SPDX-License-Identifier: Apache-2.0

 for { set i 0 } { $i < $argc } { incr i } {
  # xsa path
  if { [lindex $argv $i] == "-xsa_path" } {
    incr i
    set xsa_path [lindex $argv $i]
  }
  # SDT path
  if { [lindex $argv $i] == "-sdt_path" } {
    incr i
    set sdt_path [lindex $argv $i]
  }
  #board dts name
  if { [lindex $argv $i] == "-board_dts" } {
    incr i
    set board_dts [lindex $argv $i]
  }
}

set_dt_param -debug enable
set_dt_param -dir $sdt_path -zocl "enable"
set_dt_param -xsa $xsa_path
set_dt_param -board_dts $board_dts
generate_sdt
