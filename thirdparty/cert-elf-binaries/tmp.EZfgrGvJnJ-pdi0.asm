;
; Code
;
pdi0:
START_JOB 10
  uC_DMA_WRITE_DES_SYNC @uc_dma_pdi_bd0
END_JOB

EOF
.endl pdi0

;
; Data
;

  .ALIGN             16
uc_dma_pdi_bd0:
  UC_DMA_BD    0, 0x021B0024, @WRITE_label_0, 1, 0, 1
  UC_DMA_BD    0, 0x021B0118, @WRITE_label_1, 1, 0, 1
  UC_DMA_BD    0, 0x0203F200, @WRITE_label_2, 1, 0, 1
  UC_DMA_BD    0, 0x0203F300, @WRITE_label_3, 1, 0, 1
  UC_DMA_BD    0, 0x0203F140, @WRITE_label_4, 1, 0, 1
  UC_DMA_BD    0, 0x0203F100, @WRITE_label_5, 1, 0, 1
  UC_DMA_BD    0, 0x0203F058, @WRITE_label_6, 1, 0, 1
  UC_DMA_BD    0, 0x001B0024, @WRITE_label_7, 1, 0, 1
  UC_DMA_BD    0, 0x001B0118, @WRITE_label_8, 1, 0, 1
  UC_DMA_BD    0, 0x0003F200, @WRITE_label_9, 1, 0, 1
  UC_DMA_BD    0, 0x0003F300, @WRITE_label_10, 1, 0, 1
  UC_DMA_BD    0, 0x0003F140, @WRITE_label_11, 1, 0, 1
  UC_DMA_BD    0, 0x0003F100, @WRITE_label_12, 1, 0, 1
  UC_DMA_BD    0, 0x0003F058, @WRITE_label_13, 1, 0, 1
  UC_DMA_BD    0, 0x001B0020, @WRITE_label_14, 1, 0, 1
  UC_DMA_BD    0, 0x001B0100, @WRITE_label_15, 1, 0, 1
  UC_DMA_BD    0, 0x0003F04C, @WRITE_label_16, 1, 0, 1
  UC_DMA_BD    0, 0x0003F13C, @WRITE_label_17, 1, 0, 1
  UC_DMA_BD    0, 0x0203F034, @WRITE_label_18, 1, 0, 1
  UC_DMA_BD    0, 0x0203F12C, @WRITE_label_19, 1, 0, 1
  UC_DMA_BD    0, 0x021B0000, @WRITE_label_20, 1, 0, 1
  UC_DMA_BD    0, 0x021B0120, @WRITE_label_21, 1, 0, 0

  .ALIGN             4
WRITE_label_0:
  .long 0x80000006
WRITE_label_1:
  .long 0x80000000
WRITE_label_2:
  .long 0x00010100
WRITE_label_3:
  .long 0x00010100
WRITE_label_4:
  .long 0xC0000000
WRITE_label_5:
  .long 0xC0000000
WRITE_label_6:
  .long 0xC0000078
WRITE_label_7:
  .long 0x80000006
WRITE_label_8:
  .long 0x80000000
WRITE_label_9:
  .long 0x00010100
WRITE_label_10:
  .long 0x00010100
WRITE_label_11:
  .long 0xC0000000
WRITE_label_12:
  .long 0xC0000000
WRITE_label_13:
  .long 0xC0000078
WRITE_label_14:
  .long 0x80000000
WRITE_label_15:
  .long 0x80000000
WRITE_label_16:
  .long 0x8000000F
WRITE_label_17:
  .long 0x80000000
WRITE_label_18:
  .long 0x8000000B
WRITE_label_19:
  .long 0x80000000
WRITE_label_20:
  .long 0x80000008
WRITE_label_21:
  .long 0x80000000
