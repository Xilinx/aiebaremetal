;
; Code
;

START_JOB 0
  LOAD_PDI 0, @pdi
END_JOB
.eop

START_JOB 1
  ; Set graph iteration(s)
  WRITE_32            0x50eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x60eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x70eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x80eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x250eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x260eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x270eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x280eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x450eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x460eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x470eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x480eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x650eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x660eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x670eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x680eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x850eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x860eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x870eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0x880eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0xa50eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0xa60eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0xa70eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0xa80eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0xc50eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0xc60eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0xc70eb44, 0x1
  ; Set graph iteration(s)
  WRITE_32            0xc80eb44, 0x1
  ; Run graph
  WRITE_32            0x538000, 0x1
  ; Run graph
  WRITE_32            0x638000, 0x1
  ; Run graph
  WRITE_32            0x738000, 0x1
  ; Run graph
  WRITE_32            0x838000, 0x1
  ; Run graph
  WRITE_32            0x2538000, 0x1
  ; Run graph
  WRITE_32            0x2638000, 0x1
  ; Run graph
  WRITE_32            0x2738000, 0x1
  ; Run graph
  WRITE_32            0x2838000, 0x1
  ; Run graph
  WRITE_32            0x4538000, 0x1
  ; Run graph
  WRITE_32            0x4638000, 0x1
  ; Run graph
  WRITE_32            0x4738000, 0x1
  ; Run graph
  WRITE_32            0x4838000, 0x1
  ; Run graph
  WRITE_32            0x6538000, 0x1
  ; Run graph
  WRITE_32            0x6638000, 0x1
  ; Run graph
  WRITE_32            0x6738000, 0x1
  ; Run graph
  WRITE_32            0x6838000, 0x1
  ; Run graph
  WRITE_32            0x8538000, 0x1
  ; Run graph
  WRITE_32            0x8638000, 0x1
  ; Run graph
  WRITE_32            0x8738000, 0x1
  ; Run graph
  WRITE_32            0x8838000, 0x1
  ; Run graph
  WRITE_32            0xa538000, 0x1
  ; Run graph
  WRITE_32            0xa638000, 0x1
  ; Run graph
  WRITE_32            0xa738000, 0x1
  ; Run graph
  WRITE_32            0xa838000, 0x1
  ; Run graph
  WRITE_32            0xc538000, 0x1
  ; Run graph
  WRITE_32            0xc638000, 0x1
  ; Run graph
  WRITE_32            0xc738000, 0x1
  ; Run graph
  WRITE_32            0xc838000, 0x1
  ; Initialize lock col-2, row-0, lockID-0, value=0
  WRITE_32            0x4000000, 0x0
  ; Initialize lock col-2, row-0, lockID-1, value=0
  WRITE_32            0x4000010, 0x0
  ; Initialize lock col-2, row-0, lockID-0, value=0
  WRITE_32            0x41c0000, 0x0
  ; Initialize lock col-2, row-0, lockID-1, value=0
  WRITE_32            0x41c0010, 0x0
  APPLY_OFFSET_57     @shim_bd0, 1, 0
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd0
  ; enqueueTask col 2, row-0, channel 0, direction 1, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0x4009314, 0x0
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd1
  ; enqueueTask col 1, row-1, channel 3, direction 0, startBD 24, repeat count 1, enable TCT 0
  WRITE_32            0x21a061c, 0x18
  ; Acquire lock col-2, row-0, lockID-1, value=1
  POLL_32             0x41d0604, 0x1
  ; Initialize lock col-3, row-0, lockID-0, value=0
  WRITE_32            0x6000000, 0x0
  ; Initialize lock col-3, row-0, lockID-1, value=0
  WRITE_32            0x6000010, 0x0
  ; Initialize lock col-6, row-0, lockID-0, value=0
  WRITE_32            0xc000000, 0x0
  ; Initialize lock col-6, row-0, lockID-1, value=0
  WRITE_32            0xc000010, 0x0
  ; Initialize lock col-4, row-0, lockID-0, value=0
  WRITE_32            0x81c0000, 0x0
  ; Initialize lock col-4, row-0, lockID-1, value=0
  WRITE_32            0x81c0010, 0x0
  APPLY_OFFSET_57     @shim_bd1, 1, 1
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd2
  ; enqueueTask col 3, row-0, channel 0, direction 1, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0x6009314, 0x0
  APPLY_OFFSET_57     @shim_bd2, 1, 1
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd3
  ; enqueueTask col 3, row-0, channel 1, direction 1, startBD 1, repeat count 1, enable TCT 0
  WRITE_32            0x600931c, 0x1
  APPLY_OFFSET_57     @shim_bd3, 1, 1
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd4
  ; enqueueTask col 6, row-0, channel 0, direction 1, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0xc009314, 0x0
  APPLY_OFFSET_57     @shim_bd4, 1, 1
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd5
  ; enqueueTask col 6, row-0, channel 1, direction 1, startBD 1, repeat count 1, enable TCT 0
  WRITE_32            0xc00931c, 0x1
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd6
  ; enqueueTask col 4, row-1, channel 0, direction 0, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0x81a0604, 0x0
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd7
  ; enqueueTask col 4, row-1, channel 1, direction 0, startBD 24, repeat count 1, enable TCT 0
  WRITE_32            0x81a060c, 0x18
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd8
  ; enqueueTask col 4, row-1, channel 2, direction 0, startBD 1, repeat count 1, enable TCT 0
  WRITE_32            0x81a0614, 0x1
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd9
  ; enqueueTask col 4, row-1, channel 3, direction 0, startBD 25, repeat count 1, enable TCT 0
  WRITE_32            0x81a061c, 0x19
  ; Acquire lock col-4, row-0, lockID-1, value=4
  POLL_32             0x81d0610, 0x1
  ; Initialize lock col-2, row-0, lockID-2, value=0
  WRITE_32            0x41c0020, 0x0
  ; Initialize lock col-2, row-0, lockID-3, value=0
  WRITE_32            0x41c0030, 0x0
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd10
  ; enqueueTask col 1, row-1, channel 0, direction 1, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0x21a0634, 0x0
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd11
  ; enqueueTask col 1, row-1, channel 1, direction 1, startBD 24, repeat count 1, enable TCT 0
  WRITE_32            0x21a063c, 0x18
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd12
  ; enqueueTask col 2, row-1, channel 0, direction 1, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0x41a0634, 0x0
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd13
  ; enqueueTask col 2, row-1, channel 1, direction 1, startBD 24, repeat count 1, enable TCT 0
  WRITE_32            0x41a063c, 0x18
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd14
  ; enqueueTask col 2, row-1, channel 2, direction 1, startBD 2, repeat count 1, enable TCT 0
  WRITE_32            0x41a0644, 0x2
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd15
  ; enqueueTask col 3, row-1, channel 0, direction 1, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0x61a0634, 0x0
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd16
  ; enqueueTask col 3, row-1, channel 1, direction 1, startBD 24, repeat count 1, enable TCT 0
  WRITE_32            0x61a063c, 0x18
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd17
  ; enqueueTask col 4, row-1, channel 0, direction 1, startBD 0, repeat count 2, enable TCT 0
  WRITE_32            0x81a0634, 0x10000
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd18
  ; enqueueTask col 4, row-1, channel 1, direction 1, startBD 24, repeat count 2, enable TCT 0
  WRITE_32            0x81a063c, 0x10018
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd19
  ; enqueueTask col 4, row-1, channel 2, direction 1, startBD 1, repeat count 2, enable TCT 0
  WRITE_32            0x81a0644, 0x10001
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd20
  ; enqueueTask col 4, row-1, channel 3, direction 1, startBD 25, repeat count 2, enable TCT 0
  WRITE_32            0x81a064c, 0x10019
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd21
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd22
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd23
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd24
  ; enqueueTask col 1, row-1, channel 0, direction 0, startBD 0, repeat count 8, enable TCT 0
  WRITE_32            0x21a0604, 0x70000
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd25
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd26
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd27
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd28
  ; enqueueTask col 1, row-1, channel 1, direction 0, startBD 24, repeat count 8, enable TCT 0
  WRITE_32            0x21a060c, 0x70018
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd29
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd30
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd31
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd32
  ; enqueueTask col 2, row-1, channel 0, direction 0, startBD 0, repeat count 8, enable TCT 0
  WRITE_32            0x41a0604, 0x70000
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd33
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd34
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd35
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd36
  ; enqueueTask col 2, row-1, channel 1, direction 0, startBD 24, repeat count 8, enable TCT 0
  WRITE_32            0x41a060c, 0x70018
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd37
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd38
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd39
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd40
  ; enqueueTask col 2, row-1, channel 2, direction 0, startBD 0, repeat count 8, enable TCT 0
  WRITE_32            0x41a0614, 0x70000
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd41
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd42
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd43
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd44
  ; enqueueTask col 3, row-1, channel 0, direction 0, startBD 0, repeat count 8, enable TCT 0
  WRITE_32            0x61a0604, 0x70000
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd45
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd46
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd47
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd48
  ; enqueueTask col 3, row-1, channel 1, direction 0, startBD 24, repeat count 8, enable TCT 0
  WRITE_32            0x61a060c, 0x70018
  ; Acquire lock col-2, row-0, lockID-3, value=56
  POLL_32             0x41d0ee0, 0x1
  ; Initialize lock col-3, row-0, lockID-0, value=0
  WRITE_32            0x6000000, 0x0
  ; Initialize lock col-3, row-0, lockID-1, value=0
  WRITE_32            0x6000010, 0x0
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd49
  ; enqueueTask col 2, row-1, channel 3, direction 1, startBD 24, repeat count 1, enable TCT 0
  WRITE_32            0x41a064c, 0x18
  APPLY_OFFSET_57     @shim_bd5, 1, 2
  ; Descriptor_ptr
  UC_DMA_WRITE_DES_SYNC    @uc_dma_bd50
  ; enqueueTask col 3, row-0, channel 0, direction 0, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0x6009304, 0x0
  ; Acquire lock col-3, row-0, lockID-1, value=1
  POLL_32             0x600c604, 0x1
  ; End graph
  WRITE_32            0x538000, 0x0
  ; End graph
  WRITE_32            0x638000, 0x0
  ; End graph
  WRITE_32            0x738000, 0x0
  ; End graph
  WRITE_32            0x838000, 0x0
  ; End graph
  WRITE_32            0x2538000, 0x0
  ; End graph
  WRITE_32            0x2638000, 0x0
  ; End graph
  WRITE_32            0x2738000, 0x0
  ; End graph
  WRITE_32            0x2838000, 0x0
  ; End graph
  WRITE_32            0x4538000, 0x0
  ; End graph
  WRITE_32            0x4638000, 0x0
  ; End graph
  WRITE_32            0x4738000, 0x0
  ; End graph
  WRITE_32            0x4838000, 0x0
  ; End graph
  WRITE_32            0x6538000, 0x0
  ; End graph
  WRITE_32            0x6638000, 0x0
  ; End graph
  WRITE_32            0x6738000, 0x0
  ; End graph
  WRITE_32            0x6838000, 0x0
  ; End graph
  WRITE_32            0x8538000, 0x0
  ; End graph
  WRITE_32            0x8638000, 0x0
  ; End graph
  WRITE_32            0x8738000, 0x0
  ; End graph
  WRITE_32            0x8838000, 0x0
  ; End graph
  WRITE_32            0xa538000, 0x0
  ; End graph
  WRITE_32            0xa638000, 0x0
  ; End graph
  WRITE_32            0xa738000, 0x0
  ; End graph
  WRITE_32            0xa838000, 0x0
  ; End graph
  WRITE_32            0xc538000, 0x0
  ; End graph
  WRITE_32            0xc638000, 0x0
  ; End graph
  WRITE_32            0xc738000, 0x0
  ; End graph
  WRITE_32            0xc838000, 0x0
END_JOB

.include resnpdi.asm
EOF

;
; Data
;

  .align            16
uc_dma_bd0:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x4009000, @shim_bd0, 9, 0, 0

  .align            4
shim_bd0:
  .long             0xe000
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x2001001
  .long             0x0

  .align            16
uc_dma_bd1:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a0300, @mem_bd0, 8, 0, 0

  .align            4
mem_bd0:
  .long             0xe000
  .long             0x46000
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x81818080

  .align            16
uc_dma_bd2:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x6009000, @shim_bd1, 9, 0, 0

  .align            4
shim_bd1:
  .long             0x9c0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x2001001
  .long             0x0

  .align            16
uc_dma_bd3:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x6009030, @shim_bd2, 9, 0, 0

  .align            4
shim_bd2:
  .long             0x9c0
  .long             0x2700
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x2001001
  .long             0x0

  .align            16
uc_dma_bd4:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xc009000, @shim_bd3, 9, 0, 0

  .align            4
shim_bd3:
  .long             0x9c0
  .long             0x4e00
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x2001001
  .long             0x0

  .align            16
uc_dma_bd5:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xc009030, @shim_bd4, 9, 0, 0

  .align            4
shim_bd4:
  .long             0x9c0
  .long             0x7500
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x2001001
  .long             0x0

  .align            16
uc_dma_bd6:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x81a0000, @mem_bd1, 8, 0, 0

  .align            4
mem_bd1:
  .long             0x9c0
  .long             0x40000
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x81418040

  .align            16
uc_dma_bd7:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x81a0300, @mem_bd2, 8, 0, 0

  .align            4
mem_bd2:
  .long             0x9c0
  .long             0x409c0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x81418040

  .align            16
uc_dma_bd8:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x81a0020, @mem_bd3, 8, 0, 0

  .align            4
mem_bd3:
  .long             0x9c0
  .long             0x41380
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x81418040

  .align            16
uc_dma_bd9:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x81a0320, @mem_bd4, 8, 0, 0

  .align            4
mem_bd4:
  .long             0x9c0
  .long             0x41d40
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x81418040

  .align            16
uc_dma_bd10:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a0000, @mem_bd5, 8, 0, 1

  UC_DMA_BD         0x0, 0x21a0020, @mem_bd6, 8, 0, 0

  .align            4
mem_bd5:
  .long             0x1680
  .long             0x81c6000
  .long             0x8c0000
  .long             0x4007f
  .long             0x81203ff
  .long             0xff
  .long             0x0
  .long             0x80808181

  .align            4
mem_bd6:
  .long             0x1680
  .long             0x4603e
  .long             0x640000
  .long             0x4007f
  .long             0x81203ff
  .long             0x2c00ff
  .long             0x0
  .long             0x80808181

  .align            16
uc_dma_bd11:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a0300, @mem_bd7, 8, 0, 1

  UC_DMA_BD         0x0, 0x21a0320, @mem_bd8, 8, 0, 0

  .align            4
mem_bd7:
  .long             0x1680
  .long             0x99c7c00
  .long             0x8c0000
  .long             0x4007f
  .long             0x1403ff
  .long             0xff
  .long             0x0
  .long             0x80808181

  .align            4
mem_bd8:
  .long             0x1680
  .long             0x47c3e
  .long             0x640000
  .long             0x4007f
  .long             0x1403ff
  .long             0x2c00ff
  .long             0x0
  .long             0x80808181

  .align            16
uc_dma_bd12:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a0000, @mem_bd9, 8, 0, 1

  UC_DMA_BD         0x0, 0x41a0020, @mem_bd10, 8, 0, 0

  .align            4
mem_bd9:
  .long             0x1680
  .long             0x81a9c00
  .long             0x8c0000
  .long             0x4007f
  .long             0x1403ff
  .long             0xff
  .long             0x0
  .long             0x80408141

  .align            4
mem_bd10:
  .long             0x1680
  .long             0x29c3e
  .long             0x640000
  .long             0x4007f
  .long             0x1403ff
  .long             0x2c00ff
  .long             0x0
  .long             0x80408141

  .align            16
uc_dma_bd13:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a0300, @mem_bd11, 8, 0, 1

  UC_DMA_BD         0x0, 0x41a0320, @mem_bd12, 8, 0, 0

  .align            4
mem_bd11:
  .long             0x1680
  .long             0x99abc00
  .long             0x8c0000
  .long             0x4007f
  .long             0x1403ff
  .long             0xff
  .long             0x0
  .long             0x80408141

  .align            4
mem_bd12:
  .long             0x1680
  .long             0x2bc3e
  .long             0x640000
  .long             0x4007f
  .long             0x1403ff
  .long             0x2c00ff
  .long             0x0
  .long             0x80408141

  .align            16
uc_dma_bd14:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a0040, @mem_bd13, 8, 0, 1

  UC_DMA_BD         0x0, 0x41a0060, @mem_bd14, 8, 0, 0

  .align            4
mem_bd13:
  .long             0x1680
  .long             0x83adc00
  .long             0x8c0000
  .long             0x4007f
  .long             0x1403ff
  .long             0xff
  .long             0x0
  .long             0x80408141

  .align            4
mem_bd14:
  .long             0x1680
  .long             0x2dc3e
  .long             0x640000
  .long             0x4007f
  .long             0x1403ff
  .long             0x2c00ff
  .long             0x0
  .long             0x80408141

  .align            16
uc_dma_bd15:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x61a0000, @mem_bd15, 8, 0, 1

  UC_DMA_BD         0x0, 0x61a0020, @mem_bd16, 8, 0, 0

  .align            4
mem_bd15:
  .long             0x1680
  .long             0x818fc00
  .long             0x8c0000
  .long             0x4007f
  .long             0x1403ff
  .long             0xff
  .long             0x0
  .long             0x80008101

  .align            4
mem_bd16:
  .long             0x1680
  .long             0xfc3e
  .long             0x640000
  .long             0x4007f
  .long             0x1403ff
  .long             0x2c00ff
  .long             0x0
  .long             0x80008101

  .align            16
uc_dma_bd16:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x61a0300, @mem_bd17, 8, 0, 1

  UC_DMA_BD         0x0, 0x61a0320, @mem_bd18, 8, 0, 0

  .align            4
mem_bd17:
  .long             0x1680
  .long             0x9991c00
  .long             0x8c0000
  .long             0x4007f
  .long             0x1203ff
  .long             0x100000ff
  .long             0x0
  .long             0x80008101

  .align            4
mem_bd18:
  .long             0x1680
  .long             0x11c3e
  .long             0x640000
  .long             0x4007f
  .long             0x1203ff
  .long             0x102c00ff
  .long             0x0
  .long             0x80008101

  .align            16
uc_dma_bd17:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x81a0000, @mem_bd19, 8, 0, 0

  .align            4
mem_bd19:
  .long             0x9c0
  .long             0x40000
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x80408441

  .align            16
uc_dma_bd18:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x81a0300, @mem_bd20, 8, 0, 0

  .align            4
mem_bd20:
  .long             0x9c0
  .long             0x409c0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x80408441

  .align            16
uc_dma_bd19:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x81a0020, @mem_bd21, 8, 0, 0

  .align            4
mem_bd21:
  .long             0x9c0
  .long             0x41380
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x80408441

  .align            16
uc_dma_bd20:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x81a0320, @mem_bd22, 8, 0, 0

  .align            4
mem_bd22:
  .long             0x9c0
  .long             0x41d40
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x80408441

  .align            16
uc_dma_bd21:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a02e0, @mem_bd23, 8, 0, 0

  .align            4
mem_bd23:
  .long             0x400
  .long             0x38000
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81838082

  .align            16
uc_dma_bd22:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a02c0, @mem_bd24, 8, 0, 0

  .align            4
mem_bd24:
  .long             0x400
  .long             0x38100
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81838082

  .align            16
uc_dma_bd23:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a02a0, @mem_bd25, 8, 0, 0

  .align            4
mem_bd25:
  .long             0x400
  .long             0x38200
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81838082

  .align            16
uc_dma_bd24:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a0280, @mem_bd26, 8, 0, 0

  .align            4
mem_bd26:
  .long             0x400
  .long             0x38300
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81838082

  .align            16
uc_dma_bd25:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a05e0, @mem_bd27, 8, 0, 0

  .align            4
mem_bd27:
  .long             0x400
  .long             0x3a000
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81838082

  .align            16
uc_dma_bd26:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a05c0, @mem_bd28, 8, 0, 0

  .align            4
mem_bd28:
  .long             0x400
  .long             0x3a100
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81838082

  .align            16
uc_dma_bd27:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a05a0, @mem_bd29, 8, 0, 0

  .align            4
mem_bd29:
  .long             0x400
  .long             0x3a200
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81838082

  .align            16
uc_dma_bd28:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x21a0580, @mem_bd30, 8, 0, 0

  .align            4
mem_bd30:
  .long             0x400
  .long             0x3a300
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81838082

  .align            16
uc_dma_bd29:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a02e0, @mem_bd31, 8, 0, 0

  .align            4
mem_bd31:
  .long             0x400
  .long             0x1c000
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd30:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a02c0, @mem_bd32, 8, 0, 0

  .align            4
mem_bd32:
  .long             0x400
  .long             0x1c100
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd31:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a02a0, @mem_bd33, 8, 0, 0

  .align            4
mem_bd33:
  .long             0x400
  .long             0x1c200
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd32:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a0280, @mem_bd34, 8, 0, 0

  .align            4
mem_bd34:
  .long             0x400
  .long             0x1c300
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd33:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a05e0, @mem_bd35, 8, 0, 0

  .align            4
mem_bd35:
  .long             0x400
  .long             0x1e000
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd34:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a05c0, @mem_bd36, 8, 0, 0

  .align            4
mem_bd36:
  .long             0x400
  .long             0x1e100
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd35:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a05a0, @mem_bd37, 8, 0, 0

  .align            4
mem_bd37:
  .long             0x400
  .long             0x1e200
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd36:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a0580, @mem_bd38, 8, 0, 0

  .align            4
mem_bd38:
  .long             0x400
  .long             0x1e300
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd37:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a0260, @mem_bd39, 8, 0, 0

  .align            4
mem_bd39:
  .long             0x400
  .long             0x20000
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd38:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a0240, @mem_bd40, 8, 0, 0

  .align            4
mem_bd40:
  .long             0x400
  .long             0x20100
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd39:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a0220, @mem_bd41, 8, 0, 0

  .align            4
mem_bd41:
  .long             0x400
  .long             0x20200
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd40:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a0200, @mem_bd42, 8, 0, 0

  .align            4
mem_bd42:
  .long             0x400
  .long             0x20300
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81438042

  .align            16
uc_dma_bd41:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x61a02e0, @mem_bd43, 8, 0, 0

  .align            4
mem_bd43:
  .long             0x400
  .long             0x2000
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81038002

  .align            16
uc_dma_bd42:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x61a02c0, @mem_bd44, 8, 0, 0

  .align            4
mem_bd44:
  .long             0x400
  .long             0x2100
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81038002

  .align            16
uc_dma_bd43:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x61a02a0, @mem_bd45, 8, 0, 0

  .align            4
mem_bd45:
  .long             0x400
  .long             0x2200
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81038002

  .align            16
uc_dma_bd44:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x61a0280, @mem_bd46, 8, 0, 0

  .align            4
mem_bd46:
  .long             0x400
  .long             0x2300
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81038002

  .align            16
uc_dma_bd45:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x61a05e0, @mem_bd47, 8, 0, 0

  .align            4
mem_bd47:
  .long             0x400
  .long             0x4000
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81038002

  .align            16
uc_dma_bd46:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x61a05c0, @mem_bd48, 8, 0, 0

  .align            4
mem_bd48:
  .long             0x400
  .long             0x4100
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81038002

  .align            16
uc_dma_bd47:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x61a05a0, @mem_bd49, 8, 0, 0

  .align            4
mem_bd49:
  .long             0x400
  .long             0x4200
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81038002

  .align            16
uc_dma_bd48:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x61a0580, @mem_bd50, 8, 0, 0

  .align            4
mem_bd50:
  .long             0x400
  .long             0x4300
  .long             0x800000
  .long             0x4007f
  .long             0x1003ff
  .long             0x0
  .long             0x2003f
  .long             0x81038002

  .align            16
uc_dma_bd49:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x41a0300, @mem_bd51, 8, 0, 0

  .align            4
mem_bd51:
  .long             0xe000
  .long             0x18000
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x8042b843

  .align            16
uc_dma_bd50:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x6009000, @shim_bd5, 9, 0, 0

  .align            4
shim_bd5:
  .long             0xe000
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x0
  .long             0x2043000
  .long             0x0

