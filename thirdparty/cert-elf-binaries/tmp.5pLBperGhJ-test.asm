;
;text
;
START_JOB 0
LOAD_PDI 0, @pdi
END_JOB
.eop

START_JOB 1
UC_DMA_WRITE_DES_SYNC	 @UCBD_label_0
APPLY_OFFSET_57	 @DMAWRITE_data_0, 1, 1
UC_DMA_WRITE_DES_SYNC	 @UCBD_label_1
APPLY_OFFSET_57	 @DMAWRITE_data_2, 1, 0
UC_DMA_WRITE_DES_SYNC	 @UCBD_label_2
MASK_POLL_32	 0x4118604, 0x1, 0x1
MASK_POLL_32	 0x118604, 0x1, 0x1
UC_DMA_WRITE_DES_SYNC	 @UCBD_label_3
MASK_POLL_32	 0x118e04, 0x1, 0x1
UC_DMA_WRITE_DES_SYNC	 @UCBD_label_4
MASK_POLL_32	 0x2118604, 0x1, 0x1
UC_DMA_WRITE_DES_SYNC	 @UCBD_label_5
APPLY_OFFSET_57	 @DMAWRITE_data_11, 1, 2
UC_DMA_WRITE_DES_SYNC	 @UCBD_label_6
MASK_POLL_32	 0x14604, 0x1, 0x1
END_JOB

.eop
pdi:
.include pdi.asm
.endl pdi

EOF

;
;data
;
.align    16
UCBD_label_0:
	 UC_DMA_BD	 0, 0x10000, @WRITE_data_0, 1, 0, 1
	 UC_DMA_BD	 0, 0x10010, @WRITE_data_1, 1, 0, 1
	 UC_DMA_BD	 0, 0x4110000, @WRITE_data_2, 1, 0, 1
	 UC_DMA_BD	 0, 0x4110010, @WRITE_data_3, 1, 0, 0
UCBD_label_1:
	 UC_DMA_BD	 0, 0x18000, @DMAWRITE_data_0, 0x9, 0, 1
	 UC_DMA_BD	 0, 0x1855c, @WRITE_data_4, 1, 0, 1
	 UC_DMA_BD	 0, 0x4108000, @DMAWRITE_data_1, 0xb, 0, 1
	 UC_DMA_BD	 0, 0x4109e04, @WRITE_data_5, 1, 0, 1
	 UC_DMA_BD	 0, 0x10020, @WRITE_data_6, 1, 0, 1
	 UC_DMA_BD	 0, 0x10030, @WRITE_data_7, 1, 0, 1
	 UC_DMA_BD	 0, 0x110000, @WRITE_data_8, 1, 0, 1
	 UC_DMA_BD	 0, 0x110010, @WRITE_data_9, 1, 0, 0
UCBD_label_2:
	 UC_DMA_BD	 0, 0x18030, @DMAWRITE_data_2, 0x9, 0, 1
	 UC_DMA_BD	 0, 0x18554, @WRITE_data_10, 1, 0, 1
	 UC_DMA_BD	 0, 0x108000, @DMAWRITE_data_3, 0xb, 0, 1
	 UC_DMA_BD	 0, 0x109e04, @WRITE_data_11, 1, 0, 0
UCBD_label_3:
	 UC_DMA_BD	 0, 0x110020, @WRITE_data_12, 1, 0, 1
	 UC_DMA_BD	 0, 0x110030, @WRITE_data_13, 1, 0, 1
	 UC_DMA_BD	 0, 0x108f00, @DMAWRITE_data_4, 0xb, 0, 1
	 UC_DMA_BD	 0, 0x109e6c, @WRITE_data_14, 1, 0, 1
	 UC_DMA_BD	 0, 0x4108c00, @DMAWRITE_data_5, 0xb, 0, 1
	 UC_DMA_BD	 0, 0x4109e64, @WRITE_data_15, 1, 0, 1
	 UC_DMA_BD	 0, 0x108300, @DMAWRITE_data_6, 0xb, 0, 1
	 UC_DMA_BD	 0, 0x109e0c, @WRITE_data_16, 1, 0, 0
UCBD_label_4:
	 UC_DMA_BD	 0, 0x2110000, @WRITE_data_17, 1, 0, 1
	 UC_DMA_BD	 0, 0x2110010, @WRITE_data_18, 1, 0, 1
	 UC_DMA_BD	 0, 0x4108f00, @DMAWRITE_data_7, 0xb, 0, 1
	 UC_DMA_BD	 0, 0x4109e6c, @WRITE_data_19, 1, 0, 1
	 UC_DMA_BD	 0, 0x108c00, @DMAWRITE_data_8, 0xb, 0, 1
	 UC_DMA_BD	 0, 0x109e64, @WRITE_data_20, 1, 0, 1
	 UC_DMA_BD	 0, 0x2108000, @DMAWRITE_data_9, 0xb, 0, 1
	 UC_DMA_BD	 0, 0x2109e04, @WRITE_data_21, 1, 0, 0
UCBD_label_5:
	 UC_DMA_BD	 0, 0x10000, @WRITE_data_22, 1, 0, 1
	 UC_DMA_BD	 0, 0x10010, @WRITE_data_23, 1, 0, 1
	 UC_DMA_BD	 0, 0x2108f00, @DMAWRITE_data_10, 0xb, 0, 1
	 UC_DMA_BD	 0, 0x2109e6c, @WRITE_data_24, 1, 0, 0
UCBD_label_6:
	 UC_DMA_BD	 0, 0x18000, @DMAWRITE_data_11, 0x9, 0, 1
	 UC_DMA_BD	 0, 0x18544, @WRITE_data_25, 1, 0, 0
.align    4
WRITE_data_0:
	.long 0x00000000
WRITE_data_1:
	.long 0x00000000
WRITE_data_2:
	.long 0x00000000
WRITE_data_3:
	.long 0x00000000
WRITE_data_4:
	.long 0x00000000
WRITE_data_5:
	.long 0x00000000
WRITE_data_6:
	.long 0x00000000
WRITE_data_7:
	.long 0x00000000
WRITE_data_8:
	.long 0x00000000
WRITE_data_9:
	.long 0x00000000
WRITE_data_10:
	.long 0x00000001
WRITE_data_11:
	.long 0x00000000
WRITE_data_12:
	.long 0x00000000
WRITE_data_13:
	.long 0x00000000
WRITE_data_14:
	.long 0x00000000
WRITE_data_15:
	.long 0x00000000
WRITE_data_16:
	.long 0x00000000
WRITE_data_17:
	.long 0x00000000
WRITE_data_18:
	.long 0x00000000
WRITE_data_19:
	.long 0x00000000
WRITE_data_20:
	.long 0x00000000
WRITE_data_21:
	.long 0x00000000
WRITE_data_22:
	.long 0x00000000
WRITE_data_23:
	.long 0x00000000
WRITE_data_24:
	.long 0x00000000
WRITE_data_25:
	.long 0x00000000
DMAWRITE_data_0:
	.long 0x00000000
	.long 0x00000000
	.long 0x00000010
	.long 0x0f000000
	.long 0x00000000
	.long 0x00000001
	.long 0x00000001
	.long 0x20000001
	.long 0x00000400
DMAWRITE_data_1:
	.long 0x00800000
	.long 0x00000010
	.long 0x1c000b83
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
DMAWRITE_data_2:
	.long 0x00000000
	.long 0x00000000
	.long 0x00000020
	.long 0x1f000000
	.long 0x00000100
	.long 0x00000001
	.long 0x00000001
	.long 0x20000001
	.long 0x00000400
DMAWRITE_data_3:
	.long 0x00800000
	.long 0x00000020
	.long 0x1c000b83
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
DMAWRITE_data_4:
	.long 0x00800000
	.long 0x02000020
	.long 0x1c100381
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
DMAWRITE_data_5:
	.long 0x00800000
	.long 0x02000010
	.long 0x1c100381
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
DMAWRITE_data_6:
	.long 0x00800400
	.long 0x00000020
	.long 0x1c200b87
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
DMAWRITE_data_7:
	.long 0x00800000
	.long 0x02000010
	.long 0x1c100381
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
DMAWRITE_data_8:
	.long 0x00800400
	.long 0x02000020
	.long 0x1c300385
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
DMAWRITE_data_9:
	.long 0x00800000
	.long 0x00000020
	.long 0x1c000b83
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
DMAWRITE_data_10:
	.long 0x00800000
	.long 0x02000020
	.long 0x1c100381
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000001
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
	.long 0x00000000
DMAWRITE_data_11:
	.long 0x00000000
	.long 0x00000000
	.long 0x00000020
	.long 0x07000000
	.long 0x00001080
	.long 0x00000001
	.long 0x00000001
	.long 0x20000001
	.long 0x00000400
