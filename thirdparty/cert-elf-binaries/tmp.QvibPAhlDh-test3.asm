;
; Code
;

.attach_to_group 3

START_JOB 0
  LOAD_PDI 0, @pdi3
END_JOB
.eop

START_JOB 1
  NOP
END_JOB

.include pdi3.asm
EOF
