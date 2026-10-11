//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

#if defined(_MSC_VER) && !defined(__clang__)
#define DAGOR_FORCE_INLINE_LAMBDA
#define DAGOR_NOINLINE_LAMBDA
#else
#define DAGOR_FORCE_INLINE_LAMBDA __attribute__((always_inline))
#define DAGOR_NOINLINE_LAMBDA     __attribute__((noinline))
#endif

#ifndef DAGOR_NO_VTABLE
#if defined(__cplusplus) && (_MSC_VER >= 1100)
#define DAGOR_NO_VTABLE __declspec(novtable)
#else
#define DAGOR_NO_VTABLE
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

#ifndef DAGOR_UNREACHABLE
#if (defined(__GNUC__) && (__GNUC__ >= 3)) || defined(__clang__)
#define DAGOR_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#define DAGOR_UNREACHABLE __assume(false)
#else
#define DAGOR_UNREACHABLE
#endif
#endif

#ifndef DAGOR_THREAD_SANITIZER
#if defined(__SANITIZE_THREAD__)
#define DAGOR_THREAD_SANITIZER 1
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define DAGOR_THREAD_SANITIZER 1
#endif
#endif
#endif

#ifndef DAGOR_ADDRESS_SANITIZER
#if defined(__SANITIZE_ADDRESS__)
#define DAGOR_ADDRESS_SANITIZER 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(hwaddress_sanitizer)
#define DAGOR_ADDRESS_SANITIZER 1
#endif
#endif
#endif

#if defined(__clang__)
#define DAGOR_LIFETIMEBOUND [[clang::lifetimebound]] // the built/returned object borrows from this arg, so a temporary here dangles
#elif defined(_MSC_VER)
#define DAGOR_LIFETIMEBOUND [[msvc::lifetimebound]] // same, but only diagnosed under /analyze
#else
#define DAGOR_LIFETIMEBOUND
#endif

#if defined(__clang__)
#define DAGOR_POINTER_LIKE [[gsl::Pointer]] // non-owning view type, so -Wdangling also sees assignment from a temporary
#else
#define DAGOR_POINTER_LIKE // MSVC and gcc do not know this attribute
#endif

// accesses through this type are not subject to type-based alias analysis,
// they may alias any other type, like a character type does
#if defined(__GNUC__) || defined(__clang__)
#define DAGOR_MAY_ALIAS __attribute__((__may_alias__))
#else
#define DAGOR_MAY_ALIAS // msvc has no TBAA
#endif

// -Wunused-variable counts a non-trivial ctor/dtor call as a use; this turns the check back
// on for value types (containers, strings). Never on RAII guards, where that call is the point.
#if (defined(__GNUC__) || defined(__clang__)) && !defined(PVS_STUDIO) // PVS-Studio reads it as nodiscard
#define DAGOR_WARN_IF_UNUSED __attribute__((warn_unused))
#else
#define DAGOR_WARN_IF_UNUSED
#endif
