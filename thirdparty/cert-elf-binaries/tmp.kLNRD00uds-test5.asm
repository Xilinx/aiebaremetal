;
; Code
;

.attach_to_group 5

START_JOB 0
  LOAD_PDI 0, @pdi5
END_JOB
.eop

START_JOB 1
  NOP
END_JOB

.include pdi5.asm
EOF
