;
; Code
;

.attach_to_group 1

START_JOB 0
  LOAD_PDI 0, @pdi1
END_JOB
.eop

START_JOB 1
  uC_DMA_WRITE_DES    $r0, @uc_dma_bd0
  WAIT_uC_DMA         $r0
  LOCAL_BARRIER       $lb0, 2
END_JOB

START_JOB 2
  LOCAL_BARRIER       $lb0, 2
  WRITE_32            0x21A0604, 0x80000000
  WAIT_TCTS           TILE_1_1, S2MM_0, 1
  REMOTE_BARRIER      $rb0, 0x6
END_JOB

.include pdi1.asm
EOF

;
; Data
;

  .ALIGN             16
uc_dma_bd0:
  UC_DMA_BD         0, 0x021A0000, @mem31_bd0, 8, 0, 0

  .ALIGN             4
mem31_bd0:
  .long              0x00000001
  .long              0x00020000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x80000000
