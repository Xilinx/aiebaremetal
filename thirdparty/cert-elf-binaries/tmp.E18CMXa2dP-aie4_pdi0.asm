;
; Code
;
pdi0:
START_JOB 10
  uC_DMA_WRITE_DES_SYNC @uc_dma_bd_pdi0
END_JOB

EOF
.endl pdi0

;
; Data
;

.align             16
uc_dma_bd_pdi0:
;
; tct 0_1
;
; slave tile ctl 32b for tct 0_1
  UC_DMA_BD    0, 0x01201C0, @WRITE_label_19, 1, 0, 1
; master south 32b from tile ctl 0_1 for tct 0_1
  UC_DMA_BD    0, 0x01201A0, @WRITE_label_20, 1, 0, 1
; slave north 32b 0_0 packet routing for tct of 0_1 
  UC_DMA_BD    0, 0x000081C0, @WRITE_label_7, 1, 0, 1
; slave north 32b slot0 0_0 for tct 0_1. packet routing for master 0 arbiter 0
  UC_DMA_BD    0, 0x00008210, @WRITE_label_5, 1, 0, 1
;
; tcts to uC 0_0
;
; master of uc 32b 0_0 packet routing for all master arbiter 0
  UC_DMA_BD    0, 0x00008190, @WRITE_label_10, 1, 0, 1
;
; tct 1_1
;
; slave tile ctl 32b for tct 1_1
  UC_DMA_BD    0, 0x021201C0, @WRITE_label_21, 1, 0, 1
; master south 32b from tile ctl 0_1 for tct 1_1
  UC_DMA_BD    0, 0x021201A0, @WRITE_label_22, 1, 0, 1
; slave north 32b 0_0 packet routing for tct of 1_1 
  UC_DMA_BD    0, 0x020081C0, @WRITE_label_8, 1, 0, 1
; slave north 32b slot0 0_0 for tct 1_1. packet routing for master 0 arbiter 0
  UC_DMA_BD    0, 0x02008210, @WRITE_label_6, 1, 0, 1
;
; tcts to uC 1_0
;
; master of uc 32b 1_0 packet routing for all master arbiter 0
  UC_DMA_BD    0, 0x02008190, @WRITE_label_11, 1, 0, 1
;
; data from 0_1 to 1_1 
;
; slave dma1 (mm2s) 0_1 for data 0->1
  UC_DMA_BD    0, 0x00120044, @WRITE_label_35, 1, 0, 1
; master south1 from mm2s1 0_1 for data 0->1
  UC_DMA_BD    0, 0x00120018, @WRITE_label_36, 1, 0, 1
; slave north1 0_0 for data 0->1
  UC_DMA_BD    0, 0x00008068, @WRITE_label_37, 1, 0, 1
; master east1 0_0 from slave north1 for data 0->1
  UC_DMA_BD    0, 0x00008030, @WRITE_label_38, 1, 0, 1
; slave west1 1_0 for data 0->1
  UC_DMA_BD    0, 0x02008060, @WRITE_label_39, 1, 0, 1
; master north1 1_0 from slave west1 for data 0->1
  UC_DMA_BD    0, 0x02008024, @WRITE_label_40, 1, 0, 1
; slave south1 1_1 for data 0->1
  UC_DMA_BD    0, 0x02120060, @WRITE_label_41, 1, 0, 1
; master dma1 (s2mm) 1_1 from slave south1 for data 0->1
  UC_DMA_BD    0, 0x02120004, @WRITE_label_42, 1, 0, 0

  .align             4
WRITE_label_5:
  .long 0x00010100
WRITE_label_6:
  .long 0x00010100
WRITE_label_7:
  .long 0xC0000000
WRITE_label_8:
  .long 0xC0000000
WRITE_label_10:
  .long 0xC0000078
WRITE_label_11:
  .long 0xC0000078
WRITE_label_19:
  .long 0x80000000
WRITE_label_20:
  .long 0x80000000
WRITE_label_21:
  .long 0x80000000
WRITE_label_22:
  .long 0x80000000
WRITE_label_35:
  .long 0x80000000
WRITE_label_36:
  .long 0x80000001
WRITE_label_37:
  .long 0x80000000
WRITE_label_38:
  .long 0x80000013
WRITE_label_39:
  .long 0x80000000
WRITE_label_40:
  .long 0x8000000F
WRITE_label_41:
  .long 0x80000000
WRITE_label_42:
  .long 0x8000000F
