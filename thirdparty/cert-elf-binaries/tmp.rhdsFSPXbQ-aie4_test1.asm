;
; Code
;

.attach_to_group 2

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
; enqueue bd0 1_1 to s2mm1
  WRITE_32            0x2109E0C, 0x80000000
; wait tcts
  WAIT_TCTS           TILE_1_1, MEM_S2MM_1, 1
  REMOTE_BARRIER      $rb0, 0xC
END_JOB

.include aie4_pdi1.asm
EOF

;
; Data
;

  .ALIGN             16
uc_dma_bd0:
; configure s2mm1 bd0 1_1
  UC_DMA_BD         0, 0x02108300, @mem11_s2mm1_bd0, 11, 0, 0

  .ALIGN             4
mem11_s2mm1_bd0:
  .long              0x00800000
  .long              0x00000002
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
