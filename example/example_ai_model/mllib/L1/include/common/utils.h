/*
    Copyright (C) 2014 - 2022 Xilinx, Inc. All rights reserved.
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


#ifndef __UTILS_H__
#define __UTILS_H__

#include <adf.h>
#include "aie.hpp"

//MLLib defines that are now changed to default
#define FC_EN
#define USE_RTP_ACT
#define USE_RTP_SIGN
#ifndef USE_OPMODE_1
#define USE_OPMODE_1 1
#endif
#ifndef RELU_INT8_ENABLE
#define RELU_INT8_ENABLE 0
#endif
#ifndef ELTWISE_ACT
#define ELTWISE_ACT RELU
#endif

#define ADD_3D_BYTE add_3d_byte
#define ADD_2D_BYTE add_2d_byte
#define interleave shuffle


#define INLINE inline __attribute__((always_inline))
#define NOINLINE __attribute__ ((noinline))

#define uns4  undef_v128int4()
#define uns8  undef_v64int8()
#define uns16 undef_v32int16()
#define uns32 undef_v16int32()

#define unu4  undef_v128uint4()
#define unu8  undef_v64uint8()
#define unu16 undef_v32uint16()
#define unu32 undef_v16uint32()


#ifndef IN_CASC
#define IN_CASC input_cascade
#endif
#ifndef OUT_CASC
#define OUT_CASC output_cascade
#endif

#define DM_BANK(a) __aie_dm_resource_##a

#if defined(__chess__)
#define AIE_LOOP_UNROLL(x) [[chess::unroll_loop(x)]]
#define AIE_LOOP_UNROLL_FULL [[chess::unroll_loop()]]
#define AIE_LOOP_NO_UNROLL [[chess::no_unroll]]
#define AIE_LOOP_MIN_ITERATION_COUNT(x) [[chess::min_loop_count(x)]]
#define AIE_LOOP_MAX_ITERATION_COUNT(x) [[chess::max_loop_count(x)]]
#define AIE_LOOP_RANGE(a,...) [[chess::min_loop_count(a)]]__VA_OPT__([[chess::max_loop_count(__VA_ARGS__)]])
#define AIE_PREPARE_FOR_PIPELINING [[chess::prepare_for_pipelining]]
#define AIE_NO_PREPARE_FOR_PIPELINING [[chess::no_prepare_for_pipelining]]

#elif defined(__AIECC__)
#ifndef __STRINGIFY
  #define __STRINGIFY(a) #a
#endif
#define AIE_LOOP_UNROLL(x) _Pragma(__STRINGIFY(clang loop unroll_count(x)))
#define AIE_LOOP_UNROLL_FULL _Pragma("clang loop unroll(full)")
#define AIE_LOOP_NO_UNROLL _Pragma("clang loop unroll(disable)")
#define AIE_LOOP_MIN_ITERATION_COUNT(x) _Pragma(__STRINGIFY(clang loop min_iteration_count(x)))
#define AIE_LOOP_MAX_ITERATION_COUNT(x) _Pragma(__STRINGIFY(clang loop max_iteration_count(x)))
#define AIE_LOOP_RANGE(a,...) AIE_LOOP_MIN_ITERATION_COUNT(a)__VA_OPT__(AIE_LOOP_MAX_ITERATION_COUNT(__VA_ARGS__))
#define AIE_PREPARE_FOR_PIPELINING
#define AIE_NO_PREPARE_FOR_PIPELINING 

#else
#define AIE_LOOP_UNROLL(x)
#define AIE_LOOP_UNROLL_FULL
#define AIE_LOOP_NO_UNROLL
#define AIE_LOOP_MIN_ITERATION_COUNT(x)
#define AIE_LOOP_MAX_ITERATION_COUNT(x)
#define AIE_LOOP_RANGE(a,...)
#define AIE_PREPARE_FOR_PIPELINING
#define AIE_NO_PREPARE_FOR_PIPELINING 
#endif

#if __AIE_ARCH__ == 20
struct alignas(v16uint16) Vecs4_ui8 property(keep_in_registers) {
    v64uint8 x0;
    v64uint8 x1;
    v64uint8 x2;
    v64uint8 x3;
};
inline void undef(Vecs4_ui8 &vecs) {
    vecs.x0 = undef_v64uint8();
    vecs.x1 = undef_v64uint8();
    vecs.x2 = undef_v64uint8();
    vecs.x3 = undef_v64uint8();
}

struct alignas(v16uint16) buffer_config
{
   bool bdenq           = false;
   unsigned prodLockId  = 0;
   unsigned consLockId  = 1;
   void* pingBuf;
   void* pongBuf;
   int index            = 0;
   int length           = 0;
   bool enable_pkt      = false;  // no pkt merge
   bool inmode          = true;   //input buffer
};
#endif

#ifdef NON_MULTILAYER
inline size_t mllib_min(size_t a, size_t b) { return (a < b ? a : b);}
inline unsigned mllib_abs(int a) {return (a < 0 ? -a : a);}

constexpr unsigned Lock_Offset_For_Core = 48;
constexpr unsigned DM_Offset_For_Core = 0x70000;

#if __AIE_ARCH__ == 20
inline void config_bd_enq(void* inout, buffer_config &buf_cfg, int num_iter) {
    const adf::core_dma_config* core_dma_config_ptr = adf::get_core_dma_config(inout);
    buf_cfg.prodLockId = core_dma_config_ptr->locks[0];
    buf_cfg.consLockId = core_dma_config_ptr->locks[1];
    short ch = core_dma_config_ptr->channel, startBD = core_dma_config_ptr->bd_ids[0], pktID = core_dma_config_ptr->pkt_id, outOfOrderBD = core_dma_config_ptr->out_of_order_bd;
    size_t pingBufAddr = core_dma_config_ptr->buffer_offsets[0];
    size_t pongBufAddr = core_dma_config_ptr->buffer_offsets[1];
    buf_cfg.pingBuf = (void*)(DM_Offset_For_Core + pingBufAddr);
    buf_cfg.pongBuf = (void*)(DM_Offset_For_Core + pongBufAddr);

    struct adf::buffer_descriptor bd =
    {
        .address = (uint32)mllib_min(pingBufAddr, pongBufAddr)/4, // in 32-bit words
        .length = (uint32)(buf_cfg.length>>2), // in 32-bit words
        .enable_packet = buf_cfg.enable_pkt,
        .out_of_order_bd = (uint32)(buf_cfg.inmode ? 0 : outOfOrderBD),
        .packet_id = (uint32)(buf_cfg.inmode ? 0 : pktID),
        .stepsize = {1, 1, 1},
        .wrap = {0, 0},
        .iteration_current = (uint32)((pongBufAddr > pingBufAddr) ? 0 : 1),
        .iteration_wrap = 2,
        .iteration_stepsize = mllib_abs(pongBufAddr-pingBufAddr)/4, // in 32-bit words
        .lock_acq_enable = true,
        .lock_acq_value = -1,
        .lock_acq_id = (buf_cfg.inmode ? buf_cfg.prodLockId : buf_cfg.consLockId),
        .lock_rel_value = 1,
        .lock_rel_id = (buf_cfg.inmode ? buf_cfg.consLockId : buf_cfg.prodLockId)
    };

    if(!buf_cfg.bdenq){
        adf::initialize_lock(buf_cfg.prodLockId, 2); // initialize producer lock with num buffers (2 here)
        adf::enqueue_task(ch, startBD, num_iter, false, bd);
    }
}
#endif
#endif

using namespace aie;

static NOINLINE void relu_int8_post_process(
    int * __restrict input,
    int in_sign,
    int ofm_len
){
    int DM_BANK(a) *input_data  = (int DM_BANK(a) *) input;
    int DM_BANK(a) *output_data = (int DM_BANK(a) *) input;

    //8*32 = 256
    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_MIN_ITERATION_COUNT(2)
    for (uint16_t i=0; i<ofm_len; i=i+256) {

        vector<uint8_t,32> vec_0 = load_v<32>((uint8_t DM_BANK(a) *)input_data);
        accum<acc32,32> accum_0;
        accum_0.template from_vector_sign<uint8_t>(vec_0, in_sign, 0);
        input_data = byte_incr(input_data, 32);

        vector<uint8_t,32> vec_1 = load_v<32>((uint8_t DM_BANK(a) *)input_data);
        accum<acc32,32> accum_1;
        accum_1.template from_vector_sign<uint8_t>(vec_1, in_sign, 0);
        input_data = byte_incr(input_data, 32);

        vector<uint8_t,32> vec_2 = load_v<32>((uint8_t DM_BANK(a) *)input_data);
        accum<acc32,32> accum_2;
        accum_2.template from_vector_sign<uint8_t>(vec_2, in_sign, 0);
        input_data = byte_incr(input_data, 32);

        vector<uint8_t,32> vec_3 = load_v<32>((uint8_t DM_BANK(a) *)input_data);
        accum<acc32,32> accum_3;
        accum_3.template from_vector_sign<uint8_t>(vec_3, in_sign, 0);
        input_data = byte_incr(input_data, 32);

        vector<uint8_t,32> vec_4 = load_v<32>((uint8_t DM_BANK(a) *)input_data);
        accum<acc32,32> accum_4;
        accum_4.template from_vector_sign<uint8_t>(vec_4, in_sign, 0);
        input_data = byte_incr(input_data, 32);

        vector<uint8_t,32> vec_5 = load_v<32>((uint8_t DM_BANK(a) *)input_data);
        accum<acc32,32> accum_5;
        accum_5.template from_vector_sign<uint8_t>(vec_5, in_sign, 0);
        input_data = byte_incr(input_data, 32);

        vector<uint8_t,32> vec_6 = load_v<32>((uint8_t DM_BANK(a) *)input_data);
        accum<acc32,32> accum_6;
        accum_6.template from_vector_sign<uint8_t>(vec_6, in_sign, 0);
        input_data = byte_incr(input_data, 32);

        vector<uint8_t,32> vec_7 = load_v<32>((uint8_t DM_BANK(a) *)input_data);
        accum<acc32,32> accum_7;
        accum_7.template from_vector_sign<uint8_t>(vec_7, in_sign, 0);
        input_data = byte_incr(input_data, 32);


        store_v((int8_t DM_BANK(a) *)output_data, accum_0.template to_vector<int8_t>(0));
        output_data = byte_incr(output_data, 32);
        store_v((int8_t DM_BANK(a) *)output_data, accum_1.template to_vector<int8_t>(0));
        output_data = byte_incr(output_data, 32);
        store_v((int8_t DM_BANK(a) *)output_data, accum_2.template to_vector<int8_t>(0));
        output_data = byte_incr(output_data, 32);
        store_v((int8_t DM_BANK(a) *)output_data, accum_3.template to_vector<int8_t>(0));
        output_data = byte_incr(output_data, 32);
        store_v((int8_t DM_BANK(a) *)output_data, accum_4.template to_vector<int8_t>(0));
        output_data = byte_incr(output_data, 32);
        store_v((int8_t DM_BANK(a) *)output_data, accum_5.template to_vector<int8_t>(0));
        output_data = byte_incr(output_data, 32);
        store_v((int8_t DM_BANK(a) *)output_data, accum_6.template to_vector<int8_t>(0));
        output_data = byte_incr(output_data, 32);
        store_v((int8_t DM_BANK(a) *)output_data, accum_7.template to_vector<int8_t>(0));
        output_data = byte_incr(output_data, 32);
    }


}
// Glue for 16-bit to 8-bit with activation, in-place
static void act_post_process
(
    int data_len,             // number of elements to be processed, typically h*w*depth*batch_size (not bytes)
    int out_sign,              // dynamic sign/activation for output (0:unsigned or relu, 1: signed or linear)
    int shift_out,             // shift needed when converting from 16-bit to 8-bit
    int16 *__restrict bufin_ptr, // pointer to buffer in L1 memory from where to pick up and write in-place
    int8 *__restrict bufout_ptr // pointer to buffer in L1 memory from where to pick up and write in-place
){
    int16 DM_BANK(a) *__restrict buf_ptr = (int16 DM_BANK(a) * __restrict) bufin_ptr;
    int8 DM_BANK(a) *__restrict out_ptr = (int8 DM_BANK(a) * __restrict) bufout_ptr; // pointer to buffer in L1 memory from where to pick up and write in-place
    accum<acc32,32> acc0,acc1,acc2,acc3 ;
    unsigned int loop_len=data_len>>7; // ->/(4*32) accs unrolled, lanes per acc
    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_MIN_ITERATION_COUNT(4)
    for (unsigned k=0;k<loop_len;k++) {

        acc0.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        acc1.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        acc2.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        acc3.from_vector(load_v<32>(buf_ptr),0); buf_ptr+=32 ;
        store_v(out_ptr, acc0.to_vector_sign<int8>(out_sign,shift_out)); out_ptr += 32;
        store_v(out_ptr, acc1.to_vector_sign<int8>(out_sign,shift_out)); out_ptr += 32;
        store_v(out_ptr, acc2.to_vector_sign<int8>(out_sign,shift_out)); out_ptr += 32;
        store_v(out_ptr, acc3.to_vector_sign<int8>(out_sign,shift_out)); out_ptr += 32;
    }
}

using namespace aie;
#if __AIE_ARCH__ == 20
static NOINLINE void relu_post_process(
    bfloat16 * __restrict input,
    int ofm_len
){
    bfloat16  * __restrict input_data  =  input;
    bfloat16  * __restrict output_data =  input;
    vector<bfloat16, 16> zero_vec  = aie::zeros<bfloat16, 16>();

    uint16_t ofm_length = ofm_len*2;
    //8*32 = 256
    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_MIN_ITERATION_COUNT(4)
    for (uint16_t i=0; i<ofm_length; i=i+1) {

        vector<bfloat16,16> vec_0 = load_v<16>(input_data);
        vector<bfloat16,16> max_vec0 = aie::max(vec_0, zero_vec);
        input_data = byte_incr(input_data, 32);

        vector<bfloat16,16> vec_1 = load_v<16>(input_data);
        vector<bfloat16,16> max_vec1 = aie::max(vec_1, zero_vec);
        input_data = byte_incr(input_data, 32);

        vector<bfloat16,16> vec_2 = load_v<16>(input_data);
        vector<bfloat16,16> max_vec2 = aie::max(vec_2, zero_vec);
        input_data = byte_incr(input_data, 32);

        vector<bfloat16,16> vec_3 = load_v<16>(input_data);
        vector<bfloat16,16> max_vec3 = aie::max(vec_3, zero_vec);
        input_data = byte_incr(input_data, 32);

        vector<bfloat16,16> vec_4 = load_v<16>(input_data);
        vector<bfloat16,16> max_vec4 = aie::max(vec_4, zero_vec);
        input_data = byte_incr(input_data, 32);

        vector<bfloat16,16> vec_5 = load_v<16>(input_data);
        vector<bfloat16,16> max_vec5 = aie::max(vec_5, zero_vec);
        input_data = byte_incr(input_data, 32);

        vector<bfloat16,16> vec_6 = load_v<16>(input_data);
        vector<bfloat16,16> max_vec6 = aie::max(vec_6, zero_vec);
        input_data = byte_incr(input_data, 32);

        vector<bfloat16,16> vec_7 = load_v<16>(input_data);
        vector<bfloat16,16> max_vec7 = aie::max(vec_7, zero_vec);
        input_data = byte_incr(input_data, 32);



        store_v(output_data, max_vec0);
        output_data = byte_incr(output_data, 32);

        store_v(output_data, max_vec1);
        output_data = byte_incr(output_data, 32);

        store_v(output_data, max_vec2);
        output_data = byte_incr(output_data, 32);

        store_v(output_data, max_vec3);
        output_data = byte_incr(output_data, 32);

        store_v(output_data, max_vec4);
        output_data = byte_incr(output_data, 32);

        store_v(output_data, max_vec5);
        output_data = byte_incr(output_data, 32);

        store_v(output_data, max_vec6);
        output_data = byte_incr(output_data, 32);

        store_v(output_data, max_vec7);
        output_data = byte_incr(output_data, 32);

    }
}
#else
static NOINLINE void relu_post_process(
    bfloat16 * __restrict input,
    int ofm_len
){
    bfloat16 DM_BANK(a) * __restrict input_data  = (bfloat16 DM_BANK(a) * __restrict) input;
    bfloat16 DM_BANK(b) * __restrict output_data = (bfloat16 DM_BANK(b) * __restrict) input;
    vector<bfloat16, 32> zero_vec  = aie::zeros<bfloat16, 32>();

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_MIN_ITERATION_COUNT(6)
    for (int32_t i=0; i<ofm_len; i=i+1) {

        vector<bfloat16,32> vec_0 = load_v<32>(input_data);
        vector<bfloat16,32> max_vec0 = aie::max(vec_0, zero_vec);
        input_data = byte_incr(input_data, 64);

        store_v(output_data, max_vec0);
        output_data = byte_incr(output_data, 64);
    }
}
#endif

// Copy data with offset at the output, simple ptr interface
static void copy_data
(
        uint8_t * __restrict ifm_ptr,
        int input_len,
        uint8_t * __restrict ofm_ptr,
        int ofm_offset
){

    uint8_t *input_ptr = ifm_ptr;
    uint8_t *output_ptr = ofm_ptr+ofm_offset;

    uint32_t len = input_len>>5;

    AIE_PREPARE_FOR_PIPELINING
    AIE_LOOP_MIN_ITERATION_COUNT(4)
    for (uint32_t j=0; j< len; j++) {

        vector<uint8_t,32> input_buff;
        input_buff = load_v<32>(input_ptr);
        input_ptr += 32;
        store_v(output_ptr,input_buff);
        output_ptr += 32;
    }
}

#define SETUP_ITR3D_COUNTERS(iterator_name) \
         addr_t iterator_name##_cnt0 = 0;  \
         addr_t iterator_name##_cnt1 = 0;

//Use the macro below to declare a bunch of variables which together act as iterator
#define DECL_ITR3D(iterator_name)\
      int32_t iterator_name##_incr0 ;  \
      int32_t iterator_name##_incr1 ;  \
      uint32_t iterator_name##_wrap0 ;  \
      uint32_t iterator_name##_wrap1 ;  \
      int32_t iterator_name##_incr2 ;

// Iterator setup inside a parent struct referred here as 'param'
#define SETUP_ITR3D_PARAM(iterator_name,incr0,wrap0,incr1,wrap1,incr2,param) \
      param.iterator_name##_incr0 = incr0;  \
      param.iterator_name##_wrap0 = wrap0;  \
      param.iterator_name##_incr1 = incr1;  \
      param.iterator_name##_wrap1 = wrap1;  \
      param.iterator_name##_incr2 = incr2;


// Increment pointer using iterator from a parent struct referred here as 'param'
#define INCR_ITR3D_PARAM(p_in,iterator_name,param)\
    p_in = ADD_3D_BYTE(p_in,param.iterator_name##_incr2,param.iterator_name##_wrap0,iterator_name##_cnt0,param.iterator_name##_incr0,param.iterator_name##_wrap1,iterator_name##_cnt1,param.iterator_name##_incr1);

// Simple iterator setup without using any struct

#define SETUP_ITR3D(iterator_name,incr0,wrap0,incr1,wrap1,incr2) \
      iterator_name##_incr0 = incr0;  \
      iterator_name##_wrap0 = wrap0;  \
      iterator_name##_incr1 = incr1;  \
      iterator_name##_wrap1 = wrap1;  \
      iterator_name##_incr2 = incr2;

// Increment pointer simply without using iterator without struct
#define INCR_ITR3D(p_in,iterator_name)\
    p_in = ADD_3D_BYTE(p_in,iterator_name##_incr2,iterator_name##_wrap0,iterator_name##_cnt0,iterator_name##_incr0,iterator_name##_wrap1,iterator_name##_cnt1,iterator_name##_incr1);

#define SETUP_ITR2D_COUNTERS(iterator_name) \
         addr_t iterator_name##_cnt0 = 0;

//Use the macro below to declare a bunch of variables which together act as iterator
#define DECL_ITR2D(iterator_name)\
      int32_t iterator_name##_incr0 ;  \
      uint32_t iterator_name##_wrap0 ;  \
      int32_t iterator_name##_incr1 ;

// Iterator setup inside a parent struct referred here as 'param'
#define SETUP_ITR2D_PARAM(iterator_name,incr0,wrap0,incr1,param) \
      param.iterator_name##_incr0 = incr0;  \
      param.iterator_name##_wrap0 = wrap0;  \
      param.iterator_name##_incr1 = incr1;


// Increment pointer using iterator from a parent struct referred here as 'param'
#define INCR_ITR2D_PARAM(p_in,iterator_name,param)\
    p_in = ADD_2D_BYTE(p_in,param.iterator_name##_incr1,param.iterator_name##_wrap0,iterator_name##_cnt0,param.iterator_name##_incr0);

// Simple iterator setup without using any struct

#define SETUP_ITR2D(iterator_name,incr0,wrap0,incr1) \
      iterator_name##_incr0 = incr0;  \
      iterator_name##_wrap0 = wrap0;  \
      iterator_name##_incr1 = incr1;

// Increment pointer simply without using iterator without struct
#define INCR_ITR2D(p_in,iterator_name)\
    p_in = ADD_2D_BYTE(p_in,iterator_name##_incr1,iterator_name##_wrap0,iterator_name##_cnt0,iterator_name##_incr0);

namespace mllib::utils {
template <typename T, typename... Types>
struct is_one_of
{
    static constexpr bool value = (... || std::is_same_v<T, Types>);
};

/*
 * Says whether the first time matches any of the rest of given types
 */
template <typename T, typename... Types>
static constexpr bool is_one_of_v = is_one_of<T, Types...>::value;
} // namespace mllib::utils

#endif //__UTILS_H__
