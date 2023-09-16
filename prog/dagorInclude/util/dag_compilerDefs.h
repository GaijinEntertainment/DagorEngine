//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#ifndef DAGOR_NOINLINE
#if defined(__GNUC__)
#define DAGOR_NOINLINE __attribute__((noinline))
#elif _MSC_VER >= 1300
#define DAGOR_NOINLINE __declspec(noinline)
#else
#define DAGOR_NOINLINE
#endif
#endif

#ifndef DAGOR_LIKELY
#if (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__clang__)
#if defined(__cplusplus)
#define DAGOR_LIKELY(x)   __builtin_expect(!!(x), true)
#define DAGOR_UNLIKELY(x) __builtin_expect(!!(x), false)
#else
#define DAGOR_LIKELY(x)   __builtin_expect(!!(x), 1)
#define DAGOR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif
#else
#define DAGOR_LIKELY(x)   (x)
#define DAGOR_UNLIKELY(x) (x)
#endif
#endif

#ifndef DAGOR_THREAD_SANITIZER
#if defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define DAGOR_THREAD_SANITIZER 1
#endif
#else
#if defined(__SANITIZE_THREAD__)
#define DAGOR_THREAD_SANITIZER 1
#endif
#endif
#endif

#ifndef DAGOR_ADDRESS_SANITIZER
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define DAGOR_ADDRESS_SANITIZER 1
#endif
#else
#if defined(__SANITIZE_ADDRESS__)
#define DAGOR_ADDRESS_SANITIZER 1
#endif
#endif
#endif
