; Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
; SPDX-License-Identifier: Apache-2.0
;
; Code
;

.attach_to_group 6

START_JOB 0
  LOAD_PDI 0, @pdi6
END_JOB
.eop

START_JOB 1
  NOP
END_JOB

.include pdi6.asm
EOF
