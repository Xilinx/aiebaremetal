// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef _UNDEFELF_H_
#undef _UNDEFELF_H_

/* Type for a 16-bit quantity.  */
#undef Elf32_Half
/* The ELF file header.  This appears at the start of every ELF file.  */

#undef EI_NIDENT
/* Fields in the e_ident array.  The EI_* macros are indices into the
   array.  The macros under each EI_* macro are the values the byte
   may have.  */

#undef EI_MAG0
#undef ELFMAG0

#undef EI_MAG1
#undef ELFMAG1

#undef EI_MAG2
#undef ELFMAG2

#undef EI_MAG3
#undef ELFMAG3

/* Conglomeration of the identification bytes, for easy testing as a word.  */
#undef ELFMAG
#undef SELFMAG

#undef EI_CLASS
#undef ELFCLASSNONE
#undef ELFCLASS32
#undef ELFCLASS64
#undef ELFCLASSNUM

#undef EI_DATA
#undef ELFDATANONE
#undef ELFDATA2LSB
#undef ELFDATA2MSB
#undef ELFDATANUM

#undef EI_VERSION

#undef EI_OSABI
#undef ELFOSABI_NONE
#undef ELFOSABI_SYSV
#undef ELFOSABI_HPUX
#undef ELFOSABI_NETBSD
#undef ELFOSABI_GNU
#undef ELFOSABI_LINUX
#undef ELFOSABI_SOLARIS
#undef ELFOSABI_AIX
#undef ELFOSABI_IRIX
#undef ELFOSABI_FREEBSD
#undef ELFOSABI_TRU64
#undef ELFOSABI_MODESTO
#undef ELFOSABI_OPENBSD
#undef ELFOSABI_ARM_AEABI
#undef ELFOSABI_ARM
#undef ELFOSABI_STANDALONE

#undef EI_ABIVERSION

#undef EI_PAD

/* Legal values for e_type (object file type).  */

#undef ET_NONE
#undef ET_REL
#undef ET_EXEC
#undef ET_DYN
#undef ET_CORE
#undef ET_NUM
#undef ET_LOOS
#undef ET_HIOS
#undef ET_LOPROC
#undef ET_HIPROC

/* Legal values for e_machine (architecture).  */

#undef EM_NONE
#undef EM_M32
#undef EM_SPARC
#undef EM_386
#undef EM_68K
#undef EM_88K
#undef EM_IAMCU
#undef EM_860
#undef EM_MIPS
#undef EM_S370
#undef EM_MIPS_RS3_LE
/* reserved 11-14 */
#undef EM_PARISC
/* reserved 16 */
#undef EM_VPP500
#undef EM_SPARC32PLUS
#undef EM_960
#undef EM_PPC
#undef EM_PPC64
#undef EM_S390
#undef EM_SPU
/* reserved 24-35 */
#undef EM_V800
#undef EM_FR20
#undef EM_RH32
#undef EM_RCE
#undef EM_ARM
#undef EM_FAKE_ALPHA
#undef EM_SH
#undef EM_SPARCV9
#undef EM_TRICORE
#undef EM_ARC
#undef EM_H8_300
#undef EM_H8_300H
#undef EM_H8S
#undef EM_H8_500
#undef EM_IA_64
#undef EM_MIPS_X
#undef EM_COLDFIRE
#undef EM_68HC12
#undef EM_MMA
#undef EM_PCP
#undef EM_NCPU
#undef EM_NDR1
#undef EM_STARCORE
#undef EM_ME16
#undef EM_ST100
#undef EM_TINYJ
#undef EM_X86_64
#undef EM_PDSP
#undef EM_PDP10
#undef EM_PDP11
#undef EM_FX66
#undef EM_ST9PLUS
#undef EM_ST7
#undef EM_68HC16
#undef EM_68HC11
#undef EM_68HC08
#undef EM_68HC05
#undef EM_SVX
#undef EM_ST19
#undef EM_VAX
#undef EM_CRIS
#undef EM_JAVELIN
#undef EM_FIREPATH
#undef EM_ZSP
#undef EM_MMIX
#undef EM_HUANY
#undef EM_PRISM
#undef EM_AVR
#undef EM_FR30
#undef EM_D10V
#undef EM_D30V
#undef EM_V850
#undef EM_M32R
#undef EM_MN10300
#undef EM_MN10200
#undef EM_PJ
#undef EM_OPENRISC
#undef EM_ARC_COMPACT
#undef EM_XTENSA
#undef EM_VIDEOCORE
#undef EM_TMM_GPP
#undef EM_NS32K
#undef EM_TPC
#undef EM_SNP1K
#undef EM_ST200
#undef EM_IP2K
#undef EM_MAX
#undef EM_CR
#undef EM_F2MC16
#undef EM_MSP430
#undef EM_BLACKFIN
#undef EM_SE_C33
#undef EM_SEP
#undef EM_ARCA
#undef EM_UNICORE
#undef EM_EXCESS
#undef EM_DXP
#undef EM_ALTERA_NIOS2
#undef EM_CRX
#undef EM_XGATE
#undef EM_C166
#undef EM_M16C
#undef EM_DSPIC30F
#undef EM_CE
#undef EM_M32C
/* reserved 121-130 */
#undef EM_TSK3000
#undef EM_RS08
#undef EM_SHARC
#undef EM_ECOG2
#undef EM_SCORE7
#undef EM_DSP24
#undef EM_VIDEOCORE3
#undef EM_LATTICEMICO32
#undef EM_SE_C17
#undef EM_TI_C6000
#undef EM_TI_C2000
#undef EM_TI_C5500
#undef EM_TI_ARP32
#undef EM_TI_PRU
/* reserved 145-159 */
#undef EM_MMDSP_PLUS
#undef EM_CYPRESS_M8C
#undef EM_R32C
#undef EM_TRIMEDIA
#undef EM_QDSP6
#undef EM_8051
#undef EM_STXP7X
#undef EM_NDS32
#undef EM_ECOG1X
#undef EM_MAXQ30
#undef EM_XIMO16
#undef EM_MANIK
#undef EM_CRAYNV2
#undef EM_RX
#undef EM_METAG
#undef EM_MCST_ELBRUS
#undef EM_ECOG16
#undef EM_CR16
#undef EM_ETPU
#undef EM_SLE9X
#undef EM_L10M
#undef EM_K10M
#undef EM_AARCH64
#undef EM_AVR32
#undef EM_STM8
#undef EM_TILE64
#undef EM_TILEPRO
#undef EM_MICROBLAZE
#undef EM_CUDA
#undef EM_TILEGX
#undef EM_CLOUDSHIELD
#undef EM_COREA_1ST
#undef EM_COREA_2ND
#undef EM_ARCV2
#undef EM_OPEN8
#undef EM_RL78
#undef EM_VIDEOCORE5
#undef EM_78KOR
#undef EM_56800EX
#undef EM_BA1
#undef EM_BA2
#undef EM_XCORE
#undef EM_MCHP_PIC
#undef EM_INTELGT
/* reserved 206-209 */
#undef EM_KM32
#undef EM_KMX32
#undef EM_EMX16
#undef EM_EMX8
#undef EM_KVARC
#undef EM_CDP
#undef EM_COGE
#undef EM_COOL
#undef EM_NORC
#undef EM_CSR_KALIMBA
#undef EM_Z80
#undef EM_VISIUM
#undef EM_FT32
#undef EM_MOXIE
#undef EM_AMDGPU
#undef EM_RISCV

#undef EM_BPF
#undef EM_CSKY

#undef EM_NUM

/* Old spellings/synonyms.  */

#undef EM_ARC_A5

/* If it is necessary to assign new unofficial EM_* values, please
   pick large random numbers (0x8523, 0xa7f2, etc.) to minimize the
   chances of collision with official or non-GNU unofficial values.  */

#undef EM_ALPHA

/* Legal values for e_version (version).  */

#undef EV_NONE
#undef EV_CURRENT
#undef EV_NUM

/* Section header.  */
/* Special section indices.  */

#undef SHN_UNDEF
#undef SHN_LORESERVE
#undef SHN_LOPROC
#undef SHN_BEFORE
#undef SHN_AFTER
#undef SHN_HIPROC
#undef SHN_LOOS
#undef SHN_HIOS
#undef SHN_ABS
#undef SHN_COMMON
#undef SHN_XINDEX
#undef SHN_HIRESERVE

/* Legal values for sh_type (section type).  */

#undef SHT_NULL
#undef SHT_PROGBITS
#undef SHT_SYMTAB
#undef SHT_STRTAB
#undef SHT_RELA
#undef SHT_HASH
#undef SHT_DYNAMIC
#undef SHT_NOTE
#undef SHT_NOBITS
#undef SHT_REL
#undef SHT_SHLIB
#undef SHT_DYNSYM
#undef SHT_INIT_ARRAY
#undef SHT_FINI_ARRAY
#undef SHT_PREINIT_ARRAY
#undef SHT_GROUP
#undef SHT_SYMTAB_SHNDX
#undef SHT_NUM
#undef SHT_LOOS
#undef SHT_GNU_ATTRIBUTES
#undef SHT_GNU_HASH
#undef SHT_GNU_LIBLIST
#undef SHT_CHECKSUM
#undef SHT_LOSUNW
#undef SHT_SUNW_move
#undef SHT_SUNW_COMDAT
#undef SHT_SUNW_syminfo
#undef SHT_GNU_verdef
#undef SHT_GNU_verneed
#undef SHT_GNU_versym
#undef SHT_HISUNW
#undef SHT_HIOS
#undef SHT_LOPROC
#undef SHT_HIPROC
#undef SHT_LOUSER
#undef SHT_HIUSER

/* Legal values for sh_flags (section flags).  */

#undef SHF_WRITE
#undef SHF_ALLOC
#undef SHF_EXECINSTR
#undef SHF_MERGE
#undef SHF_STRINGS
#undef SHF_INFO_LINK
#undef SHF_LINK_ORDER
#undef SHF_OS_NONCONFORMING
#undef SHF_GROUP
#undef SHF_TLS
#undef SHF_COMPRESSED
#undef SHF_MASKOS
#undef SHF_MASKPROC
#undef SHF_GNU_RETAIN
#undef SHF_ORDERED
#undef SHF_EXCLUDE

/* Section compression header.  Used when SHF_COMPRESSED is set.  */
/* Legal values for ch_type (compression algorithm).  */
#undef ELFCOMPRESS_ZLIB
#undef ELFCOMPRESS_LOOS
#undef ELFCOMPRESS_HIOS
#undef ELFCOMPRESS_LOPROC
#undef ELFCOMPRESS_HIPROC

/* Section group handling.  */
#undef GRP_COMDAT

/* Symbol table entry.  */
/* Possible values for si_boundto.  */
#undef SYMINFO_BT_SELF
#undef SYMINFO_BT_PARENT
#undef SYMINFO_BT_LOWRESERVE

/* Possible bitmasks for si_flags.  */
#undef SYMINFO_FLG_DIRECT
#undef SYMINFO_FLG_PASSTHRU
#undef SYMINFO_FLG_COPY
#undef SYMINFO_FLG_LAZYLOAD
/* Syminfo version values.  */
#undef SYMINFO_NONE
#undef SYMINFO_CURRENT
#undef SYMINFO_NUM

/* How to extract and insert information held in the st_info field.  */

#undef ELF32_ST_BIND
#undef ELF32_ST_TYPE
#undef ELF32_ST_INFO

/* Both Elf32_Sym and Elf64_Sym use the same one-byte st_info field.  */
#undef ELF64_ST_BIND
#undef ELF64_ST_TYPE
#undef ELF64_ST_INFO

/* Legal values for ST_BIND subfield of st_info (symbol binding).  */

#undef STB_LOCAL
#undef STB_GLOBAL
#undef STB_WEAK
#undef STB_NUM
#undef STB_LOOS
#undef STB_GNU_UNIQUE
#undef STB_HIOS
#undef STB_LOPROC
#undef STB_HIPROC

/* Legal values for ST_TYPE subfield of st_info (symbol type).  */

#undef STT_NOTYPE
#undef STT_OBJECT
#undef STT_FUNC
#undef STT_SECTION
#undef STT_FILE
#undef STT_COMMON
#undef STT_TLS
#undef STT_NUM
#undef STT_LOOS
#undef STT_GNU_IFUNC
#undef STT_HIOS
#undef STT_LOPROC
#undef STT_HIPROC

/* Symbol table indices are found in the hash buckets and chain table
   of a symbol hash table section.  This special index value indicates
   the end of a chain, meaning no further symbols are found in that bucket.  */

#undef STN_UNDEF

/* How to extract and insert information held in the st_other field.  */

#undef ELF32_ST_VISIBILITY

/* For ELF64 the definitions are the same.  */
#undef ELF64_ST_VISIBILITY

/* Symbol visibility specification encoded in the st_other field.  */
#undef STV_DEFAULT
#undef STV_INTERNAL
#undef STV_HIDDEN
#undef STV_PROTECTED

/* Relocation table entry without addend (in section of type SHT_REL).  */
/* How to extract and insert information held in the r_info field.  */

#undef ELF32_R_SYM
#undef ELF32_R_TYPE
#undef ELF32_R_INFO

#undef ELF64_R_SYM
#undef ELF64_R_TYPE
#undef ELF64_R_INFO

/* Program segment header.  */
/* Special value for e_phnum.  This indicates that the real number of
   program headers is too large to fit into e_phnum.  Instead the real
   value is in the field sh_info of section 0.  */

#undef PN_XNUM

/* Legal values for p_type (segment type).  */

#undef PT_NULL
#undef PT_LOAD
#undef PT_DYNAMIC
#undef PT_INTERP
#undef PT_NOTE
#undef PT_SHLIB
#undef PT_PHDR
#undef PT_TLS
#undef PT_NUM
#undef PT_LOOS
#undef PT_GNU_EH_FRAME
#undef PT_GNU_STACK
#undef PT_GNU_RELRO
#undef PT_GNU_PROPERTY
#undef PT_LOSUNW
#undef PT_SUNWBSS
#undef PT_SUNWSTACK
#undef PT_HISUNW
#undef PT_HIOS
#undef PT_LOPROC
#undef PT_HIPROC

/* Legal values for p_flags (segment flags).  */

#undef PF_X
#undef PF_W
#undef PF_R
#undef PF_MASKOS
#undef PF_MASKPROC

/* Legal values for note segment descriptor types for core files. */

#undef NT_PRSTATUS
#undef NT_PRFPREG
#undef NT_FPREGSET
#undef NT_PRPSINFO
#undef NT_PRXREG
#undef NT_TASKSTRUCT
#undef NT_PLATFORM
#undef NT_AUXV
#undef NT_GWINDOWS
#undef NT_ASRS
#undef NT_PSTATUS
#undef NT_PSINFO
#undef NT_PRCRED
#undef NT_UTSNAME
#undef NT_LWPSTATUS
#undef NT_LWPSINFO
#undef NT_PRFPXREG
#undef NT_SIGINFO
#undef NT_FILE
#undef NT_PRXFPREG
#undef NT_PPC_VMX
#undef NT_PPC_SPE
#undef NT_PPC_VSX
#undef NT_PPC_TAR
#undef NT_PPC_PPR
#undef NT_PPC_DSCR
#undef NT_PPC_EBB
#undef NT_PPC_PMU
#undef NT_PPC_TM_CGPR
#undef NT_PPC_TM_CFPR
#undef NT_PPC_TM_CVMX
#undef NT_PPC_TM_CVSX
#undef NT_PPC_TM_SPR
#undef NT_PPC_TM_CTAR
#undef NT_PPC_TM_CPPR
#undef NT_PPC_TM_CDSCR
#undef NT_PPC_PKEY
#undef NT_386_TLS
#undef NT_386_IOPERM
#undef NT_X86_XSTATE
#undef NT_S390_HIGH_GPRS
#undef NT_S390_TIMER
#undef NT_S390_TODCMP
#undef NT_S390_TODPREG
#undef NT_S390_CTRS
#undef NT_S390_PREFIX
#undef NT_S390_LAST_BREAK
#undef NT_S390_SYSTEM_CALL
#undef NT_S390_TDB
#undef NT_S390_VXRS_LOW
#undef NT_S390_VXRS_HIGH
#undef NT_S390_GS_CB
#undef NT_S390_GS_BC
#undef NT_S390_RI_CB
#undef NT_ARM_VFP
#undef NT_ARM_TLS
#undef NT_ARM_HW_BREAK
#undef NT_ARM_HW_WATCH
#undef NT_ARM_SYSTEM_CALL
#undef NT_ARM_SVE
#undef NT_ARM_PAC_MASK
#undef NT_ARM_PACA_KEYS
#undef NT_ARM_PACG_KEYS
#undef NT_ARM_TAGGED_ADDR_CTRL
#undef NT_ARM_PAC_ENABLED_KEYS
#undef NT_VMCOREDD
#undef NT_MIPS_DSP
#undef NT_MIPS_FP_MODE
#undef NT_MIPS_MSA

/* Legal values for the note segment descriptor types for object files.  */

#undef NT_VERSION

/* Dynamic section entry.  */
/* Legal values for d_tag (dynamic entry type).  */

#undef DT_NULL
#undef DT_NEEDED
#undef DT_PLTRELSZ
#undef DT_PLTGOT
#undef DT_HASH
#undef DT_STRTAB
#undef DT_SYMTAB
#undef DT_RELA
#undef DT_RELASZ
#undef DT_RELAENT
#undef DT_STRSZ
#undef DT_SYMENT
#undef DT_INIT
#undef DT_FINI
#undef DT_SONAME
#undef DT_RPATH
#undef DT_SYMBOLIC
#undef DT_REL
#undef DT_RELSZ
#undef DT_RELENT
#undef DT_PLTREL
#undef DT_DEBUG
#undef DT_TEXTREL
#undef DT_JMPREL
#undef DT_BIND_NOW
#undef DT_INIT_ARRAY
#undef DT_FINI_ARRAY
#undef DT_INIT_ARRAYSZ
#undef DT_FINI_ARRAYSZ
#undef DT_RUNPATH
#undef DT_FLAGS
#undef DT_ENCODING
#undef DT_PREINIT_ARRAY
#undef DT_PREINIT_ARRAYSZ
#undef DT_SYMTAB_SHNDX
#undef DT_NUM
#undef DT_LOOS
#undef DT_HIOS
#undef DT_LOPROC
#undef DT_HIPROC
#undef DT_PROCNUM

/* DT_* entries which fall between DT_VALRNGHI & DT_VALRNGLO use the
   Dyn.d_un.d_val field of the Elf*_Dyn structure.  This follows Sun's
   approach.  */
#undef DT_VALRNGLO
#undef DT_GNU_PRELINKED
#undef DT_GNU_CONFLICTSZ
#undef DT_GNU_LIBLISTSZ
#undef DT_CHECKSUM
#undef DT_PLTPADSZ
#undef DT_MOVEENT
#undef DT_MOVESZ
#undef DT_FEATURE_1
#undef DT_POSFLAG_1
#undef DT_SYMINSZ
#undef DT_SYMINENT
#undef DT_VALRNGHI
#undef DT_VALTAGIDX
#undef DT_VALNUM

/* DT_* entries which fall between DT_ADDRRNGHI & DT_ADDRRNGLO use the
   Dyn.d_un.d_ptr field of the Elf*_Dyn structure.

   If any adjustment is made to the ELF object after it has been
   built these entries will need to be adjusted.  */
#undef DT_ADDRRNGLO
#undef DT_GNU_HASH
#undef DT_TLSDESC_PLT
#undef DT_TLSDESC_GOT
#undef DT_GNU_CONFLICT
#undef DT_GNU_LIBLIST
#undef DT_CONFIG
#undef DT_DEPAUDIT
#undef DT_AUDIT
#undef DT_PLTPAD
#undef DT_MOVETAB
#undef DT_SYMINFO
#undef DT_ADDRRNGHI
#undef DT_ADDRTAGIDX
#undef DT_ADDRNUM

/* The versioning entry types.  The next are defined as part of the
   GNU extension.  */
#undef DT_VERSYM

#undef DT_RELACOUNT
#undef DT_RELCOUNT

/* These were chosen by Sun.  */
#undef DT_FLAGS_1
#undef DT_VERDEF
#undef DT_VERDEFNUM
#undef DT_VERNEED
#undef DT_VERNEEDNUM
#undef DT_VERSIONTAGIDX
#undef DT_VERSIONTAGNUM

/* Sun added these machine-independent extensions in the "processor-specific"
   range.  Be compatible.  */
#undef DT_AUXILIARY
#undef DT_FILTER
#undef DT_EXTRATAGIDX
#undef DT_EXTRANUM

/* Values of `d_un.d_val' in the DT_FLAGS entry.  */
#undef DF_ORIGIN
#undef DF_SYMBOLIC
#undef DF_TEXTREL
#undef DF_BIND_NOW
#undef DF_STATIC_TLS

/* State flags selectable in the `d_un.d_val' element of the DT_FLAGS_1
   entry in the dynamic section.  */
#undef DF_1_NOW
#undef DF_1_GLOBAL
#undef DF_1_GROUP
#undef DF_1_NODELETE
#undef DF_1_LOADFLTR
#undef DF_1_INITFIRST
#undef DF_1_NOOPEN
#undef DF_1_ORIGIN
#undef DF_1_DIRECT
#undef DF_1_TRANS
#undef DF_1_INTERPOSE
#undef DF_1_NODEFLIB
#undef DF_1_NODUMP
#undef DF_1_CONFALT
#undef DF_1_ENDFILTEE
#undef DF_1_DISPRELDNE
#undef DF_1_DISPRELPND
#undef DF_1_NODIRECT
#undef DF_1_IGNMULDEF
#undef DF_1_NOKSYMS
#undef DF_1_NOHDR
#undef DF_1_EDITED
#undef DF_1_NORELOC
#undef DF_1_SYMINTPOSE
#undef DF_1_GLOBAUDIT
#undef DF_1_SINGLETON
#undef DF_1_STUB
#undef DF_1_PIE
#undef DF_1_KMOD
#undef DF_1_WEAKFILTER
#undef DF_1_NOCOMMON

/* Flags for the feature selection in DT_FEATURE_1.  */
#undef DTF_1_PARINIT
#undef DTF_1_CONFEXP

/* Flags in the DT_POSFLAG_1 entry effecting only the next DT_* entry.  */
#undef DF_P1_LAZYLOAD
#undef DF_P1_GROUPPERM

/* Legal values for vd_version (version revision).  */
#undef VER_DEF_NONE
#undef VER_DEF_CURRENT
#undef VER_DEF_NUM

/* Legal values for vd_flags (version information flags).  */
#undef VER_FLG_BASE
#undef VER_FLG_WEAK

/* Versym symbol index values.  */
#undef VER_NDX_LOCAL
#undef VER_NDX_GLOBAL
#undef VER_NDX_LORESERVE
#undef VER_NDX_ELIMINATE

/* Auxiliary version information.  */

/* Legal values for vna_flags.  */
#undef VER_FLG_WEAK
/* Legal values for a_type (entry type).  */

#undef AT_NULL
#undef AT_IGNORE
#undef AT_EXECFD
#undef AT_PHDR
#undef AT_PHENT
#undef AT_PHNUM
#undef AT_PAGESZ
#undef AT_BASE
#undef AT_FLAGS
#undef AT_ENTRY
#undef AT_NOTELF
#undef AT_UID
#undef AT_EUID
#undef AT_GID
#undef AT_EGID
#undef AT_CLKTCK

/* Some more special a_type values describing the hardware.  */
#undef AT_PLATFORM
#undef AT_HWCAP

/* This entry gives some information about the FPU initialization
   performed by the kernel.  */
#undef AT_FPUCW

/* Cache block sizes.  */
#undef AT_DCACHEBSIZE
#undef AT_ICACHEBSIZE
#undef AT_UCACHEBSIZE

/* A special ignored value for PPC, used by the kernel to control the
   interpretation of the AUXV. Must be > 16.  */
#undef AT_IGNOREPPC

#undef AT_SECURE

#undef AT_BASE_PLATFORM

#undef AT_RANDOM

#undef AT_HWCAP2

#undef AT_EXECFN

/* Pointer to the global system page used for system calls and other
   nice things.  */
#undef AT_SYSINFO
#undef AT_SYSINFO_EHDR

/* Shapes of the caches.  Bits 0-3 contains associativity; bits 4-7 contains
   log2 of line size; mask those to get cache size.  */
#undef AT_L1I_CACHESHAPE
#undef AT_L1D_CACHESHAPE
#undef AT_L2_CACHESHAPE
#undef AT_L3_CACHESHAPE

/* Shapes of the caches, with more room to describe them.
   *GEOMETRY are comprised of cache line size in bytes in the bottom 16 bits
   and the cache associativity in the next 16 bits.  */
#undef AT_L1I_CACHESIZE
#undef AT_L1I_CACHEGEOMETRY
#undef AT_L1D_CACHESIZE
#undef AT_L1D_CACHEGEOMETRY
#undef AT_L2_CACHESIZE
#undef AT_L2_CACHEGEOMETRY
#undef AT_L3_CACHESIZE
#undef AT_L3_CACHEGEOMETRY

#undef AT_MINSIGSTKSZ

/* Note section contents.  Each entry in the note section begins with
   a header of a fixed form.  */
/* Known names of notes.  */

/* Solaris entries in the note section have this name.  */
#undef ELF_NOTE_SOLARIS

/* Note entries for GNU systems have this name.  */
#undef ELF_NOTE_GNU

/* Note entries for freedesktop.org have this name.  */
#undef ELF_NOTE_FDO

/* Defined types of notes for Solaris.  */

/* Value of descriptor (one word) is desired pagesize for the binary.  */
#undef ELF_NOTE_PAGESIZE_HINT

/* Defined note types for GNU systems.  */

/* ABI information.  The descriptor consists of words:
   word 0: OS descriptor
   word 1: major version of the ABI
   word 2: minor version of the ABI
   word 3: subminor version of the ABI
*/
#undef NT_GNU_ABI_TAG
#undef ELF_NOTE_ABI

/* Known OSes.  These values can appear in word 0 of an
   NT_GNU_ABI_TAG note section entry.  */
#undef ELF_NOTE_OS_LINUX
#undef ELF_NOTE_OS_GNU
#undef ELF_NOTE_OS_SOLARIS2
#undef ELF_NOTE_OS_FREEBSD

/* Synthetic hwcap information.  The descriptor begins with two words:
   word 0: number of entries
   word 1: bitmask of enabled entries
   Then follow variable-length entries, one byte followed by a
   '\0'-terminated hwcap name string.  The byte gives the bit
   number to test if enabled, (1U << bit) & bitmask.  */
#undef NT_GNU_HWCAP

/* Build ID bits as generated by ld --build-id.
   The descriptor consists of any nonzero number of bytes.  */
#undef NT_GNU_BUILD_ID

/* Version note generated by GNU gold containing a version string.  */
#undef NT_GNU_GOLD_VERSION

/* Program property.  */
#undef NT_GNU_PROPERTY_TYPE_0

/* Packaging metadata as defined on
   https://systemd.io/COREDUMP_PACKAGE_METADATA/ */
#undef NT_FDO_PACKAGING_METADATA

/* Note section name of program property.   */
#undef NOTE_GNU_PROPERTY_SECTION_NAME

/* Values used in GNU .note.gnu.property notes (NT_GNU_PROPERTY_TYPE_0).  */

/* Stack size.  */
#undef GNU_PROPERTY_STACK_SIZE
/* No copy relocation on protected data symbol.  */
#undef GNU_PROPERTY_NO_COPY_ON_PROTECTED

/* A 4-byte unsigned integer property: A bit is set if it is set in all
   relocatable inputs.  */
#undef GNU_PROPERTY_UINT32_AND_LO
#undef GNU_PROPERTY_UINT32_AND_HI

/* A 4-byte unsigned integer property: A bit is set if it is set in any
   relocatable inputs.  */
#undef GNU_PROPERTY_UINT32_OR_LO
#undef GNU_PROPERTY_UINT32_OR_HI

/* The needed properties by the object file.  */
#undef GNU_PROPERTY_1_NEEDED

/* Set if the object file requires canonical function pointers and
   cannot be used with copy relocation.  */
#undef GNU_PROPERTY_1_NEEDED_INDIRECT_EXTERN_ACCESS

/* Processor-specific semantics, lo */
#undef GNU_PROPERTY_LOPROC
/* Processor-specific semantics, hi */
#undef GNU_PROPERTY_HIPROC
/* Application-specific semantics, lo */
#undef GNU_PROPERTY_LOUSER
/* Application-specific semantics, hi */
#undef GNU_PROPERTY_HIUSER

/* AArch64 specific GNU properties.  */
#undef GNU_PROPERTY_AARCH64_FEATURE_1_AND

#undef GNU_PROPERTY_AARCH64_FEATURE_1_BTI
#undef GNU_PROPERTY_AARCH64_FEATURE_1_PAC

/* The x86 instruction sets indicated by the corresponding bits are
   used in program.  Their support in the hardware is optional.  */
#undef GNU_PROPERTY_X86_ISA_1_USED
/* The x86 instruction sets indicated by the corresponding bits are
   used in program and they must be supported by the hardware.   */
#undef GNU_PROPERTY_X86_ISA_1_NEEDED
/* X86 processor-specific features used in program.  */
#undef GNU_PROPERTY_X86_FEATURE_1_AND

/* GNU_PROPERTY_X86_ISA_1_BASELINE: CMOV, CX8 (cmpxchg8b), FPU (fld),
   MMX, OSFXSR (fxsave), SCE (syscall), SSE and SSE2.  */
#undef GNU_PROPERTY_X86_ISA_1_BASELINE
/* GNU_PROPERTY_X86_ISA_1_V2: GNU_PROPERTY_X86_ISA_1_BASELINE,
   CMPXCHG16B (cmpxchg16b), LAHF-SAHF (lahf), POPCNT (popcnt), SSE3,
   SSSE3, SSE4.1 and SSE4.2.  */
#undef GNU_PROPERTY_X86_ISA_1_V2
/* GNU_PROPERTY_X86_ISA_1_V3: GNU_PROPERTY_X86_ISA_1_V2, AVX, AVX2, BMI1,
   BMI2, F16C, FMA, LZCNT, MOVBE, XSAVE.  */
#undef GNU_PROPERTY_X86_ISA_1_V3
/* GNU_PROPERTY_X86_ISA_1_V4: GNU_PROPERTY_X86_ISA_1_V3, AVX512F,
   AVX512BW, AVX512CD, AVX512DQ and AVX512VL.  */
#undef GNU_PROPERTY_X86_ISA_1_V4

/* This indicates that all executable sections are compatible with
   IBT.  */
#undef GNU_PROPERTY_X86_FEATURE_1_IBT
/* This indicates that all executable sections are compatible with
   SHSTK.  */
#undef GNU_PROPERTY_X86_FEATURE_1_SHSTK
/* Macro to construct move records.  */
#undef ELF32_M_SYM
#undef ELF32_M_SIZE
#undef ELF32_M_INFO

#undef ELF64_M_SYM
#undef ELF64_M_SIZE
#undef ELF64_M_INFO

/* Motorola 68k specific definitions.  */

/* Values for Elf32_Ehdr.e_flags.  */
#undef EF_CPU32

/* m68k relocs.  */

#undef R_68K_NONE
#undef R_68K_32
#undef R_68K_16
#undef R_68K_8
#undef R_68K_PC32
#undef R_68K_PC16
#undef R_68K_PC8
#undef R_68K_GOT32
#undef R_68K_GOT16
#undef R_68K_GOT8
#undef R_68K_GOT32O
#undef R_68K_GOT16O
#undef R_68K_GOT8O
#undef R_68K_PLT32
#undef R_68K_PLT16
#undef R_68K_PLT8
#undef R_68K_PLT32O
#undef R_68K_PLT16O
#undef R_68K_PLT8O
#undef R_68K_COPY
#undef R_68K_GLOB_DAT
#undef R_68K_JMP_SLOT
#undef R_68K_RELATIVE
#undef R_68K_TLS_GD32
#undef R_68K_TLS_GD16
#undef R_68K_TLS_GD8
#undef R_68K_TLS_LDM32
#undef R_68K_TLS_LDM16
#undef R_68K_TLS_LDM8
#undef R_68K_TLS_LDO32
#undef R_68K_TLS_LDO16
#undef R_68K_TLS_LDO8
#undef R_68K_TLS_IE32
#undef R_68K_TLS_IE16
#undef R_68K_TLS_IE8
#undef R_68K_TLS_LE32
#undef R_68K_TLS_LE16
#undef R_68K_TLS_LE8
#undef R_68K_TLS_DTPMOD32
#undef R_68K_TLS_DTPREL32
#undef R_68K_TLS_TPREL32
/* Keep this the last entry.  */
#undef R_68K_NUM

/* Intel 80386 specific definitions.  */

/* i386 relocs.  */

#undef R_386_NONE
#undef R_386_32
#undef R_386_PC32
#undef R_386_GOT32
#undef R_386_PLT32
#undef R_386_COPY
#undef R_386_GLOB_DAT
#undef R_386_JMP_SLOT
#undef R_386_RELATIVE
#undef R_386_GOTOFF
#undef R_386_GOTPC
#undef R_386_32PLT
#undef R_386_TLS_TPOFF
#undef R_386_TLS_IE
#undef R_386_TLS_GOTIE
#undef R_386_TLS_LE
#undef R_386_TLS_GD
#undef R_386_TLS_LDM
#undef R_386_16
#undef R_386_PC16
#undef R_386_8
#undef R_386_PC8
#undef R_386_TLS_GD_32
#undef R_386_TLS_GD_PUSH
#undef R_386_TLS_GD_CALL
#undef R_386_TLS_GD_POP
#undef R_386_TLS_LDM_32
#undef R_386_TLS_LDM_PUSH
#undef R_386_TLS_LDM_CALL
#undef R_386_TLS_LDM_POP
#undef R_386_TLS_LDO_32
#undef R_386_TLS_IE_32
#undef R_386_TLS_LE_32
#undef R_386_TLS_DTPMOD32
#undef R_386_TLS_DTPOFF32
#undef R_386_TLS_TPOFF32
#undef R_386_SIZE32
#undef R_386_TLS_GOTDESC
#undef R_386_TLS_DESC_CALL
#undef R_386_TLS_DESC
#undef R_386_IRELATIVE
#undef R_386_GOT32X
/* Keep this the last entry.  */
#undef R_386_NUM

/* SUN SPARC specific definitions.  */

/* Legal values for ST_TYPE subfield of st_info (symbol type).  */

#undef STT_SPARC_REGISTER

/* Values for Elf64_Ehdr.e_flags.  */

#undef EF_SPARCV9_MM
#undef EF_SPARCV9_TSO
#undef EF_SPARCV9_PSO
#undef EF_SPARCV9_RMO
#undef EF_SPARC_LEDATA
#undef EF_SPARC_EXT_MASK
#undef EF_SPARC_32PLUS
#undef EF_SPARC_SUN_US1
#undef EF_SPARC_HAL_R1
#undef EF_SPARC_SUN_US3

/* SPARC relocs.  */

#undef R_SPARC_NONE
#undef R_SPARC_8
#undef R_SPARC_16
#undef R_SPARC_32
#undef R_SPARC_DISP8
#undef R_SPARC_DISP16
#undef R_SPARC_DISP32
#undef R_SPARC_WDISP30
#undef R_SPARC_WDISP22
#undef R_SPARC_HI22
#undef R_SPARC_22
#undef R_SPARC_13
#undef R_SPARC_LO10
#undef R_SPARC_GOT10
#undef R_SPARC_GOT13
#undef R_SPARC_GOT22
#undef R_SPARC_PC10
#undef R_SPARC_PC22
#undef R_SPARC_WPLT30
#undef R_SPARC_COPY
#undef R_SPARC_GLOB_DAT
#undef R_SPARC_JMP_SLOT
#undef R_SPARC_RELATIVE
#undef R_SPARC_UA32

/* Additional Sparc64 relocs.  */

#undef R_SPARC_PLT32
#undef R_SPARC_HIPLT22
#undef R_SPARC_LOPLT10
#undef R_SPARC_PCPLT32
#undef R_SPARC_PCPLT22
#undef R_SPARC_PCPLT10
#undef R_SPARC_10
#undef R_SPARC_11
#undef R_SPARC_64
#undef R_SPARC_OLO10
#undef R_SPARC_HH22
#undef R_SPARC_HM10
#undef R_SPARC_LM22
#undef R_SPARC_PC_HH22
#undef R_SPARC_PC_HM10
#undef R_SPARC_PC_LM22
#undef R_SPARC_WDISP16
#undef R_SPARC_WDISP19
#undef R_SPARC_GLOB_JMP
#undef R_SPARC_7
#undef R_SPARC_5
#undef R_SPARC_6
#undef R_SPARC_DISP64
#undef R_SPARC_PLT64
#undef R_SPARC_HIX22
#undef R_SPARC_LOX10
#undef R_SPARC_H44
#undef R_SPARC_M44
#undef R_SPARC_L44
#undef R_SPARC_REGISTER
#undef R_SPARC_UA64
#undef R_SPARC_UA16
#undef R_SPARC_TLS_GD_HI22
#undef R_SPARC_TLS_GD_LO10
#undef R_SPARC_TLS_GD_ADD
#undef R_SPARC_TLS_GD_CALL
#undef R_SPARC_TLS_LDM_HI22
#undef R_SPARC_TLS_LDM_LO10
#undef R_SPARC_TLS_LDM_ADD
#undef R_SPARC_TLS_LDM_CALL
#undef R_SPARC_TLS_LDO_HIX22
#undef R_SPARC_TLS_LDO_LOX10
#undef R_SPARC_TLS_LDO_ADD
#undef R_SPARC_TLS_IE_HI22
#undef R_SPARC_TLS_IE_LO10
#undef R_SPARC_TLS_IE_LD
#undef R_SPARC_TLS_IE_LDX
#undef R_SPARC_TLS_IE_ADD
#undef R_SPARC_TLS_LE_HIX22
#undef R_SPARC_TLS_LE_LOX10
#undef R_SPARC_TLS_DTPMOD32
#undef R_SPARC_TLS_DTPMOD64
#undef R_SPARC_TLS_DTPOFF32
#undef R_SPARC_TLS_DTPOFF64
#undef R_SPARC_TLS_TPOFF32
#undef R_SPARC_TLS_TPOFF64
#undef R_SPARC_GOTDATA_HIX22
#undef R_SPARC_GOTDATA_LOX10
#undef R_SPARC_GOTDATA_OP_HIX22
#undef R_SPARC_GOTDATA_OP_LOX10
#undef R_SPARC_GOTDATA_OP
#undef R_SPARC_H34
#undef R_SPARC_SIZE32
#undef R_SPARC_SIZE64
#undef R_SPARC_WDISP10
#undef R_SPARC_JMP_IREL
#undef R_SPARC_IRELATIVE
#undef R_SPARC_GNU_VTINHERIT
#undef R_SPARC_GNU_VTENTRY
#undef R_SPARC_REV32
/* Keep this the last entry.  */
#undef R_SPARC_NUM

/* For Sparc64, legal values for d_tag of Elf64_Dyn.  */

#undef DT_SPARC_REGISTER
#undef DT_SPARC_NUM

/* MIPS R3000 specific definitions.  */

/* Legal values for e_flags field of Elf32_Ehdr.  */

#undef EF_MIPS_NOREORDER
#undef EF_MIPS_PIC
#undef EF_MIPS_CPIC
#undef EF_MIPS_XGOT
#undef EF_MIPS_64BIT_WHIRL
#undef EF_MIPS_ABI2
#undef EF_MIPS_ABI_ON32
#undef EF_MIPS_FP64
#undef EF_MIPS_NAN2008
#undef EF_MIPS_ARCH

/* Legal values for MIPS architecture level.  */

#undef EF_MIPS_ARCH_1
#undef EF_MIPS_ARCH_2
#undef EF_MIPS_ARCH_3
#undef EF_MIPS_ARCH_4
#undef EF_MIPS_ARCH_5
#undef EF_MIPS_ARCH_32
#undef EF_MIPS_ARCH_64
#undef EF_MIPS_ARCH_32R2
#undef EF_MIPS_ARCH_64R2

/* The following are unofficial names and should not be used.  */

#undef E_MIPS_ARCH_1
#undef E_MIPS_ARCH_2
#undef E_MIPS_ARCH_3
#undef E_MIPS_ARCH_4
#undef E_MIPS_ARCH_5
#undef E_MIPS_ARCH_32
#undef E_MIPS_ARCH_64

/* Special section indices.  */

#undef SHN_MIPS_ACOMMON
#undef SHN_MIPS_TEXT
#undef SHN_MIPS_DATA
#undef SHN_MIPS_SCOMMON
#undef SHN_MIPS_SUNDEFINED

/* Legal values for sh_type field of Elf32_Shdr.  */

#undef SHT_MIPS_LIBLIST
#undef SHT_MIPS_MSYM
#undef SHT_MIPS_CONFLICT
#undef SHT_MIPS_GPTAB
#undef SHT_MIPS_UCODE
#undef SHT_MIPS_DEBUG
#undef SHT_MIPS_REGINFO
#undef SHT_MIPS_PACKAGE
#undef SHT_MIPS_PACKSYM
#undef SHT_MIPS_RELD
#undef SHT_MIPS_IFACE
#undef SHT_MIPS_CONTENT
#undef SHT_MIPS_OPTIONS
#undef SHT_MIPS_SHDR
#undef SHT_MIPS_FDESC
#undef SHT_MIPS_EXTSYM
#undef SHT_MIPS_DENSE
#undef SHT_MIPS_PDESC
#undef SHT_MIPS_LOCSYM
#undef SHT_MIPS_AUXSYM
#undef SHT_MIPS_OPTSYM
#undef SHT_MIPS_LOCSTR
#undef SHT_MIPS_LINE
#undef SHT_MIPS_RFDESC
#undef SHT_MIPS_DELTASYM
#undef SHT_MIPS_DELTAINST
#undef SHT_MIPS_DELTACLASS
#undef SHT_MIPS_DWARF
#undef SHT_MIPS_DELTADECL
#undef SHT_MIPS_SYMBOL_LIB
#undef SHT_MIPS_EVENTS
#undef SHT_MIPS_TRANSLATE
#undef SHT_MIPS_PIXIE
#undef SHT_MIPS_XLATE
#undef SHT_MIPS_XLATE_DEBUG
#undef SHT_MIPS_WHIRL
#undef SHT_MIPS_EH_REGION
#undef SHT_MIPS_XLATE_OLD
#undef SHT_MIPS_PDR_EXCEPTION
#undef SHT_MIPS_XHASH

/* Legal values for sh_flags field of Elf32_Shdr.  */

#undef SHF_MIPS_GPREL
#undef SHF_MIPS_MERGE
#undef SHF_MIPS_ADDR
#undef SHF_MIPS_STRINGS
#undef SHF_MIPS_NOSTRIP
#undef SHF_MIPS_LOCAL
#undef SHF_MIPS_NAMES
#undef SHF_MIPS_NODUPE

/* Symbol tables.  */

/* MIPS specific values for `st_other'.  */
#undef STO_MIPS_DEFAULT
#undef STO_MIPS_INTERNAL
#undef STO_MIPS_HIDDEN
#undef STO_MIPS_PROTECTED
#undef STO_MIPS_PLT
#undef STO_MIPS_SC_ALIGN_UNUSED

/* MIPS specific values for `st_info'.  */
#undef STB_MIPS_SPLIT_COMMON

/* Values for `kind' field in Elf_Options.  */

#undef ODK_NULL
#undef ODK_REGINFO
#undef ODK_EXCEPTIONS
#undef ODK_PAD
#undef ODK_HWPATCH
#undef ODK_FILL
#undef ODK_TAGS
#undef ODK_HWAND
#undef ODK_HWOR

/* Values for `info' in Elf_Options for ODK_EXCEPTIONS entries.  */

#undef OEX_FPU_MIN
#undef OEX_FPU_MAX
#undef OEX_PAGE0
#undef OEX_SMM
#undef OEX_FPDBUG
#undef OEX_PRECISEFP
#undef OEX_DISMISS

#undef OEX_FPU_INVAL
#undef OEX_FPU_DIV0
#undef OEX_FPU_OFLO
#undef OEX_FPU_UFLO
#undef OEX_FPU_INEX

/* Masks for `info' in Elf_Options for an ODK_HWPATCH entry.  */

#undef OHW_R4KEOP
#undef OHW_R8KPFETCH
#undef OHW_R5KEOP
#undef OHW_R5KCVTL

#undef OPAD_PREFIX
#undef OPAD_POSTFIX
#undef OPAD_SYMBOL

/* Entry found in `.options' section.  */
/* Masks for `info' in ElfOptions for ODK_HWAND and ODK_HWOR entries.  */

#undef OHWA0_R4KEOP_CHECKED
#undef OHWA1_R4KEOP_CLEAN

/* MIPS relocs.  */

#undef R_MIPS_NONE
#undef R_MIPS_16
#undef R_MIPS_32
#undef R_MIPS_REL32
#undef R_MIPS_26
#undef R_MIPS_HI16
#undef R_MIPS_LO16
#undef R_MIPS_GPREL16
#undef R_MIPS_LITERAL
#undef R_MIPS_GOT16
#undef R_MIPS_PC16
#undef R_MIPS_CALL16
#undef R_MIPS_GPREL32

#undef R_MIPS_SHIFT5
#undef R_MIPS_SHIFT6
#undef R_MIPS_64
#undef R_MIPS_GOT_DISP
#undef R_MIPS_GOT_PAGE
#undef R_MIPS_GOT_OFST
#undef R_MIPS_GOT_HI16
#undef R_MIPS_GOT_LO16
#undef R_MIPS_SUB
#undef R_MIPS_INSERT_A
#undef R_MIPS_INSERT_B
#undef R_MIPS_DELETE
#undef R_MIPS_HIGHER
#undef R_MIPS_HIGHEST
#undef R_MIPS_CALL_HI16
#undef R_MIPS_CALL_LO16
#undef R_MIPS_SCN_DISP
#undef R_MIPS_REL16
#undef R_MIPS_ADD_IMMEDIATE
#undef R_MIPS_PJUMP
#undef R_MIPS_RELGOT
#undef R_MIPS_JALR
#undef R_MIPS_TLS_DTPMOD32
#undef R_MIPS_TLS_DTPREL32
#undef R_MIPS_TLS_DTPMOD64
#undef R_MIPS_TLS_DTPREL64
#undef R_MIPS_TLS_GD
#undef R_MIPS_TLS_LDM
#undef R_MIPS_TLS_DTPREL_HI16
#undef R_MIPS_TLS_DTPREL_LO16
#undef R_MIPS_TLS_GOTTPREL
#undef R_MIPS_TLS_TPREL32
#undef R_MIPS_TLS_TPREL64
#undef R_MIPS_TLS_TPREL_HI16
#undef R_MIPS_TLS_TPREL_LO16
#undef R_MIPS_GLOB_DAT
#undef R_MIPS_COPY
#undef R_MIPS_JUMP_SLOT
/* Keep this the last entry.  */
#undef R_MIPS_NUM

/* Legal values for p_type field of Elf32_Phdr.  */

#undef PT_MIPS_REGINFO
#undef PT_MIPS_RTPROC
#undef PT_MIPS_OPTIONS
#undef PT_MIPS_ABIFLAGS

/* Special program header types.  */

#undef PF_MIPS_LOCAL

/* Legal values for d_tag field of Elf32_Dyn.  */

#undef DT_MIPS_RLD_VERSION
#undef DT_MIPS_TIME_STAMP
#undef DT_MIPS_ICHECKSUM
#undef DT_MIPS_IVERSION
#undef DT_MIPS_FLAGS
#undef DT_MIPS_BASE_ADDRESS
#undef DT_MIPS_MSYM
#undef DT_MIPS_CONFLICT
#undef DT_MIPS_LIBLIST
#undef DT_MIPS_LOCAL_GOTNO
#undef DT_MIPS_CONFLICTNO
#undef DT_MIPS_LIBLISTNO
#undef DT_MIPS_SYMTABNO
#undef DT_MIPS_UNREFEXTNO
#undef DT_MIPS_GOTSYM
#undef DT_MIPS_HIPAGENO
#undef DT_MIPS_RLD_MAP
#undef DT_MIPS_DELTA_CLASS
#undef DT_MIPS_DELTA_CLASS_NO
#undef DT_MIPS_DELTA_INSTANCE
#undef DT_MIPS_DELTA_INSTANCE_NO
#undef DT_MIPS_DELTA_RELOC
#undef DT_MIPS_DELTA_RELOC_NO
#undef DT_MIPS_DELTA_SYM
#undef DT_MIPS_DELTA_SYM_NO
#undef DT_MIPS_DELTA_CLASSSYM
#undef DT_MIPS_DELTA_CLASSSYM_NO
#undef DT_MIPS_CXX_FLAGS
#undef DT_MIPS_PIXIE_INIT
#undef DT_MIPS_SYMBOL_LIB
#undef DT_MIPS_LOCALPAGE_GOTIDX
#undef DT_MIPS_LOCAL_GOTIDX
#undef DT_MIPS_HIDDEN_GOTIDX
#undef DT_MIPS_PROTECTED_GOTIDX
#undef DT_MIPS_OPTIONS
#undef DT_MIPS_INTERFACE
#undef DT_MIPS_DYNSTR_ALIGN
#undef DT_MIPS_INTERFACE_SIZE
#undef DT_MIPS_RLD_TEXT_RESOLVE_ADDR
#undef DT_MIPS_PERF_SUFFIX
#undef DT_MIPS_COMPACT_SIZE
#undef DT_MIPS_GP_VALUE
#undef DT_MIPS_AUX_DYNAMIC
/* The address of .got.plt in an executable using the new non-PIC ABI.  */
#undef DT_MIPS_PLTGOT
/* The base of the PLT in an executable using the new non-PIC ABI if that
   PLT is writable.  For a non-writable PLT, this is omitted or has a zero
   value.  */
#undef DT_MIPS_RWPLT
/* An alternative description of the classic MIPS RLD_MAP that is usable
   in a PIE as it stores a relative offset from the address of the tag
   rather than an absolute address.  */
#undef DT_MIPS_RLD_MAP_REL
/* GNU-style hash table with xlat.  */
#undef DT_MIPS_XHASH
#undef DT_MIPS_NUM

/* Legal values for DT_MIPS_FLAGS Elf32_Dyn entry.  */

#undef RHF_NONE
#undef RHF_QUICKSTART
#undef RHF_NOTPOT
#undef RHF_NO_LIBRARY_REPLACEMENT
#undef RHF_NO_MOVE
#undef RHF_SGI_ONLY
#undef RHF_GUARANTEE_INIT
#undef RHF_DELTA_C_PLUS_PLUS
#undef RHF_GUARANTEE_START_INIT
#undef RHF_PIXIE
#undef RHF_DEFAULT_DELAY_LOAD
#undef RHF_REQUICKSTART
#undef RHF_REQUICKSTARTED
#undef RHF_CORD
#undef RHF_NO_UNRES_UNDEF
#undef RHF_RLD_ORDER_SAFE

/* Entries found in sections of type SHT_MIPS_LIBLIST.  */

/* Legal values for l_flags.  */

#undef LL_NONE
#undef LL_EXACT_MATCH
#undef LL_IGNORE_INT_VER
#undef LL_REQUIRE_MINOR
#undef LL_EXPORTS
#undef LL_DELAY_LOAD
#undef LL_DELTA

/* Entries found in sections of type SHT_MIPS_CONFLICT.  */
/* Values for the register size bytes of an abi flags structure.  */

#undef MIPS_AFL_REG_NONE
#undef MIPS_AFL_REG_32
#undef MIPS_AFL_REG_64
#undef MIPS_AFL_REG_128

/* Masks for the ases word of an ABI flags structure.  */

#undef MIPS_AFL_ASE_DSP
#undef MIPS_AFL_ASE_DSPR2
#undef MIPS_AFL_ASE_EVA
#undef MIPS_AFL_ASE_MCU
#undef MIPS_AFL_ASE_MDMX
#undef MIPS_AFL_ASE_MIPS3D
#undef MIPS_AFL_ASE_MT
#undef MIPS_AFL_ASE_SMARTMIPS
#undef MIPS_AFL_ASE_VIRT
#undef MIPS_AFL_ASE_MSA
#undef MIPS_AFL_ASE_MIPS16
#undef MIPS_AFL_ASE_MICROMIPS
#undef MIPS_AFL_ASE_XPA
#undef MIPS_AFL_ASE_MASK

/* Values for the isa_ext word of an ABI flags structure.  */

#undef MIPS_AFL_EXT_XLR
#undef MIPS_AFL_EXT_OCTEON2
#undef MIPS_AFL_EXT_OCTEONP
#undef MIPS_AFL_EXT_LOONGSON_3A
#undef MIPS_AFL_EXT_OCTEON
#undef MIPS_AFL_EXT_5900
#undef MIPS_AFL_EXT_4650
#undef MIPS_AFL_EXT_4010
#undef MIPS_AFL_EXT_4100
#undef MIPS_AFL_EXT_3900
#undef MIPS_AFL_EXT_10000
#undef MIPS_AFL_EXT_SB1
#undef MIPS_AFL_EXT_4111
#undef MIPS_AFL_EXT_4120
#undef MIPS_AFL_EXT_5400
#undef MIPS_AFL_EXT_5500
#undef MIPS_AFL_EXT_LOONGSON_2E
#undef MIPS_AFL_EXT_LOONGSON_2F

/* Masks for the flags1 word of an ABI flags structure.  */
#undef MIPS_AFL_FLAGS1_ODDSPREG

/* HPPA specific definitions.  */

/* Legal values for e_flags field of Elf32_Ehdr.  */

#undef EF_PARISC_TRAPNIL
#undef EF_PARISC_EXT
#undef EF_PARISC_LSB
#undef EF_PARISC_WIDE
#undef EF_PARISC_NO_KABP
#undef EF_PARISC_LAZYSWAP
#undef EF_PARISC_ARCH

/* Defined values for `e_flags & EF_PARISC_ARCH' are:  */

#undef EFA_PARISC_1_0
#undef EFA_PARISC_1_1
#undef EFA_PARISC_2_0

/* Additional section indices.  */

#undef SHN_PARISC_ANSI_COMMON
#undef SHN_PARISC_HUGE_COMMON

/* Legal values for sh_type field of Elf32_Shdr.  */

#undef SHT_PARISC_EXT
#undef SHT_PARISC_UNWIND
#undef SHT_PARISC_DOC

/* Legal values for sh_flags field of Elf32_Shdr.  */

#undef SHF_PARISC_SHORT
#undef SHF_PARISC_HUGE
#undef SHF_PARISC_SBP

/* Legal values for ST_TYPE subfield of st_info (symbol type).  */

#undef STT_PARISC_MILLICODE

#undef STT_HP_OPAQUE
#undef STT_HP_STUB

/* HPPA relocs.  */

#undef R_PARISC_NONE
#undef R_PARISC_DIR32
#undef R_PARISC_DIR21L
#undef R_PARISC_DIR17R
#undef R_PARISC_DIR17F
#undef R_PARISC_DIR14R
#undef R_PARISC_PCREL32
#undef R_PARISC_PCREL21L
#undef R_PARISC_PCREL17R
#undef R_PARISC_PCREL17F
#undef R_PARISC_PCREL14R
#undef R_PARISC_DPREL21L
#undef R_PARISC_DPREL14R
#undef R_PARISC_GPREL21L
#undef R_PARISC_GPREL14R
#undef R_PARISC_LTOFF21L
#undef R_PARISC_LTOFF14R
#undef R_PARISC_SECREL32
#undef R_PARISC_SEGBASE
#undef R_PARISC_SEGREL32
#undef R_PARISC_PLTOFF21L
#undef R_PARISC_PLTOFF14R
#undef R_PARISC_LTOFF_FPTR32
#undef R_PARISC_LTOFF_FPTR21L
#undef R_PARISC_LTOFF_FPTR14R
#undef R_PARISC_FPTR64
#undef R_PARISC_PLABEL32
#undef R_PARISC_PLABEL21L
#undef R_PARISC_PLABEL14R
#undef R_PARISC_PCREL64
#undef R_PARISC_PCREL22F
#undef R_PARISC_PCREL14WR
#undef R_PARISC_PCREL14DR
#undef R_PARISC_PCREL16F
#undef R_PARISC_PCREL16WF
#undef R_PARISC_PCREL16DF
#undef R_PARISC_DIR64
#undef R_PARISC_DIR14WR
#undef R_PARISC_DIR14DR
#undef R_PARISC_DIR16F
#undef R_PARISC_DIR16WF
#undef R_PARISC_DIR16DF
#undef R_PARISC_GPREL64
#undef R_PARISC_GPREL14WR
#undef R_PARISC_GPREL14DR
#undef R_PARISC_GPREL16F
#undef R_PARISC_GPREL16WF
#undef R_PARISC_GPREL16DF
#undef R_PARISC_LTOFF64
#undef R_PARISC_LTOFF14WR
#undef R_PARISC_LTOFF14DR
#undef R_PARISC_LTOFF16F
#undef R_PARISC_LTOFF16WF
#undef R_PARISC_LTOFF16DF
#undef R_PARISC_SECREL64
#undef R_PARISC_SEGREL64
#undef R_PARISC_PLTOFF14WR
#undef R_PARISC_PLTOFF14DR
#undef R_PARISC_PLTOFF16F
#undef R_PARISC_PLTOFF16WF
#undef R_PARISC_PLTOFF16DF
#undef R_PARISC_LTOFF_FPTR64
#undef R_PARISC_LTOFF_FPTR14WR
#undef R_PARISC_LTOFF_FPTR14DR
#undef R_PARISC_LTOFF_FPTR16F
#undef R_PARISC_LTOFF_FPTR16WF
#undef R_PARISC_LTOFF_FPTR16DF
#undef R_PARISC_LORESERVE
#undef R_PARISC_COPY
#undef R_PARISC_IPLT
#undef R_PARISC_EPLT
#undef R_PARISC_TPREL32
#undef R_PARISC_TPREL21L
#undef R_PARISC_TPREL14R
#undef R_PARISC_LTOFF_TP21L
#undef R_PARISC_LTOFF_TP14R
#undef R_PARISC_LTOFF_TP14F
#undef R_PARISC_TPREL64
#undef R_PARISC_TPREL14WR
#undef R_PARISC_TPREL14DR
#undef R_PARISC_TPREL16F
#undef R_PARISC_TPREL16WF
#undef R_PARISC_TPREL16DF
#undef R_PARISC_LTOFF_TP64
#undef R_PARISC_LTOFF_TP14WR
#undef R_PARISC_LTOFF_TP14DR
#undef R_PARISC_LTOFF_TP16F
#undef R_PARISC_LTOFF_TP16WF
#undef R_PARISC_LTOFF_TP16DF
#undef R_PARISC_GNU_VTENTRY
#undef R_PARISC_GNU_VTINHERIT
#undef R_PARISC_TLS_GD21L
#undef R_PARISC_TLS_GD14R
#undef R_PARISC_TLS_GDCALL
#undef R_PARISC_TLS_LDM21L
#undef R_PARISC_TLS_LDM14R
#undef R_PARISC_TLS_LDMCALL
#undef R_PARISC_TLS_LDO21L
#undef R_PARISC_TLS_LDO14R
#undef R_PARISC_TLS_DTPMOD32
#undef R_PARISC_TLS_DTPMOD64
#undef R_PARISC_TLS_DTPOFF32
#undef R_PARISC_TLS_DTPOFF64
#undef R_PARISC_TLS_LE21L
#undef R_PARISC_TLS_LE14R
#undef R_PARISC_TLS_IE21L
#undef R_PARISC_TLS_IE14R
#undef R_PARISC_TLS_TPREL32
#undef R_PARISC_TLS_TPREL64
#undef R_PARISC_HIRESERVE

/* Legal values for p_type field of Elf32_Phdr/Elf64_Phdr.  */

#undef PT_HP_TLS
#undef PT_HP_CORE_NONE
#undef PT_HP_CORE_VERSION
#undef PT_HP_CORE_KERNEL
#undef PT_HP_CORE_COMM
#undef PT_HP_CORE_PROC
#undef PT_HP_CORE_LOADABLE
#undef PT_HP_CORE_STACK
#undef PT_HP_CORE_SHM
#undef PT_HP_CORE_MMF
#undef PT_HP_PARALLEL
#undef PT_HP_FASTBIND
#undef PT_HP_OPT_ANNOT
#undef PT_HP_HSL_ANNOT
#undef PT_HP_STACK

#undef PT_PARISC_ARCHEXT
#undef PT_PARISC_UNWIND

/* Legal values for p_flags field of Elf32_Phdr/Elf64_Phdr.  */

#undef PF_PARISC_SBP

#undef PF_HP_PAGE_SIZE
#undef PF_HP_FAR_SHARED
#undef PF_HP_NEAR_SHARED
#undef PF_HP_CODE
#undef PF_HP_MODIFY
#undef PF_HP_LAZYSWAP
#undef PF_HP_SBP

/* Alpha specific definitions.  */

/* Legal values for e_flags field of Elf64_Ehdr.  */

#undef EF_ALPHA_32BIT
#undef EF_ALPHA_CANRELAX

/* Legal values for sh_type field of Elf64_Shdr.  */

/* These two are primerily concerned with ECOFF debugging info.  */
#undef SHT_ALPHA_DEBUG
#undef SHT_ALPHA_REGINFO

/* Legal values for sh_flags field of Elf64_Shdr.  */

#undef SHF_ALPHA_GPREL

/* Legal values for st_other field of Elf64_Sym.  */
#undef STO_ALPHA_NOPV
#undef STO_ALPHA_STD_GPLOAD

/* Alpha relocs.  */

#undef R_ALPHA_NONE
#undef R_ALPHA_REFLONG
#undef R_ALPHA_REFQUAD
#undef R_ALPHA_GPREL32
#undef R_ALPHA_LITERAL
#undef R_ALPHA_LITUSE
#undef R_ALPHA_GPDISP
#undef R_ALPHA_BRADDR
#undef R_ALPHA_HINT
#undef R_ALPHA_SREL16
#undef R_ALPHA_SREL32
#undef R_ALPHA_SREL64
#undef R_ALPHA_GPRELHIGH
#undef R_ALPHA_GPRELLOW
#undef R_ALPHA_GPREL16
#undef R_ALPHA_COPY
#undef R_ALPHA_GLOB_DAT
#undef R_ALPHA_JMP_SLOT
#undef R_ALPHA_RELATIVE
#undef R_ALPHA_TLS_GD_HI
#undef R_ALPHA_TLSGD
#undef R_ALPHA_TLS_LDM
#undef R_ALPHA_DTPMOD64
#undef R_ALPHA_GOTDTPREL
#undef R_ALPHA_DTPREL64
#undef R_ALPHA_DTPRELHI
#undef R_ALPHA_DTPRELLO
#undef R_ALPHA_DTPREL16
#undef R_ALPHA_GOTTPREL
#undef R_ALPHA_TPREL64
#undef R_ALPHA_TPRELHI
#undef R_ALPHA_TPRELLO
#undef R_ALPHA_TPREL16
/* Keep this the last entry.  */
#undef R_ALPHA_NUM

/* Magic values of the LITUSE relocation addend.  */
#undef LITUSE_ALPHA_ADDR
#undef LITUSE_ALPHA_BASE
#undef LITUSE_ALPHA_BYTOFF
#undef LITUSE_ALPHA_JSR
#undef LITUSE_ALPHA_TLS_GD
#undef LITUSE_ALPHA_TLS_LDM

/* Legal values for d_tag of Elf64_Dyn.  */
#undef DT_ALPHA_PLTRO
#undef DT_ALPHA_NUM

/* PowerPC specific declarations */

/* Values for Elf32/64_Ehdr.e_flags.  */
#undef EF_PPC_EMB

/* Cygnus local bits below */
#undef EF_PPC_RELOCATABLE
#undef EF_PPC_RELOCATABLE_LIB

/* PowerPC relocations defined by the ABIs */
#undef R_PPC_NONE
#undef R_PPC_ADDR32
#undef R_PPC_ADDR24
#undef R_PPC_ADDR16
#undef R_PPC_ADDR16_LO
#undef R_PPC_ADDR16_HI
#undef R_PPC_ADDR16_HA
#undef R_PPC_ADDR14
#undef R_PPC_ADDR14_BRTAKEN
#undef R_PPC_ADDR14_BRNTAKEN
#undef R_PPC_REL24
#undef R_PPC_REL14
#undef R_PPC_REL14_BRTAKEN
#undef R_PPC_REL14_BRNTAKEN
#undef R_PPC_GOT16
#undef R_PPC_GOT16_LO
#undef R_PPC_GOT16_HI
#undef R_PPC_GOT16_HA
#undef R_PPC_PLTREL24
#undef R_PPC_COPY
#undef R_PPC_GLOB_DAT
#undef R_PPC_JMP_SLOT
#undef R_PPC_RELATIVE
#undef R_PPC_LOCAL24PC
#undef R_PPC_UADDR32
#undef R_PPC_UADDR16
#undef R_PPC_REL32
#undef R_PPC_PLT32
#undef R_PPC_PLTREL32
#undef R_PPC_PLT16_LO
#undef R_PPC_PLT16_HI
#undef R_PPC_PLT16_HA
#undef R_PPC_SDAREL16
#undef R_PPC_SECTOFF
#undef R_PPC_SECTOFF_LO
#undef R_PPC_SECTOFF_HI
#undef R_PPC_SECTOFF_HA

/* PowerPC relocations defined for the TLS access ABI.  */
#undef R_PPC_TLS
#undef R_PPC_DTPMOD32
#undef R_PPC_TPREL16
#undef R_PPC_TPREL16_LO
#undef R_PPC_TPREL16_HI
#undef R_PPC_TPREL16_HA
#undef R_PPC_TPREL32
#undef R_PPC_DTPREL16
#undef R_PPC_DTPREL16_LO
#undef R_PPC_DTPREL16_HI
#undef R_PPC_DTPREL16_HA
#undef R_PPC_DTPREL32
#undef R_PPC_GOT_TLSGD16
#undef R_PPC_GOT_TLSGD16_LO
#undef R_PPC_GOT_TLSGD16_HI
#undef R_PPC_GOT_TLSGD16_HA
#undef R_PPC_GOT_TLSLD16
#undef R_PPC_GOT_TLSLD16_LO
#undef R_PPC_GOT_TLSLD16_HI
#undef R_PPC_GOT_TLSLD16_HA
#undef R_PPC_GOT_TPREL16
#undef R_PPC_GOT_TPREL16_LO
#undef R_PPC_GOT_TPREL16_HI
#undef R_PPC_GOT_TPREL16_HA
#undef R_PPC_GOT_DTPREL16
#undef R_PPC_GOT_DTPREL16_LO
#undef R_PPC_GOT_DTPREL16_HI
#undef R_PPC_GOT_DTPREL16_HA
#undef R_PPC_TLSGD
#undef R_PPC_TLSLD

/* The remaining relocs are from the Embedded ELF ABI, and are not
   in the SVR4 ELF ABI.  */
#undef R_PPC_EMB_NADDR32
#undef R_PPC_EMB_NADDR16
#undef R_PPC_EMB_NADDR16_LO
#undef R_PPC_EMB_NADDR16_HI
#undef R_PPC_EMB_NADDR16_HA
#undef R_PPC_EMB_SDAI16
#undef R_PPC_EMB_SDA2I16
#undef R_PPC_EMB_SDA2REL
#undef R_PPC_EMB_SDA21
#undef R_PPC_EMB_MRKREF
#undef R_PPC_EMB_RELSEC16
#undef R_PPC_EMB_RELST_LO
#undef R_PPC_EMB_RELST_HI
#undef R_PPC_EMB_RELST_HA
#undef R_PPC_EMB_BIT_FLD
#undef R_PPC_EMB_RELSDA

/* Diab tool relocations.  */
#undef R_PPC_DIAB_SDA21_LO
#undef R_PPC_DIAB_SDA21_HI
#undef R_PPC_DIAB_SDA21_HA
#undef R_PPC_DIAB_RELSDA_LO
#undef R_PPC_DIAB_RELSDA_HI
#undef R_PPC_DIAB_RELSDA_HA

/* GNU extension to support local ifunc.  */
#undef R_PPC_IRELATIVE

/* GNU relocs used in PIC code sequences.  */
#undef R_PPC_REL16
#undef R_PPC_REL16_LO
#undef R_PPC_REL16_HI
#undef R_PPC_REL16_HA

/* This is a phony reloc to handle any old fashioned TOC16 references
   that may still be in object files.  */
#undef R_PPC_TOC16

/* PowerPC specific values for the Dyn d_tag field.  */
#undef DT_PPC_GOT
#undef DT_PPC_OPT
#undef DT_PPC_NUM

/* PowerPC specific values for the DT_PPC_OPT Dyn entry.  */
#undef PPC_OPT_TLS

/* PowerPC64 relocations defined by the ABIs */
#undef R_PPC64_NONE
#undef R_PPC64_ADDR32
#undef R_PPC64_ADDR24
#undef R_PPC64_ADDR16
#undef R_PPC64_ADDR16_LO
#undef R_PPC64_ADDR16_HI
#undef R_PPC64_ADDR16_HA
#undef R_PPC64_ADDR14
#undef R_PPC64_ADDR14_BRTAKEN
#undef R_PPC64_ADDR14_BRNTAKEN
#undef R_PPC64_REL24
#undef R_PPC64_REL14
#undef R_PPC64_REL14_BRTAKEN
#undef R_PPC64_REL14_BRNTAKEN
#undef R_PPC64_GOT16
#undef R_PPC64_GOT16_LO
#undef R_PPC64_GOT16_HI
#undef R_PPC64_GOT16_HA

#undef R_PPC64_COPY
#undef R_PPC64_GLOB_DAT
#undef R_PPC64_JMP_SLOT
#undef R_PPC64_RELATIVE

#undef R_PPC64_UADDR32
#undef R_PPC64_UADDR16
#undef R_PPC64_REL32
#undef R_PPC64_PLT32
#undef R_PPC64_PLTREL32
#undef R_PPC64_PLT16_LO
#undef R_PPC64_PLT16_HI
#undef R_PPC64_PLT16_HA

#undef R_PPC64_SECTOFF
#undef R_PPC64_SECTOFF_LO
#undef R_PPC64_SECTOFF_HI
#undef R_PPC64_SECTOFF_HA
#undef R_PPC64_ADDR30
#undef R_PPC64_ADDR64
#undef R_PPC64_ADDR16_HIGHER
#undef R_PPC64_ADDR16_HIGHERA
#undef R_PPC64_ADDR16_HIGHEST
#undef R_PPC64_ADDR16_HIGHESTA
#undef R_PPC64_UADDR64
#undef R_PPC64_REL64
#undef R_PPC64_PLT64
#undef R_PPC64_PLTREL64
#undef R_PPC64_TOC16
#undef R_PPC64_TOC16_LO
#undef R_PPC64_TOC16_HI
#undef R_PPC64_TOC16_HA
#undef R_PPC64_TOC
#undef R_PPC64_PLTGOT16
#undef R_PPC64_PLTGOT16_LO
#undef R_PPC64_PLTGOT16_HI
#undef R_PPC64_PLTGOT16_HA

#undef R_PPC64_ADDR16_DS
#undef R_PPC64_ADDR16_LO_DS
#undef R_PPC64_GOT16_DS
#undef R_PPC64_GOT16_LO_DS
#undef R_PPC64_PLT16_LO_DS
#undef R_PPC64_SECTOFF_DS
#undef R_PPC64_SECTOFF_LO_DS
#undef R_PPC64_TOC16_DS
#undef R_PPC64_TOC16_LO_DS
#undef R_PPC64_PLTGOT16_DS
#undef R_PPC64_PLTGOT16_LO_DS

/* PowerPC64 relocations defined for the TLS access ABI.  */
#undef R_PPC64_TLS
#undef R_PPC64_DTPMOD64
#undef R_PPC64_TPREL16
#undef R_PPC64_TPREL16_LO
#undef R_PPC64_TPREL16_HI
#undef R_PPC64_TPREL16_HA
#undef R_PPC64_TPREL64
#undef R_PPC64_DTPREL16
#undef R_PPC64_DTPREL16_LO
#undef R_PPC64_DTPREL16_HI
#undef R_PPC64_DTPREL16_HA
#undef R_PPC64_DTPREL64
#undef R_PPC64_GOT_TLSGD16
#undef R_PPC64_GOT_TLSGD16_LO
#undef R_PPC64_GOT_TLSGD16_HI
#undef R_PPC64_GOT_TLSGD16_HA
#undef R_PPC64_GOT_TLSLD16
#undef R_PPC64_GOT_TLSLD16_LO
#undef R_PPC64_GOT_TLSLD16_HI
#undef R_PPC64_GOT_TLSLD16_HA
#undef R_PPC64_GOT_TPREL16_DS
#undef R_PPC64_GOT_TPREL16_LO_DS
#undef R_PPC64_GOT_TPREL16_HI
#undef R_PPC64_GOT_TPREL16_HA
#undef R_PPC64_GOT_DTPREL16_DS
#undef R_PPC64_GOT_DTPREL16_LO_DS
#undef R_PPC64_GOT_DTPREL16_HI
#undef R_PPC64_GOT_DTPREL16_HA
#undef R_PPC64_TPREL16_DS
#undef R_PPC64_TPREL16_LO_DS
#undef R_PPC64_TPREL16_HIGHER
#undef R_PPC64_TPREL16_HIGHERA
#undef R_PPC64_TPREL16_HIGHEST
#undef R_PPC64_TPREL16_HIGHESTA
#undef R_PPC64_DTPREL16_DS
#undef R_PPC64_DTPREL16_LO_DS
#undef R_PPC64_DTPREL16_HIGHER
#undef R_PPC64_DTPREL16_HIGHERA
#undef R_PPC64_DTPREL16_HIGHEST
#undef R_PPC64_DTPREL16_HIGHESTA
#undef R_PPC64_TLSGD
#undef R_PPC64_TLSLD
#undef R_PPC64_TOCSAVE

/* Added when HA and HI relocs were changed to report overflows.  */
#undef R_PPC64_ADDR16_HIGH
#undef R_PPC64_ADDR16_HIGHA
#undef R_PPC64_TPREL16_HIGH
#undef R_PPC64_TPREL16_HIGHA
#undef R_PPC64_DTPREL16_HIGH
#undef R_PPC64_DTPREL16_HIGHA

/* GNU extension to support local ifunc.  */
#undef R_PPC64_JMP_IREL
#undef R_PPC64_IRELATIVE
#undef R_PPC64_REL16
#undef R_PPC64_REL16_LO
#undef R_PPC64_REL16_HI
#undef R_PPC64_REL16_HA

/* e_flags bits specifying ABI.
   1 for original function descriptor using ABI,
   2 for revised ABI without function descriptors,
   0 for unspecified or not using any features affected by the differences.  */
#undef EF_PPC64_ABI

/* PowerPC64 specific values for the Dyn d_tag field.  */
#undef DT_PPC64_GLINK
#undef DT_PPC64_OPD
#undef DT_PPC64_OPDSZ
#undef DT_PPC64_OPT
#undef DT_PPC64_NUM

/* PowerPC64 specific bits in the DT_PPC64_OPT Dyn entry.  */
#undef PPC64_OPT_TLS
#undef PPC64_OPT_MULTI_TOC
#undef PPC64_OPT_LOCALENTRY

/* PowerPC64 specific values for the Elf64_Sym st_other field.  */
#undef STO_PPC64_LOCAL_BIT
#undef STO_PPC64_LOCAL_MASK
#undef PPC64_LOCAL_ENTRY_OFFSET

/* ARM specific declarations */

/* Processor specific flags for the ELF header e_flags field.  */
#undef EF_ARM_RELEXEC
#undef EF_ARM_HASENTRY
#undef EF_ARM_INTERWORK
#undef EF_ARM_APCS_26
#undef EF_ARM_APCS_FLOAT
#undef EF_ARM_PIC
#undef EF_ARM_ALIGN8
#undef EF_ARM_NEW_ABI
#undef EF_ARM_OLD_ABI
#undef EF_ARM_SOFT_FLOAT
#undef EF_ARM_VFP_FLOAT
#undef EF_ARM_MAVERICK_FLOAT

#undef EF_ARM_ABI_FLOAT_SOFT
#undef EF_ARM_ABI_FLOAT_HARD

/* Other constants defined in the ARM ELF spec. version B-01.  */
/* NB. These conflict with values defined above.  */
#undef EF_ARM_SYMSARESORTED
#undef EF_ARM_DYNSYMSUSESEGIDX
#undef EF_ARM_MAPSYMSFIRST
#undef EF_ARM_EABIMASK

/* Constants defined in AAELF.  */
#undef EF_ARM_BE8
#undef EF_ARM_LE8

#undef EF_ARM_EABI_VERSION
#undef EF_ARM_EABI_UNKNOWN
#undef EF_ARM_EABI_VER1
#undef EF_ARM_EABI_VER2
#undef EF_ARM_EABI_VER3
#undef EF_ARM_EABI_VER4
#undef EF_ARM_EABI_VER5

/* Additional symbol types for Thumb.  */
#undef STT_ARM_TFUNC
#undef STT_ARM_16BIT

/* ARM-specific values for sh_flags */
#undef SHF_ARM_ENTRYSECT
#undef SHF_ARM_COMDEF

/* ARM-specific program header flags */
#undef PF_ARM_SB
#undef PF_ARM_PI
#undef PF_ARM_ABS

/* Processor specific values for the Phdr p_type field.  */
#undef PT_ARM_EXIDX

/* Processor specific values for the Shdr sh_type field.  */
#undef SHT_ARM_EXIDX
#undef SHT_ARM_PREEMPTMAP
#undef SHT_ARM_ATTRIBUTES

/* AArch64 relocs.  */

#undef R_AARCH64_NONE

/* ILP32 AArch64 relocs.  */
#undef R_AARCH64_P32_ABS32
#undef R_AARCH64_P32_COPY
#undef R_AARCH64_P32_GLOB_DAT
#undef R_AARCH64_P32_JUMP_SLOT
#undef R_AARCH64_P32_RELATIVE
#undef R_AARCH64_P32_TLS_DTPMOD
#undef R_AARCH64_P32_TLS_DTPREL
#undef R_AARCH64_P32_TLS_TPREL
#undef R_AARCH64_P32_TLSDESC
#undef R_AARCH64_P32_IRELATIVE

/* LP64 AArch64 relocs.  */
#undef R_AARCH64_ABS64
#undef R_AARCH64_ABS32
#undef R_AARCH64_ABS16
#undef R_AARCH64_PREL64
#undef R_AARCH64_PREL32
#undef R_AARCH64_PREL16
#undef R_AARCH64_MOVW_UABS_G0
#undef R_AARCH64_MOVW_UABS_G0_NC
#undef R_AARCH64_MOVW_UABS_G1
#undef R_AARCH64_MOVW_UABS_G1_NC
#undef R_AARCH64_MOVW_UABS_G2
#undef R_AARCH64_MOVW_UABS_G2_NC
#undef R_AARCH64_MOVW_UABS_G3
#undef R_AARCH64_MOVW_SABS_G0
#undef R_AARCH64_MOVW_SABS_G1
#undef R_AARCH64_MOVW_SABS_G2
#undef R_AARCH64_LD_PREL_LO19
#undef R_AARCH64_ADR_PREL_LO21
#undef R_AARCH64_ADR_PREL_PG_HI21
#undef R_AARCH64_ADR_PREL_PG_HI21_NC
#undef R_AARCH64_ADD_ABS_LO12_NC
#undef R_AARCH64_LDST8_ABS_LO12_NC
#undef R_AARCH64_TSTBR14
#undef R_AARCH64_CONDBR19
#undef R_AARCH64_JUMP26
#undef R_AARCH64_CALL26
#undef R_AARCH64_LDST16_ABS_LO12_NC
#undef R_AARCH64_LDST32_ABS_LO12_NC
#undef R_AARCH64_LDST64_ABS_LO12_NC
#undef R_AARCH64_MOVW_PREL_G0
#undef R_AARCH64_MOVW_PREL_G0_NC
#undef R_AARCH64_MOVW_PREL_G1
#undef R_AARCH64_MOVW_PREL_G1_NC
#undef R_AARCH64_MOVW_PREL_G2
#undef R_AARCH64_MOVW_PREL_G2_NC
#undef R_AARCH64_MOVW_PREL_G3
#undef R_AARCH64_LDST128_ABS_LO12_NC
#undef R_AARCH64_MOVW_GOTOFF_G0
#undef R_AARCH64_MOVW_GOTOFF_G0_NC
#undef R_AARCH64_MOVW_GOTOFF_G1
#undef R_AARCH64_MOVW_GOTOFF_G1_NC
#undef R_AARCH64_MOVW_GOTOFF_G2
#undef R_AARCH64_MOVW_GOTOFF_G2_NC
#undef R_AARCH64_MOVW_GOTOFF_G3
#undef R_AARCH64_GOTREL64
#undef R_AARCH64_GOTREL32
#undef R_AARCH64_GOT_LD_PREL19
#undef R_AARCH64_LD64_GOTOFF_LO15
#undef R_AARCH64_ADR_GOT_PAGE
#undef R_AARCH64_LD64_GOT_LO12_NC
#undef R_AARCH64_LD64_GOTPAGE_LO15
#undef R_AARCH64_TLSGD_ADR_PREL21
#undef R_AARCH64_TLSGD_ADR_PAGE21
#undef R_AARCH64_TLSGD_ADD_LO12_NC
#undef R_AARCH64_TLSGD_MOVW_G1
#undef R_AARCH64_TLSGD_MOVW_G0_NC
#undef R_AARCH64_TLSLD_ADR_PREL21
#undef R_AARCH64_TLSLD_ADR_PAGE21
#undef R_AARCH64_TLSLD_ADD_LO12_NC
#undef R_AARCH64_TLSLD_MOVW_G1
#undef R_AARCH64_TLSLD_MOVW_G0_NC
#undef R_AARCH64_TLSLD_LD_PREL19
#undef R_AARCH64_TLSLD_MOVW_DTPREL_G2
#undef R_AARCH64_TLSLD_MOVW_DTPREL_G1
#undef R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC
#undef R_AARCH64_TLSLD_MOVW_DTPREL_G0
#undef R_AARCH64_TLSLD_MOVW_DTPREL_G0_NC
#undef R_AARCH64_TLSLD_ADD_DTPREL_HI12
#undef R_AARCH64_TLSLD_ADD_DTPREL_LO12
#undef R_AARCH64_TLSLD_ADD_DTPREL_LO12_NC
#undef R_AARCH64_TLSLD_LDST8_DTPREL_LO12
#undef R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC
#undef R_AARCH64_TLSLD_LDST16_DTPREL_LO12
#undef R_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC
#undef R_AARCH64_TLSLD_LDST32_DTPREL_LO12
#undef R_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC
#undef R_AARCH64_TLSLD_LDST64_DTPREL_LO12
#undef R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC
#undef R_AARCH64_TLSIE_MOVW_GOTTPREL_G1
#undef R_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC
#undef R_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21
#undef R_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC
#undef R_AARCH64_TLSIE_LD_GOTTPREL_PREL19
#undef R_AARCH64_TLSLE_MOVW_TPREL_G2
#undef R_AARCH64_TLSLE_MOVW_TPREL_G1
#undef R_AARCH64_TLSLE_MOVW_TPREL_G1_NC
#undef R_AARCH64_TLSLE_MOVW_TPREL_G0
#undef R_AARCH64_TLSLE_MOVW_TPREL_G0_NC
#undef R_AARCH64_TLSLE_ADD_TPREL_HI12
#undef R_AARCH64_TLSLE_ADD_TPREL_LO12
#undef R_AARCH64_TLSLE_ADD_TPREL_LO12_NC
#undef R_AARCH64_TLSLE_LDST8_TPREL_LO12
#undef R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC
#undef R_AARCH64_TLSLE_LDST16_TPREL_LO12
#undef R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC
#undef R_AARCH64_TLSLE_LDST32_TPREL_LO12
#undef R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC
#undef R_AARCH64_TLSLE_LDST64_TPREL_LO12
#undef R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC
#undef R_AARCH64_TLSDESC_LD_PREL19
#undef R_AARCH64_TLSDESC_ADR_PREL21
#undef R_AARCH64_TLSDESC_ADR_PAGE21
#undef R_AARCH64_TLSDESC_LD64_LO12
#undef R_AARCH64_TLSDESC_ADD_LO12
#undef R_AARCH64_TLSDESC_OFF_G1
#undef R_AARCH64_TLSDESC_OFF_G0_NC
#undef R_AARCH64_TLSDESC_LDR
#undef R_AARCH64_TLSDESC_ADD
#undef R_AARCH64_TLSDESC_CALL
#undef R_AARCH64_TLSLE_LDST128_TPREL_LO12
#undef R_AARCH64_TLSLE_LDST128_TPREL_LO12_NC
#undef R_AARCH64_TLSLD_LDST128_DTPREL_LO12
#undef R_AARCH64_TLSLD_LDST128_DTPREL_LO12_NC
#undef R_AARCH64_COPY
#undef R_AARCH64_GLOB_DAT
#undef R_AARCH64_JUMP_SLOT
#undef R_AARCH64_RELATIVE
#undef R_AARCH64_TLS_DTPMOD
#undef R_AARCH64_TLS_DTPREL
#undef R_AARCH64_TLS_TPREL
#undef R_AARCH64_TLSDESC
#undef R_AARCH64_IRELATIVE

/* AArch64 specific values for the Dyn d_tag field.  */
#undef DT_AARCH64_BTI_PLT
#undef DT_AARCH64_PAC_PLT
#undef DT_AARCH64_VARIANT_PCS
#undef DT_AARCH64_NUM

/* AArch64 specific values for the st_other field.  */
#undef STO_AARCH64_VARIANT_PCS

/* ARM relocs.  */

#undef R_ARM_NONE
#undef R_ARM_PC24
#undef R_ARM_ABS32
#undef R_ARM_REL32
#undef R_ARM_PC13
#undef R_ARM_ABS16
#undef R_ARM_ABS12
#undef R_ARM_THM_ABS5
#undef R_ARM_ABS8
#undef R_ARM_SBREL32
#undef R_ARM_THM_PC22
#undef R_ARM_THM_PC8
#undef R_ARM_AMP_VCALL9
#undef R_ARM_SWI24
#undef R_ARM_TLS_DESC
#undef R_ARM_THM_SWI8
#undef R_ARM_XPC25
#undef R_ARM_THM_XPC22
#undef R_ARM_TLS_DTPMOD32
#undef R_ARM_TLS_DTPOFF32
#undef R_ARM_TLS_TPOFF32
#undef R_ARM_COPY
#undef R_ARM_GLOB_DAT
#undef R_ARM_JUMP_SLOT
#undef R_ARM_RELATIVE
#undef R_ARM_GOTOFF
#undef R_ARM_GOTPC
#undef R_ARM_GOT32
#undef R_ARM_PLT32
#undef R_ARM_CALL
#undef R_ARM_JUMP24
#undef R_ARM_THM_JUMP24
#undef R_ARM_BASE_ABS
#undef R_ARM_ALU_PCREL_7_0
#undef R_ARM_ALU_PCREL_15_8
#undef R_ARM_ALU_PCREL_23_15
#undef R_ARM_LDR_SBREL_11_0
#undef R_ARM_ALU_SBREL_19_12
#undef R_ARM_ALU_SBREL_27_20
#undef R_ARM_TARGET1
#undef R_ARM_SBREL31
#undef R_ARM_V4BX
#undef R_ARM_TARGET2
#undef R_ARM_PREL31
#undef R_ARM_MOVW_ABS_NC
#undef R_ARM_MOVT_ABS
#undef R_ARM_MOVW_PREL_NC
#undef R_ARM_MOVT_PREL
#undef R_ARM_THM_MOVW_ABS_NC
#undef R_ARM_THM_MOVT_ABS
#undef R_ARM_THM_MOVW_PREL_NC
#undef R_ARM_THM_MOVT_PREL
#undef R_ARM_THM_JUMP19
#undef R_ARM_THM_JUMP6
#undef R_ARM_THM_ALU_PREL_11_0
#undef R_ARM_THM_PC12
#undef R_ARM_ABS32_NOI
#undef R_ARM_REL32_NOI
#undef R_ARM_ALU_PC_G0_NC
#undef R_ARM_ALU_PC_G0
#undef R_ARM_ALU_PC_G1_NC
#undef R_ARM_ALU_PC_G1
#undef R_ARM_ALU_PC_G2
#undef R_ARM_LDR_PC_G1
#undef R_ARM_LDR_PC_G2
#undef R_ARM_LDRS_PC_G0
#undef R_ARM_LDRS_PC_G1
#undef R_ARM_LDRS_PC_G2
#undef R_ARM_LDC_PC_G0
#undef R_ARM_LDC_PC_G1
#undef R_ARM_LDC_PC_G2
#undef R_ARM_ALU_SB_G0_NC
#undef R_ARM_ALU_SB_G0
#undef R_ARM_ALU_SB_G1_NC
#undef R_ARM_ALU_SB_G1
#undef R_ARM_ALU_SB_G2
#undef R_ARM_LDR_SB_G0
#undef R_ARM_LDR_SB_G1
#undef R_ARM_LDR_SB_G2
#undef R_ARM_LDRS_SB_G0
#undef R_ARM_LDRS_SB_G1
#undef R_ARM_LDRS_SB_G2
#undef R_ARM_LDC_SB_G0
#undef R_ARM_LDC_SB_G1
#undef R_ARM_LDC_SB_G2
#undef R_ARM_MOVW_BREL_NC
#undef R_ARM_MOVT_BREL
#undef R_ARM_MOVW_BREL
#undef R_ARM_THM_MOVW_BREL_NC
#undef R_ARM_THM_MOVT_BREL
#undef R_ARM_THM_MOVW_BREL
#undef R_ARM_TLS_GOTDESC
#undef R_ARM_TLS_CALL
#undef R_ARM_TLS_DESCSEQ
#undef R_ARM_THM_TLS_CALL
#undef R_ARM_PLT32_ABS
#undef R_ARM_GOT_ABS
#undef R_ARM_GOT_PREL
#undef R_ARM_GOT_BREL12
#undef R_ARM_GOTOFF12
#undef R_ARM_GOTRELAX
#undef R_ARM_GNU_VTENTRY
#undef R_ARM_GNU_VTINHERIT
#undef R_ARM_THM_PC11
#undef R_ARM_THM_PC9
#undef R_ARM_TLS_GD32
#undef R_ARM_TLS_LDM32
#undef R_ARM_TLS_LDO32
#undef R_ARM_TLS_IE32
#undef R_ARM_TLS_LE32
#undef R_ARM_TLS_LDO12
#undef R_ARM_TLS_LE12
#undef R_ARM_TLS_IE12GP
#undef R_ARM_ME_TOO
#undef R_ARM_THM_TLS_DESCSEQ
#undef R_ARM_THM_TLS_DESCSEQ16
#undef R_ARM_THM_TLS_DESCSEQ32
#undef R_ARM_THM_GOT_BREL12
#undef R_ARM_IRELATIVE
#undef R_ARM_RXPC25
#undef R_ARM_RSBREL32
#undef R_ARM_THM_RPC22
#undef R_ARM_RREL32
#undef R_ARM_RABS22
#undef R_ARM_RPC24
#undef R_ARM_RBASE
/* Keep this the last entry.  */
#undef R_ARM_NUM

/* C-SKY */
#undef R_CKCORE_NONE
#undef R_CKCORE_ADDR32
#undef R_CKCORE_PCRELIMM8BY4
#undef R_CKCORE_PCRELIMM11BY2
#undef R_CKCORE_PCREL32
#undef R_CKCORE_PCRELJSR_IMM11BY2
#undef R_CKCORE_RELATIVE
#undef R_CKCORE_COPY
#undef R_CKCORE_GLOB_DAT
#undef R_CKCORE_JUMP_SLOT
#undef R_CKCORE_GOTOFF
#undef R_CKCORE_GOTPC
#undef R_CKCORE_GOT32
#undef R_CKCORE_PLT32
#undef R_CKCORE_ADDRGOT
#undef R_CKCORE_ADDRPLT
#undef R_CKCORE_PCREL_IMM26BY2
#undef R_CKCORE_PCREL_IMM16BY2
#undef R_CKCORE_PCREL_IMM16BY4
#undef R_CKCORE_PCREL_IMM10BY2
#undef R_CKCORE_PCREL_IMM10BY4
#undef R_CKCORE_ADDR_HI16
#undef R_CKCORE_ADDR_LO16
#undef R_CKCORE_GOTPC_HI16
#undef R_CKCORE_GOTPC_LO16
#undef R_CKCORE_GOTOFF_HI16
#undef R_CKCORE_GOTOFF_LO16
#undef R_CKCORE_GOT12
#undef R_CKCORE_GOT_HI16
#undef R_CKCORE_GOT_LO16
#undef R_CKCORE_PLT12
#undef R_CKCORE_PLT_HI16
#undef R_CKCORE_PLT_LO16
#undef R_CKCORE_ADDRGOT_HI16
#undef R_CKCORE_ADDRGOT_LO16
#undef R_CKCORE_ADDRPLT_HI16
#undef R_CKCORE_ADDRPLT_LO16
#undef R_CKCORE_PCREL_JSR_IMM26BY2
#undef R_CKCORE_TOFFSET_LO16
#undef R_CKCORE_DOFFSET_LO16
#undef R_CKCORE_PCREL_IMM18BY2
#undef R_CKCORE_DOFFSET_IMM18
#undef R_CKCORE_DOFFSET_IMM18BY2
#undef R_CKCORE_DOFFSET_IMM18BY4
#undef R_CKCORE_GOT_IMM18BY4
#undef R_CKCORE_PLT_IMM18BY4
#undef R_CKCORE_PCREL_IMM7BY4
#undef R_CKCORE_TLS_LE32
#undef R_CKCORE_TLS_IE32
#undef R_CKCORE_TLS_GD32
#undef R_CKCORE_TLS_LDM32
#undef R_CKCORE_TLS_LDO32
#undef R_CKCORE_TLS_DTPMOD32
#undef R_CKCORE_TLS_DTPOFF32
#undef R_CKCORE_TLS_TPOFF32

/* C-SKY elf header definition.  */
#undef EF_CSKY_ABIMASK
#undef EF_CSKY_OTHER
#undef EF_CSKY_PROCESSOR

#undef EF_CSKY_ABIV1
#undef EF_CSKY_ABIV2

/* C-SKY attributes section.  */
#undef SHT_CSKY_ATTRIBUTES

/* IA-64 specific declarations.  */

/* Processor specific flags for the Ehdr e_flags field.  */
#undef EF_IA_64_MASKOS
#undef EF_IA_64_ABI64
#undef EF_IA_64_ARCH

/* Processor specific values for the Phdr p_type field.  */
#undef PT_IA_64_ARCHEXT
#undef PT_IA_64_UNWIND
#undef PT_IA_64_HP_OPT_ANOT
#undef PT_IA_64_HP_HSL_ANOT
#undef PT_IA_64_HP_STACK

/* Processor specific flags for the Phdr p_flags field.  */
#undef PF_IA_64_NORECOV

/* Processor specific values for the Shdr sh_type field.  */
#undef SHT_IA_64_EXT
#undef SHT_IA_64_UNWIND

/* Processor specific flags for the Shdr sh_flags field.  */
#undef SHF_IA_64_SHORT
#undef SHF_IA_64_NORECOV

/* Processor specific values for the Dyn d_tag field.  */
#undef DT_IA_64_PLT_RESERVE
#undef DT_IA_64_NUM

/* IA-64 relocations.  */
#undef R_IA64_NONE
#undef R_IA64_IMM14
#undef R_IA64_IMM22
#undef R_IA64_IMM64
#undef R_IA64_DIR32MSB
#undef R_IA64_DIR32LSB
#undef R_IA64_DIR64MSB
#undef R_IA64_DIR64LSB
#undef R_IA64_GPREL22
#undef R_IA64_GPREL64I
#undef R_IA64_GPREL32MSB
#undef R_IA64_GPREL32LSB
#undef R_IA64_GPREL64MSB
#undef R_IA64_GPREL64LSB
#undef R_IA64_LTOFF22
#undef R_IA64_LTOFF64I
#undef R_IA64_PLTOFF22
#undef R_IA64_PLTOFF64I
#undef R_IA64_PLTOFF64MSB
#undef R_IA64_PLTOFF64LSB
#undef R_IA64_FPTR64I
#undef R_IA64_FPTR32MSB
#undef R_IA64_FPTR32LSB
#undef R_IA64_FPTR64MSB
#undef R_IA64_FPTR64LSB
#undef R_IA64_PCREL60B
#undef R_IA64_PCREL21B
#undef R_IA64_PCREL21M
#undef R_IA64_PCREL21F
#undef R_IA64_PCREL32MSB
#undef R_IA64_PCREL32LSB
#undef R_IA64_PCREL64MSB
#undef R_IA64_PCREL64LSB
#undef R_IA64_LTOFF_FPTR22
#undef R_IA64_LTOFF_FPTR64I
#undef R_IA64_LTOFF_FPTR32MSB
#undef R_IA64_LTOFF_FPTR32LSB
#undef R_IA64_LTOFF_FPTR64MSB
#undef R_IA64_LTOFF_FPTR64LSB
#undef R_IA64_SEGREL32MSB
#undef R_IA64_SEGREL32LSB
#undef R_IA64_SEGREL64MSB
#undef R_IA64_SEGREL64LSB
#undef R_IA64_SECREL32MSB
#undef R_IA64_SECREL32LSB
#undef R_IA64_SECREL64MSB
#undef R_IA64_SECREL64LSB
#undef R_IA64_REL32MSB
#undef R_IA64_REL32LSB
#undef R_IA64_REL64MSB
#undef R_IA64_REL64LSB
#undef R_IA64_LTV32MSB
#undef R_IA64_LTV32LSB
#undef R_IA64_LTV64MSB
#undef R_IA64_LTV64LSB
#undef R_IA64_PCREL21BI
#undef R_IA64_PCREL22
#undef R_IA64_PCREL64I
#undef R_IA64_IPLTMSB
#undef R_IA64_IPLTLSB
#undef R_IA64_COPY
#undef R_IA64_SUB
#undef R_IA64_LTOFF22X
#undef R_IA64_LDXMOV
#undef R_IA64_TPREL14
#undef R_IA64_TPREL22
#undef R_IA64_TPREL64I
#undef R_IA64_TPREL64MSB
#undef R_IA64_TPREL64LSB
#undef R_IA64_LTOFF_TPREL22
#undef R_IA64_DTPMOD64MSB
#undef R_IA64_DTPMOD64LSB
#undef R_IA64_LTOFF_DTPMOD22
#undef R_IA64_DTPREL14
#undef R_IA64_DTPREL22
#undef R_IA64_DTPREL64I
#undef R_IA64_DTPREL32MSB
#undef R_IA64_DTPREL32LSB
#undef R_IA64_DTPREL64MSB
#undef R_IA64_DTPREL64LSB
#undef R_IA64_LTOFF_DTPREL22

/* SH specific declarations */

/* Processor specific flags for the ELF header e_flags field.  */
#undef EF_SH_MACH_MASK
#undef EF_SH_UNKNOWN
#undef EF_SH1
#undef EF_SH2
#undef EF_SH3
#undef EF_SH_DSP
#undef EF_SH3_DSP
#undef EF_SH4AL_DSP
#undef EF_SH3E
#undef EF_SH4
#undef EF_SH2E
#undef EF_SH4A
#undef EF_SH2A
#undef EF_SH4_NOFPU
#undef EF_SH4A_NOFPU
#undef EF_SH4_NOMMU_NOFPU
#undef EF_SH2A_NOFPU
#undef EF_SH3_NOMMU
#undef EF_SH2A_SH4_NOFPU
#undef EF_SH2A_SH3_NOFPU
#undef EF_SH2A_SH4
#undef EF_SH2A_SH3E

/* SH relocs.  */
#undef R_SH_NONE
#undef R_SH_DIR32
#undef R_SH_REL32
#undef R_SH_DIR8WPN
#undef R_SH_IND12W
#undef R_SH_DIR8WPL
#undef R_SH_DIR8WPZ
#undef R_SH_DIR8BP
#undef R_SH_DIR8W
#undef R_SH_DIR8L
#undef R_SH_SWITCH16
#undef R_SH_SWITCH32
#undef R_SH_USES
#undef R_SH_COUNT
#undef R_SH_ALIGN
#undef R_SH_CODE
#undef R_SH_DATA
#undef R_SH_LABEL
#undef R_SH_SWITCH8
#undef R_SH_GNU_VTINHERIT
#undef R_SH_GNU_VTENTRY
#undef R_SH_TLS_GD_32
#undef R_SH_TLS_LD_32
#undef R_SH_TLS_LDO_32
#undef R_SH_TLS_IE_32
#undef R_SH_TLS_LE_32
#undef R_SH_TLS_DTPMOD32
#undef R_SH_TLS_DTPOFF32
#undef R_SH_TLS_TPOFF32
#undef R_SH_GOT32
#undef R_SH_PLT32
#undef R_SH_COPY
#undef R_SH_GLOB_DAT
#undef R_SH_JMP_SLOT
#undef R_SH_RELATIVE
#undef R_SH_GOTOFF
#undef R_SH_GOTPC
/* Keep this the last entry.  */
#undef R_SH_NUM

/* S/390 specific definitions.  */

/* Valid values for the e_flags field.  */

#undef EF_S390_HIGH_GPRS

/* Additional s390 relocs */

#undef R_390_NONE
#undef R_390_8
#undef R_390_12
#undef R_390_16
#undef R_390_32
#undef R_390_PC32
#undef R_390_GOT12
#undef R_390_GOT32
#undef R_390_PLT32
#undef R_390_COPY
#undef R_390_GLOB_DAT
#undef R_390_JMP_SLOT
#undef R_390_RELATIVE
#undef R_390_GOTOFF32
#undef R_390_GOTPC
#undef R_390_GOT16
#undef R_390_PC16
#undef R_390_PC16DBL
#undef R_390_PLT16DBL
#undef R_390_PC32DBL
#undef R_390_PLT32DBL
#undef R_390_GOTPCDBL
#undef R_390_64
#undef R_390_PC64
#undef R_390_GOT64
#undef R_390_PLT64
#undef R_390_GOTENT
#undef R_390_GOTOFF16
#undef R_390_GOTOFF64
#undef R_390_GOTPLT12
#undef R_390_GOTPLT16
#undef R_390_GOTPLT32
#undef R_390_GOTPLT64
#undef R_390_GOTPLTENT
#undef R_390_PLTOFF16
#undef R_390_PLTOFF32
#undef R_390_PLTOFF64
#undef R_390_TLS_LOAD
#undef R_390_TLS_GDCALL
#undef R_390_TLS_LDCALL
#undef R_390_TLS_GD32
#undef R_390_TLS_GD64
#undef R_390_TLS_GOTIE12
#undef R_390_TLS_GOTIE32
#undef R_390_TLS_GOTIE64
#undef R_390_TLS_LDM32
#undef R_390_TLS_LDM64
#undef R_390_TLS_IE32
#undef R_390_TLS_IE64
#undef R_390_TLS_IEENT
#undef R_390_TLS_LE32
#undef R_390_TLS_LE64
#undef R_390_TLS_LDO32
#undef R_390_TLS_LDO64
#undef R_390_TLS_DTPMOD
#undef R_390_TLS_DTPOFF
#undef R_390_TLS_TPOFF
#undef R_390_20
#undef R_390_GOT20
#undef R_390_GOTPLT20
#undef R_390_TLS_GOTIE20
#undef R_390_IRELATIVE
/* Keep this the last entry.  */
#undef R_390_NUM

/* CRIS relocations.  */
#undef R_CRIS_NONE
#undef R_CRIS_8
#undef R_CRIS_16
#undef R_CRIS_32
#undef R_CRIS_8_PCREL
#undef R_CRIS_16_PCREL
#undef R_CRIS_32_PCREL
#undef R_CRIS_GNU_VTINHERIT
#undef R_CRIS_GNU_VTENTRY
#undef R_CRIS_COPY
#undef R_CRIS_GLOB_DAT
#undef R_CRIS_JUMP_SLOT
#undef R_CRIS_RELATIVE
#undef R_CRIS_16_GOT
#undef R_CRIS_32_GOT
#undef R_CRIS_16_GOTPLT
#undef R_CRIS_32_GOTPLT
#undef R_CRIS_32_GOTREL
#undef R_CRIS_32_PLT_GOTREL
#undef R_CRIS_32_PLT_PCREL

#undef R_CRIS_NUM

/* AMD x86-64 relocations.  */
#undef R_X86_64_NONE
#undef R_X86_64_64
#undef R_X86_64_PC32
#undef R_X86_64_GOT32
#undef R_X86_64_PLT32
#undef R_X86_64_COPY
#undef R_X86_64_GLOB_DAT
#undef R_X86_64_JUMP_SLOT
#undef R_X86_64_RELATIVE
#undef R_X86_64_GOTPCREL
#undef R_X86_64_32
#undef R_X86_64_32S
#undef R_X86_64_16
#undef R_X86_64_PC16
#undef R_X86_64_8
#undef R_X86_64_PC8
#undef R_X86_64_DTPMOD64
#undef R_X86_64_DTPOFF64
#undef R_X86_64_TPOFF64
#undef R_X86_64_TLSGD
#undef R_X86_64_TLSLD
#undef R_X86_64_DTPOFF32
#undef R_X86_64_GOTTPOFF
#undef R_X86_64_TPOFF32
#undef R_X86_64_PC64
#undef R_X86_64_GOTOFF64
#undef R_X86_64_GOTPC32
#undef R_X86_64_GOT64
#undef R_X86_64_GOTPCREL64
#undef R_X86_64_GOTPC64
#undef R_X86_64_GOTPLT64
#undef R_X86_64_PLTOFF64
#undef R_X86_64_SIZE32
#undef R_X86_64_SIZE64
#undef R_X86_64_GOTPC32_TLSDESC
#undef R_X86_64_TLSDESC_CALL
#undef R_X86_64_TLSDESC
#undef R_X86_64_IRELATIVE
#undef R_X86_64_RELATIVE64
#undef R_X86_64_GOTPCRELX
#undef R_X86_64_REX_GOTPCRELX
#undef R_X86_64_NUM

/* x86-64 sh_type values.  */
#undef SHT_X86_64_UNWIND

/* AM33 relocations.  */
#undef R_MN10300_NONE
#undef R_MN10300_32
#undef R_MN10300_16
#undef R_MN10300_8
#undef R_MN10300_PCREL32
#undef R_MN10300_PCREL16
#undef R_MN10300_PCREL8
#undef R_MN10300_GNU_VTINHERIT
#undef R_MN10300_GNU_VTENTRY
#undef R_MN10300_24
#undef R_MN10300_GOTPC32
#undef R_MN10300_GOTPC16
#undef R_MN10300_GOTOFF32
#undef R_MN10300_GOTOFF24
#undef R_MN10300_GOTOFF16
#undef R_MN10300_PLT32
#undef R_MN10300_PLT16
#undef R_MN10300_GOT32
#undef R_MN10300_GOT24
#undef R_MN10300_GOT16
#undef R_MN10300_COPY
#undef R_MN10300_GLOB_DAT
#undef R_MN10300_JMP_SLOT
#undef R_MN10300_RELATIVE
#undef R_MN10300_TLS_GD
#undef R_MN10300_TLS_LD
#undef R_MN10300_TLS_LDO
#undef R_MN10300_TLS_GOTIE
#undef R_MN10300_TLS_IE
#undef R_MN10300_TLS_LE
#undef R_MN10300_TLS_DTPMOD
#undef R_MN10300_TLS_DTPOFF
#undef R_MN10300_TLS_TPOFF
#undef R_MN10300_SYM_DIFF
#undef R_MN10300_ALIGN
#undef R_MN10300_NUM

/* M32R relocs.  */
#undef R_M32R_NONE
#undef R_M32R_16
#undef R_M32R_32
#undef R_M32R_24
#undef R_M32R_10_PCREL
#undef R_M32R_18_PCREL
#undef R_M32R_26_PCREL
#undef R_M32R_HI16_ULO
#undef R_M32R_HI16_SLO
#undef R_M32R_LO16
#undef R_M32R_SDA16
#undef R_M32R_GNU_VTINHERIT
#undef R_M32R_GNU_VTENTRY
/* M32R relocs use SHT_RELA.  */
#undef R_M32R_16_RELA
#undef R_M32R_32_RELA
#undef R_M32R_24_RELA
#undef R_M32R_10_PCREL_RELA
#undef R_M32R_18_PCREL_RELA
#undef R_M32R_26_PCREL_RELA
#undef R_M32R_HI16_ULO_RELA
#undef R_M32R_HI16_SLO_RELA
#undef R_M32R_LO16_RELA
#undef R_M32R_SDA16_RELA
#undef R_M32R_RELA_GNU_VTINHERIT
#undef R_M32R_RELA_GNU_VTENTRY
#undef R_M32R_REL32

#undef R_M32R_GOT24
#undef R_M32R_26_PLTREL
#undef R_M32R_COPY
#undef R_M32R_GLOB_DAT
#undef R_M32R_JMP_SLOT
#undef R_M32R_RELATIVE
#undef R_M32R_GOTOFF
#undef R_M32R_GOTPC24
#undef R_M32R_GOT16_HI_ULO
#undef R_M32R_GOT16_HI_SLO
#undef R_M32R_GOT16_LO
#undef R_M32R_GOTPC_HI_ULO
#undef R_M32R_GOTPC_HI_SLO
#undef R_M32R_GOTPC_LO
#undef R_M32R_GOTOFF_HI_ULO
#undef R_M32R_GOTOFF_HI_SLO
#undef R_M32R_GOTOFF_LO
#undef R_M32R_NUM

/* MicroBlaze relocations */
#undef R_MICROBLAZE_NONE
#undef R_MICROBLAZE_32
#undef R_MICROBLAZE_32_PCREL
#undef R_MICROBLAZE_64_PCREL
#undef R_MICROBLAZE_32_PCREL_LO
#undef R_MICROBLAZE_64
#undef R_MICROBLAZE_32_LO
#undef R_MICROBLAZE_SRO32
#undef R_MICROBLAZE_SRW32
#undef R_MICROBLAZE_64_NONE
#undef R_MICROBLAZE_32_SYM_OP_SYM
#undef R_MICROBLAZE_GNU_VTINHERIT
#undef R_MICROBLAZE_GNU_VTENTRY
#undef R_MICROBLAZE_GOTPC_64
#undef R_MICROBLAZE_GOT_64
#undef R_MICROBLAZE_PLT_64
#undef R_MICROBLAZE_REL
#undef R_MICROBLAZE_JUMP_SLOT
#undef R_MICROBLAZE_GLOB_DAT
#undef R_MICROBLAZE_GOTOFF_64
#undef R_MICROBLAZE_GOTOFF_32
#undef R_MICROBLAZE_COPY
#undef R_MICROBLAZE_TLS
#undef R_MICROBLAZE_TLSGD
#undef R_MICROBLAZE_TLSLD
#undef R_MICROBLAZE_TLSDTPMOD32
#undef R_MICROBLAZE_TLSDTPREL32
#undef R_MICROBLAZE_TLSDTPREL64
#undef R_MICROBLAZE_TLSGOTTPREL32
#undef R_MICROBLAZE_TLSTPREL32

/* Legal values for d_tag (dynamic entry type).  */
#undef DT_NIOS2_GP

/* Nios II relocations.  */
#undef R_NIOS2_NONE
#undef R_NIOS2_S16
#undef R_NIOS2_U16
#undef R_NIOS2_PCREL16
#undef R_NIOS2_CALL26
#undef R_NIOS2_IMM5
#undef R_NIOS2_CACHE_OPX
#undef R_NIOS2_IMM6
#undef R_NIOS2_IMM8
#undef R_NIOS2_HI16
#undef R_NIOS2_LO16
#undef R_NIOS2_HIADJ16
#undef R_NIOS2_BFD_RELOC_32
#undef R_NIOS2_BFD_RELOC_16
#undef R_NIOS2_BFD_RELOC_8
#undef R_NIOS2_GPREL
#undef R_NIOS2_GNU_VTINHERIT
#undef R_NIOS2_GNU_VTENTRY
#undef R_NIOS2_UJMP
#undef R_NIOS2_CJMP
#undef R_NIOS2_CALLR
#undef R_NIOS2_ALIGN
#undef R_NIOS2_GOT16
#undef R_NIOS2_CALL16
#undef R_NIOS2_GOTOFF_LO
#undef R_NIOS2_GOTOFF_HA
#undef R_NIOS2_PCREL_LO
#undef R_NIOS2_PCREL_HA
#undef R_NIOS2_TLS_GD16
#undef R_NIOS2_TLS_LDM16
#undef R_NIOS2_TLS_LDO16
#undef R_NIOS2_TLS_IE16
#undef R_NIOS2_TLS_LE16
#undef R_NIOS2_TLS_DTPMOD
#undef R_NIOS2_TLS_DTPREL
#undef R_NIOS2_TLS_TPREL
#undef R_NIOS2_COPY
#undef R_NIOS2_GLOB_DAT
#undef R_NIOS2_JUMP_SLOT
#undef R_NIOS2_RELATIVE
#undef R_NIOS2_GOTOFF
#undef R_NIOS2_CALL26_NOAT
#undef R_NIOS2_GOT_LO
#undef R_NIOS2_GOT_HA
#undef R_NIOS2_CALL_LO
#undef R_NIOS2_CALL_HA

/* TILEPro relocations.  */
#undef R_TILEPRO_NONE
#undef R_TILEPRO_32
#undef R_TILEPRO_16
#undef R_TILEPRO_8
#undef R_TILEPRO_32_PCREL
#undef R_TILEPRO_16_PCREL
#undef R_TILEPRO_8_PCREL
#undef R_TILEPRO_LO16
#undef R_TILEPRO_HI16
#undef R_TILEPRO_HA16
#undef R_TILEPRO_COPY
#undef R_TILEPRO_GLOB_DAT
#undef R_TILEPRO_JMP_SLOT
#undef R_TILEPRO_RELATIVE
#undef R_TILEPRO_BROFF_X1
#undef R_TILEPRO_JOFFLONG_X1
#undef R_TILEPRO_JOFFLONG_X1_PLT
#undef R_TILEPRO_IMM8_X0
#undef R_TILEPRO_IMM8_Y0
#undef R_TILEPRO_IMM8_X1
#undef R_TILEPRO_IMM8_Y1
#undef R_TILEPRO_MT_IMM15_X1
#undef R_TILEPRO_MF_IMM15_X1
#undef R_TILEPRO_IMM16_X0
#undef R_TILEPRO_IMM16_X1
#undef R_TILEPRO_IMM16_X0_LO
#undef R_TILEPRO_IMM16_X1_LO
#undef R_TILEPRO_IMM16_X0_HI
#undef R_TILEPRO_IMM16_X1_HI
#undef R_TILEPRO_IMM16_X0_HA
#undef R_TILEPRO_IMM16_X1_HA
#undef R_TILEPRO_IMM16_X0_PCREL
#undef R_TILEPRO_IMM16_X1_PCREL
#undef R_TILEPRO_IMM16_X0_LO_PCREL
#undef R_TILEPRO_IMM16_X1_LO_PCREL
#undef R_TILEPRO_IMM16_X0_HI_PCREL
#undef R_TILEPRO_IMM16_X1_HI_PCREL
#undef R_TILEPRO_IMM16_X0_HA_PCREL
#undef R_TILEPRO_IMM16_X1_HA_PCREL
#undef R_TILEPRO_IMM16_X0_GOT
#undef R_TILEPRO_IMM16_X1_GOT
#undef R_TILEPRO_IMM16_X0_GOT_LO
#undef R_TILEPRO_IMM16_X1_GOT_LO
#undef R_TILEPRO_IMM16_X0_GOT_HI
#undef R_TILEPRO_IMM16_X1_GOT_HI
#undef R_TILEPRO_IMM16_X0_GOT_HA
#undef R_TILEPRO_IMM16_X1_GOT_HA
#undef R_TILEPRO_MMSTART_X0
#undef R_TILEPRO_MMEND_X0
#undef R_TILEPRO_MMSTART_X1
#undef R_TILEPRO_MMEND_X1
#undef R_TILEPRO_SHAMT_X0
#undef R_TILEPRO_SHAMT_X1
#undef R_TILEPRO_SHAMT_Y0
#undef R_TILEPRO_SHAMT_Y1
#undef R_TILEPRO_DEST_IMM8_X1
/* Relocs 56-59 are currently not defined.  */
#undef R_TILEPRO_TLS_GD_CALL
#undef R_TILEPRO_IMM8_X0_TLS_GD_ADD
#undef R_TILEPRO_IMM8_X1_TLS_GD_ADD
#undef R_TILEPRO_IMM8_Y0_TLS_GD_ADD
#undef R_TILEPRO_IMM8_Y1_TLS_GD_ADD
#undef R_TILEPRO_TLS_IE_LOAD
#undef R_TILEPRO_IMM16_X0_TLS_GD
#undef R_TILEPRO_IMM16_X1_TLS_GD
#undef R_TILEPRO_IMM16_X0_TLS_GD_LO
#undef R_TILEPRO_IMM16_X1_TLS_GD_LO
#undef R_TILEPRO_IMM16_X0_TLS_GD_HI
#undef R_TILEPRO_IMM16_X1_TLS_GD_HI
#undef R_TILEPRO_IMM16_X0_TLS_GD_HA
#undef R_TILEPRO_IMM16_X1_TLS_GD_HA
#undef R_TILEPRO_IMM16_X0_TLS_IE
#undef R_TILEPRO_IMM16_X1_TLS_IE
#undef R_TILEPRO_IMM16_X0_TLS_IE_LO
#undef R_TILEPRO_IMM16_X1_TLS_IE_LO
#undef R_TILEPRO_IMM16_X0_TLS_IE_HI
#undef R_TILEPRO_IMM16_X1_TLS_IE_HI
#undef R_TILEPRO_IMM16_X0_TLS_IE_HA
#undef R_TILEPRO_IMM16_X1_TLS_IE_HA
#undef R_TILEPRO_TLS_DTPMOD32
#undef R_TILEPRO_TLS_DTPOFF32
#undef R_TILEPRO_TLS_TPOFF32
#undef R_TILEPRO_IMM16_X0_TLS_LE
#undef R_TILEPRO_IMM16_X1_TLS_LE
#undef R_TILEPRO_IMM16_X0_TLS_LE_LO
#undef R_TILEPRO_IMM16_X1_TLS_LE_LO
#undef R_TILEPRO_IMM16_X0_TLS_LE_HI
#undef R_TILEPRO_IMM16_X1_TLS_LE_HI
#undef R_TILEPRO_IMM16_X0_TLS_LE_HA
#undef R_TILEPRO_IMM16_X1_TLS_LE_HA

#undef R_TILEPRO_GNU_VTINHERIT
#undef R_TILEPRO_GNU_VTENTRY

#undef R_TILEPRO_NUM

/* TILE-Gx relocations.  */
#undef R_TILEGX_NONE
#undef R_TILEGX_64
#undef R_TILEGX_32
#undef R_TILEGX_16
#undef R_TILEGX_8
#undef R_TILEGX_64_PCREL
#undef R_TILEGX_32_PCREL
#undef R_TILEGX_16_PCREL
#undef R_TILEGX_8_PCREL
#undef R_TILEGX_HW0
#undef R_TILEGX_HW1
#undef R_TILEGX_HW2
#undef R_TILEGX_HW3
#undef R_TILEGX_HW0_LAST
#undef R_TILEGX_HW1_LAST
#undef R_TILEGX_HW2_LAST
#undef R_TILEGX_COPY
#undef R_TILEGX_GLOB_DAT
#undef R_TILEGX_JMP_SLOT
#undef R_TILEGX_RELATIVE
#undef R_TILEGX_BROFF_X1
#undef R_TILEGX_JUMPOFF_X1
#undef R_TILEGX_JUMPOFF_X1_PLT
#undef R_TILEGX_IMM8_X0
#undef R_TILEGX_IMM8_Y0
#undef R_TILEGX_IMM8_X1
#undef R_TILEGX_IMM8_Y1
#undef R_TILEGX_DEST_IMM8_X1
#undef R_TILEGX_MT_IMM14_X1
#undef R_TILEGX_MF_IMM14_X1
#undef R_TILEGX_MMSTART_X0
#undef R_TILEGX_MMEND_X0
#undef R_TILEGX_SHAMT_X0
#undef R_TILEGX_SHAMT_X1
#undef R_TILEGX_SHAMT_Y0
#undef R_TILEGX_SHAMT_Y1
#undef R_TILEGX_IMM16_X0_HW0
#undef R_TILEGX_IMM16_X1_HW0
#undef R_TILEGX_IMM16_X0_HW1
#undef R_TILEGX_IMM16_X1_HW1
#undef R_TILEGX_IMM16_X0_HW2
#undef R_TILEGX_IMM16_X1_HW2
#undef R_TILEGX_IMM16_X0_HW3
#undef R_TILEGX_IMM16_X1_HW3
#undef R_TILEGX_IMM16_X0_HW0_LAST
#undef R_TILEGX_IMM16_X1_HW0_LAST
#undef R_TILEGX_IMM16_X0_HW1_LAST
#undef R_TILEGX_IMM16_X1_HW1_LAST
#undef R_TILEGX_IMM16_X0_HW2_LAST
#undef R_TILEGX_IMM16_X1_HW2_LAST
#undef R_TILEGX_IMM16_X0_HW0_PCREL
#undef R_TILEGX_IMM16_X1_HW0_PCREL
#undef R_TILEGX_IMM16_X0_HW1_PCREL
#undef R_TILEGX_IMM16_X1_HW1_PCREL
#undef R_TILEGX_IMM16_X0_HW2_PCREL
#undef R_TILEGX_IMM16_X1_HW2_PCREL
#undef R_TILEGX_IMM16_X0_HW3_PCREL
#undef R_TILEGX_IMM16_X1_HW3_PCREL
#undef R_TILEGX_IMM16_X0_HW0_LAST_PCREL
#undef R_TILEGX_IMM16_X1_HW0_LAST_PCREL
#undef R_TILEGX_IMM16_X0_HW1_LAST_PCREL
#undef R_TILEGX_IMM16_X1_HW1_LAST_PCREL
#undef R_TILEGX_IMM16_X0_HW2_LAST_PCREL
#undef R_TILEGX_IMM16_X1_HW2_LAST_PCREL
#undef R_TILEGX_IMM16_X0_HW0_GOT
#undef R_TILEGX_IMM16_X1_HW0_GOT
#undef R_TILEGX_IMM16_X0_HW0_PLT_PCREL
#undef R_TILEGX_IMM16_X1_HW0_PLT_PCREL
#undef R_TILEGX_IMM16_X0_HW1_PLT_PCREL
#undef R_TILEGX_IMM16_X1_HW1_PLT_PCREL
#undef R_TILEGX_IMM16_X0_HW2_PLT_PCREL
#undef R_TILEGX_IMM16_X1_HW2_PLT_PCREL
#undef R_TILEGX_IMM16_X0_HW0_LAST_GOT
#undef R_TILEGX_IMM16_X1_HW0_LAST_GOT
#undef R_TILEGX_IMM16_X0_HW1_LAST_GOT
#undef R_TILEGX_IMM16_X1_HW1_LAST_GOT
#undef R_TILEGX_IMM16_X0_HW3_PLT_PCREL
#undef R_TILEGX_IMM16_X1_HW3_PLT_PCREL
#undef R_TILEGX_IMM16_X0_HW0_TLS_GD
#undef R_TILEGX_IMM16_X1_HW0_TLS_GD
#undef R_TILEGX_IMM16_X0_HW0_TLS_LE
#undef R_TILEGX_IMM16_X1_HW0_TLS_LE
#undef R_TILEGX_IMM16_X0_HW0_LAST_TLS_LE
#undef R_TILEGX_IMM16_X1_HW0_LAST_TLS_LE
#undef R_TILEGX_IMM16_X0_HW1_LAST_TLS_LE
#undef R_TILEGX_IMM16_X1_HW1_LAST_TLS_LE
#undef R_TILEGX_IMM16_X0_HW0_LAST_TLS_GD
#undef R_TILEGX_IMM16_X1_HW0_LAST_TLS_GD
#undef R_TILEGX_IMM16_X0_HW1_LAST_TLS_GD
#undef R_TILEGX_IMM16_X1_HW1_LAST_TLS_GD
/* Relocs 90-91 are currently not defined.  */
#undef R_TILEGX_IMM16_X0_HW0_TLS_IE
#undef R_TILEGX_IMM16_X1_HW0_TLS_IE
#undef R_TILEGX_IMM16_X0_HW0_LAST_PLT_PCREL
#undef R_TILEGX_IMM16_X1_HW0_LAST_PLT_PCREL
#undef R_TILEGX_IMM16_X0_HW1_LAST_PLT_PCREL
#undef R_TILEGX_IMM16_X1_HW1_LAST_PLT_PCREL
#undef R_TILEGX_IMM16_X0_HW2_LAST_PLT_PCREL
#undef R_TILEGX_IMM16_X1_HW2_LAST_PLT_PCREL
#undef R_TILEGX_IMM16_X0_HW0_LAST_TLS_IE
#undef R_TILEGX_IMM16_X1_HW0_LAST_TLS_IE
#undef R_TILEGX_IMM16_X0_HW1_LAST_TLS_IE
#undef R_TILEGX_IMM16_X1_HW1_LAST_TLS_IE
/* Relocs 104-105 are currently not defined.  */
#undef R_TILEGX_TLS_DTPMOD64
#undef R_TILEGX_TLS_DTPOFF64
#undef R_TILEGX_TLS_TPOFF64
#undef R_TILEGX_TLS_DTPMOD32
#undef R_TILEGX_TLS_DTPOFF32
#undef R_TILEGX_TLS_TPOFF32
#undef R_TILEGX_TLS_GD_CALL
#undef R_TILEGX_IMM8_X0_TLS_GD_ADD
#undef R_TILEGX_IMM8_X1_TLS_GD_ADD
#undef R_TILEGX_IMM8_Y0_TLS_GD_ADD
#undef R_TILEGX_IMM8_Y1_TLS_GD_ADD
#undef R_TILEGX_TLS_IE_LOAD
#undef R_TILEGX_IMM8_X0_TLS_ADD
#undef R_TILEGX_IMM8_X1_TLS_ADD
#undef R_TILEGX_IMM8_Y0_TLS_ADD
#undef R_TILEGX_IMM8_Y1_TLS_ADD

#undef R_TILEGX_GNU_VTINHERIT
#undef R_TILEGX_GNU_VTENTRY

#undef R_TILEGX_NUM

/* RISC-V ELF Flags */
#undef EF_RISCV_RVC
#undef EF_RISCV_FLOAT_ABI
#undef EF_RISCV_FLOAT_ABI_SOFT
#undef EF_RISCV_FLOAT_ABI_SINGLE
#undef EF_RISCV_FLOAT_ABI_DOUBLE
#undef EF_RISCV_FLOAT_ABI_QUAD

/* RISC-V relocations.  */
#undef R_RISCV_NONE
#undef R_RISCV_32
#undef R_RISCV_64
#undef R_RISCV_RELATIVE
#undef R_RISCV_COPY
#undef R_RISCV_JUMP_SLOT
#undef R_RISCV_TLS_DTPMOD32
#undef R_RISCV_TLS_DTPMOD64
#undef R_RISCV_TLS_DTPREL32
#undef R_RISCV_TLS_DTPREL64
#undef R_RISCV_TLS_TPREL32
#undef R_RISCV_TLS_TPREL64
#undef R_RISCV_BRANCH
#undef R_RISCV_JAL
#undef R_RISCV_CALL
#undef R_RISCV_CALL_PLT
#undef R_RISCV_GOT_HI20
#undef R_RISCV_TLS_GOT_HI20
#undef R_RISCV_TLS_GD_HI20
#undef R_RISCV_PCREL_HI20
#undef R_RISCV_PCREL_LO12_I
#undef R_RISCV_PCREL_LO12_S
#undef R_RISCV_HI20
#undef R_RISCV_LO12_I
#undef R_RISCV_LO12_S
#undef R_RISCV_TPREL_HI20
#undef R_RISCV_TPREL_LO12_I
#undef R_RISCV_TPREL_LO12_S
#undef R_RISCV_TPREL_ADD
#undef R_RISCV_ADD8
#undef R_RISCV_ADD16
#undef R_RISCV_ADD32
#undef R_RISCV_ADD64
#undef R_RISCV_SUB8
#undef R_RISCV_SUB16
#undef R_RISCV_SUB32
#undef R_RISCV_SUB64
#undef R_RISCV_GNU_VTINHERIT
#undef R_RISCV_GNU_VTENTRY
#undef R_RISCV_ALIGN
#undef R_RISCV_RVC_BRANCH
#undef R_RISCV_RVC_JUMP
#undef R_RISCV_RVC_LUI
#undef R_RISCV_GPREL_I
#undef R_RISCV_GPREL_S
#undef R_RISCV_TPREL_I
#undef R_RISCV_TPREL_S
#undef R_RISCV_RELAX
#undef R_RISCV_SUB6
#undef R_RISCV_SET6
#undef R_RISCV_SET8
#undef R_RISCV_SET16
#undef R_RISCV_SET32
#undef R_RISCV_32_PCREL
#undef R_RISCV_IRELATIVE

#undef R_RISCV_NUM

/* BPF specific declarations.  */

#undef R_BPF_NONE
#undef R_BPF_64_64
#undef R_BPF_64_32

/* Imagination Meta specific relocations. */

#undef R_METAG_HIADDR16
#undef R_METAG_LOADDR16
#undef R_METAG_ADDR32
#undef R_METAG_NONE
#undef R_METAG_RELBRANCH
#undef R_METAG_GETSETOFF

/* Backward compatibility */
#undef R_METAG_REG32OP1
#undef R_METAG_REG32OP2
#undef R_METAG_REG32OP3
#undef R_METAG_REG16OP1
#undef R_METAG_REG16OP2
#undef R_METAG_REG16OP3
#undef R_METAG_REG32OP4

#undef R_METAG_HIOG
#undef R_METAG_LOOG

#undef R_METAG_REL8
#undef R_METAG_REL16

/* GNU */
#undef R_METAG_GNU_VTINHERIT
#undef R_METAG_GNU_VTENTRY

/* PIC relocations */
#undef R_METAG_HI16_GOTOFF
#undef R_METAG_LO16_GOTOFF
#undef R_METAG_GETSET_GOTOFF
#undef R_METAG_GETSET_GOT
#undef R_METAG_HI16_GOTPC
#undef R_METAG_LO16_GOTPC
#undef R_METAG_HI16_PLT
#undef R_METAG_LO16_PLT
#undef R_METAG_RELBRANCH_PLT
#undef R_METAG_GOTOFF
#undef R_METAG_PLT
#undef R_METAG_COPY
#undef R_METAG_JMP_SLOT
#undef R_METAG_RELATIVE
#undef R_METAG_GLOB_DAT

/* TLS relocations */
#undef R_METAG_TLS_GD
#undef R_METAG_TLS_LDM
#undef R_METAG_TLS_LDO_HI16
#undef R_METAG_TLS_LDO_LO16
#undef R_METAG_TLS_LDO
#undef R_METAG_TLS_IE
#undef R_METAG_TLS_IENONPIC
#undef R_METAG_TLS_IENONPIC_HI16
#undef R_METAG_TLS_IENONPIC_LO16
#undef R_METAG_TLS_TPOFF
#undef R_METAG_TLS_DTPMOD
#undef R_METAG_TLS_DTPOFF
#undef R_METAG_TLS_LE
#undef R_METAG_TLS_LE_HI16
#undef R_METAG_TLS_LE_LO16

/* NDS32 relocations.  */
#undef R_NDS32_NONE
#undef R_NDS32_32_RELA
#undef R_NDS32_COPY
#undef R_NDS32_GLOB_DAT
#undef R_NDS32_JMP_SLOT
#undef R_NDS32_RELATIVE
#undef R_NDS32_TLS_TPOFF
#undef R_NDS32_TLS_DESC

/* ARCompact/ARCv2 specific relocs.  */
#undef R_ARC_NONE
#undef R_ARC_8
#undef R_ARC_16
#undef R_ARC_24
#undef R_ARC_32
#undef R_ARC_B26
#undef R_ARC_B22_PCREL
#undef R_ARC_H30
#undef R_ARC_N8
#undef R_ARC_N16
#undef R_ARC_N24
#undef R_ARC_N32
#undef R_ARC_SDA
#undef R_ARC_SECTOFF
#undef R_ARC_S21H_PCREL
#undef R_ARC_S21W_PCREL
#undef R_ARC_S25H_PCREL
#undef R_ARC_S25W_PCREL
#undef R_ARC_SDA32
#undef R_ARC_SDA_LDST
#undef R_ARC_SDA_LDST1
#undef R_ARC_SDA_LDST2
#undef R_ARC_SDA16_LD
#undef R_ARC_SDA16_LD1
#undef R_ARC_SDA16_LD2
#undef R_ARC_S13_PCREL
#undef R_ARC_W
#undef R_ARC_32_ME
#undef R_ARC_N32_ME
#undef R_ARC_SECTOFF_ME
#undef R_ARC_SDA32_ME
#undef R_ARC_W_ME
#undef R_ARC_H30_ME
#undef R_ARC_SECTOFF_U8
#undef R_ARC_SECTOFF_S9
#undef R_AC_SECTOFF_U8
#undef R_AC_SECTOFF_U8_1
#undef R_AC_SECTOFF_U8_2
#undef R_AC_SECTOFF_S9
#undef R_AC_SECTOFF_S9_1
#undef R_AC_SECTOFF_S9_2
#undef R_ARC_SECTOFF_ME_1
#undef R_ARC_SECTOFF_ME_2
#undef R_ARC_SECTOFF_1
#undef R_ARC_SECTOFF_2
#undef R_ARC_PC32
#undef R_ARC_GOTPC32
#undef R_ARC_PLT32
#undef R_ARC_COPY
#undef R_ARC_GLOB_DAT
#undef R_ARC_JUMP_SLOT
#undef R_ARC_RELATIVE
#undef R_ARC_GOTOFF
#undef R_ARC_GOTPC
#undef R_ARC_GOT32

#undef R_ARC_TLS_DTPMOD
#undef R_ARC_TLS_DTPOFF
#undef R_ARC_TLS_TPOFF
#undef R_ARC_TLS_GD_GOT
#undef R_ARC_TLS_GD_LD
#undef R_ARC_TLS_GD_CALL
#undef R_ARC_TLS_IE_GOT
#undef R_ARC_TLS_DTPOFF_S9
#undef R_ARC_TLS_LE_S9
#undef R_ARC_TLS_LE_32

/* OpenRISC 1000 specific relocs.  */
#undef R_OR1K_NONE
#undef R_OR1K_32
#undef R_OR1K_16
#undef R_OR1K_8
#undef R_OR1K_LO_16_IN_INSN
#undef R_OR1K_HI_16_IN_INSN
#undef R_OR1K_INSN_REL_26
#undef R_OR1K_GNU_VTENTRY
#undef R_OR1K_GNU_VTINHERIT
#undef R_OR1K_32_PCREL
#undef R_OR1K_16_PCREL
#undef R_OR1K_8_PCREL
#undef R_OR1K_GOTPC_HI16
#undef R_OR1K_GOTPC_LO16
#undef R_OR1K_GOT16
#undef R_OR1K_PLT26
#undef R_OR1K_GOTOFF_HI16
#undef R_OR1K_GOTOFF_LO16
#undef R_OR1K_COPY
#undef R_OR1K_GLOB_DAT
#undef R_OR1K_JMP_SLOT
#undef R_OR1K_RELATIVE
#undef R_OR1K_TLS_GD_HI16
#undef R_OR1K_TLS_GD_LO16
#undef R_OR1K_TLS_LDM_HI16
#undef R_OR1K_TLS_LDM_LO16
#undef R_OR1K_TLS_LDO_HI16
#undef R_OR1K_TLS_LDO_LO16
#undef R_OR1K_TLS_IE_HI16
#undef R_OR1K_TLS_IE_LO16
#undef R_OR1K_TLS_LE_HI16
#undef R_OR1K_TLS_LE_LO16
#undef R_OR1K_TLS_TPOFF
#undef R_OR1K_TLS_DTPOFF
#undef R_OR1K_TLS_DTPMOD

#undef EM_ARC_COMPACT2
#undef R_AARCH64_TLS_DTPMOD64
#undef R_AARCH64_TLS_DTPREL64
#undef R_AARCH64_TLS_TPREL64

#endif /* _UNDEFELF_H_ */
