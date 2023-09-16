//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#if defined(_MSC_VER)
#undef DECLSPEC_ALIGN
#define DECLSPEC_ALIGN(X) __declspec(align(X))
#define ATTRIBUTE_ALIGN(X)
#elif defined(__GNUC__) || defined(__SNC__)
#define DECLSPEC_ALIGN(X)
#define ATTRIBUTE_ALIGN(x) __attribute__((aligned(x)))
#define STRUCT_PAD(x)      char struct__pad[0] __attribute__((aligned(x)))
#define GCC_LIKELY(x)      __builtin_expect(!!(x), 1)
#define GCC_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#if !defined(__SNC__)
#define SNC_LIKELY_TARGET
#define SNC_FASTCALL
#define GCC_HOT
#define GCC_COLD
#else // SNC
#define SNC_LIKELY_TARGET __attribute__((likely_target))
#define SNC_FASTCALL      __fastcall
#define GCC_HOT           __attribute__((hot))
#define GCC_COLD          __attribute__((cold))
#endif
#else
#define DECLSPEC_ALIGN(X)
#define ATTRIBUTE_ALIGN(X)
#endif

#if !defined(__GNUC__) && !defined(__SNC__)
#define STRUCT_PAD(x)
#define GCC_LIKELY(x)   (x)
#define GCC_UNLIKELY(x) (x)
#define GCC_HOT
#define GCC_COLD
#define SNC_LIKELY_TARGET
#define SNC_FASTCALL
#endif

#if (defined(__clang__) || __GNUC__ >= 7)
#define ASSUME_ALIGNED(ptr, sz) (__builtin_assume_aligned((const void *)ptr, sz))
#elif defined(_MSC_VER)
#define ASSUME_ALIGNED(ptr, sz) (__assume((((const char *)ptr) - ((const char *)0)) % (sz) == 0), (ptr))
#else
#define ASSUME_ALIGNED(ptr, sz) (ptr)
#endif
