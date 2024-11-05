// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "net/dedicated.h"
#include <stdlib.h>
#include <util/dag_string.h>
#include "main/appProfile.h"
#include <matching/types.h>
#include <memory/dag_framemem.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_globDef.h>

String dedicated::setup_log()
{
  const char *log_prefix = dgs_get_argv("session_log_prefix");
  if (!log_prefix || app_profile::get().serverSessionId.empty())
    return {};

  String prefix(framemem_ptr());
  char *end = nullptr;
  matching::SessionId sessionId = strtoull(app_profile::get().serverSessionId.c_str(), &end, 0);
  if (bool sessionId_is_num = end && *end == '\0')
    prefix.aprintf(0, "%s/%02x/%016llx/log.txt", log_prefix, sessionId & 0xFF, sessionId);
  else
    prefix.aprintf(0, "%s/%s/log.txt", log_prefix, app_profile::get().serverSessionId);
  return prefix;
}
