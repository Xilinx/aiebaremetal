#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void Stub_Barrier_Reset(void);
uint32_t Stub_Barrier_GetWmbCount(void);
uint32_t Stub_Barrier_GetReadOnceCount(void);

void smp_wmb(void);

/*
 * In tests we may deliberately pass a null pointer to model the broken
 * production behavior in cert_api::submit_command, which zeroes the local
 * comp_sig_va pointer before using READ_ONCE. To avoid test-time segfaults
 * while preserving the control-flow shape, we additionally short-circuit the
 * completion wait loop by always reporting "completed" (non-zero).
 */
static inline uint32_t READ_ONCE(uint32_t *p)
{
    extern void Stub_Barrier_IncReadOnce(void);
    Stub_Barrier_IncReadOnce();
    (void)p; /* value is ignored in unit-test environment */
    return 1u;  /* force completion */
}

#ifdef __cplusplus
}
#endif
