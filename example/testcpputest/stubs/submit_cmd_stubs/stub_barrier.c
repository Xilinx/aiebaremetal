#include "stub_barrier.h"

static uint32_t g_wmb_count = 0;
static uint32_t g_read_once_count = 0;

void Stub_Barrier_Reset(void)
{
    g_wmb_count = 0;
    g_read_once_count = 0;
}

uint32_t Stub_Barrier_GetWmbCount(void)
{
    return g_wmb_count;
}

uint32_t Stub_Barrier_GetReadOnceCount(void)
{
    return g_read_once_count;
}

void Stub_Barrier_IncReadOnce(void)
{
    g_read_once_count++;
}

void smp_wmb(void)
{
    g_wmb_count++;
}
