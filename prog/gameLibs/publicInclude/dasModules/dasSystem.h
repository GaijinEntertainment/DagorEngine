//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <osApiWrappers/dag_miscApi.h>
#include <daScript/simulate/bind_enum.h>
#include <vecmath/dag_vecMathDecl.h>
#include <startup/dag_globalSettings.h>
#include <workCycle/dag_workCycle.h>
#include <osApiWrappers/dag_clipboard.h>
#include <dasModules/dasDataBlock.h>
#include <daECS/core/internal/typesAndLimits.h>

DAS_BIND_ENUM_CAST(ConsoleModel);

namespace bind_dascript
{
typedef void (*DasExitFunctionPtr)(int);
void set_das_exit_function_ptr(DasExitFunctionPtr func);

inline void fatal_func(const char *a) { fatal("Fatal: %s", a); }

void exit_func(int exit_code);

inline void logmsg_func(int a, const char *s)
{
  G_UNUSED(a);
  G_UNUSED(s);
  logmessage(a, "das:%s", s);
}
inline void logerr_func(const char *s, das::Context *context, das::LineInfoArg *at)
{
  G_UNUSED(s);
  debug("%s", context->getStackWalk(at, false, false).c_str());
  logerr("das:%s", s);
}
inline void logwarn_func(const char *s)
{
  G_UNUSED(s);
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
