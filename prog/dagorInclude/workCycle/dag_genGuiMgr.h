//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

//! generic interface for game's GUI supported by dagor_work_cycle()
class IGeneralGuiManager
{
public:
  virtual ~IGeneralGuiManager() {}

  virtual void act() = 0;
  virtual void beforeRender(int ticks_usec) = 0;
  virtual void draw(int ticks_usec) = 0;

  virtual void changeRenderSettings(bool &draw_scene, bool &clr_scene) = 0;
  virtual bool canCloseNow() = 0;
  virtual bool canActScene() = 0;

  virtual void drawCopyrightMessage(int ticks) = 0;
  virtual void drawFps(float minfps, float maxfps, float lastfps) = 0;
};

//! additional interface to be optionally used in GUI manager to
//! interact with external Gui libraries
class IExternalGui
{
public:
  virtual void beforeRender() = 0;
  virtual void renderFirst() = 0;
  virtual void renderLast() = 0;

  virtual void mouseEvent(int x, int y, int buttons, int wheel_ch, bool just_move) = 0;
  virtual void kbdEvent(int vk, bool pressed) = 0;
  virtual bool canCloseNow() = 0;
};


//! GUI manager that accessed from dagor_work_cycle()
extern IGeneralGuiManager *dagor_gui_manager;

//! optional external Gui interface (to be accessed from custom GUI manager)
extern IExternalGui *dagor_ext_gui;
