// Copyright (C) 2025 - 2026 Advanced Micro Devices, Inc.
// SPDX-License-Identifier: Apache-2.0
#ifndef BARRIER_H
#define BARRIER_H

#include <memory>
#include <type_traits>

namespace mem_barrier {

inline void barrier() noexcept {
    __asm__ __volatile__("" : : : "memory");
}

template <typename T>
inline bool likely(T x) {
    return __builtin_expect(static_cast<bool>(x), 1);
}

template <typename T>
inline bool unlikely(T x) {
    return __builtin_expect(static_cast<bool>(x), 0);
}

template <typename T>
inline typename std::enable_if<!std::is_pointer<T>::value, T>::type READ_ONCE(const T &var) {
    static_assert(std::is_scalar<T>::value, "READ_ONCE supports scalar types only");
    const volatile T *const src = std::addressof(var);
    return *src;
}

template <typename T>
inline T READ_ONCE(const T *const ptr) {
    static_assert(std::is_scalar<T>::value, "READ_ONCE supports scalar types only");
    T value{};
    if (ptr != nullptr) {
        const volatile T *const src = ptr;
        value = *src;
    }
    return value;
}

template <typename T>
inline typename std::enable_if<!std::is_pointer<T>::value>::type WRITE_ONCE(T &var, const T &val) {
    static_assert(std::is_scalar<T>::value, "WRITE_ONCE supports scalar types only");
    volatile T *const dst = std::addressof(var);
    *dst = val;
}

template <typename T>
inline void WRITE_ONCE(T *const ptr, const T &val) {
    static_assert(std::is_scalar<T>::value, "WRITE_ONCE supports scalar types only");
    if (ptr != nullptr) {
        volatile T *const dst = ptr;
        *dst = val;
    }
}

/* Prevent CPU from reordering instructions */
#if defined(__arm__)
inline void smp_mb() { __asm__ volatile("" ::: "memory"); }
inline void smp_wmb() { __asm__ volatile("" ::: "memory"); }
inline void smp_rmb() { __asm__ volatile("" ::: "memory"); }

inline void mb() { smp_mb(); }
inline void wmb() { smp_wmb(); }
inline void rmb() { smp_rmb(); }

#elif defined(__aarch64__)
inline void smp_mb() { asm volatile("dmb ish" ::: "memory"); }
inline void smp_wmb() { asm volatile("dmb ishst" ::: "memory"); }
inline void smp_rmb() { asm volatile("dmb ishld" ::: "memory"); }

inline void mb() { smp_mb(); }
inline void wmb() { smp_wmb(); }
inline void rmb() { smp_rmb(); }

#elif defined(__i386__)
inline void mb() { asm volatile("lock; addl $0,0(%%esp)" ::: "memory"); }
inline void rmb() { asm volatile("lock; addl $0,0(%%esp)" ::: "memory"); }
inline void wmb() { asm volatile("lock; addl $0,0(%%esp)" ::: "memory"); }

inline void smp_mb() { mb(); }
inline void smp_wmb() { wmb(); }
inline void smp_rmb() { rmb(); }

#elif defined(__x86_64__)
inline void mb() { asm volatile("mfence" ::: "memory"); }
inline void rmb() { asm volatile("lfence" ::: "memory"); }
inline void wmb() { asm volatile("sfence" ::: "memory"); }

inline void smp_rmb() { barrier(); }
inline void smp_wmb() { barrier(); }
inline void smp_mb() { asm volatile("lock; addl $0,-132(%%rsp)" ::: "memory", "cc"); }

#endif

template <typename T>
inline void smp_store_release(T *const ptr, const T &val) {
    barrier();
    WRITE_ONCE(ptr, val);
}

template <typename T>
inline T smp_load_acquire(const T *const ptr) {
    const T value = READ_ONCE(ptr);
    barrier();
    return value;
}

} // namespace mem_barrier

#endif // BARRIER_H
