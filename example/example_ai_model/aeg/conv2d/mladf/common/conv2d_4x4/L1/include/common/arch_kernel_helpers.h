// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
/*
(c) Copyright 2019 - 2021 Xilinx, Inc. All Rights reserved.
(c) Copyright 2022 - 2024 Advanced Micro Devices, Inc. All Rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */

#ifndef __ARCH_KERNEL_HELPERS_H__
#define __ARCH_KERNEL_HELPERS_H__


struct Accs4_i32 property(keep_in_registers) {
    v32acc32 a0;
    v32acc32 a1;
    v32acc32 a2;
    v32acc32 a3;
};

inline void undef(Accs4_i32 &accs) {
    accs.a0 = undef_v32acc32();
    accs.a1 = undef_v32acc32();
    accs.a2 = undef_v32acc32();
    accs.a3 = undef_v32acc32();
}

enum KernelConfig {
    KC_ZERO,
    KC_RESULT4,
    KC_RESULT8,
    KC_RESULT16,
    KC_RESULT32,
    KC_CASC,
    KC_TDM16,
    KC_TDM32,
    KC_TDM64,
    KC_TDM16_CASC,
    KC_TDM32_CASC,
    KC_TDM64_CASC,
    KC_RESULT32_CASC,
};

#ifndef ALWAYS_INLINE
#ifdef  __clang__
#define ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define ALWAYS_INLINE inline
#endif
#endif

#ifdef __clang__
#define RSTRCT __restrict
#else
#define RSTRCT
#endif

inline void load_full(v32acc32 &acc, v8int32 __aie_dm_resource_c *& ptr) {
    v8acc32 __aie_dm_resource_c * pIn = (v8acc32 __aie_dm_resource_c *) ptr;
    //acc = undef_v32acc32();
    acc = insert(acc, 0, *pIn++);
    acc = insert(acc, 1, *pIn++);
    acc = insert(acc, 2, *pIn++);
    acc = insert(acc, 3, *pIn);
    ptr = (v8int32 __aie_dm_resource_c *) pIn;
}
#ifndef __X86SIM__
inline void load_full(v32acc32 &acc, v8int32 __aie_dm_resource_d *& ptr) {
    v8acc32 __aie_dm_resource_d * pIn = (v8acc32 __aie_dm_resource_d *) ptr;
    //acc = undef_v32acc32();
    acc = insert(acc, 0, *pIn++);
    acc = insert(acc, 1, *pIn++);
    acc = insert(acc, 2, *pIn++);
    acc = insert(acc, 3, *pIn);
    ptr = (v8int32 __aie_dm_resource_d *) pIn;
}
#endif

inline void store_full(v8int32 __aie_dm_resource_c * RSTRCT & ptr, v32acc32 acc) {
    v8acc32 __aie_dm_resource_c * pOut = (v8acc32 __aie_dm_resource_c *) ptr;
    *pOut++ = extract_v8acc32(acc, 0);
    *pOut++ = extract_v8acc32(acc, 1);
    *pOut++ = extract_v8acc32(acc, 2);
    *pOut   = extract_v8acc32(acc, 3);
    ptr = (v8int32 __aie_dm_resource_c *) pOut;
}
#ifndef __X86SIM__
inline void store_full(v8int32 __aie_dm_resource_d * RSTRCT & ptr, v32acc32 acc) {
    v8acc32 __aie_dm_resource_d * pOut = (v8acc32 __aie_dm_resource_d *) ptr;
    *pOut++ = extract_v8acc32(acc, 0);
    *pOut++ = extract_v8acc32(acc, 1);
    *pOut++ = extract_v8acc32(acc, 2);
    *pOut   = extract_v8acc32(acc, 3);
    ptr = (v8int32 __aie_dm_resource_d *) pOut;
}
#endif



inline int ceil( int val, int significance ) {
    int mask = significance - 1;
    return ( val + mask ) & ~mask;
}

inline int div_ceil( int val, int div ) {
    int mask = div - 1;
    return ( val + mask ) / div;
}

inline int div_ceil_p2( int val, int div ) {
    int mask = div - 1;
    return ( val + mask ) >> ( 31 - clb( div ));
}


inline int add_dimension( int &reset, int num, int incr ) {
    int mod = reset + incr;
    reset -= num * incr;
    return mod;
}

#endif

