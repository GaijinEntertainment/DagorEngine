//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_miscApi.h>
#include <daScript/simulate/bind_enum.h>
#include <startup/dag_globalSettings.h>
#include <workCycle/dag_workCycle.h>
#include <dasModules/dasDataBlock.h>
#include <daECS/core/internal/typesAndLimits.h>
#include <util/dag_hash.h>

DAS_BIND_ENUM_CAST(ConsoleModel);

namespace bind_dascript
{
typedef void (*DasExitFunctionPtr)(int);
void set_das_exit_function_ptr(DasExitFunctionPtr func);

inline void fatal_func(const char *a) { DAG_FATAL("Fatal: %s", a); }

void exit_func(int exit_code);

#if _TARGET_PC_LINUX
// system memory getter can take 2-10ms, caller does not expect it, override with with_crt = false
int get_memory_allocated_kb_linux(bool use_crt);
#endif

inline void logmsg_func(int a, const char *s, das::LineInfoArg *at)
{
  G_UNUSED(a);
  G_UNUSED(s);
  G_UNUSED(at);
#if DAGOR_DBGLEVEL > 0
  if (at && at->fileInfo)
  {
    logmessage(a, "das:%s:%d: %s", at->fileInfo->name.c_str(), at->line, s);
    return;
  }
#endif
  logmessage(a, "das:%s", s);
}
inline void logerr_func(const char *s, das::Context *context, das::LineInfoArg *at)
{
  G_UNUSED(s);
  G_UNUSED(context);
  G_UNUSED(at);
#if DAGOR_DBGLEVEL > 0
  debug("%s", context->getStackWalk(at, false, false).c_str());
  if (at && at->fileInfo)
  {
    __log_set_ctx_hash(str_hash_fnv1(s));
    logerr("%s:%d: %s", at->fileInfo->name.c_str(), at->line, s);
    __log_set_ctx_hash(0);
    return;
  }
#endif
  // note: it's important here that we pass s directly, and not something like "%s", s
  // otherwise log message hash will be calculated for the format string "%s" and not the actual string
  logerr(s);
}
inline void logerr_func_hash(const char *s, uint32_t hash, das::Context *context, das::LineInfoArg *at)
{
  G_UNUSED(s);
  G_UNUSED(hash);
  G_UNUSED(context);
  G_UNUSED(at);

#if DAGOR_DBGLEVEL > 0
  debug("%s", context->getStackWalk(at, false, false).c_str());
  if (at && at->fileInfo)
  {
    __log_set_ctx_hash(hash);
    logerr("%s:%d: %s", at->fileInfo->name.c_str(), at->line, s);
    __log_set_ctx_hash(0);
    return;
  }
#endif
  __log_set_ctx_hash(hash);
  logerr(s);
  __log_set_ctx_hash(0);
}
inline void logwarn_func(const char *s, das::LineInfoArg *at)
{
  G_UNUSED(s);
  G_UNUSED(at);
#if DAGOR_DBGLEVEL > 0
  if (at && at->fileInfo)
  {
    logwarn("das:%s:%d: %s", at->fileInfo->name.c_str(), at->line, s);
    return;
  }
#endif
  logwarn("das:%s", s);
}

inline uint32_t get_dagor_frame_no() { return ::dagor_frame_no(); }
inline float get_dagor_game_act_time() { return ::dagor_game_act_time; }

inline int get_DAGOR_DBGLEVEL() { return DAGOR_DBGLEVEL; }
inline int get_DAGOR_ADDRESS_SANITIZER()
{
#ifdef DAGOR_ADDRESS_SANITIZER
  return DAGOR_ADDRESS_SANITIZER;
#else
  return 0;
#endif
}
inline int get_DAGOR_THREAD_SANITIZER()
{
#ifdef DAGOR_THREAD_SANITIZER
  return DAGOR_THREAD_SANITIZER;
#else
  return 0;
#endif
}
inline int get_ARCH_BITS()
{
#if defined(_TARGET_64BIT)
  return 64;
#else
  return 32;
#endif
}
inline int get_DAECS_EXTENSIVE_CHECKS()
{
#if defined(DAECS_EXTENSIVE_CHECKS)
  return DAECS_EXTENSIVE_CHECKS;
#else
  return 0;
#endif
}

inline const DataBlock &dgs_get_settings()
{
  const DataBlock *res = ::dgs_get_settings();
  return res ? *res : DataBlock::emptyBlock;
};
} // namespace bind_dascript
