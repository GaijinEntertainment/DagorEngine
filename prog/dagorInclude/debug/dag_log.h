//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdarg.h>
#include <util/dag_baseDef.h>

#include <supp/dag_define_KRNLIMP.h>

enum
{
  LOGLEVEL_FATAL,
  LOGLEVEL_ERR,
  LOGLEVEL_WARN,
  LOGLEVEL_DEBUG,
  LOGLEVEL_REMARK,
};

#if DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

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
//! sets hash of the message to the context for next use of logerr/logwarn/loginfo functions
//! be sure to reset the hash after logerr use
KRNLIMP void __log_set_ctx_hash(unsigned int hash);
//! gets hash of the message from the context for next use of logerr/logwarn/loginfo functions
KRNLIMP unsigned int __log_get_ctx_hash();

#ifdef __cplusplus

#include <util/dag_safeArg.h>

#define DSA_OVERLOADS_PARAM_DECL int l,
#define DSA_OVERLOADS_PARAM_PASS l,
DECLARE_DSA_OVERLOADS_FAMILY(static inline void logmessage, KRNLIMP void logmessage_fmt, logmessage_fmt);
#undef DSA_OVERLOADS_PARAM_DECL
#undef DSA_OVERLOADS_PARAM_PASS

#endif // __cplusplus

#else

struct DagorSafeArg;
inline void cvlogmessage(int, const char *, va_list) {}
inline void logmessage_fmt(int, const char *, const DagorSafeArg *, int) {}
inline void __log_set_ctx(const char *, int, int = LOGLEVEL_DEBUG, bool = true) {}
inline void __log_set_ctx_ln(bool = true) {}
inline void __log_set_ctx_hash(unsigned int) {}
inline unsigned int __log_get_ctx_hash() { return 0u; }
#ifdef __cplusplus
template <typename... Args>
inline void logmessage(int, const Args &...)
{}
#endif

#endif // DAGOR_DBGLEVEL > 0 || DAGOR_FORCE_LOGS

#ifdef __cplusplus

template <typename... Args>
inline void logerr(const Args &...args)
{
  logmessage(LOGLEVEL_ERR, args...);
}
template <typename... Args>
inline void logwarn(const Args &...args)
{
  logmessage(LOGLEVEL_WARN, args...);
}
template <typename... Args>
inline void logdbg(const Args &...args)
{
  logmessage(LOGLEVEL_DEBUG, args...);
}

template <typename... Args>
inline void logmessage_(int l, const Args &...args)
{
  __log_set_ctx_ln(false);
  logmessage(l, args...);
}
template <typename... Args>
inline void logerr_(const Args &...args)
{
  logmessage_(LOGLEVEL_ERR, args...);
}
template <typename... Args>
inline void logwarn_(const Args &...args)
{
  logmessage_(LOGLEVEL_WARN, args...);
}
template <typename... Args>
inline void logdbg_(const Args &...args)
{
  logmessage_(LOGLEVEL_DEBUG, args...);
}

#define LOGMESSAGE_CTX(l, ...)            \
  do                                      \
  {                                       \
    __log_set_ctx(__FILE__, __LINE__, l); \
    logmessage(l, __VA_ARGS__);           \
  } while (0)
#define LOGERR_CTX(...)  LOGMESSAGE_CTX(LOGLEVEL_ERR, __VA_ARGS__)
#define LOGWARN_CTX(...) LOGMESSAGE_CTX(LOGLEVEL_WARN, __VA_ARGS__)
#define LOGDBG_CTX(...)  LOGMESSAGE_CTX(LOGLEVEL_DEBUG, __VA_ARGS__)

#endif // __cplusplus

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

#define LOGWARN_ONCE(...)        \
  do                             \
  {                              \
    static bool logged_ = false; \
    if (!logged_)                \
    {                            \
      logwarn(__VA_ARGS__);      \
      logged_ = true;            \
    }                            \
  } while (0)

#include <supp/dag_undef_KRNLIMP.h>
#include <debug/dag_debug.h>
