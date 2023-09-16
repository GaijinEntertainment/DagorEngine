//
// Dagor Tech 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <sepGui/wndCommon.h>


class IMenuEventHandler
{
public:
  /// Should return 1 if menu was processed
  virtual int onMenuItemClick(unsigned id) = 0;
};


enum
{
  ROOT_MENU_ITEM = (unsigned)-1,
};


class IMenu
{
public:
  virtual ~IMenu() {}

  // creators
  static IMenu *createMainMenu(void *handle, IMenuEventHandler *event_header = NULL);
  static IMenu *createPopupMenu(void *handle, IMenuEventHandler *event_header = NULL);

  // for internal use
  virtual void showPopupMenu() = 0;
  virtual int checkMenu(unsigned msg, TSgWParam w_param) = 0;

  // for external use
  virtual void addItem(unsigned pid, unsigned id, const char *name) = 0;
  virtual void addSeparator(unsigned pid, unsigned id = 0) = 0;
  virtual void addSubMenu(unsigned pid, unsigned id, const char *name) = 0;

  virtual void clearMenu(unsigned id) = 0;

  virtual bool setRadioById(unsigned id, unsigned f_id = 0, unsigned l_id = 0) = 0;
  virtual void setCheckById(unsigned id, bool checked = true) = 0;
  virtual void setEnabledById(unsigned id, bool enabled = true) = 0;
  virtual void setCaptionById(unsigned id, const char caption[]) = 0;

  virtual void setEventHandler(IMenuEventHandler *event_handler) = 0;
  virtual int click(unsigned id) = 0;
};
