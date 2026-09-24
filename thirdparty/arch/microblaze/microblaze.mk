BSP_ROOT = $(ARCH_ROOT)/microblaze/
CFLAGS   = -I$(BSP_ROOT)/include/ -mlittle-endian -mcpu=v11.0 -mxl-barrel-shift -mxl-pattern-compare -mno-xl-soft-div -mno-xl-soft-mul -mxl-reorder -fno-zero-initialized-in-bss -mxl-frequency
LFLAGS   = -L$(BSP_ROOT)/lib/ -Wl,--gc-sections -Wl,--start-group,-lxil,-lgcc,-lc,--end-group
CC 			 = mb-gcc
