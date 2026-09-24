;
; Code
;
START_JOB 0
  LOAD_PDI 0, @pdi
END_JOB
.eop

START_JOB 1
  WRITE_32            0x100000, 0xabcdabcd
  uC_DMA_WRITE_DES    $r0, @uc_dma_bd0
  WAIT_uC_DMA         $r0
  LOCAL_BARRIER       $lb0, 2
END_JOB

START_JOB 2
  LOCAL_BARRIER       $lb0, 2
; enqueue bd1 0_1
  WRITE_32            0x01A0634, 0x80000001
; enqueue bd1 1_1
  WRITE_32            0x21A0604, 0x80000001
; wait 2 tcts, one is enqueued through uc_dma, the other is through this job
  WAIT_TCTS           TILE_0_1, MEM_MM2S_0, 2
  WAIT_TCTS           TILE_1_1, MEM_S2MM_0, 2
  POLL_32             0x2100000, 0xabcdabcd
END_JOB

.include pdi.asm 
EOF

;
; Data
;

  .ALIGN             16
uc_dma_bd0:
; configure bd0 0_1
  UC_DMA_BD         0, 0x001A0000, @mem01_bd0, 8, 0, 1
; configure bd1 0_1
  UC_DMA_BD         0, 0x001A0020, @mem01_bd1, 8, 0, 1
; enqueue bd0 0_1
  UC_DMA_BD         0, 0x001A0634, @mem01_data0, 1, 0, 1
; configure bd0 1_1
  UC_DMA_BD         0, 0x021A0000, @mem11_bd0, 8, 0, 1
; configure bd1 1_1
  UC_DMA_BD         0, 0x021A0020, @mem11_bd1, 8, 0, 1
; enqueue bd0 1_1
  UC_DMA_BD         0, 0x021A0604, @mem11_data0, 1, 0, 0

  .ALIGN             4
mem01_bd0:
  .long              0x00000001
  .long              0x00020000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x80000000
mem01_data0:
  .long              0x80000000
mem01_bd1:
  .long              0x00000001
  .long              0x00020001
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x80000000

mem11_bd0:
  .long              0x00000001
  .long              0x00020000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x80000000
mem11_data0:
  .long              0x80000000
mem11_bd1:
  .long              0x00000001
  .long              0x00020001
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x80000000
