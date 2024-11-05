//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if defined(__e2k__)
#define __cpuid(L, R0, R1, R2, R3)           R0 = R1 = R2 = R3 = 0, (void)L
#define __cpuid_count(L, SL, R0, R1, R2, R3) R0 = R1 = R2 = R3 = 0, (void)L, (void)SL
static inline int __get_cpuid(unsigned l, unsigned *r0, unsigned *r1, unsigned *r2, unsigned *r3)
{
  __cpuid(l, *r0, *r1, *r2, *r3);
  return 1;
}
static inline int __get_cpuid_count(unsigned l, unsigned sl, unsigned *r0, unsigned *r1, unsigned *r2, unsigned *r3)
{
  __cpuid_count(l, sl, *r0, *r1, *r2, *r3);
  return 1;
}
#endif
