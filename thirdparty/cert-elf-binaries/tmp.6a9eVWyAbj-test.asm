;
; Code
;
START_JOB 0
  LOAD_PDI 0, @pdi
END_JOB
.eop

START_JOB 1
  WRITE_32            0x200000, 0xabcdabcd
  uC_DMA_WRITE_DES    $r0, @uc_dma_bd0
  WAIT_uC_DMA         $r0
  LOCAL_BARRIER       $lb0, 2
END_JOB

START_JOB 2
  LOCAL_BARRIER       $lb0, 2
; enqueue bd0 0_1 to mm2s1
  WRITE_32            0x0109E6C, 0x80000000
; enqueue bd0 1_1 to s2mm1
  WRITE_32            0x2109E0C, 0x80000000
; wait tcts
  WAIT_TCTS           TILE_0_1, MEM_MM2S_1, 1
  WAIT_TCTS           TILE_1_1, MEM_S2MM_1, 1
  POLL_32             0x2200000, 0xabcdabcd
END_JOB

.include aie4_pdi.asm
EOF

;
; Data
;

  .ALIGN             16
uc_dma_bd0:
; configure mm2s1 bd0 0_1
  UC_DMA_BD         0, 0x00108F00, @mem01_mm2s1_bd0, 11, 0, 1
; configure s2mm1 bd0 1_1
  UC_DMA_BD         0, 0x02108300, @mem11_s2mm1_bd0, 11, 0, 0

  .ALIGN             4
mem01_mm2s1_bd0:
  .long              0x00800000
  .long              0x00000001
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
mem11_s2mm1_bd0:
  .long              0x00800000
  .long              0x00000001
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
