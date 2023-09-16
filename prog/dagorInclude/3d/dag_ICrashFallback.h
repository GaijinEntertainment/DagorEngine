//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/unique_ptr.h>

// This interface is a temporary solution until dx12 driver is stable enough.
// If the game crashes before getting to the menu so that users can change the
// driver setting back to dx11, it'll automatically fall back to "auto",
// and show a popup message next launch.
//
// It's currently implemented in daNetGame games in the DriverFallbackCallback class.
// It counts the number of frames rendered with WorldRenderer, and if the game didn't
// crash within a certain number of frames (10 currently) it considers the game having
// launched successfully.
//
// If the game closes properly, it's also considered a successful start.
// This is to allow users to alt + f4 on the login screen for example.
//
// This feature is enabled by the dx12/previous_run_crash_fallback_enabled
// setting. It's true by default in release builds, false otherwise.

#define CRASH_FALLBACK_ENABLED         _TARGET_PC_WIN
#define CRASH_FALLBACK_ENABLED_DEFAULT (DAGOR_DBGLEVEL == 0)

struct ICrashFallback
{
  virtual bool previousStartupFailed() = 0;
  virtual void beforeStartup() = 0;
  virtual void successfullyLoaded() = 0;
  virtual void setSettingToAuto() = 0;
  virtual ~ICrashFallback() = default;
};

extern eastl::unique_ptr<ICrashFallback> crash_fallback_helper;
