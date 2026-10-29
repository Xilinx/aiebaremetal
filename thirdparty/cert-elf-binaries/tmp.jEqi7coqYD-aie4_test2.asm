; Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
; SPDX-License-Identifier: Apache-2.0
;
; Code
;

.attach_to_group 3

START_JOB 0
  LOAD_PDI 0, @pdi2
END_JOB
.eop

START_JOB 1
  REMOTE_BARRIER      $rb0, 0xC
  READ_32             $r0, 0x2200000
  WRITE_32_D          2, 0x4200000, 0
END_JOB

.include aie4_pdi2.asm
EOF
