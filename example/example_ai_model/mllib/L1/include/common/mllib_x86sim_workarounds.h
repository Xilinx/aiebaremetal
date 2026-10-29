// Copyright (C) 2025 Advanced Micro Devices, Inc.
#pragma once
#ifndef __X86SIM__
#define FLOAT_TO_BFLOAT16(X) X
#else
// CR-1185383
static VBit<16, false> constexpr floatToBfloat16(float value) noexcept {
  uint32_t hexValue = __builtin_bit_cast(uint32_t, value);
  uint32_t lsb = (hexValue >> 16) & 1;
  hexValue += 0x7fff + lsb;
  return VBit<16, false>(hexValue >> 16) ;
}
#define FLOAT_TO_BFLOAT16(X) floatToBfloat16(X)
#endif
