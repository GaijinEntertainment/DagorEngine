//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#ifndef __cplusplus
#include <stdlib.h>
#include <math.h>
#else
#include <cstdlib>
#include <cmath>
#endif

// Workaround the VS bug. It incorrectly uses double precision x87 fmul instruction for expf.
#if defined(_MSC_VER) && !defined(__clang__) && defined(_M_IX86_FP) && _M_IX86_FP == 0
#define DAGOR_EXPF_NOINLINE
#undef expf
#define expf expf_noinline
__declspec(noinline) float expf_noinline(float arg);
#endif

#if !_TARGET_PC_LINUX
static inline float __fabsf(float f) { return fabsf(f); }
#endif

#if _TARGET_C1 | _TARGET_C2























#endif
