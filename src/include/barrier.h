// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#pragma once

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* Prevent compiler from re-ordering instructions */
#define barrier() __asm__ __volatile__("" : : : "memory")

template <typename T> inline T READ_ONCE(T &var) { return *reinterpret_cast<volatile T *>(&var); }

template <typename T> inline T READ_ONCE(T *var) { return *reinterpret_cast<volatile T *>(var); }

template <typename T> inline void WRITE_ONCE(T &x, T val) { *reinterpret_cast<volatile T *>(&x) = val; }

template <typename T> inline void WRITE_ONCE(T *x, T val) { *reinterpret_cast<volatile T *>(x) = val; }

/* Prevent CPU from reordering instructions */
#if defined(__arm__)
#define smp_mb() __asm__ volatile("" ::: "memory")
#define smp_wmb() __asm__ volatile("" ::: "memory")
#define smp_rmb() __asm__ volatile("" ::: "memory")

#define mb() smp_mb()
#define wmb() smp_wmb()
#define rmb() smp_rmb()

#elif defined(__aarch64__)
#define smp_mb() asm volatile("dmb ish" ::: "memory")
#define smp_wmb() asm volatile("dmb ishst" ::: "memory")
#define smp_rmb() asm volatile("dmb ishld" ::: "memory")

#define mb() smp_mb()
#define wmb() smp_wmb()
#define rmb() smp_rmb()

#elif defined(__i386__)
#define mb() asm volatile("lock; addl $0,0(%%esp)" ::: "memory")
#define rmb() asm volatile("lock; addl $0,0(%%esp)" ::: "memory")
#define wmb() asm volatile("lock; addl $0,0(%%esp)" ::: "memory")

#elif defined(__x86_64__)

#define mb() asm volatile("mfence" ::: "memory")
#define rmb() asm volatile("lfence" ::: "memory")
#define wmb() asm volatile("sfence" ::: "memory")

#define smp_rmb() barrier()
#define smp_wmb() barrier()
#define smp_mb() asm volatile("lock; addl $0,-132(%%rsp)" ::: "memory", "cc")

#define smp_store_release(p, v)                                                                                        \
    do {                                                                                                               \
        barrier();                                                                                                     \
        WRITE_ONCE(*p, v);                                                                                             \
    } while (0)

#define smp_load_acquire(p)                                                                                            \
    ({                                                                                                                 \
        typeof(*p) ___p1 = READ_ONCE(*p);                                                                              \
        barrier();                                                                                                     \
        ___p1;                                                                                                         \
    })

#endif
