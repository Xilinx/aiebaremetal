;
; Code
;

START_JOB 0
  LOAD_PDI 0, @pdi
END_JOB
.eop

START_JOB 1
  WRITE_32            0x0100000, val
  uC_DMA_WRITE_DES    $r0, @uc_dma_bd0
  SAVE_TIMESTAMPS     0
  WAIT_uC_DMA         $r0
  LOCAL_BARRIER       $lb0, 2
END_JOB

START_JOB 2
  LOCAL_BARRIER       $lb0, 2
  SAVE_TIMESTAMPS     1
  WRITE_32            0x01A0634, 0x80000000
  WAIT_TCTS           TILE_0_1, MEM_MM2S_0, 1
  WRITE_32            0x21A0604, 0x80000000
  WAIT_TCTS           TILE_1_1, MEM_S2MM_0, 1
  LOCAL_BARRIER       $lb1, 9
  SAVE_TIMESTAMPS     2
END_JOB

START_JOB 3
  LOCAL_BARRIER       $lb1, 9
  SAVE_TIMESTAMPS     3
  READ_32             $r0, 0x2100000
  WRITE_32_D          2, 0x4100000, 0
  SAVE_TIMESTAMPS     4
  SAVE_TIMESTAMPS     5
  SAVE_TIMESTAMPS     6
END_JOB

START_JOB 4
  LOCAL_BARRIER       $lb1, 9
END_JOB

START_JOB 5
  LOCAL_BARRIER       $lb1, 9
END_JOB

START_JOB 6
  LOCAL_BARRIER       $lb1, 9
END_JOB

START_JOB 7
  LOCAL_BARRIER       $lb1, 9
END_JOB

START_JOB 8
  LOCAL_BARRIER       $lb1, 9
END_JOB

START_JOB 9
  LOCAL_BARRIER       $lb1, 9
END_JOB

START_JOB 10
  LOCAL_BARRIER       $lb1, 9
END_JOB

.include pdi.asm
EOF

;
; Data
;

  .ALIGN             16
uc_dma_bd0:
  UC_DMA_BD         0, 0x001A0000, @mem21_bd0, 8, 0, 1
  UC_DMA_BD         0, 0x021A0000, @mem31_bd0, 8, 0, 0

  .ALIGN             4
mem21_bd0:
  .long              0x00000001
  .long              0x00020000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x80000000

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
