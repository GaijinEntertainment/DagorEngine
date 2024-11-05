// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <sepGui/wndMenuInterface.h>
#include <generic/dag_tab.h>

typedef void *TMenuHandle;


class Menu : public IMenu
{
public:
  Menu(void *handle, unsigned id, IMenuEventHandler *event_header = NULL, bool is_popup = false);
  ~Menu();

  // IMenu
  virtual void showPopupMenu();
  virtual int checkMenu(unsigned msg, TSgWParam w_param);

  virtual void addItem(unsigned pid, unsigned id, const char *name);
  virtual void addSeparator(unsigned pid, unsigned id = 0);
  virtual void addSubMenu(unsigned pid, unsigned id, const char *name);

  virtual void clearMenu(unsigned id);

  virtual bool setRadioById(unsigned id, unsigned f_id = 0, unsigned l_id = 0);
  virtual void setCheckById(unsigned id, bool checked = true);
  virtual void setEnabledById(unsigned id, bool enabled = true);
  virtual void setCaptionById(unsigned id, const char caption[]);

  virtual void setEventHandler(IMenuEventHandler *event_handler);
  virtual int click(unsigned id);

  void setIsSubMenu(bool value) { mIsSubmenu = value; }
  bool isSubMenu() { return mIsSubmenu; }
  unsigned getId() { return mID; }
  TMenuHandle getMenuHandle() { return mMenuHandle; }

private:
  void addToList(Menu *menu) { subMenus.push_back(menu); }
  bool haveMenu(unsigned id);

  void clear();
  void update();

  Menu *findMenuByID(unsigned id);
  Menu *getParentById(unsigned id);

  bool mIsPopup, mIsSubmenu;
  Tab<Menu *> subMenus;
  unsigned mID;
  TMenuHandle mMenuHandle;
  void *mParentWindowHandle;

  IMenuEventHandler *mEventHeader;
};
