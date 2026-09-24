;
; Code
;

.attach_to_group 2

START_JOB 0
  LOAD_PDI 0, @pdi2
END_JOB
.eop

START_JOB 1
  REMOTE_BARRIER      $rb0, 0x6
  READ_32             $r0, 0x2100000
  WRITE_32_D          2, 0x4100000, 0
END_JOB

.include pdi2.asm
EOF
