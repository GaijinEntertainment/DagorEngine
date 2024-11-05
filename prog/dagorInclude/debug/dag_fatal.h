//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_globDef.h>
#include <util/dag_safeArg.h>
#include <stdarg.h>

#include <supp/dag_define_KRNLIMP.h>

#ifdef __cplusplus
extern "C"
{
#endif

  extern KRNLIMP volatile int g_debug_is_in_fatal;

#if DAGOR_DBGLEVEL > 0
#define FATAL_PREFIX
#define FATAL_SUFFIX
#define RETURN_X_AFTER_FATAL(x) return x

#elif defined(__GNUC__)
#define FATAL_PREFIX
#define FATAL_SUFFIX            __attribute__((noreturn))
#define RETURN_X_AFTER_FATAL(x) return x

#elif _TARGET_PC | _TARGET_XBOX
#define FATAL_PREFIX __declspec(noreturn)
#define FATAL_SUFFIX
#define RETURN_X_AFTER_FATAL(x) return x

#else
!error: undefined target
#endif

  KRNLIMP FATAL_PREFIX void _core_cfatal(const char *, ...) FATAL_SUFFIX PRINTF_LIKE;
  KRNLIMP FATAL_PREFIX void _core_cvfatal(const char *, va_list ap) FATAL_SUFFIX;

  KRNLIMP void _core_set_fatal_ctx(const char *fn, int ln);

  KRNLIMP void _core_fatal_context_push(const char *str);
  KRNLIMP void _core_fatal_context_pop();
  KRNLIMP void _core_fatal_set_context_stack_depth(int depth);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#define DSA_OVERLOADS_PARAM_DECL const char *fn, int ln,
#define DSA_OVERLOADS_PARAM_PASS fn, ln,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void _core_fatal, KRNLIMP void _core_fatal_fmt, _core_fatal_fmt);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

#define DAG_FATAL(...) _core_fatal(__FILE__, __LINE__, __VA_ARGS__)

#else

#define DAG_FATAL _core_set_fatal_ctx(__FILE__, __LINE__), _core_cfatal

#endif

#define fatal_context_push(s) _core_fatal_context_push(s)
#define fatal_context_pop     _core_fatal_context_pop

#ifdef __cplusplus
struct FatalContextRaii
{
  FatalContextRaii(const char *s) { _core_fatal_context_push(s); }
  ~FatalContextRaii() { _core_fatal_context_pop(); }
};
#define FATAL_CONTEXT_AUTO_SCOPE(s) FatalContextRaii _fcas(s)
#endif


#include <supp/dag_undef_KRNLIMP.h>
