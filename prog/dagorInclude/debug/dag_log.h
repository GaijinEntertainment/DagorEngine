//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <stdarg.h>
#include <util/dag_baseDef.h>

#include <supp/dag_define_COREIMP.h>

enum
{
  LOGLEVEL_FATAL,
  LOGLEVEL_ERR,
  LOGLEVEL_WARN,
  LOGLEVEL_DEBUG,
  LOGLEVEL_REMARK,
};

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

#define DAGOR_WITH_LOGS(...) __VA_ARGS__
#define DAGOR_HAS_LOGS(...)  __VA_ARGS__

//! general C function to output logs (accepts va_list)
KRNLIMP void cvlogmessage(int level, const char *, va_list);
//! general C function to output logs (accepts ...)
inline void clogmessage(int level, const char *f, ...) PRINTF_LIKE2;
inline void clogmessage(int level, const char *f, ...)
{
  va_list ap;
  va_start(ap, f);
  cvlogmessage(level, f, ap);
  va_end(ap);
}
//! sets context for logerr/logwarn/loginfo functions
KRNLIMP void __log_set_ctx(const char *fn, int ln, int lev = LOGLEVEL_DEBUG, bool new_ln = true);
//! sets context for logerr/logwarn/loginfo functions (new line only)
KRNLIMP void __log_set_ctx_ln(bool new_ln = true);

#define clogmessage_(l, ...)     __log_set_ctx_ln(false), clogmessage(l, __VA_ARGS__)
#define clogmessage_ctx(l, ...)  __log_set_ctx(__FILE__, __LINE__, l), clogmessage(l, __VA_ARGS__)
#define clogmessage_ctx_(l, ...) __log_set_ctx(__FILE__, __LINE__, l, false), clogmessage(l, __VA_ARGS__)

#define clogerr(...)  clogmessage(LOGLEVEL_ERR, __VA_ARGS__)
#define clogwarn(...) clogmessage(LOGLEVEL_WARN, __VA_ARGS__)
#define clogdbg(...)  clogmessage(LOGLEVEL_DEBUG, __VA_ARGS__)

#define clogerr_(...)  clogmessage_(LOGLEVEL_ERR, __VA_ARGS__)
#define clogwarn_(...) clogmessage_(LOGLEVEL_WARN, __VA_ARGS__)
#define clogdbg_(...)  clogmessage_(LOGLEVEL_DEBUG, __VA_ARGS__)

#define clogerr_ctx(...)  clogmessage_ctx(LOGLEVEL_ERR, __VA_ARGS__)
#define clogwarn_ctx(...) clogmessage_ctx(LOGLEVEL_WARN, __VA_ARGS__)
#define clogdbg_ctx(...)  clogmessage_ctx(LOGLEVEL_DEBUG, __VA_ARGS__)

#define clogerr_ctx_(...)  clogmessage_ctx_(LOGLEVEL_ERR, __VA_ARGS__)
#define clogwarn_ctx_(...) clogmessage_ctx_(LOGLEVEL_WARN, __VA_ARGS__)
#define clogdbg_ctx_(...)  clogmessage_ctx_(LOGLEVEL_DEBUG, __VA_ARGS__)


#ifdef __cplusplus
#include <util/dag_safeArg.h>

#define DSA_OVERLOADS_PARAM_DECL int l,
#define DSA_OVERLOADS_PARAM_PASS l,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void logmessage, KRNLIMP void logmessage_fmt, logmessage_fmt);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

#define logmessage_(l, ...)     __log_set_ctx_ln(false), logmessage(l, __VA_ARGS__)
#define logmessage_ctx(l, ...)  __log_set_ctx(__FILE__, __LINE__, l), logmessage(l, __VA_ARGS__)
#define logmessage_ctx_(l, ...) __log_set_ctx(__FILE__, __LINE__, l, false), logmessage(l, __VA_ARGS__)

#define logerr(...)  logmessage(LOGLEVEL_ERR, __VA_ARGS__)
#define logwarn(...) logmessage(LOGLEVEL_WARN, __VA_ARGS__)
#define logdbg(...)  logmessage(LOGLEVEL_DEBUG, __VA_ARGS__)

#define logerr_(...)  logmessage_(LOGLEVEL_ERR, __VA_ARGS__)
#define logwarn_(...) logmessage_(LOGLEVEL_WARN, __VA_ARGS__)
#define logdbg_(...)  logmessage_(LOGLEVEL_DEBUG, __VA_ARGS__)

#define logerr_ctx(...)  logmessage_ctx(LOGLEVEL_ERR, __VA_ARGS__)
#define logwarn_ctx(...) logmessage_ctx(LOGLEVEL_WARN, __VA_ARGS__)
#define logdbg_ctx(...)  logmessage_ctx(LOGLEVEL_DEBUG, __VA_ARGS__)

#define logerr_ctx_(...)  logmessage_ctx_(LOGLEVEL_ERR, __VA_ARGS__)
#define logwarn_ctx_(...) logmessage_ctx_(LOGLEVEL_WARN, __VA_ARGS__)
#define logdbg_ctx_(...)  logmessage_ctx_(LOGLEVEL_DEBUG, __VA_ARGS__)

#endif
#else

#define DAGOR_WITH_LOGS(...) (void)0
#define DAGOR_HAS_LOGS(...)

struct DagorSafeArg;
inline void cvlogmessage(int, const char *, va_list) {}
inline void logmessage_fmt(int, const char *, const DagorSafeArg *, int) {}
inline void __log_set_ctx(const char *, int, int = LOGLEVEL_DEBUG, bool = true) {}
inline void __log_set_ctx_ln(bool = true) {}

#define clogmessage(...) (void)0
#define clogerr(...)     (void)0
#define clogwarn(...)    (void)0
#define clogdbg(...)     (void)0

#define clogmessage_(...) (void)0
#define clogerr_(...)     (void)0
#define clogwarn_(...)    (void)0
#define clogdbg_(...)     (void)0

#define clogmessage_ctx(...) (void)0
#define clogerr_ctx(...)     (void)0
#define clogwarn_ctx(...)    (void)0
#define clogdbg_ctx(...)     (void)0

#define clogmessage_ctx_(...) (void)0
#define clogerr_ctx_(...)     (void)0
#define clogwarn_ctx_(...)    (void)0
#define clogdbg_ctx_(...)     (void)0

#ifdef __cplusplus

#define logmessage(...) (void)0
#define logerr(...)     (void)0
#define logwarn(...)    (void)0
#define logdbg(...)     (void)0

#define logmessage_(...) (void)0
#define logerr_(...)     (void)0
#define logwarn_(...)    (void)0
#define logdbg_(...)     (void)0

#define logmessage_ctx(...) (void)0
#define logerr_ctx(...)     (void)0
#define logwarn_ctx(...)    (void)0
#define logdbg_ctx(...)     (void)0

#define logmessage_ctx_(...) (void)0
#define logerr_ctx_(...)     (void)0
#define logwarn_ctx_(...)    (void)0
#define logdbg_ctx_(...)     (void)0

#endif

#endif // DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

#define LOGERR_ONCE(...)         \
  do                             \
  {                              \
    static bool logged_ = false; \
    if (!logged_)                \
    {                            \
      logerr(__VA_ARGS__);       \
      logged_ = true;            \
    }                            \
  } while (0)

#include <supp/dag_undef_COREIMP.h>
#include <debug/dag_debug.h>
