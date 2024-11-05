//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

//! Legacy interface, not related to GUI anymore.
//! This should be removed from code or at least renamed
//! to something like IWorkCycleHook/IWorkCycleCallback
class IGeneralGuiManager
{
public:
  virtual ~IGeneralGuiManager() {}

  virtual void beforeRender(int ticks_usec) = 0;
  virtual bool canCloseNow() = 0;
  virtual void drawFps(float minfps, float maxfps, float lastfps) = 0;
};

//! Global instance with methods called from dagor_work_cycle()
extern IGeneralGuiManager *dagor_gui_manager;
