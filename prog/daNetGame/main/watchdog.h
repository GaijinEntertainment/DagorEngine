// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_watchdog.h>

class SqModules;

namespace watchdog
{
enum WatchdogMode
{
  LOADING,
  GAME_SESSION,
  LOBBY,
  COUNT
};

void init();
void shutdown();
WatchdogMode change_mode(WatchdogMode mode);
void bind_sq(SqModules *mgr);
} // namespace watchdog
