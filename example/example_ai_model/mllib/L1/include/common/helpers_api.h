/*
    Copyright (C) 2019 - 2022 Xilinx, Inc. All rights reserved.
    Copyright (C) 2025 Advanced Micro Devices, Inc.

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

#ifndef __DPU_HELPERS_H__
#define __DPU_HELPERS_H__

#if __AIE_ARCH__ == 20
#define STORAGE_BIAS_V chess_storage(x9)
#define STORAGE_BIAS_A0 chess_storage(cm8)
#define STORAGE_BIAS_A1 chess_storage(cm7)
#define STORAGE_REG_X0 chess_storage(x0)
#define STORAGE_REG_X1 chess_storage(x1)
#define STORAGE_REG_X2 chess_storage(x2)
#define STORAGE_REG_X3 chess_storage(x3)
#define STORAGE_REG_X4 chess_storage(x4)
#define STORAGE_REG_X5 chess_storage(x5)
#define STORAGE_REG_X6 chess_storage(x6)
#define STORAGE_MOD_D7 chess_storage(dc7)
#define STORAGE_CNT_R1 chess_storage(r7)
#define STORAGE_MOD_DJ7 chess_storage(dj7)
#else
#define STORAGE_BIAS_V
#define STORAGE_BIAS_A0
#define STORAGE_BIAS_A1
#define STORAGE_REG_X0 chess_storage(x0)
#define STORAGE_REG_X1 chess_storage(x1)
#define STORAGE_REG_X2 chess_storage(x2)
#define STORAGE_REG_X3 chess_storage(x3)
#define STORAGE_REG_X4 chess_storage(x4)
#define STORAGE_REG_X5 chess_storage(x5)
#define STORAGE_REG_X6 chess_storage(x6)
#define STORAGE_MOD_D7 chess_storage(dc7)
#define STORAGE_CNT_R1 chess_storage(r1)
#define STORAGE_MOD_DJ7 chess_storage(dj7)
#endif
#define STORAGE_MOD_D1 chess_storage(dc1)
#define STORAGE_MOD_D5 chess_storage(dc5)
#define STORAGE_MOD_D2 chess_storage(dc2)
#define STORAGE_MOD_D6 chess_storage(dc6)
#define STORAGE_MOD_D0 chess_storage(dc0)
#define STORAGE_MOD_D4 chess_storage(dc4)
#define STORAGE_MOD_D3 chess_storage(dc3)
#define STORAGE_MOD_D7 chess_storage(dc7)
#define STORAGE_CNT_R1 chess_storage(r1)
#define STORAGE_MOD_DJ7 chess_storage(dj7)

struct acc_struct property(keep_in_registers) {
  aie::accum<acc32, 64> a0;
  aie::accum<acc32, 64> a1;
};

// struct acc_struct_16 property(keep_in_registers) {
//   aie::accum<acc64, 32> a0;
//   aie::accum<acc64, 32> a1;
// };

struct acc_struct_16_final property(keep_in_registers) {
  aie::accum<acc32, 32> a0;
  aie::accum<acc32, 32> a1;
};

template <int ld> struct unalign_buffer {
  int frac = 0;
  aie::vector<int8, 64> Xbuff;
  ALWAYS_INLINE unalign_buffer() {}
  ALWAYS_INLINE void init(int8 __aie_dm_resource_a *&pIn) {
    int chess_storage(r5) pdel = delay5((long)pIn);
    int chess_storage(r5) frac_r = (pdel & 31);
    frac_r += 33;
    frac = frac_r;
  };
  ALWAYS_INLINE void init(int incr) { frac = incr; };
  ALWAYS_INLINE void init(){};
};

#if __AIE_ARCH__ >= 21
template <> struct unalign_buffer<1> {
  fifo_state_t fA;
  ALWAYS_INLINE unalign_buffer() { fA.pos = 0; }
  ALWAYS_INLINE void init() { fA.pos = 0; };
  ALWAYS_INLINE void init(int8 __aie_dm_resource_a *&pIn){};
  ALWAYS_INLINE void init(int incr){};
};
#endif

ALWAYS_INLINE void load_accs_aie2p(int32 __aie_dm_resource_c *restrict &pTdm32a,
                                   int32 __aie_dm_resource_d *restrict &pTdm32b,
                                   acc_struct &load_acc, const int incrT0) {
  aie::accum<acc32, 32> help_acc[4];

  aie::vector<int32, 16> tdm_load0 =
      aie::load_v<16, aie_dm_resource::c>(pTdm32a);
  pTdm32a += 16;
  aie::vector<int32, 16> tdm_load1 =
      aie::load_v<16, aie_dm_resource::c>(pTdm32a);
  pTdm32a += 8;
  pTdm32a += 8 * (4 * incrT0 - 3);
  aie::vector<int32, 16> tdm_load2 =
      aie::load_v<16, aie_dm_resource::c>(pTdm32a);
  pTdm32a += 16;
  aie::vector<int32, 16> tdm_load3 =
      aie::load_v<16, aie_dm_resource::c>(pTdm32a);
  pTdm32a += 16;
  help_acc[0].from_vector(aie::concat(tdm_load0, tdm_load1), 0);
  help_acc[1].from_vector(aie::concat(tdm_load2, tdm_load3), 0);
  load_acc.a0 = aie::concat(help_acc[0], help_acc[1]);

  tdm_load0 = aie::load_v<16, aie_dm_resource::d>(pTdm32b);
  pTdm32b += 16;
  tdm_load1 = aie::load_v<16, aie_dm_resource::d>(pTdm32b);
  pTdm32b += 8;
  pTdm32b += 8 * (4 * incrT0 - 3);
  tdm_load2 = aie::load_v<16, aie_dm_resource::d>(pTdm32b);
  pTdm32b += 16;
  tdm_load3 = aie::load_v<16, aie_dm_resource::d>(pTdm32b);
  pTdm32b += 16;
  help_acc[2].from_vector(aie::concat(tdm_load0, tdm_load1), 0);
  help_acc[3].from_vector(aie::concat(tdm_load2, tdm_load3), 0);
  load_acc.a1 = aie::concat(help_acc[2], help_acc[3]);
}

ALWAYS_INLINE void load_accs_aie2(int32 __aie_dm_resource_c *restrict &pTdm32a,
                                  int32 __aie_dm_resource_d *restrict &pTdm32b,
                                  acc_struct &load_acc, const int incrT0) {
  aie::accum<acc32, 32> mac_acc0;
  aie::accum<acc32, 32> mac_acc1;
  aie::accum<acc32, 32> mac_acc2;
  aie::accum<acc32, 32> mac_acc3;
  aie::accum<acc32, 8> help_acc;

  help_acc.from_vector(aie::load_v<8, aie_dm_resource::c>((int32 *)pTdm32a), 0);
  pTdm32a += 8;
  mac_acc0.insert(0, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::c>((int32 *)pTdm32a), 0);
  pTdm32a += 8;
  mac_acc0.insert(1, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::c>((int32 *)pTdm32a), 0);
  pTdm32a += 8;
  mac_acc0.insert(2, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::c>((int32 *)pTdm32a), 0);
  pTdm32a += 8 * (4 * incrT0 - 3);
  mac_acc0.insert(3, help_acc);

  help_acc.from_vector(aie::load_v<8, aie_dm_resource::c>((int32 *)pTdm32a), 0);
  pTdm32a += 8;
  mac_acc1.insert(0, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::c>((int32 *)pTdm32a), 0);
  pTdm32a += 8;
  mac_acc1.insert(1, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::c>((int32 *)pTdm32a), 0);
  pTdm32a += 8;
  mac_acc1.insert(2, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::c>((int32 *)pTdm32a), 0);
  pTdm32a += 8;
  mac_acc1.insert(3, help_acc);

  help_acc.from_vector(aie::load_v<8, aie_dm_resource::d>((int32 *)pTdm32b), 0);
  pTdm32b += 8;
  mac_acc2.insert(0, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::d>((int32 *)pTdm32b), 0);
  pTdm32b += 8;
  mac_acc2.insert(1, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::d>((int32 *)pTdm32b), 0);
  pTdm32b += 8;
  mac_acc2.insert(2, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::d>((int32 *)pTdm32b), 0);
  pTdm32b += 8 * (4 * incrT0 - 3);
  mac_acc2.insert(3, help_acc);

  help_acc.from_vector(aie::load_v<8, aie_dm_resource::d>((int32 *)pTdm32b), 0);
  pTdm32b += 8;
  mac_acc3.insert(0, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::d>((int32 *)pTdm32b), 0);
  pTdm32b += 8;
  mac_acc3.insert(1, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::d>((int32 *)pTdm32b), 0);
  pTdm32b += 8;
  mac_acc3.insert(2, help_acc);
  help_acc.from_vector(aie::load_v<8, aie_dm_resource::d>((int32 *)pTdm32b), 0);
  pTdm32b += 8;
  mac_acc3.insert(3, help_acc);

  load_acc.a0.insert(0, mac_acc0);
  load_acc.a0.insert(1, mac_acc1);
  load_acc.a1.insert(0, mac_acc2);
  load_acc.a1.insert(1, mac_acc3);
}

ALWAYS_INLINE void load_accs_aie2p_16(int32 __aie_dm_resource_c *restrict &pTdm32a,
                                   int32 __aie_dm_resource_d *restrict &pTdm32b,
                                   acc_struct_16_final &load_acc, const int incrT0) {
  aie::accum<acc32, 16> tdm_load0(*(v16acc32*)pTdm32a);
  pTdm32a += 8;
  pTdm32a += 8 * (2*incrT0 - 1);
  aie::accum<acc32, 16> tdm_load1(*(v16acc32*)pTdm32a);
  pTdm32a += 16;
  load_acc.a0 = aie::concat(tdm_load0, tdm_load1);

  tdm_load0 = (*(v16acc32*)pTdm32b);
  pTdm32b += 8;
  pTdm32b += 8 * (2*incrT0 - 1);
  tdm_load1 = (*(v16acc32*)pTdm32b);
  pTdm32b += 16;
  load_acc.a1 = aie::concat(tdm_load0, tdm_load1);
}


template <int ld = 0, typename T_psum = int32, typename T_acctype = acc_struct>
ALWAYS_INLINE void load_accs(T_psum __aie_dm_resource_c *restrict &pTdm32a,
                             T_psum __aie_dm_resource_d *restrict &pTdm32b,
                             T_acctype &load_acc, const int incrT0) {
#if __AIE_ARCH__ >= 21
  if constexpr (ld == 1) {
    if constexpr (std::is_same_v<T_acctype,acc_struct>){
      load_accs_aie2p(pTdm32a, pTdm32b, load_acc, incrT0);
    }
    else{
      load_accs_aie2p_16(pTdm32a, pTdm32b, load_acc, incrT0);
    }
  } else {
#endif
    load_accs_aie2(pTdm32a, pTdm32b, load_acc, incrT0);
#if __AIE_ARCH__ >= 21
  }
#endif
}

ALWAYS_INLINE void
store_accs_aie2p(int32 __aie_dm_resource_c *restrict &pOut32a,
                 int32 __aie_dm_resource_d *restrict &pOut32b,
                 acc_struct &store_acc, const int incrT0) {
  aie::store_v(pOut32a,
               store_acc.a0.extract<16>(0).template to_vector<int32_t>(0));
  pOut32a += 16;
  aie::store_v(pOut32a,
               store_acc.a0.extract<16>(1).template to_vector<int32_t>(0));
  pOut32a += 8;
  pOut32a += 8 * (4 * incrT0 - 3);
  aie::store_v(pOut32a,
               store_acc.a0.extract<16>(2).template to_vector<int32_t>(0));
  pOut32a += 16;
  aie::store_v(pOut32a,
               store_acc.a0.extract<16>(3).template to_vector<int32_t>(0));
  pOut32a += 16;
  aie::store_v(pOut32b,
               store_acc.a1.extract<16>(0).template to_vector<int32_t>(0));
  pOut32b += 16;
  aie::store_v(pOut32b,
               store_acc.a1.extract<16>(1).template to_vector<int32_t>(0));
  pOut32b += 8;
  pOut32b += 8 * (4 * incrT0 - 3);
  aie::store_v(pOut32b,
               store_acc.a1.extract<16>(2).template to_vector<int32_t>(0));
  pOut32b += 16;
  aie::store_v(pOut32b,
               store_acc.a1.extract<16>(3).template to_vector<int32_t>(0));
  pOut32b += 16;
}

ALWAYS_INLINE void store_accs_aie2(int32 __aie_dm_resource_c *restrict &pOut32a,
                                   int32 __aie_dm_resource_d *restrict &pOut32b,
                                   acc_struct &store_acc, const int incrT0) {
  Accs4_i32 accs;
  accs.a0 = store_acc.a0.extract<32>(0);
  accs.a1 = store_acc.a0.extract<32>(1);
  accs.a2 = store_acc.a1.extract<32>(0);
  accs.a3 = store_acc.a1.extract<32>(1);

  store_full((v8int32 __aie_dm_resource_c *&)pOut32a, accs.a0);
  pOut32a += 8 * (4 * incrT0 - 3);
  store_full((v8int32 __aie_dm_resource_c *&)pOut32a, accs.a1);
  pOut32a += 8;
  store_full((v8int32 __aie_dm_resource_d *&)pOut32b, accs.a2);
  pOut32b += 8 * (4 * incrT0 - 3);
  store_full((v8int32 __aie_dm_resource_d *&)pOut32b, accs.a3);
  pOut32b += 8;
}
ALWAYS_INLINE void
store_accs_aie2p_16(int32 __aie_dm_resource_c *restrict &pOut32a,
                 int32 __aie_dm_resource_d *restrict &pOut32b,
                 acc_struct_16_final &store_acc, const int incrT0) {
  v16acc32* pout_32 = (v16acc32*)pOut32a;
  *pout_32 = store_acc.a0.extract<16>(0);
  // pOut32a += 16;
  pOut32a += 8;
  pOut32a += 8 * (2 * incrT0 - 1);
  pout_32 = (v16acc32*)pOut32a;
  *pout_32 = store_acc.a0.extract<16>(1);
  pOut32a += 16;
  // pOut32a += 8 * (4 * incrT0 - 3);

  pout_32 = (v16acc32*)pOut32b;
  *pout_32 = store_acc.a1.extract<16>(0);
  pOut32b += 8;
  pOut32b += 8 * (2 * incrT0 - 1);
  pout_32 = (v16acc32*)pOut32b;
  *pout_32 = store_acc.a1.extract<16>(1);
  pOut32b += 16;

}

template <int ld = 0, typename T_psum = int32, typename T_acctype = acc_struct>
ALWAYS_INLINE void store_accs(T_psum __aie_dm_resource_c *restrict &pOut32a,
                              T_psum __aie_dm_resource_d *restrict &pOut32b,
                              T_acctype &store_acc, const int incrT0) {
#if __AIE_ARCH__ >= 21
  if constexpr (ld == 1) {
    if constexpr (std::is_same_v<T_acctype,acc_struct>){
      store_accs_aie2p(pOut32a, pOut32b, store_acc, incrT0);
    }
    else{
      store_accs_aie2p_16(pOut32a, pOut32b, store_acc, incrT0);
    }
  } else {
#endif
    store_accs_aie2(pOut32a, pOut32b, store_acc, incrT0);
#if __AIE_ARCH__ >= 21
  }
#endif
}


// only compiles on aie2p, need for refactoring
#if __AIE_ARCH__ >= 21
template <typename T=int8>
ALWAYS_INLINE void load_unaligned_aie2p(T __aie_dm_resource_a *&pIn,
                                        unalign_buffer<1> &fA,
                                        const conv3d_params_t &params,
                                        addr_t &cntAL1, addr_t &cntAL2,
                                        addr_t &cntB,
                                        aie::vector<T, 64> &output_vector) {
  using vrr_type = std::conditional_t<std::is_same_v<T, int8>, v64int8, v64uint8>;
  aie::vector<T, 64> sbuff_s;
  sbuff_s = fifo_ld_popx((vrr_type __aie_dm_resource_a *&)pIn, fA.fA,
                         params.step_align, 63);
  pIn = add_3d_byte(pIn, params.incAL3 - 32, params.numAL1, cntAL1,
                    params.incAL1 - 32, params.numAL2, cntAL2,
                    params.incAL2 - 32);
  output_vector = aie::interleave_unzip(sbuff_s, sbuff_s, params.shfl).first;
}
ALWAYS_INLINE void load_unaligned_aie2p_s3(int8 __aie_dm_resource_a *&pIn,
                                        unalign_buffer<1> &fA,
                                        const conv3d_params_t &params,
                                        addr_t &cntAL1, addr_t &cntAL2,
                                        addr_t &cntB,
                                        aie::vector<int8, 64> &output_vector) {
  aie::vector<int8, 16>  inbuf0;
  aie::vector<int8, 16>  inbuf1;
  aie::vector<int8, 16> res0, res1;
  auto m1 = ::aie::mask<16>::from_uint32(0b0000000011111111);
  auto m2 = ::aie::mask<16>::from_uint32(0b1111111100000000);
  aie::vector<int8, 32>  xbuf_temp;

  inbuf0 = aie::load_unaligned_v<16>(pIn); pIn += 16;	// w0(c0-c7),w1(c0-c7)
  inbuf1 = aie::load_unaligned_v<16>(pIn); pIn += 32; // w2(c0-c7),w3(c0-c7)
  res0 = aie::select(inbuf1, inbuf0, m1); 		// w0,w3
  inbuf0 = aie::load_unaligned_v<16>(pIn);pIn += 16;  // w6(c0-c7),w7(c0-c7)
  inbuf1 = aie::load_unaligned_v<16>(pIn);pIn += 32; 	// w8(c0-c7),w9(c0-c7)
  res1 = aie::select(inbuf0, inbuf1, m2);     	// w6, w9

  xbuf_temp = aie::concat(res0, res1);		//w0,w3,w6,w9
  output_vector=insert(output_vector, 0, xbuf_temp);
  output_vector=insert(output_vector, 1, xbuf_temp);

     pIn = add_3d_byte(pIn, params.incAL3 , params.numAL1, cntAL1,
                    params.incAL1 , params.numAL2, cntAL2,
                    params.incAL2);

}

#endif

template <int ld = 0>
ALWAYS_INLINE void load_unaligned_aie2(int8 __aie_dm_resource_a *&pIn,
                                       unalign_buffer<ld> &fA,
                                       const conv3d_params_t &params,
                                       addr_t &cntAL1, addr_t &cntAL2,
                                       addr_t &cntB,
                                       aie::vector<int8, 64> &output_vector) {

  aie::vector<int8, 64> sbuff_s;
  sbuff_s.insert(0, aie::load_v<32, aie_dm_resource::a>(pIn));
  pIn = byte_incr(pIn, params.incAI1);
  sbuff_s.insert(1, aie::load_v<32, aie_dm_resource::a>(pIn));
  fA.Xbuff = shiftx(fA.Xbuff, sbuff_s, params.step_align, fA.frac);
  //output_vector = shuffle(fA.Xbuff, fA.Xbuff, params.shfl);
  output_vector = aie::interleave_unzip(fA.Xbuff, fA.Xbuff, params.shfl).first;
  pIn = add_3d_byte(pIn, params.incAL3, params.numAL1, cntAL1, params.incAL1,
                    params.numAL2, cntAL2, params.incAL2);

  int pdel = delay5((long)pIn);
  int frac_r = (pdel & 31);
  frac_r += 33;
  fA.frac = frac_r;
}

template <int ld = 0>
ALWAYS_INLINE void
load_unaligned_aie2_bias(int8 __aie_dm_resource_a *&pIn, unalign_buffer<ld> &fA,
                         const conv3d_params_t &params, addr_t &cntAL1,
                         addr_t &cntAL2, addr_t &cntB,
                         aie::vector<int8, 64> &output_vector) {

  aie::vector<int8, 64> sbuff_s;
  sbuff_s.insert(0, aie::load_v<32, aie_dm_resource::a>(pIn));
  pIn = byte_incr(pIn, params.incAI1);
  sbuff_s.insert(1, aie::load_v<32, aie_dm_resource::a>(pIn));
  fA.Xbuff = sbuff_s;
  //output_vector = shuffle(sbuff_s, sbuff_s, params.shfl);
  output_vector = aie::interleave_unzip(sbuff_s, sbuff_s, params.shfl).first;
  
  pIn = add_3d_byte(pIn, params.incAL3, params.numAL1, cntAL1, params.incAL1,
                    params.numAL2, cntAL2, params.incAL2);
}

#if __AIE_ARCH__ >= 21
template <typename T=int16>
ALWAYS_INLINE void load_unaligned_aie2p_16(T __aie_dm_resource_a *&pIn,
                                        unalign_buffer<1> &fA,
                                        const conv3d_params_t &params,
                                        addr_t &cntAL1, addr_t &cntAL2,
                                        addr_t &cntB,
                                        aie::vector<T, 32> &output_vector) {
  using vrr_type = std::conditional_t<std::is_same_v<T, int16>, v32int16, v32uint16>;
  aie::vector<T, 32> sbuff_s;
  vrr_type * tt = (vrr_type*) pIn;
  sbuff_s = fifo_ld_popx((vrr_type __aie_dm_resource_a *&)pIn, fA.fA,
                         params.step_align, 63);
  pIn = add_3d_byte(pIn, params.incAL3 - 32, params.numAL1, cntAL1,
                    params.incAL1 - 32, params.numAL2, cntAL2,
                    params.incAL2 - 32);
  output_vector = aie::interleave_unzip(sbuff_s, sbuff_s, params.shfl).first;
}
#endif

template <int ld = 0, bool bias = false, typename T = int8, typename T_vec = aie::vector<int8, 64>>
ALWAYS_INLINE void
load_unaligned(T __aie_dm_resource_a *&pIn, unalign_buffer<ld> &fA,
               const conv3d_params_t &params, addr_t &cntAL1, addr_t &cntAL2,
               addr_t &cntB, T_vec &output_vector) {
#if __AIE_ARCH__ >= 21
  if constexpr (ld == 1) {
    if constexpr (sizeof(T) == 1){
      load_unaligned_aie2p<T>(pIn, fA, params, cntAL1, cntAL2, cntB, output_vector);
    }
    else{
      load_unaligned_aie2p_16<T>(pIn, fA, params, cntAL1, cntAL2, cntB, output_vector);
    }
  } else {
#endif
    if constexpr (bias == 1) {
      load_unaligned_aie2_bias(pIn, fA, params, cntAL1, cntAL2, cntB,
                               output_vector);
    } else {
      load_unaligned_aie2(pIn, fA, params, cntAL1, cntAL2, cntB, output_vector);
    }
#if __AIE_ARCH__ >= 21
  }
#endif
}
template <int ld = 0, bool bias = false>
ALWAYS_INLINE void
load_unaligned_s3(int8 __aie_dm_resource_a *&pIn, unalign_buffer<ld> &fA,
               const conv3d_params_t &params, addr_t &cntAL1, addr_t &cntAL2,
               addr_t &cntB, aie::vector<int8, 64> &output_vector) {
#if __AIE_ARCH__ >= 21
  if constexpr (ld == 1) {
    load_unaligned_aie2p_s3(pIn, fA, params, cntAL1, cntAL2, cntB, output_vector);
  } else {
#endif
    if constexpr (bias == 1) {
      load_unaligned_aie2_bias(pIn, fA, params, cntAL1, cntAL2, cntB,
                               output_vector);
    } else {
      load_unaligned_aie2(pIn, fA, params, cntAL1, cntAL2, cntB, output_vector);
    }
#if __AIE_ARCH__ >= 21
  }
#endif
}

ALWAYS_INLINE void load_weights_aie2p(int8 __aie_dm_resource_b *&pW,
                                      aie::vector<int8, 64> &output_vector) {
  output_vector = aie::load_v<64, aie_dm_resource::b>(pW);
  pW += 64;
}

ALWAYS_INLINE void load_weights_aie2(int8 __aie_dm_resource_b *&pW,
                                     aie::vector<int8, 64> &output_vector) {
  output_vector.insert(0, aie::load_v<32, aie_dm_resource::b>(pW));
  pW += 32;
  output_vector.insert(1, aie::load_v<32, aie_dm_resource::b>(pW));
  pW += 32;
}

template <int ld = 0>
ALWAYS_INLINE void load_weights(int8 __aie_dm_resource_b *&pW,
                                aie::vector<int8, 64> &output_vector) {
#if __AIE_ARCH__ >= 21
  if constexpr (ld == 1) {
    load_weights_aie2p(pW, output_vector);
  } else {
#endif
    load_weights_aie2(pW, output_vector);
#if __AIE_ARCH__ >= 21
  }
#endif
}

template <int ld = 0>
ALWAYS_INLINE void setup_pin(int8 __aie_dm_resource_a *&pIn) {
  pIn = byte_incr(pIn, 31);
}

#if __AIE_ARCH__ >= 21
template <> ALWAYS_INLINE void setup_pin<1>(int8 __aie_dm_resource_a *&pIn) {}
#endif

#endif //__BIASED_CONV_INT8X8_LD2_KERNEL_H__
