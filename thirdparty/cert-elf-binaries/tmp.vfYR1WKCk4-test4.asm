;
; Code
;

.attach_to_group 4

START_JOB 0
  LOAD_PDI 0, @pdi4
END_JOB
.eop

START_JOB 1
  NOP
END_JOB

.include pdi4.asm
EOF
