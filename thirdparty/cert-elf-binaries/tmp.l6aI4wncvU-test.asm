;
; Code
;

START_JOB 0
  LOAD_PDI 0, @pdi
END_JOB
.eop

START_JOB 1
  ; Set graph iteration(s)
  WRITE_32            0xc50c004, 0x1
  ; Set graph iteration(s)
  WRITE_32            0xe50c004, 0x1
  ; Run graph
  WRITE_32            0xc538000, 0x1
  ; Run graph
  WRITE_32            0xe538000, 0x1
  ; Initialize lock col-6, row-0, lockID-0, value=0
  WRITE_32            0xc000000, 0x0
  ; Initialize lock col-6, row-0, lockID-1, value=0
  WRITE_32            0xc000010, 0x0
  ; Initialize lock col-6, row-0, lockID-0, value=0
  WRITE_32            0xc1c0000, 0x0
  ; Initialize lock col-6, row-0, lockID-1, value=0
  WRITE_32            0xc1c0010, 0x0
  APPLY_OFFSET_57     @shim_bd0, 1, 0
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd0
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 6, row-0, channel 0, direction 1, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0xc009314, 0x0
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd1
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 6, row-1, channel 0, direction 0, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0xc1a0604, 0x0
  ; Acquire lock col-6, row-0, lockID-0, value=1
  POLL_32             0xc1d0204, 0x1
  ; Initialize lock col-6, row-0, lockID-0, value=0
  WRITE_32            0xc000000, 0x0
  ; Initialize lock col-6, row-0, lockID-1, value=0
  WRITE_32            0xc000010, 0x0
  ; Initialize lock col-8, row-0, lockID-0, value=0
  WRITE_32            0x101c0000, 0x0
  ; Initialize lock col-8, row-0, lockID-1, value=0
  WRITE_32            0x101c0010, 0x0
  APPLY_OFFSET_57     @shim_bd1, 1, 1
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd2
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 6, row-0, channel 1, direction 1, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0xc00931c, 0x0
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd3
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 8, row-1, channel 0, direction 0, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0x101a0604, 0x0
  ; Acquire lock col-8, row-0, lockID-0, value=1
  POLL_32             0x101d0204, 0x1
  ; Initialize lock col-6, row-0, lockID-2, value=0
  WRITE_32            0xc1c0020, 0x0
  ; Initialize lock col-6, row-0, lockID-3, value=0
  WRITE_32            0xc1c0030, 0x0
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd4
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 6, row-1, channel 1, direction 1, startBD 24, repeat count 1, enable TCT 0
  WRITE_32            0xc1a063c, 0x18
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd5
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 8, row-1, channel 0, direction 1, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0x101a0634, 0x0
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd6
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 6, row-1, channel 1, direction 0, startBD 25, repeat count 1, enable TCT 0
  WRITE_32            0xc1a060c, 0x19
  ; Acquire lock col-6, row-0, lockID-2, value=1
  POLL_32             0xc1d0a04, 0x1
  ; Initialize lock col-7, row-0, lockID-0, value=0
  WRITE_32            0xe1c0000, 0x0
  ; Initialize lock col-7, row-0, lockID-1, value=0
  WRITE_32            0xe1c0010, 0x0
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd7
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 8, row-1, channel 1, direction 1, startBD 24, repeat count 1, enable TCT 0
  WRITE_32            0x101a063c, 0x18
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd8
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 6, row-1, channel 0, direction 1, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0xc1a0634, 0x0
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd9
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 7, row-1, channel 0, direction 0, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0xe1a0604, 0x0
  ; Acquire lock col-7, row-0, lockID-0, value=1
  POLL_32             0xe1d0204, 0x1
  ; Initialize lock col-6, row-0, lockID-0, value=0
  WRITE_32            0xc000000, 0x0
  ; Initialize lock col-6, row-0, lockID-1, value=0
  WRITE_32            0xc000010, 0x0
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd10
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 7, row-1, channel 1, direction 1, startBD 24, repeat count 1, enable TCT 0
  WRITE_32            0xe1a063c, 0x18
  APPLY_OFFSET_57     @shim_bd2, 1, 2
  ; Wait_handle  descriptor_ptr
  UC_DMA_WRITE_DES    $r0, @uc_dma_bd11
  ; Wait_handle
  WAIT_UC_DMA         $r0
  ; enqueueTask col 6, row-0, channel 0, direction 0, startBD 0, repeat count 1, enable TCT 0
  WRITE_32            0xc009304, 0x0
  ; Acquire lock col-6, row-0, lockID-0, value=1
  POLL_32             0xc00c204, 0x1
  ; End graph
  WRITE_32            0xc538000, 0x0
  ; End graph
  WRITE_32            0xe538000, 0x0
END_JOB

.include vaddpdi.asm
EOF

;
; Data
;

  .align             16
uc_dma_bd0:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xc009000, @shim_bd0, 9, 0, 0

  .align             4
shim_bd0:
  .long              0x20
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x2003000
  .long              0x0

  .align             16
uc_dma_bd1:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xc1a0000, @mem_bd0, 8, 0, 0

  .align             4
mem_bd0:
  .long              0x20
  .long              0x20000
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x81408041

  .align             16
uc_dma_bd2:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xc009000, @shim_bd1, 9, 0, 0

  .align             4
shim_bd1:
  .long              0x10
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x2003000
  .long              0x0

  .align             16
uc_dma_bd3:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x101a0000, @mem_bd1, 8, 0, 0

  .align             4
mem_bd1:
  .long              0x10
  .long              0x20000
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x81408041

  .align             16
uc_dma_bd4:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xc1a0300, @mem_bd2, 8, 0, 0

  .align             4
mem_bd2:
  .long              0x20
  .long              0x20000
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x80418140

  .align             16
uc_dma_bd5:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x101a0000, @mem_bd3, 8, 0, 0

  .align             4
mem_bd3:
  .long              0x10
  .long              0x20000
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x80418140

  .align             16
uc_dma_bd6:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xc1a0320, @mem_bd4, 8, 0, 0

  .align             4
mem_bd4:
  .long              0x20
  .long              0x20400
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x81428043

  .align             16
uc_dma_bd7:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0x101a0300, @mem_bd5, 8, 0, 0

  .align             4
mem_bd5:
  .long              0x10
  .long              0x20000
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x80418140

  .align             16
uc_dma_bd8:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xc1a0000, @mem_bd6, 8, 0, 0

  .align             4
mem_bd6:
  .long              0x20
  .long              0x20400
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x80438142

  .align             16
uc_dma_bd9:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xe1a0000, @mem_bd7, 8, 0, 0

  .align             4
mem_bd7:
  .long              0x20
  .long              0x20000
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x81408041

  .align             16
uc_dma_bd10:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xe1a0300, @mem_bd8, 8, 0, 0

  .align             4
mem_bd8:
  .long              0x20
  .long              0x20000
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x80418140

  .align             16
uc_dma_bd11:
  ; remote_ptr_high, remote_ptr_low, local_ptr_absolute, size (in 32-bit), ctrl_external, ctrl_local_next_BD
  UC_DMA_BD         0x0, 0xc009000, @shim_bd2, 9, 0, 0

  .align             4
shim_bd2:
  .long              0x20
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x0
  .long              0x2041001
  .long              0x0

