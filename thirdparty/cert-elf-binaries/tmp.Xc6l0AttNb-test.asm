;
; Code
;

;
; This is for POC
; the elf will have 8 pages
;   main control code : page0 - page3
;   save control code with its own routing: page4
;   restore control code with its own routing: page5 - page6
;   pdi control code: page7
;
;   preemption_checkpoint opcode is on page2
;   load_pdi opcode in main control code is on page0
;   load_last_pdi opcode in restore control code is on page6 
;
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
; enqueue bd0 0_0 to mm2s0
  WRITE_32            0x018554, 0x80000000
; enqueue bd0 1_1 to s2mm0
  WRITE_32            0x2109E04, 0x80000000
  WAIT_TCTS           TILE_0_0, SHIM_MM2S_0, 1 
  WAIT_TCTS           TILE_1_1, MEM_S2MM_0, 1
; enqueue bd0 1_1 mm2s1
  WRITE_32            0x2109E6C, 0x80000000
; enqueue bd0 0_1 s2mm1
  WRITE_32            0x0109E0C, 0x80000000
  WAIT_TCTS           TILE_1_1, MEM_MM2S_1, 1 
  WAIT_TCTS           TILE_0_1, MEM_S2MM_1, 1
END_JOB

.eop
START_JOB 3
  PREEMPTION_CHECKPOINT 0, @save, @restore
END_JOB

.eop
START_JOB 4
  APPLY_OFFSET_57     @shim00_bd1, 1, 1
  uC_DMA_WRITE_DES    $r0, @uc_dma_bd1
  WAIT_uC_DMA         $r0
  LOCAL_BARRIER       $lb1, 2
END_JOB

START_JOB 5
  LOCAL_BARRIER       $lb1, 2
; enqueue bd0 0_1 mm2s1
  WRITE_32            0x0109E6C, 0x80000000
; enqueue bd0 1_1 s2mm1
  WRITE_32            0x2109E0C, 0x80000000
  WAIT_TCTS           TILE_0_1, MEM_MM2S_1, 1 
  WAIT_TCTS           TILE_1_1, MEM_S2MM_1, 1
; enqueue bd0 1_1 to mm2s0
  WRITE_32            0x2109E64, 0x80000000
; enqueue bd1 0_0 to s2mm0
  WRITE_32            0x018544, 0x80000001
  WAIT_TCTS           TILE_1_1, MEM_MM2S_0, 1
  WAIT_TCTS           TILE_0_0, SHIM_S2MM_0, 1
END_JOB
save:
.include single_save.asm
.endl save
restore:
.include single_restore.asm
.endl restore
pdi:
.include single_pdi.asm
.endl pdi
EOF

;
; Data
;

  .align             16
uc_dma_bd0:
  UC_DMA_BD         0, 0x00018000, @shim00_bd0, 9, 0, 1
  UC_DMA_BD         0, 0x02108000, @mem11_s2mm0_bd0, 11, 0, 1
  UC_DMA_BD         0, 0x02108F00, @mem11_mm2s1_bd0, 11, 0, 1
  UC_DMA_BD         0, 0x00108300, @mem01_s2mm1_bd0, 11, 0, 0
uc_dma_bd1:
  UC_DMA_BD         0, 0x00018030, @shim00_bd1, 9, 0, 1
  UC_DMA_BD         0, 0x02108C00, @mem11_mm2s0_bd0, 11, 0, 1
  UC_DMA_BD         0, 0x02108300, @mem11_s2mm1_bd0, 11, 0, 1
  UC_DMA_BD         0, 0x00108F00, @mem01_mm2s1_bd0, 11, 0, 0

  .align             4
shim00_bd0:
  .long              0x00000000
  .long              0x00000000
  .long              0x00000001
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
shim00_bd1:
  .long              0x00000000
  .long              0x00000000
  .long              0x00000001
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000

mem11_s2mm0_bd0:
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
mem11_mm2s0_bd0:
  .long              0x00800001
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
mem11_mm2s1_bd0:
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
  .long              0x00800001
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
mem01_s2mm1_bd0:
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

.section .ctrlbss
; 1kB buffer for memtile
.setpad memtile_buf, 0x100

