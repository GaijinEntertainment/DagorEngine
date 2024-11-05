//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_compilerDefs.h>
#include <util/dag_safeArg.h>
#include <supp/dag_define_KRNLIMP.h>

//! delegate to be called when assertion/verification fails; shall return true to halt execution, or false for continue
extern KRNLIMP bool (*dgs_assertion_handler)(bool verify, const char *file, int line, const char *func, const char *cond,
  const char *fmt, const DagorSafeArg *arg, int anum);

#define DSA_OVERLOADS_PARAM_DECL bool verify, const char *file, int line, const char *func, const char *cond,
#define DSA_OVERLOADS_PARAM_PASS verify, file, line, func, cond,
DECLARE_DSA_OVERLOADS_FAMILY_LT(inline bool dgs_assertion_handler_inl, return dgs_assertion_handler);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

#ifdef __analysis_assume
#define G_ANALYSIS_ASSUME __analysis_assume
#else
#define G_ANALYSIS_ASSUME(expr)
#endif

#ifdef _MSC_VER
#define G_DEBUG_BREAK_FORCED __debugbreak()
#elif defined(__i386__) || defined(__x86_64__)
#define G_DEBUG_BREAK_FORCED       \
  do                               \
  {                                \
    __asm__ volatile("int $0x03"); \
  } while (0)
#elif defined(__has_builtin)
#if __has_builtin(__builtin_debugtrap)
#define G_DEBUG_BREAK_FORCED __builtin_debugtrap()
#endif
#endif
#ifndef G_DEBUG_BREAK_FORCED
#define G_DEBUG_BREAK_FORCED __builtin_trap()
#endif

#if DAGOR_DBGLEVEL < 1
// No side-effects in release
#define G_ASSERT_EX(expression, expr_str)            ((void)0)
#define G_ASSERTF_EX(expression, expr_str, fmt, ...) ((void)0)
#define G_ASSERT(expression)                         ((void)0)
#define G_ASSERTF(expression, fmt, ...)              ((void)0)
#define G_ASSERTF_ONCE(expression, fmt, ...)         ((void)0)
#define G_FAST_ASSERT(expression)                    ((void)0)
#define G_ASSERT_FAIL(fmt, ...)                      ((void)0)
#else
#define G_ASSERT_EX(expression, expr_str)                                                                              \
  do                                                                                                                   \
  {                                                                                                                    \
    const bool g_assert_result_ = !!(expression);                                                                      \
    G_ANALYSIS_ASSUME(g_assert_result_);                                                                               \
    if (DAGOR_UNLIKELY(!g_assert_result_))                                                                             \
      if (dgs_assertion_handler && dgs_assertion_handler_inl(false, __FILE__, __LINE__, __FUNCTION__, expr_str, NULL)) \
        G_DEBUG_BREAK_FORCED;                                                                                          \
  } while (0)

#define G_ASSERTF_EX(expression, expr_str, fmt, ...)                                                                                 \
  do                                                                                                                                 \
  {                                                                                                                                  \
    const bool g_assert_result_ = !!(expression);                                                                                    \
    G_ANALYSIS_ASSUME(g_assert_result_);                                                                                             \
    if (DAGOR_UNLIKELY(!g_assert_result_))                                                                                           \
      if (dgs_assertion_handler && dgs_assertion_handler_inl(false, __FILE__, __LINE__, __FUNCTION__, expr_str, fmt, ##__VA_ARGS__)) \
        G_DEBUG_BREAK_FORCED;                                                                                                        \
  } while (0)

#define G_ASSERT_FAIL(fmt, ...)                                                                                                  \
  {                                                                                                                              \
    if (dgs_assertion_handler && dgs_assertion_handler_inl(false, __FILE__, __LINE__, __FUNCTION__, "fail", fmt, ##__VA_ARGS__)) \
      G_DEBUG_BREAK_FORCED;                                                                                                      \
  }

#define G_ASSERTF_ONCE(expression, fmt, ...)                                                                                          \
  do                                                                                                                                  \
  {                                                                                                                                   \
    static bool showed_ = false;                                                                                                      \
    const bool g_assert_result_ = !!(expression);                                                                                     \
    G_ANALYSIS_ASSUME(g_assert_result_);                                                                                              \
    if (DAGOR_UNLIKELY(!g_assert_result_) && !showed_)                                                                                \
    {                                                                                                                                 \
      if (                                                                                                                            \
        dgs_assertion_handler && dgs_assertion_handler_inl(false, __FILE__, __LINE__, __FUNCTION__, #expression, fmt, ##__VA_ARGS__)) \
        G_DEBUG_BREAK_FORCED;                                                                                                         \
      else                                                                                                                            \
        showed_ = true;                                                                                                               \
    }                                                                                                                                 \
  } while (0)

#define G_ASSERT(expression)            G_ASSERT_EX(expression, #expression)
#define G_ASSERTF(expression, fmt, ...) G_ASSERTF_EX(expression, #expression, fmt, ##__VA_ARGS__)

// This assertion API is faster because it's won't do any function calls within, therefore not translating
// functions that call it in non-leaf functions (and preventing optimizer to inline it for example)
// It's also smaller in terms of code size: on x86[_64] it's only 3 (test+jmp+int3) instructions long
// Downside of it - no pretty message box with message when something gone wrong (only standard Unhandled exception/Segfault one)
// It's programmer's job to choose wisely which one to use for each specific case (but generally it's better to stick to G_ASSERT)
#if DAGOR_DBGLEVEL > 1
#define G_FAST_ASSERT G_ASSERT
#else
#define G_FAST_ASSERT(expr)      \
  do                             \
  {                              \
    if (DAGOR_UNLIKELY(!(expr))) \
      G_DEBUG_BREAK_FORCED;      \
  } while (0)
#endif
#endif

// _LOG functions check the condition and produce error messages even
// in production builds, which allows them to be reported. Use with care,
// bandwidth costs money!
#if DAGOR_DBGLEVEL < 1
#define G_ASSERT_LOG(expression, ...)  \
  do                                   \
  {                                    \
    if (DAGOR_UNLIKELY(!(expression))) \
      logerr(__VA_ARGS__);             \
  } while (0)
// Does and action before logging. Useful for providing additional
// context about what went worng.
#define G_ASSERT_DO_AND_LOG(expression, action, ...) \
  do                                                 \
  {                                                  \
    if (DAGOR_UNLIKELY(!(expression)))               \
    {                                                \
      action;                                        \
      logerr(__VA_ARGS__);                           \
    }                                                \
  } while (0)
#else
#define G_ASSERT_LOG(expression, ...) G_ASSERTF(expression, ##__VA_ARGS__)
#define G_ASSERT_DO_AND_LOG(expr, action, ...)               \
  do                                                         \
  {                                                          \
    const bool g_assert_result_do_ = !!(expr);               \
    if (DAGOR_UNLIKELY(!g_assert_result_do_))                \
      action;                                                \
    G_ASSERTF_EX(g_assert_result_do_, #expr, ##__VA_ARGS__); \
  } while (0)
#endif

#if DAGOR_DBGLEVEL < 1
#define G_VERIFY(expression) \
  do                         \
  {                          \
    (void)(expression);      \
  } while (0)
#define G_VERIFYF(expression, fmt, ...) \
  do                                    \
  {                                     \
    (void)(expression);                 \
  } while (0)
#else
#define G_VERIFY(expression)                                                                                             \
  do                                                                                                                     \
  {                                                                                                                      \
    bool g_verify_result_ = !!(expression);                                                                              \
    G_ANALYSIS_ASSUME(g_verify_result_);                                                                                 \
    if (DAGOR_UNLIKELY(!g_verify_result_))                                                                               \
      if (dgs_assertion_handler && dgs_assertion_handler_inl(true, __FILE__, __LINE__, __FUNCTION__, #expression, NULL)) \
        G_DEBUG_BREAK_FORCED;                                                                                            \
  } while (0)

#define G_VERIFYF(expression, fmt, ...)                                                                                               \
  do                                                                                                                                  \
  {                                                                                                                                   \
    bool g_verify_result_ = !!(expression);                                                                                           \
    G_ANALYSIS_ASSUME(g_verify_result_);                                                                                              \
    if (DAGOR_UNLIKELY(!g_verify_result_))                                                                                            \
      if (                                                                                                                            \
        dgs_assertion_handler && dgs_assertion_handler_inl(false, __FILE__, __LINE__, __FUNCTION__, #expression, fmt, ##__VA_ARGS__)) \
        G_DEBUG_BREAK_FORCED;                                                                                                         \
  } while (0)
#endif


#if !defined(G_STATIC_ASSERT)
#define G_STATIC_ASSERT(x) static_assert((x), "assertion failed: " #x)
#endif


// Warning: this macro is unhygienic! Use with care around ifs and loops.
#define G_ASSERT_AND_DO_UNHYGIENIC(expr, cmd)  \
  {                                            \
    const bool g_assert_result_do_ = !!(expr); \
    G_ASSERT_EX(g_assert_result_do_, #expr);   \
    if (DAGOR_UNLIKELY(!g_assert_result_do_))  \
      cmd;                                     \
  }

// Warning: this macro is unhygienic! Use with care around ifs and loops.
#define G_ASSERTF_AND_DO_UNHYGIENIC(expr, cmd, fmt, ...)          \
  {                                                               \
    const bool g_assert_result_do_ = !!(expr);                    \
    G_ASSERTF_EX(g_assert_result_do_, #expr, fmt, ##__VA_ARGS__); \
    if (DAGOR_UNLIKELY(!g_assert_result_do_))                     \
      cmd;                                                        \
  }

#define G_ASSERT_AND_DO(expr, cmd)        \
  do                                      \
    G_ASSERT_AND_DO_UNHYGIENIC(expr, cmd) \
  while (0)

#define G_ASSERTF_AND_DO(expr, cmd, fmt, ...)                  \
  do                                                           \
    G_ASSERTF_AND_DO_UNHYGIENIC(expr, cmd, fmt, ##__VA_ARGS__) \
  while (0)

#define G_ASSERT_RETURN(expr, returnValue) G_ASSERT_AND_DO(expr, return returnValue)
#define G_ASSERT_BREAK(expr)               G_ASSERT_AND_DO_UNHYGIENIC(expr, break)
#define G_ASSERT_CONTINUE(expr)            G_ASSERT_AND_DO_UNHYGIENIC(expr, continue)

#define G_ASSERTF_RETURN(expr, returnValue, fmt, ...) G_ASSERTF_AND_DO(expr, return returnValue, fmt, ##__VA_ARGS__)
#define G_ASSERTF_BREAK(expr, fmt, ...)               G_ASSERTF_AND_DO_UNHYGIENIC(expr, break, fmt, ##__VA_ARGS__)
#define G_ASSERTF_CONTINUE(expr, fmt, ...)            G_ASSERTF_AND_DO_UNHYGIENIC(expr, continue, fmt, ##__VA_ARGS__)

#if DAGOR_DBGLEVEL > 0
#define G_DEBUG_BREAK G_DEBUG_BREAK_FORCED
#else
#define G_DEBUG_BREAK ((void)0)
#endif

#include <supp/dag_undef_KRNLIMP.h>
