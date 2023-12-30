//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

namespace event_log
{
const char *get_net_assert_version();
void set_net_assert_version(const char *);
extern bool fatal_on_net_assert;
} // namespace event_log

#if DAGOR_DBGLEVEL < 1
#include <util/dag_string.h>
#include <osApiWrappers/dag_stackHlp.h>
#include "eventLog.h"
#if _TARGET_PC_WIN
#define NET_EVENT_STRING(str, exp_string)                                                                         \
  String str;                                                                                                     \
  {                                                                                                               \
    stackhelp::CallStackCaptureStore<32> stack;                                                                   \
    stackhelp::ext::CallStackCaptureStore extStack;                                                               \
    stack.capture();                                                                                              \
    extStack.capture();                                                                                           \
    str.printf(256, "%s:%s:\nBP=%p\nST:\n%s", event_log::get_net_assert_version(), exp_string, stackhlp_get_bp(), \
      get_call_stack_str(stack, extStack));                                                                       \
  }
#else
#define NET_EVENT_STRING(str, exp_string)                                                                                     \
  String str;                                                                                                                 \
  {                                                                                                                           \
    stackhelp::CallStackCaptureStore<32> stack;                                                                               \
    stackhelp::ext::CallStackCaptureStore extStack;                                                                           \
    stack.capture();                                                                                                          \
    extStack.capture();                                                                                                       \
    str.printf(256, "%s:%s:\nST:\n%s", event_log::get_net_assert_version(), exp_string, get_call_stack_str(stack, extStack)); \
  }
#endif

#define G_NET_ASSERT(expression)                                \
  do                                                            \
  {                                                             \
    static bool assert_sent##__LINE__ = false;                  \
    bool g_assert_result_ = (expression);                       \
    G_ANALYSIS_ASSUME(g_assert_result_);                        \
    if (!g_assert_result_ && !assert_sent##__LINE__)            \
    {                                                           \
      assert_sent##__LINE__ = true;                             \
      NET_EVENT_STRING(str, #expression);                       \
      event_log::send_udp("assert", str.str(), str.length());   \
      if (event_log::fatal_on_net_assert)                       \
        DAG_FATAL(str);                                         \
      else                                                      \
        event_log::send_udp("assert", str.str(), str.length()); \
    }                                                           \
  } while (0)
#define G_NET_ASSERTF(expression, fmt, ...)                     \
  do                                                            \
  {                                                             \
    static bool assert_sent##__LINE__ = false;                  \
    bool g_assert_result_ = (expression);                       \
    G_ANALYSIS_ASSUME(g_assert_result_);                        \
    if (!g_assert_result_ && !assert_sent##__LINE__)            \
    {                                                           \
      assert_sent##__LINE__ = true;                             \
      String str1(256, fmt, ##__VA_ARGS__);                     \
      String str2(256, "%s (%s)", #expression, str1.str());     \
      NET_EVENT_STRING(str, str2.str());                        \
      if (event_log::fatal_on_net_assert)                       \
        DAG_FATAL("%s", str.str());                             \
      else                                                      \
        event_log::send_udp("assert", str.str(), str.length()); \
    }                                                           \
  } while (0)

#else
#include <util/dag_globDef.h>
#define G_NET_ASSERT  G_ASSERT
#define G_NET_ASSERTF G_ASSERTF
#endif
