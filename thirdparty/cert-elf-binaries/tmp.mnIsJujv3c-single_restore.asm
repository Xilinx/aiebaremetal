;
; Code
;
START_JOB 21
  UC_DMA_WRITE_DES_SYNC @UCBD_label_0
  APPLY_OFFSET_57     @shim00_bd_restore, 1, 0xFFFF
  uC_DMA_WRITE_DES    $r0, @uc_dma_bd_restore
  WAIT_uC_DMA         $r0
  LOCAL_BARRIER       $lb3, 2
END_JOB

START_JOB 22
  LOCAL_BARRIER       $lb3, 2
; enqueue bd0 0_0 to mm2s0
  WRITE_32            0x018554, 0x80000000
; enqueue bd 0_1 to s2mm0
  WRITE_32            0x0109E04, 0x80000000
  WAIT_TCTS           TILE_0_0, SHIM_MM2S_0, 1 
  WAIT_TCTS           TILE_0_1, MEM_S2MM_0, 1
END_JOB
.eop
;
; always needs a load_pdi in restore, and this load_pdi will not skip
;
START_JOB 20
  LOAD_LAST_PDI
END_JOB

EOF

;
;data
;
.align    16
UCBD_label_0:
;
;restore
;
;0_0 unicast from dma0 to north0
    UC_DMA_BD    0, 0x8040, @WRITE_data_0, 1, 0, 1
    UC_DMA_BD    0, 0x8020, @WRITE_data_1, 1, 0, 1
;0_1 unicast from south0 to dma0
    UC_DMA_BD    0, 0x12005c, @WRITE_data_2, 1, 0, 1
    UC_DMA_BD    0, 0x120000, @WRITE_data_3, 1, 0, 1
;
;save
;
;0_0 unicast from north0 to dma0
    UC_DMA_BD    0, 0x8064, @WRITE_data_4, 1, 0, 1
    UC_DMA_BD    0, 0x8000, @WRITE_data_5, 1, 0, 1
;0_1 unicast from dma0 to south0
    UC_DMA_BD    0, 0x120040, @WRITE_data_6, 1, 0, 1
    UC_DMA_BD    0, 0x120014, @WRITE_data_7, 1, 0, 1
;
;tct
;
;0_0 packet routing from north0 and tile_ctrl 0
    UC_DMA_BD    0, 0x81c0, @WRITE_data_8, 1, 0, 1
    UC_DMA_BD    0, 0x8210, @WRITE_data_9, 1, 0, 1
    UC_DMA_BD    0, 0x81b0, @WRITE_data_10, 1, 0, 1
    UC_DMA_BD    0, 0x81d0, @WRITE_data_11, 1, 0, 1
    UC_DMA_BD    0, 0x8190, @WRITE_data_12, 1, 0, 1
;0_1 circuit routing from tile_ctrl 0 to south0
    UC_DMA_BD    0, 0x1201c0, @WRITE_data_13, 1, 0, 1
    UC_DMA_BD    0, 0x1201a0, @WRITE_data_14, 1, 0, 0
.align    4
WRITE_data_0:
    .long 0x80000000
WRITE_data_1:
    .long 0x80000000
WRITE_data_2:
    .long 0x80000000
WRITE_data_3:
    .long 0x8000000e
WRITE_data_4:
    .long 0x80000000
WRITE_data_5:
    .long 0x80000012
WRITE_data_6:
    .long 0x80000000
WRITE_data_7:
    .long 0x80000000
WRITE_data_8:
    .long 0xc0000000
WRITE_data_9:
    .long 0x00010100
WRITE_data_10:
    .long 0xc0000000
WRITE_data_11:
    .long 0x00010100
WRITE_data_12:
    .long 0xc0000078
WRITE_data_13:
    .long 0x80000000
WRITE_data_14:
    .long 0x80000000

;
; Data
;

  .align             16
uc_dma_bd_restore:
  UC_DMA_BD         0, 0x00018000, @shim00_bd_restore, 9, 0, 1, @memtile_buf
  UC_DMA_BD         0, 0x00108000, @mem01_bd_restore, 11, 0, 0

  .align             4
shim00_bd_restore:
  .long              0x00000000
  .long              0x00000000
  .long              0x00000001
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000
  .long              0x00000000

mem01_bd_restore:
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
