// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <3d/dag_ICrashFallback.h>

struct CrashFallbackHelper : public ICrashFallback
{
  bool previousStartupFailed() override;
  void beforeStartup() override;
  void successfullyLoaded() override;
  void setSettingToAuto() override;

private:
  void saveSettings(bool failed);
};