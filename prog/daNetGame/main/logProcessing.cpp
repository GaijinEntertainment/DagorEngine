// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "logProcessing.h"

#include <EASTL/string.h>

#include <debug/dag_logSys.h>

static debug_log_callback_t orig_debug_log = nullptr;
static eastl::string logerr_mask;

static int raise_assert_on_logerr(int lev_tag, const char *fmt, const void *arg, int anum, const char *ctx_file, int ctx_line)
{
  int retValue = 1;
  if (orig_debug_log)
    retValue = orig_debug_log(lev_tag, fmt, arg, anum, ctx_file, ctx_line);

  if (!logerr_mask.empty() && strstr(fmt, logerr_mask.c_str()) == nullptr)
    return retValue;

  static const int D3DE_TAG = _MAKE4C('D3DE');
  if (lev_tag == LOGLEVEL_ERR || lev_tag == D3DE_TAG)
  {
    G_ASSERTF(false, fmt, arg);
  }
  return retValue;
}

void enable_assert_on_logerrs(const char *mask)
{
  G_ASSERTF(orig_debug_log == nullptr, "Don't call this function twice!");
  orig_debug_log = debug_set_log_callback(raise_assert_on_logerr);
  logerr_mask = mask;
}
