;
;text
;
pdi:
START_JOB 10
    UC_DMA_WRITE_DES_SYNC @UCBD_label_0
    MASK_WRITE_32    0xc50c038, 0xff, 0x0
    UC_DMA_WRITE_DES_SYNC @UCBD_label_1
    MASK_WRITE_32    0xe50c038, 0xff, 0x0
    UC_DMA_WRITE_DES_SYNC @UCBD_label_2
    MASK_WRITE_32    0xc538000, 0x2, 0x2
    MASK_WRITE_32    0xc538000, 0x2, 0x0
    MASK_WRITE_32    0xe538000, 0x2, 0x2
    MASK_WRITE_32    0xe538000, 0x2, 0x0
    MASK_WRITE_32    0xc002104, 0xc00, 0x400
    MASK_WRITE_32    0xc002104, 0xc000, 0x4000
    MASK_WRITE_32    0xc002108, 0xc, 0x4
    MASK_WRITE_32    0xc538038, 0x1, 0x1
    MASK_WRITE_32    0xe538038, 0x1, 0x1
    MASK_WRITE_32    0xc538000, 0x1, 0x1
    MASK_WRITE_32    0xe538000, 0x1, 0x1
END_JOB


EOF
.endl pdi
;
;data
;
.align    16
UCBD_label_0:
    UC_DMA_BD    0, 0xc50c000, @DMAWRITE_data_0, 0x8, 0, 1
    UC_DMA_BD    0, 0xc50c020, @WRITE_data_0, 1, 0, 1
    UC_DMA_BD    0, 0xc50c024, @DMAWRITE_data_1, 0x5, 0, 0
UCBD_label_1:
    UC_DMA_BD    0, 0xc50c03c, @WRITE_data_1, 1, 0, 1
    UC_DMA_BD    0, 0xc50c440, @DMAWRITE_data_2, 0xb, 0, 1
    UC_DMA_BD    0, 0xc520000, @DMAWRITE_data_3, 0x133, 0, 1
    UC_DMA_BD    0, 0xc5204d0, @DMAWRITE_data_4, 0x7b, 0, 1
    UC_DMA_BD    0, 0xc5206c0, @DMAWRITE_data_5, 0x29, 0, 1
    UC_DMA_BD    0, 0xc520770, @DMAWRITE_data_6, 0x9, 0, 1
    UC_DMA_BD    0, 0xc5207a0, @DMAWRITE_data_7, 0x17, 0, 1
    UC_DMA_BD    0, 0xc520800, @DMAWRITE_data_8, 0x9, 0, 1
    UC_DMA_BD    0, 0xc520830, @DMAWRITE_data_9, 0x2b, 0, 1
    UC_DMA_BD    0, 0xe50c000, @DMAWRITE_data_10, 0x8, 0, 1
    UC_DMA_BD    0, 0xe50c020, @WRITE_data_2, 1, 0, 1
    UC_DMA_BD    0, 0xe50c024, @DMAWRITE_data_11, 0x5, 0, 0
UCBD_label_2:
    UC_DMA_BD    0, 0xe50c03c, @WRITE_data_3, 1, 0, 1
    UC_DMA_BD    0, 0xe50c440, @DMAWRITE_data_12, 0xb, 0, 1
    UC_DMA_BD    0, 0xe520000, @DMAWRITE_data_13, 0x133, 0, 1
    UC_DMA_BD    0, 0xe5204d0, @DMAWRITE_data_14, 0x7b, 0, 1
    UC_DMA_BD    0, 0xe5206c0, @DMAWRITE_data_15, 0x29, 0, 1
    UC_DMA_BD    0, 0xe520770, @DMAWRITE_data_16, 0x9, 0, 1
    UC_DMA_BD    0, 0xe5207a0, @DMAWRITE_data_17, 0x17, 0, 1
    UC_DMA_BD    0, 0xe520800, @DMAWRITE_data_18, 0x9, 0, 1
    UC_DMA_BD    0, 0xe520830, @DMAWRITE_data_19, 0x2b, 0, 1
    UC_DMA_BD    0, 0xe53f030, @WRITE_data_4, 1, 0, 1
    UC_DMA_BD    0, 0xe53f154, @WRITE_data_5, 1, 0, 1
    UC_DMA_BD    0, 0xc53f008, @WRITE_data_6, 1, 0, 1
    UC_DMA_BD    0, 0xc53f158, @WRITE_data_7, 1, 0, 1
    UC_DMA_BD    0, 0xe53f008, @WRITE_data_8, 1, 0, 1
    UC_DMA_BD    0, 0xe53f158, @WRITE_data_9, 1, 0, 1
    UC_DMA_BD    0, 0xc1b0040, @WRITE_data_10, 1, 0, 1
    UC_DMA_BD    0, 0xc1b0100, @WRITE_data_11, 1, 0, 1
    UC_DMA_BD    0, 0x101b003c, @WRITE_data_12, 1, 0, 1
    UC_DMA_BD    0, 0x101b0100, @WRITE_data_13, 1, 0, 1
    UC_DMA_BD    0, 0xc1b003c, @WRITE_data_14, 1, 0, 1
    UC_DMA_BD    0, 0xc1b0104, @WRITE_data_15, 1, 0, 1
    UC_DMA_BD    0, 0xe1b0028, @WRITE_data_16, 1, 0, 1
    UC_DMA_BD    0, 0xe1b0104, @WRITE_data_17, 1, 0, 1
    UC_DMA_BD    0, 0x101b0040, @WRITE_data_18, 1, 0, 1
    UC_DMA_BD    0, 0x101b0104, @WRITE_data_19, 1, 0, 1
    UC_DMA_BD    0, 0xc1b0004, @WRITE_data_20, 1, 0, 1
    UC_DMA_BD    0, 0xc1b0140, @WRITE_data_21, 1, 0, 1
    UC_DMA_BD    0, 0xc2b0028, @WRITE_data_22, 1, 0, 1
    UC_DMA_BD    0, 0xc2b0140, @WRITE_data_23, 1, 0, 1
    UC_DMA_BD    0, 0xc3b0028, @WRITE_data_24, 1, 0, 1
    UC_DMA_BD    0, 0xc3b0140, @WRITE_data_25, 1, 0, 1
    UC_DMA_BD    0, 0xc4b0028, @WRITE_data_26, 1, 0, 1
    UC_DMA_BD    0, 0xc4b0140, @WRITE_data_27, 1, 0, 1
    UC_DMA_BD    0, 0xe1b0000, @WRITE_data_28, 1, 0, 1
    UC_DMA_BD    0, 0xe1b0140, @WRITE_data_29, 1, 0, 1
    UC_DMA_BD    0, 0xe2b0028, @WRITE_data_30, 1, 0, 1
    UC_DMA_BD    0, 0xe2b0140, @WRITE_data_31, 1, 0, 1
    UC_DMA_BD    0, 0xe3b0028, @WRITE_data_32, 1, 0, 1
    UC_DMA_BD    0, 0xe3b0140, @WRITE_data_33, 1, 0, 1
    UC_DMA_BD    0, 0xe4b0028, @WRITE_data_34, 1, 0, 1
    UC_DMA_BD    0, 0xe4b0140, @WRITE_data_35, 1, 0, 1
    UC_DMA_BD    0, 0xc2b003c, @WRITE_data_36, 1, 0, 1
    UC_DMA_BD    0, 0xc2b012c, @WRITE_data_37, 1, 0, 1
    UC_DMA_BD    0, 0xc3b003c, @WRITE_data_38, 1, 0, 1
    UC_DMA_BD    0, 0xc3b012c, @WRITE_data_39, 1, 0, 1
    UC_DMA_BD    0, 0xc4b003c, @WRITE_data_40, 1, 0, 1
    UC_DMA_BD    0, 0xc4b012c, @WRITE_data_41, 1, 0, 1
    UC_DMA_BD    0, 0x102b003c, @WRITE_data_42, 1, 0, 1
    UC_DMA_BD    0, 0x102b012c, @WRITE_data_43, 1, 0, 1
    UC_DMA_BD    0, 0x103b003c, @WRITE_data_44, 1, 0, 1
    UC_DMA_BD    0, 0x103b012c, @WRITE_data_45, 1, 0, 1
    UC_DMA_BD    0, 0x104b003c, @WRITE_data_46, 1, 0, 1
    UC_DMA_BD    0, 0x104b012c, @WRITE_data_47, 1, 0, 1
    UC_DMA_BD    0, 0xc1b0000, @WRITE_data_48, 1, 0, 1
    UC_DMA_BD    0, 0xc1b0130, @WRITE_data_49, 1, 0, 1
    UC_DMA_BD    0, 0xc2b0040, @WRITE_data_50, 1, 0, 1
    UC_DMA_BD    0, 0xc2b0130, @WRITE_data_51, 1, 0, 1
    UC_DMA_BD    0, 0xc3b0040, @WRITE_data_52, 1, 0, 1
    UC_DMA_BD    0, 0xc3b0130, @WRITE_data_53, 1, 0, 1
    UC_DMA_BD    0, 0xc4b0040, @WRITE_data_54, 1, 0, 1
    UC_DMA_BD    0, 0xc4b0130, @WRITE_data_55, 1, 0, 1
    UC_DMA_BD    0, 0x101b0000, @WRITE_data_56, 1, 0, 1
    UC_DMA_BD    0, 0x101b0130, @WRITE_data_57, 1, 0, 1
    UC_DMA_BD    0, 0x102b0040, @WRITE_data_58, 1, 0, 1
    UC_DMA_BD    0, 0x102b0130, @WRITE_data_59, 1, 0, 1
    UC_DMA_BD    0, 0x103b0040, @WRITE_data_60, 1, 0, 1
    UC_DMA_BD    0, 0x103b0130, @WRITE_data_61, 1, 0, 1
    UC_DMA_BD    0, 0x104b0040, @WRITE_data_62, 1, 0, 1
    UC_DMA_BD    0, 0x104b0130, @WRITE_data_63, 1, 0, 1
    UC_DMA_BD    0, 0xc53f020, @WRITE_data_64, 1, 0, 1
    UC_DMA_BD    0, 0xc53f104, @WRITE_data_65, 1, 0, 1
    UC_DMA_BD    0, 0xe53f020, @WRITE_data_66, 1, 0, 1
    UC_DMA_BD    0, 0xe53f104, @WRITE_data_67, 1, 0, 1
    UC_DMA_BD    0, 0xc03f00c, @WRITE_data_68, 1, 0, 1
    UC_DMA_BD    0, 0xc03f154, @WRITE_data_69, 1, 0, 1
    UC_DMA_BD    0, 0xe03f02c, @WRITE_data_70, 1, 0, 1
    UC_DMA_BD    0, 0xe03f144, @WRITE_data_71, 1, 0, 1
    UC_DMA_BD    0, 0xc03f044, @WRITE_data_72, 1, 0, 1
    UC_DMA_BD    0, 0xc03f114, @WRITE_data_73, 1, 0, 1
    UC_DMA_BD    0, 0xc03f054, @WRITE_data_74, 1, 0, 1
    UC_DMA_BD    0, 0xc03f124, @WRITE_data_75, 1, 0, 1
    UC_DMA_BD    0, 0xe03f054, @WRITE_data_76, 1, 0, 1
    UC_DMA_BD    0, 0xe03f134, @WRITE_data_77, 1, 0, 1
    UC_DMA_BD    0, 0x1003f044, @WRITE_data_78, 1, 0, 1
    UC_DMA_BD    0, 0x1003f134, @WRITE_data_79, 1, 0, 1
    UC_DMA_BD    0, 0xc53f004, @WRITE_data_80, 1, 0, 1
    UC_DMA_BD    0, 0xc53f124, @WRITE_data_81, 1, 0, 1
    UC_DMA_BD    0, 0x1053f02c, @WRITE_data_82, 1, 0, 1
    UC_DMA_BD    0, 0x1053f124, @WRITE_data_83, 1, 0, 1
    UC_DMA_BD    0, 0xc53f058, @WRITE_data_84, 1, 0, 1
    UC_DMA_BD    0, 0xc53f128, @WRITE_data_85, 1, 0, 1
    UC_DMA_BD    0, 0x1053f030, @WRITE_data_86, 1, 0, 1
    UC_DMA_BD    0, 0x1053f128, @WRITE_data_87, 1, 0, 1
    UC_DMA_BD    0, 0xe53f004, @WRITE_data_88, 1, 0, 1
    UC_DMA_BD    0, 0xe53f138, @WRITE_data_89, 1, 0, 0
.align    4
WRITE_data_0:
    .long 0x00000000
WRITE_data_1:
    .long 0x0007c440
WRITE_data_2:
    .long 0x00000000
WRITE_data_3:
    .long 0x0007c440
WRITE_data_4:
    .long 0x80000015
WRITE_data_5:
    .long 0x80000000
WRITE_data_6:
    .long 0x80000016
WRITE_data_7:
    .long 0x80000000
WRITE_data_8:
    .long 0x80000016
WRITE_data_9:
    .long 0x80000000
WRITE_data_10:
    .long 0x80000000
WRITE_data_11:
    .long 0x80000000
WRITE_data_12:
    .long 0x80000000
WRITE_data_13:
    .long 0x80000000
WRITE_data_14:
    .long 0x80000001
WRITE_data_15:
    .long 0x80000000
WRITE_data_16:
    .long 0x80000001
WRITE_data_17:
    .long 0x80000000
WRITE_data_18:
    .long 0x80000001
WRITE_data_19:
    .long 0x80000000
WRITE_data_20:
    .long 0x80000010
WRITE_data_21:
    .long 0x80000000
WRITE_data_22:
    .long 0x80000010
WRITE_data_23:
    .long 0x80000000
WRITE_data_24:
    .long 0x80000010
WRITE_data_25:
    .long 0x80000000
WRITE_data_26:
    .long 0x80000010
WRITE_data_27:
    .long 0x80000000
WRITE_data_28:
    .long 0x80000010
WRITE_data_29:
    .long 0x80000000
WRITE_data_30:
    .long 0x80000010
WRITE_data_31:
    .long 0x80000000
WRITE_data_32:
    .long 0x80000010
WRITE_data_33:
    .long 0x80000000
WRITE_data_34:
    .long 0x80000010
WRITE_data_35:
    .long 0x80000000
WRITE_data_36:
    .long 0x8000000b
WRITE_data_37:
    .long 0x80000000
WRITE_data_38:
    .long 0x8000000b
WRITE_data_39:
    .long 0x80000000
WRITE_data_40:
    .long 0x8000000b
WRITE_data_41:
    .long 0x80000000
WRITE_data_42:
    .long 0x8000000b
WRITE_data_43:
    .long 0x80000000
WRITE_data_44:
    .long 0x8000000b
WRITE_data_45:
    .long 0x80000000
WRITE_data_46:
    .long 0x8000000b
WRITE_data_47:
    .long 0x80000000
WRITE_data_48:
    .long 0x8000000c
WRITE_data_49:
    .long 0x80000000
WRITE_data_50:
    .long 0x8000000c
WRITE_data_51:
    .long 0x80000000
WRITE_data_52:
    .long 0x8000000c
WRITE_data_53:
    .long 0x80000000
WRITE_data_54:
    .long 0x8000000c
WRITE_data_55:
    .long 0x80000000
WRITE_data_56:
    .long 0x8000000c
WRITE_data_57:
    .long 0x80000000
WRITE_data_58:
    .long 0x8000000c
WRITE_data_59:
    .long 0x80000000
WRITE_data_60:
    .long 0x8000000c
WRITE_data_61:
    .long 0x80000000
WRITE_data_62:
    .long 0x8000000c
WRITE_data_63:
    .long 0x80000000
WRITE_data_64:
    .long 0x80000001
WRITE_data_65:
    .long 0x80000000
WRITE_data_66:
    .long 0x80000001
WRITE_data_67:
    .long 0x80000000
WRITE_data_68:
    .long 0x80000015
WRITE_data_69:
    .long 0x80000000
WRITE_data_70:
    .long 0x80000011
WRITE_data_71:
    .long 0x80000000
WRITE_data_72:
    .long 0x80000005
WRITE_data_73:
    .long 0x80000000
WRITE_data_74:
    .long 0x80000009
WRITE_data_75:
    .long 0x80000000
WRITE_data_76:
    .long 0x8000000d
WRITE_data_77:
    .long 0x80000000
WRITE_data_78:
    .long 0x8000000d
WRITE_data_79:
    .long 0x80000000
WRITE_data_80:
    .long 0x80000009
WRITE_data_81:
    .long 0x80000000
WRITE_data_82:
    .long 0x80000009
WRITE_data_83:
    .long 0x80000000
WRITE_data_84:
    .long 0x8000000a
WRITE_data_85:
    .long 0x80000000
WRITE_data_86:
    .long 0x8000000a
WRITE_data_87:
    .long 0x80000000
WRITE_data_88:
    .long 0x8000000e
WRITE_data_89:
    .long 0x80000000
DMAWRITE_data_0:
    .long 0x00000000
    .long 0xffffffff
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
DMAWRITE_data_1:
    .long 0x6579616c
    .long 0x25203172
    .long 0x202b2064
    .long 0x3d206425
    .long 0x0a642520
DMAWRITE_data_2:
    .long 0x00000001
    .long 0x00000001
    .long 0x000006c0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000008
DMAWRITE_data_3:
    .long 0xcbc08046
    .long 0x00460007
    .long 0x000004a0
    .long 0x08a00046
    .long 0x17b00000
    .long 0x01862263
    .long 0x88004040
    .long 0x00000000
    .long 0x00000000
    .long 0x11380000
    .long 0xffff37fe
    .long 0xf002303f
    .long 0xbc003e15
    .long 0x0000b42c
    .long 0x58002d40
    .long 0x000c6800
    .long 0x9b5cd2f6
    .long 0x0000df83
    .long 0x00000000
    .long 0x00000000
    .long 0x00300000
    .long 0x00002088
    .long 0x00000000
    .long 0x00000000
    .long 0x236298b0
    .long 0x20400186
    .long 0x00008800
    .long 0x00000000
    .long 0x00000000
    .long 0x156cb8f4
    .long 0x5841c8c0
    .long 0x000c6800
    .long 0x70000106
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x18000106
    .long 0x01700004
    .long 0x000030c0
    .long 0x00000000
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x58002d40
    .long 0x000c6800
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x21080030
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x0000c6b0
    .long 0x68000086
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0xe020012e
    .long 0x79380011
    .long 0xe7ea2a39
    .long 0xfc3b183e
    .long 0x12330002
    .long 0xfcb31800
    .long 0x36318002
    .long 0xfe842001
    .long 0x16370002
    .long 0xfe4e2001
    .long 0x30001138
    .long 0x200001f0
    .long 0x1530fdc6
    .long 0x9930101e
    .long 0x193017fb
    .long 0x58f017ff
    .long 0x18f017e9
    .long 0x007817ed
    .long 0x39000000
    .long 0x000c6fe8
    .long 0x01fa69b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x20000186
    .long 0x35b04802
    .long 0x35b00fe2
    .long 0x35b00fee
    .long 0x11380fd8
    .long 0x027fb000
    .long 0xfc832000
    .long 0x0000c6b0
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x5bf1cc80
    .long 0x000c6800
    .long 0xe82058b8
    .long 0x58011209
    .long 0x10b8fc73
    .long 0xfff407f8
    .long 0x0020403f
    .long 0xebff90b8
    .long 0x8000000a
    .long 0x10b80664
    .long 0x810faff0
    .long 0x0654c000
    .long 0x061be046
    .long 0x00460000
    .long 0x00010520
    .long 0x12043846
    .long 0x11380000
    .long 0x00026808
    .long 0xe9bf4801
    .long 0x8ff11138
    .long 0x4800811e
    .long 0x1138e143
    .long 0x000106ec
    .long 0xe3c74800
    .long 0x011a1138
    .long 0x48000006
    .long 0x1138e3c3
    .long 0x00016808
    .long 0xe3c34802
    .long 0xe8001138
    .long 0x480001d3
    .long 0x1138e3c3
    .long 0x01d98800
    .long 0xe3df4800
    .long 0xf0001138
    .long 0x480001e1
    .long 0x9138ed77
    .long 0x8127aff2
    .long 0xe52b4800
    .long 0x86e01138
    .long 0x48000000
    .long 0xb802e9bf
    .long 0x4800d434
    .long 0x1138e143
    .long 0x00414801
    .long 0xe3cf4800
    .long 0x80f61138
    .long 0x48000005
    .long 0x1138e3c3
    .long 0x01c24800
    .long 0xe3c34800
    .long 0x68001138
    .long 0x480001e2
    .long 0xfa70e3c3
    .long 0x9a70171e
    .long 0x1a70174a
    .long 0xfa701789
    .long 0x1a70174d
    .long 0x7a70170a
    .long 0x1a70171d
    .long 0xf530171e
    .long 0xf530161f
    .long 0x1a70161f
    .long 0x9530171e
    .long 0x1a70161d
    .long 0x1530171e
    .long 0xfa70161e
    .long 0x3530171e
    .long 0xb802101e
    .long 0xa000b434
    .long 0xb8024387
    .long 0xa000f534
    .long 0xba70c3e6
    .long 0x5a70172b
    .long 0x15301769
    .long 0xb53010ee
    .long 0x3530161e
    .long 0x3530121e
    .long 0x153016ae
    .long 0x55301206
    .long 0x99301006
    .long 0x000017e3
    .long 0x00000000
    .long 0x2622e830
    .long 0x3629a5f0
    .long 0xa8a00046
    .long 0x00000007
    .long 0x05dee9b0
    .long 0x3529a5f0
    .long 0x00000000
    .long 0x00000000
    .long 0x12300000
    .long 0x353024e3
    .long 0x00001206
    .long 0x00000000
    .long 0xe8300000
    .long 0x01062682
    .long 0x00026800
    .long 0xd834b802
    .long 0x30623001
    .long 0x00000000
    .long 0x00000000
    .long 0x19acb802
    .long 0x503c3000
    .long 0x00000000
    .long 0x00000000
    .long 0x88300000
    .long 0x69b02600
    .long 0x0000002c
    .long 0x00000000
    .long 0x00000000
    .long 0x11b00000
    .long 0x35302223
    .long 0x00001016
    .long 0x00000000
    .long 0x88300000
    .long 0x087426a0
    .long 0x00e11400
    .long 0x99816180
    .long 0x10b802ee
    .long 0x01caa800
    .long 0x42ce9800
    .long 0x00000000
    .long 0x00000000
    .long 0x52300000
    .long 0x31ee2463
    .long 0x3cc6a223
    .long 0x12163530
    .long 0x00000000
    .long 0x00000000
    .long 0x2622e830
    .long 0x28001138
    .long 0x300001e2
    .long 0x00463030
    .long 0x0007a9a0
    .long 0xe9b00000
    .long 0x0000042e
    .long 0x00000000
    .long 0x00000000
    .long 0x32300000
    .long 0x35302463
    .long 0x000011e6
    .long 0x00000000
    .long 0xe8300000
    .long 0x01062682
    .long 0x00026800
    .long 0xd834b802
    .long 0x10723001
    .long 0x5b34b802
    .long 0x50283000
    .long 0x00000000
    .long 0x0016a134
    .long 0x58000000
    .long 0x000c6800
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x26008830
    .long 0x9865b2f6
    .long 0x0000e2c6
    .long 0x00000000
    .long 0x00000000
    .long 0x11b00000
    .long 0x35302223
    .long 0x000010fe
    .long 0x00000000
    .long 0x88300000
    .long 0x013026a0
    .long 0x9ab02020
    .long 0x11b02422
    .long 0x01862253
    .long 0x4800b840
    .long 0x34ca65f0
    .long 0x00000000
    .long 0x00000000
    .long 0x28000086
    .long 0x35b00002
    .long 0x00000fc8
    .long 0x00000000
    .long 0x0016a134
    .long 0x58000000
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x58002d40
    .long 0xfc835800
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x21080030
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x003c69b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x242319b0
    .long 0x98400186
    .long 0x00008800
    .long 0x00000000
    .long 0x00000000
    .long 0x2800012e
    .long 0xf2b0fda7
    .long 0x72b00fd2
    .long 0xf2b00fcb
    .long 0x35b00fc3
    .long 0x35b00ffe
    .long 0x75b00ff7
    .long 0x1e300fd0
    .long 0x00000fff
    .long 0x00000000
    .long 0x00000000
    .long 0x00300000
    .long 0x00002188
    .long 0x00000000
    .long 0x00000000
DMAWRITE_data_4:
    .long 0xc6c04046
    .long 0x00000007
    .long 0x69b00000
    .long 0x0000060c
    .long 0x00000000
    .long 0x00000000
    .long 0x01860000
    .long 0x8802a040
    .long 0x00011e30
    .long 0x17d1d8f0
    .long 0x17bf9930
    .long 0x17b09930
    .long 0xa5f00000
    .long 0x0c3038a5
    .long 0xe9b03ed1
    .long 0x69b00ffe
    .long 0xe9b00ffc
    .long 0x00000ffc
    .long 0x0eac69b0
    .long 0x00000000
    .long 0x00000000
    .long 0x24e31230
    .long 0x17063530
    .long 0x27e6fd30
    .long 0x00000000
    .long 0x38780000
    .long 0x5a800453
    .long 0x000c6800
    .long 0xf50158b8
    .long 0x00005e0b
    .long 0x48460215
    .long 0x0007c720
    .long 0x190e0000
    .long 0x29be9faf
    .long 0x9fb8390e
    .long 0x990e40f3
    .long 0x00e39fb4
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0xb2f60000
    .long 0xc0c69967
    .long 0x023ce9b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x38b38ab0
    .long 0x171e3530
    .long 0x0c3819b0
    .long 0x581e0000
    .long 0x000c6800
    .long 0x20003e15
    .long 0x00800000
    .long 0x58002d40
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x58002d40
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x5bfd3880
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x5bfb1880
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x5bfe0480
    .long 0x000c6800
    .long 0xbc003e15
    .long 0x0000f4b4
    .long 0x5840e0c0
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x58002d40
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x58002d40
    .long 0x000c6800
    .long 0x301010b8
    .long 0x400001f0
    .long 0x00000044
    .long 0xe9b00000
    .long 0x35b0000c
    .long 0x00000f61
    .long 0x00000000
    .long 0x00000000
    .long 0x24e718b0
    .long 0x48400186
    .long 0x00009803
    .long 0x0f6f35b0
    .long 0x0f5e35b0
    .long 0x00000000
    .long 0x028ce9b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x24d11830
    .long 0x0fece9b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x246331b0
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x5bfb1a80
    .long 0x000c6800
    .long 0x0f7075b0
    .long 0x000c69b0
    .long 0x0fa3f2b0
    .long 0x0f7f35b0
    .long 0x00000000
    .long 0x00300000
    .long 0x00002188
    .long 0x00000000
    .long 0x38b10130
    .long 0xe006350e
    .long 0x1138fff1
DMAWRITE_data_5:
    .long 0x28001138
    .long 0x30000002
    .long 0x11381072
    .long 0x00020800
    .long 0xf0443000
    .long 0x246307b0
    .long 0xa0400186
    .long 0x9e308803
    .long 0xd8f00000
    .long 0x393017f9
    .long 0x193017f8
    .long 0x000017fc
    .long 0x08003e15
    .long 0x0000e400
    .long 0x58002d40
    .long 0x000c6800
    .long 0x0e3839b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x20880030
    .long 0x37bda5f0
    .long 0x00000000
    .long 0x00000000
    .long 0x23a2f8b0
    .long 0x80400186
    .long 0x00008803
    .long 0x00000000
    .long 0xc2ac0000
    .long 0x586f8007
    .long 0x5a800000
    .long 0x000c6800
    .long 0x0ff075b0
    .long 0x0ff3f2b0
    .long 0x0fff35b0
    .long 0x00000000
    .long 0x00000000
    .long 0x21880030
    .long 0x00000000
    .long 0x00000000
    .long 0x0fff9e30
DMAWRITE_data_6:
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x21880030
    .long 0x00000000
    .long 0x00000000
    .long 0xd2f60000
DMAWRITE_data_7:
    .long 0xe360d2f6
    .long 0x00000011
    .long 0x19b00000
    .long 0x00000608
    .long 0x01060000
    .long 0x0003b800
    .long 0x17d09930
    .long 0x67e97938
    .long 0x203e07e8
    .long 0x7938ffa3
    .long 0x27886a39
    .long 0xf882203c
    .long 0x3086a938
    .long 0x20001048
    .long 0x350ef801
    .long 0x000c6804
    .long 0x37c325f0
    .long 0x00000000
    .long 0x0f8825b0
    .long 0x21880030
    .long 0x00000000
    .long 0x00000000
    .long 0x0fff1e30
DMAWRITE_data_8:
    .long 0xc6c07846
    .long 0x00000007
    .long 0x39b00000
    .long 0x00860608
    .long 0x0003d000
    .long 0x34c8e5f0
    .long 0x218011be
    .long 0x00004183
    .long 0x00000000
DMAWRITE_data_9:
    .long 0x323410b8
    .long 0x000001f0
    .long 0x10b80110
    .long 0x01f04a24
    .long 0xff844000
    .long 0x352c8bf0
    .long 0x9a1c45f6
    .long 0x7802008e
    .long 0xa0009811
    .long 0x8bf00092
    .long 0xa5f035c4
    .long 0x0046363f
    .long 0x00000cd1
    .long 0x00000000
    .long 0x347f0002
    .long 0x000b5000
    .long 0x54003e15
    .long 0x687bd862
    .long 0x58002d43
    .long 0x000c6800
    .long 0x0e3939b0
    .long 0x0fb839b0
    .long 0x00000000
    .long 0x00000000
    .long 0x40300000
    .long 0x00002088
    .long 0x00000000
    .long 0x0000c6b0
    .long 0xbc003e15
    .long 0x0000e40c
    .long 0x58002d40
    .long 0x000c6800
    .long 0x235ba030
    .long 0x302e65f0
    .long 0x00000000
    .long 0x00000000
    .long 0x37cc65f0
    .long 0x21880030
    .long 0x0708b3e6
    .long 0xb3e656b0
    .long 0x4f00088b
    .long 0x00000000
    .long 0x44000000
DMAWRITE_data_10:
    .long 0x00000000
    .long 0xffffffff
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
DMAWRITE_data_11:
    .long 0x6579616c
    .long 0x25203172
    .long 0x202b2064
    .long 0x3d206425
    .long 0x0a642520
DMAWRITE_data_12:
    .long 0x00000001
    .long 0x00000001
    .long 0x000006c0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000008
DMAWRITE_data_13:
    .long 0xcbc08046
    .long 0x00460007
    .long 0x000004a0
    .long 0x08a00046
    .long 0x17b00000
    .long 0x01862263
    .long 0x88004040
    .long 0x00000000
    .long 0x00000000
    .long 0x11380000
    .long 0xffff37fe
    .long 0xf002303f
    .long 0xbc003e15
    .long 0x0000b42c
    .long 0x58002d40
    .long 0x000c6800
    .long 0x9b5cd2f6
    .long 0x0000df83
    .long 0x00000000
    .long 0x00000000
    .long 0x00300000
    .long 0x00002088
    .long 0x00000000
    .long 0x00000000
    .long 0x236298b0
    .long 0x20400186
    .long 0x00008800
    .long 0x00000000
    .long 0x00000000
    .long 0x156cb8f4
    .long 0x5841c8c0
    .long 0x000c6800
    .long 0x70000106
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x18000106
    .long 0x01700004
    .long 0x000030c0
    .long 0x00000000
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x58002d40
    .long 0x000c6800
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x21080030
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x0000c6b0
    .long 0x68000086
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0xe020012e
    .long 0x79380011
    .long 0xe7ea2a39
    .long 0xfc3b183e
    .long 0x12330002
    .long 0xfcb31800
    .long 0x36318002
    .long 0xfe842001
    .long 0x16370002
    .long 0xfe4e2001
    .long 0x30001138
    .long 0x200001f0
    .long 0x1530fdc6
    .long 0x9930101e
    .long 0x193017fb
    .long 0x58f017ff
    .long 0x18f017e9
    .long 0x007817ed
    .long 0x39000000
    .long 0x000c6fe8
    .long 0x01fa69b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x20000186
    .long 0x35b04802
    .long 0x35b00fe2
    .long 0x35b00fee
    .long 0x11380fd8
    .long 0x027fb000
    .long 0xfc832000
    .long 0x0000c6b0
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x5bf1cc80
    .long 0x000c6800
    .long 0xe82058b8
    .long 0x58011209
    .long 0x10b8fc73
    .long 0xfff407f8
    .long 0x0020403f
    .long 0xebff90b8
    .long 0x8000000a
    .long 0x10b80664
    .long 0x810faff0
    .long 0x0654c000
    .long 0x061be046
    .long 0x00460000
    .long 0x00010520
    .long 0x12043846
    .long 0x11380000
    .long 0x00026808
    .long 0xe9bf4801
    .long 0x8ff11138
    .long 0x4800811e
    .long 0x1138e143
    .long 0x000106ec
    .long 0xe3c74800
    .long 0x011a1138
    .long 0x48000006
    .long 0x1138e3c3
    .long 0x00016808
    .long 0xe3c34802
    .long 0xe8001138
    .long 0x480001d3
    .long 0x1138e3c3
    .long 0x01d98800
    .long 0xe3df4800
    .long 0xf0001138
    .long 0x480001e1
    .long 0x9138ed77
    .long 0x8127aff2
    .long 0xe52b4800
    .long 0x86e01138
    .long 0x48000000
    .long 0xb802e9bf
    .long 0x4800d434
    .long 0x1138e143
    .long 0x00414801
    .long 0xe3cf4800
    .long 0x80f61138
    .long 0x48000005
    .long 0x1138e3c3
    .long 0x01c24800
    .long 0xe3c34800
    .long 0x68001138
    .long 0x480001e2
    .long 0xfa70e3c3
    .long 0x9a70171e
    .long 0x1a70174a
    .long 0xfa701789
    .long 0x1a70174d
    .long 0x7a70170a
    .long 0x1a70171d
    .long 0xf530171e
    .long 0xf530161f
    .long 0x1a70161f
    .long 0x9530171e
    .long 0x1a70161d
    .long 0x1530171e
    .long 0xfa70161e
    .long 0x3530171e
    .long 0xb802101e
    .long 0xa000b434
    .long 0xb8024387
    .long 0xa000f534
    .long 0xba70c3e6
    .long 0x5a70172b
    .long 0x15301769
    .long 0xb53010ee
    .long 0x3530161e
    .long 0x3530121e
    .long 0x153016ae
    .long 0x55301206
    .long 0x99301006
    .long 0x000017e3
    .long 0x00000000
    .long 0x2622e830
    .long 0x3629a5f0
    .long 0xa8a00046
    .long 0x00000007
    .long 0x05dee9b0
    .long 0x3529a5f0
    .long 0x00000000
    .long 0x00000000
    .long 0x12300000
    .long 0x353024e3
    .long 0x00001206
    .long 0x00000000
    .long 0xe8300000
    .long 0x01062682
    .long 0x00026800
    .long 0xd834b802
    .long 0x30623001
    .long 0x00000000
    .long 0x00000000
    .long 0x19acb802
    .long 0x503c3000
    .long 0x00000000
    .long 0x00000000
    .long 0x88300000
    .long 0x69b02600
    .long 0x0000002c
    .long 0x00000000
    .long 0x00000000
    .long 0x11b00000
    .long 0x35302223
    .long 0x00001016
    .long 0x00000000
    .long 0x88300000
    .long 0x087426a0
    .long 0x00e11400
    .long 0x99816180
    .long 0x10b802ee
    .long 0x01caa800
    .long 0x42ce9800
    .long 0x00000000
    .long 0x00000000
    .long 0x52300000
    .long 0x31ee2463
    .long 0x3cc6a223
    .long 0x12163530
    .long 0x00000000
    .long 0x00000000
    .long 0x2622e830
    .long 0x28001138
    .long 0x300001e2
    .long 0x00463030
    .long 0x0007a9a0
    .long 0xe9b00000
    .long 0x0000042e
    .long 0x00000000
    .long 0x00000000
    .long 0x32300000
    .long 0x35302463
    .long 0x000011e6
    .long 0x00000000
    .long 0xe8300000
    .long 0x01062682
    .long 0x00026800
    .long 0xd834b802
    .long 0x10723001
    .long 0x5b34b802
    .long 0x50283000
    .long 0x00000000
    .long 0x0016a134
    .long 0x58000000
    .long 0x000c6800
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x26008830
    .long 0x9865b2f6
    .long 0x0000e2c6
    .long 0x00000000
    .long 0x00000000
    .long 0x11b00000
    .long 0x35302223
    .long 0x000010fe
    .long 0x00000000
    .long 0x88300000
    .long 0x013026a0
    .long 0x9ab02020
    .long 0x11b02422
    .long 0x01862253
    .long 0x4800b840
    .long 0x34ca65f0
    .long 0x00000000
    .long 0x00000000
    .long 0x28000086
    .long 0x35b00002
    .long 0x00000fc8
    .long 0x00000000
    .long 0x0016a134
    .long 0x58000000
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x58002d40
    .long 0xfc835800
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x21080030
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x003c69b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x242319b0
    .long 0x98400186
    .long 0x00008800
    .long 0x00000000
    .long 0x00000000
    .long 0x2800012e
    .long 0xf2b0fda7
    .long 0x72b00fd2
    .long 0xf2b00fcb
    .long 0x35b00fc3
    .long 0x35b00ffe
    .long 0x75b00ff7
    .long 0x1e300fd0
    .long 0x00000fff
    .long 0x00000000
    .long 0x00000000
    .long 0x00300000
    .long 0x00002188
    .long 0x00000000
    .long 0x00000000
DMAWRITE_data_14:
    .long 0xc6c04046
    .long 0x00000007
    .long 0x69b00000
    .long 0x0000060c
    .long 0x00000000
    .long 0x00000000
    .long 0x01860000
    .long 0x8802a040
    .long 0x00011e30
    .long 0x17d1d8f0
    .long 0x17bf9930
    .long 0x17b09930
    .long 0xa5f00000
    .long 0x0c3038a5
    .long 0xe9b03ed1
    .long 0x69b00ffe
    .long 0xe9b00ffc
    .long 0x00000ffc
    .long 0x0eac69b0
    .long 0x00000000
    .long 0x00000000
    .long 0x24e31230
    .long 0x17063530
    .long 0x27e6fd30
    .long 0x00000000
    .long 0x38780000
    .long 0x5a800453
    .long 0x000c6800
    .long 0xf50158b8
    .long 0x00005e0b
    .long 0x48460215
    .long 0x0007c720
    .long 0x190e0000
    .long 0x29be9faf
    .long 0x9fb8390e
    .long 0x990e40f3
    .long 0x00e39fb4
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0xb2f60000
    .long 0xc0c69967
    .long 0x023ce9b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x38b38ab0
    .long 0x171e3530
    .long 0x0c3819b0
    .long 0x581e0000
    .long 0x000c6800
    .long 0x20003e15
    .long 0x00800000
    .long 0x58002d40
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x58002d40
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x5bfd3880
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x5bfb1880
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x5bfe0480
    .long 0x000c6800
    .long 0xbc003e15
    .long 0x0000f4b4
    .long 0x5840e0c0
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x58002d40
    .long 0x000c6800
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x58002d40
    .long 0x000c6800
    .long 0x301010b8
    .long 0x400001f0
    .long 0x00000044
    .long 0xe9b00000
    .long 0x35b0000c
    .long 0x00000f61
    .long 0x00000000
    .long 0x00000000
    .long 0x24e718b0
    .long 0x48400186
    .long 0x00009803
    .long 0x0f6f35b0
    .long 0x0f5e35b0
    .long 0x00000000
    .long 0x028ce9b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x24d11830
    .long 0x0fece9b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x246331b0
    .long 0x7c003e15
    .long 0x000002c3
    .long 0x5bfb1a80
    .long 0x000c6800
    .long 0x0f7075b0
    .long 0x000c69b0
    .long 0x0fa3f2b0
    .long 0x0f7f35b0
    .long 0x00000000
    .long 0x00300000
    .long 0x00002188
    .long 0x00000000
    .long 0x38b10130
    .long 0xe006350e
    .long 0x1138fff1
DMAWRITE_data_15:
    .long 0x28001138
    .long 0x30000002
    .long 0x11381072
    .long 0x00020800
    .long 0xf0443000
    .long 0x246307b0
    .long 0xa0400186
    .long 0x9e308803
    .long 0xd8f00000
    .long 0x393017f9
    .long 0x193017f8
    .long 0x000017fc
    .long 0x08003e15
    .long 0x0000e400
    .long 0x58002d40
    .long 0x000c6800
    .long 0x0e3839b0
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x20880030
    .long 0x37bda5f0
    .long 0x00000000
    .long 0x00000000
    .long 0x23a2f8b0
    .long 0x80400186
    .long 0x00008803
    .long 0x00000000
    .long 0xc2ac0000
    .long 0x586f8007
    .long 0x5a800000
    .long 0x000c6800
    .long 0x0ff075b0
    .long 0x0ff3f2b0
    .long 0x0fff35b0
    .long 0x00000000
    .long 0x00000000
    .long 0x21880030
    .long 0x00000000
    .long 0x00000000
    .long 0x0fff9e30
DMAWRITE_data_16:
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x00000000
    .long 0x21880030
    .long 0x00000000
    .long 0x00000000
    .long 0xd2f60000
DMAWRITE_data_17:
    .long 0xe360d2f6
    .long 0x00000011
    .long 0x19b00000
    .long 0x00000608
    .long 0x01060000
    .long 0x0003b800
    .long 0x17d09930
    .long 0x67e97938
    .long 0x203e07e8
    .long 0x7938ffa3
    .long 0x27886a39
    .long 0xf882203c
    .long 0x3086a938
    .long 0x20001048
    .long 0x350ef801
    .long 0x000c6804
    .long 0x37c325f0
    .long 0x00000000
    .long 0x0f8825b0
    .long 0x21880030
    .long 0x00000000
    .long 0x00000000
    .long 0x0fff1e30
DMAWRITE_data_18:
    .long 0xc6c07846
    .long 0x00000007
    .long 0x39b00000
    .long 0x00860608
    .long 0x0003d000
    .long 0x34c8e5f0
    .long 0x218011be
    .long 0x00004183
    .long 0x00000000
DMAWRITE_data_19:
    .long 0x323410b8
    .long 0x000001f0
    .long 0x10b80110
    .long 0x01f04a24
    .long 0xff844000
    .long 0x352c8bf0
    .long 0x9a1c45f6
    .long 0x7802008e
    .long 0xa0009811
    .long 0x8bf00092
    .long 0xa5f035c4
    .long 0x0046363f
    .long 0x00000cd1
    .long 0x00000000
    .long 0x347f0002
    .long 0x000b5000
    .long 0x54003e15
    .long 0x687bd862
    .long 0x58002d43
    .long 0x000c6800
    .long 0x0e3939b0
    .long 0x0fb839b0
    .long 0x00000000
    .long 0x00000000
    .long 0x40300000
    .long 0x00002088
    .long 0x00000000
    .long 0x0000c6b0
    .long 0xbc003e15
    .long 0x0000e40c
    .long 0x58002d40
    .long 0x000c6800
    .long 0x235ba030
    .long 0x302e65f0
    .long 0x00000000
    .long 0x00000000
    .long 0x37cc65f0
    .long 0x21880030
    .long 0x0708b3e6
    .long 0xb3e656b0
    .long 0x4f00088b
    .long 0x00000000
    .long 0x44000000
