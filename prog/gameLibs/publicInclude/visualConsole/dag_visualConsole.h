//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <gui/dag_visConsole.h>
#include <util/dag_stdint.h>
#include <util/dag_console.h>

class VisualConsoleDriver : public console::DefaultDagorVisualConsoleDriver //  IVisualConsoleDriver,
{
protected:
  int32_t refTime;
  int timeSinceLastSwitch;
  int prevCursorX;

  virtual bool renderTips();
  virtual void renderCursor();

public:
  int linesLeft;
  using console::DefaultDagorVisualConsoleDriver::bufferLines;
  using console::DefaultDagorVisualConsoleDriver::linesList;
  using console::DefaultDagorVisualConsoleDriver::screenLines;
  using console::DefaultDagorVisualConsoleDriver::scrollPos;
  bool timeSpeedHotKeys;

  VisualConsoleDriver();

  virtual void render();

  virtual void init(const char *script_filename);
  virtual void shutdown(){};
  virtual void set_cur_height(float ht);
  virtual void save_text(const char *){};
  virtual void getOutputText(console::CommandList &){};
  virtual void setOutputText(const console::CommandList &){};
  virtual void getCommandList(console::CommandList &out_list);
};

#define IS_VISUAL_CONSOLE_DRIVER(x) ((x)->get_cur_height() > 1e-6f)

console::IVisualConsoleDriver *setup_visual_console_driver();
void close_visual_console_driver();
