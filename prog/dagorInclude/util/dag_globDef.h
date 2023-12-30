//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

/// localized character type
typedef char TCHR;
typedef TCHR *PTCHR;

#include <util/dag_compilerDefs.h>
#include <util/dag_baseDef.h>
#include <debug/dag_fatal.h>
#include <debug/dag_debug.h>
#include <debug/dag_assert.h>
#include <supp/dag_define_COREIMP.h>


// general quit routine (to be used instead of exit() or abort())
#if _TARGET_PC && (DAGOR_DBGLEVEL > 0)
KRNLIMP void quit_game(int c = 0, bool restart = false, const char **extra_args = nullptr);
#elif _TARGET_PC_WIN | _TARGET_XBOX
KRNLIMP void __declspec(noreturn) quit_game(int c = 0, bool restart = false, const char **extra_args = nullptr);
#elif __GNUC__ || __clang__
KRNLIMP void quit_game(int c = 0, bool restart = false, const char **extra_args = nullptr) __attribute__((noreturn));
#else
!error: undefined target
#endif

#if defined(__cplusplus) && !defined(__GNUC__)
template <typename T, size_t N>
char (&_countof__helper_(T (&array)[N]))[N];
#define countof(x) (sizeof(_countof__helper_(x)))
#else
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif
#define c_countof(x) (sizeof(x) / sizeof((x)[0]))

#define FMT_P2     "(%f, %f)"
#define FMT_P3     "(%f, %f, %f)"
#define FMT_P3_02F "(%.02f %.02f %.02f)"
#define FMT_P4     "(%f, %f, %f, %f)"
#define FMT_TM     "\n(%f, %f, %f)\n(%f, %f, %f)\n(%f, %f, %f)\n(%f, %f, %f)\n"
#define FMT_B2     "(%f, %f)..(%f, %f)"
#define FMT_B3     "(%f,%f,%f)..(%f,%f,%f)"
#define FMT_P3I    "(%i, %i, %i)"
#define FMT_P4I    "(%i, %i, %i, %i)"

#if defined(va_copy)
#define __SHOULD_FREE_VALIST_COPY__ 1
#define DAG_VACOPY(TO, FROM) \
  va_list TO;                \
  va_copy(TO, (FROM))
#elif defined(__va_copy)
#define __SHOULD_FREE_VALIST_COPY__ 1
#define DAG_VACOPY(TO, FROM) \
  va_list TO;                \
  __va_copy(TO, (FROM))
#else
#define __SHOULD_FREE_VALIST_COPY__ 0
#define DAG_VACOPY(TO, FROM)        va_list &TO = (FROM)
#endif

#if __SHOULD_FREE_VALIST_COPY__
#define DAG_VACOPYEND(WHAT) va_end(WHAT)
#else
#define DAG_VACOPYEND(WHAT)
#endif

#undef __SHOULD_FREE_VALIST_COPY__

#undef min
#undef max

#define G_UNUSED(x)    ((void)(x))
#define G_UNREFERENCED G_UNUSED

template <typename T>
inline constexpr T min(const T val1, const T val2)
{
  return (val1 < val2 ? val1 : val2);
}
template <typename T>
inline constexpr T max(const T val1, const T val2)
{
  return (val1 > val2 ? val1 : val2);
}

inline void inplace_min(float &m, float a) { m = min(m, a); }
inline void inplace_max(float &m, float a) { m = max(m, a); }

#include <supp/dag_undef_COREIMP.h>
