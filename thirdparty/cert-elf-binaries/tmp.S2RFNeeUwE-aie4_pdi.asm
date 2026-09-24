;
; Code
;
pdi:
START_JOB 4
  uC_DMA_WRITE_DES_SYNC @uc_dma_bd_pdi
END_JOB

EOF
.endl pdi
;
; Data
;

.align             16
uc_dma_bd_pdi:
;
; tct 1_1
;
; slave tile ctl 32b for tct 1_1
  UC_DMA_BD    0, 0x021201C0, @WRITE_label_1, 1, 0, 1
; master south 32b from tile ctl 1_1 for tct 1_1
  UC_DMA_BD    0, 0x021201A0, @WRITE_label_0, 1, 0, 1
; slave north 32b 1_0 for tct 1_1
  UC_DMA_BD    0, 0x020081C0, @WRITE_label_3, 1, 0, 1
; master west 32b 1_0 from slave north for tct 1_1
  UC_DMA_BD    0, 0x02008194, @WRITE_label_2, 1, 0, 1
; slave east 32b 0_0 packet routing for tct of 1_1
  UC_DMA_BD    0, 0x000081C4, @WRITE_label_7, 1, 0, 1
; slave east 32b slot0 0_0 for tct 1_1. packet routing for master 0 arbiter 0
  UC_DMA_BD    0, 0x00008220, @WRITE_label_4, 1, 0, 1
;
; tct 0_1
;
; slave tile ctl 32b for tct 0_1
  UC_DMA_BD    0, 0x01201C0, @WRITE_label_19, 1, 0, 1
; master south 32b from tile ctl 0_1 for tct 0_1
  UC_DMA_BD    0, 0x01201A0, @WRITE_label_20, 1, 0, 1
; slave north 32b 0_0 packet routing for tct of 0_1 
  UC_DMA_BD    0, 0x000081C0, @WRITE_label_8, 1, 0, 1
; slave north 32b slot0 0_0 for tct 0_1. packet routing for master 0 arbiter 0
  UC_DMA_BD    0, 0x00008210, @WRITE_label_5, 1, 0, 1
;
; tct 0_0
;
; slave tile ctl 32b for tct 0_0. packet routing for tct of 0_0
  UC_DMA_BD    0, 0x000081B0, @WRITE_label_9, 1, 0, 1
; slave tile ctl 32b slot0 0_0 for tct 0_0. packet routing for master 0 arbiter 0
  UC_DMA_BD    0, 0x000081D0, @WRITE_label_6, 1, 0, 1
;
; tcts to uC 
;
; master of uc 32b 0_0 packet routing for all master arbiter 0
  UC_DMA_BD    0, 0x00008190, @WRITE_label_10, 1, 0, 1
;
; input
;
; slave dma0 (mm2s) 0_0 for input
  UC_DMA_BD    0, 0x00008040, @WRITE_label_12, 1, 0, 1
; master east0 0_0 from slave dma0 for input
  UC_DMA_BD    0, 0x0000802C, @WRITE_label_11, 1, 0, 1
; slave west0 1_0 for input
  UC_DMA_BD    0, 0x0200805C, @WRITE_label_14, 1, 0, 1
; master north0 1_0 from slave west0 for input
  UC_DMA_BD    0, 0x02008020, @WRITE_label_13, 1, 0, 1
; slave south0 1_1 for input
  UC_DMA_BD    0, 0x0212005C, @WRITE_label_16, 1, 0, 1
; master dma0 (s2mm) 1_1 from slave south0 for input
  UC_DMA_BD    0, 0x02120000, @WRITE_label_15, 1, 0, 1
;
; output
;
; slave dma0 (mm2s) 1_1 for output
  UC_DMA_BD    0, 0x02120040, @WRITE_label_21, 1, 0, 1
; master south0 from mm2s0 1_1 for output
  UC_DMA_BD    0, 0x02120014, @WRITE_label_22, 1, 0, 1
; slave north0 1_0 for output
  UC_DMA_BD    0, 0x02008064, @WRITE_label_23, 1, 0, 1
; master west0 1_0 from slave north0 for output
  UC_DMA_BD    0, 0x02008018, @WRITE_label_24, 1, 0, 1
; slave east0 0_0 for output
  UC_DMA_BD    0, 0x0000806C, @WRITE_label_25, 1, 0, 1
; master dma0 (s2mm) 0_0 from slave east0 for output
  UC_DMA_BD    0, 0x00008000, @WRITE_label_26, 1, 0, 1
;
; data from 1_1 to 0_1 
;
; slave dma1 (mm2s) 1_1 for data 1->0
  UC_DMA_BD    0, 0x02120044, @WRITE_label_27, 1, 0, 1
; master south1 from mm2s1 1_1 for data 1->0
  UC_DMA_BD    0, 0x02120018, @WRITE_label_28, 1, 0, 1
; slave north1 1_0 for data 1->0
  UC_DMA_BD    0, 0x02008068, @WRITE_label_29, 1, 0, 1
; master west1 1_0 from slave north1 for data 1->0
  UC_DMA_BD    0, 0x0200801C, @WRITE_label_30, 1, 0, 1
; slave east1 0_0 for data 1->0
  UC_DMA_BD    0, 0x00008070, @WRITE_label_31, 1, 0, 1
; master north1 0_0 from slave east1 for data 1->0
  UC_DMA_BD    0, 0x00008024, @WRITE_label_32, 1, 0, 1
; slave south1 0_1 for data 1->0
  UC_DMA_BD    0, 0x00120060, @WRITE_label_33, 1, 0, 1
; master dma1 (s2mm) 0_1 from slave south1 for data 1->0
  UC_DMA_BD    0, 0x00120004, @WRITE_label_34, 1, 0, 1
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
WRITE_label_0:
  .long 0x80000000
WRITE_label_1:
  .long 0x80000000
WRITE_label_2:
  .long 0x80000007
WRITE_label_3:
  .long 0x80000000
WRITE_label_4:
  .long 0x00010100
WRITE_label_5:
  .long 0x00010100
WRITE_label_6:
  .long 0x00010100
WRITE_label_7:
  .long 0xC0000000
WRITE_label_8:
  .long 0xC0000000
WRITE_label_9:
  .long 0xC0000000
WRITE_label_10:
  .long 0xC0000078
WRITE_label_11:
  .long 0x80000000
WRITE_label_12:
  .long 0x80000000
WRITE_label_13:
  .long 0x8000000E
WRITE_label_14:
  .long 0x80000000
WRITE_label_15:
  .long 0x8000000E
WRITE_label_16:
  .long 0x80000000
WRITE_label_17:
  .long 0x00000400
WRITE_label_18:
  .long 0x00000004
WRITE_label_19:
  .long 0x80000000
WRITE_label_20:
  .long 0x80000000
WRITE_label_21:
  .long 0x80000000
WRITE_label_22:
  .long 0x80000000
WRITE_label_23:
  .long 0x80000000
WRITE_label_24:
  .long 0x80000012
WRITE_label_25:
  .long 0x80000000
WRITE_label_26:
  .long 0x80000016
WRITE_label_27:
  .long 0x80000000
WRITE_label_28:
  .long 0x80000001
WRITE_label_29:
  .long 0x80000000
WRITE_label_30:
  .long 0x80000013
WRITE_label_31:
  .long 0x80000000
WRITE_label_32:
  .long 0x80000017
WRITE_label_33:
  .long 0x80000000
WRITE_label_34:
  .long 0x8000000F
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
