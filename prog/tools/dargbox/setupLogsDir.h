// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>

#define __DEBUG_MODERN_PREFIX dargbox_get_log_prefix()
#define __DEBUG_DYNAMIC_LOGSYS
#define __DEBUG_CLASSIC_LOGSYS_FILEPATH dargbox_get_log_prefix()
#define __DEBUG_USE_CLASSIC_LOGSYS()    (false)

static String dargbox_get_log_prefix()
{
  String prefix;
  const char *pathArg = ::dgs_get_argv("logspath");
  if (pathArg && *pathArg)
  {
    prefix = pathArg;
    if (prefix[prefix.length() - 1] != '/' && prefix[prefix.length() - 1] != '\\')
      prefix.append("/");
  }
  prefix.append(".logs/");
  return prefix;
}
