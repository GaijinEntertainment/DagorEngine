// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "logProcessing.h"

#include <EASTL/string.h>

#include <debug/dag_logSys.h>

static LogHandlerId assert_on_logerr_handler_id = LogHandlerId::Invalid;
static eastl::string logerr_mask;

static void raise_assert_on_logerr(void *, LogMessage &msg)
{
  if (!logerr_mask.empty() && strstr(msg.fmt, logerr_mask.c_str()) == nullptr)
    return;

  G_ASSERTF(false, msg.fmt, msg.arg);
}

void enable_assert_on_logerrs(const char *mask)
{
  G_ASSERTF(assert_on_logerr_handler_id == LogHandlerId::Invalid, "Don't call this function twice!");
  logerr_mask = mask;
  assert_on_logerr_handler_id =
    debug_add_log_handler(&raise_assert_on_logerr, nullptr, {.levelMask = LOGMASK_ERR, .tags = {LOGTAG_D3D_ERROR}});
}
