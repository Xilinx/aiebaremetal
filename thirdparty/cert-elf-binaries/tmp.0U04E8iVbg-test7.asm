;
; Code
;

.attach_to_group 7

START_JOB 0
  LOAD_PDI 0, @pdi7
END_JOB
.eop

START_JOB 1
  NOP
END_JOB

.include pdi7.asm
EOF
