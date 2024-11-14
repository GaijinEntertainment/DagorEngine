// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "globalConfig.h"

static shc::CompilerConfig g_config{};

const shc::CompilerConfig &shc::config() { return g_config; }
shc::CompilerConfig &shc::acquire_rw_config()
{
  static bool acquired = false;
  G_ASSERT(!acquired);
  acquired = true;
  return g_config;
}
