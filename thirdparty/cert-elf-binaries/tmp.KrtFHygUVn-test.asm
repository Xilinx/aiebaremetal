;
; Code
;

START_JOB 0
  LOAD_PDI 0, @pdi
END_JOB
.eop

START_JOB 1
  APPLY_OFFSET_57     @shim00_bd0, 1, 0
  uC_DMA_WRITE_DES    $r0, @uc_dma_bd0
  WAIT_uC_DMA         $r0
  LOCAL_BARRIER       $lb0, 2
END_JOB

START_JOB 2
  LOCAL_BARRIER       $lb0, 2
; enqueue bd0 0_0
  WRITE_32            0x009314, 0x80000000
; enqueue bd0 1_1
  WRITE_32            0x21A0604, 0x80000000
; wait tct
  WAIT_TCTS           TILE_0_0, SHIM_MM2S_0, 1
  WAIT_TCTS           TILE_1_1, MEM_S2MM_0, 1
END_JOB

.include pdi.asm
EOF

;
; Data
;

  .ALIGN             16
uc_dma_bd0:
  UC_DMA_BD         0, 0x00009000, @shim00_bd0, 9, 0, 1
  UC_DMA_BD         0, 0x021A0000, @mem11_bd0, 8, 0, 0

  .ALIGN             4
shim00_bd0:
  .long              0x00000002
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x02000000
  .long              0x00000000

  .ALIGN             4
mem11_bd0:
  .long              0x00000002
  .long              0x00020000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x80000000
