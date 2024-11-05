// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <windows.h>

#include "wndMenu.h"
#include <debug/dag_debug.h>


IMenu *IMenu::createMainMenu(void *handle, IMenuEventHandler *event_header)
{
  return new Menu(handle, ROOT_MENU_ITEM, event_header, false);
}


IMenu *IMenu::createPopupMenu(void *handle, IMenuEventHandler *event_header)
{
  return new Menu(handle, ROOT_MENU_ITEM, event_header, true);
}


///////////////////////////////////////////////////////////////////////////////


Menu::Menu(void *handle, unsigned id, IMenuEventHandler *event_header, bool is_popup) :

  subMenus(midmem), mParentWindowHandle(0), mEventHeader(event_header), mIsPopup(is_popup), mIsSubmenu(false)
{
  mMenuHandle = (TMenuHandle)((!is_popup) ? CreateMenu() : CreatePopupMenu());
  mID = id;

  if (handle)
  {
    mParentWindowHandle = handle;
    if (!is_popup)
    {
      SetMenu((HWND)handle, (HMENU)mMenuHandle);
      DrawMenuBar((HWND)mParentWindowHandle);
    }
  }
}


Menu::~Menu()
{
  for (int i = 0; i < subMenus.size(); i++)
    delete subMenus[i];
  DestroyMenu((HMENU)mMenuHandle);
}


void Menu::update()
{
  if (mParentWindowHandle && !mIsPopup)
    DrawMenuBar((HWND)mParentWindowHandle);
}


void Menu::clear()
{
  for (int i = 0; i < subMenus.size(); i++)
  {
    subMenus[i]->clear();
    RemoveMenu((HMENU)getMenuHandle(), 0, MF_BYPOSITION);
    delete subMenus[i];
  }

  clear_and_shrink(subMenus);
  update();
}


void Menu::clearMenu(unsigned id)
{
  if (Menu *_menu = findMenuByID(id))
    _menu->clear();
}


Menu *Menu::findMenuByID(unsigned id)
{
  if (id == mID)
    return this;
  for (int i = 0; i < subMenus.size(); i++)
    if (Menu *_pmenu = subMenus[i]->findMenuByID(id))
      return _pmenu;
  return NULL;
}


bool Menu::haveMenu(unsigned id)
{
  if (id == mID)
    return true;
  for (int i = 0; i < subMenus.size(); i++)
  {
    if (subMenus[i]->haveMenu(id))
      return true;
  }
  return false;
}


Menu *Menu::getParentById(unsigned id)
{
  Menu *_p_menu = findMenuByID(id);
  _p_menu = (_p_menu) ? _p_menu : this;
  return _p_menu;
}


void Menu::addItem(unsigned pid, unsigned id, const char *name)
{
  Menu *newMenu = new Menu(0, id);
  Menu *parentMenu = getParentById(pid);

  AppendMenu((HMENU)parentMenu->getMenuHandle(), MF_ENABLED, id, name);
  parentMenu->addToList(newMenu);

  update();
}


void Menu::addSeparator(unsigned pid, unsigned id)
{
  Menu *newMenu = new Menu(0, id);
  Menu *parentMenu = getParentById(pid);

  AppendMenu((HMENU)parentMenu->getMenuHandle(), MF_SEPARATOR, id, NULL);
  parentMenu->addToList(newMenu);

  update();
}


void Menu::addSubMenu(unsigned pid, unsigned id, const char *name)
{
  Menu *newMenu = new Menu(0, id);
  Menu *parentMenu = getParentById(pid);

  AppendMenu((HMENU)parentMenu->getMenuHandle(), MF_ENABLED | MF_POPUP, (UINT_PTR)newMenu->getMenuHandle(), name);
  parentMenu->addToList(newMenu);
  newMenu->setIsSubMenu(true);

  update();
}


void Menu::showPopupMenu()
{
  if (!mIsPopup)
  {
    debug("Menu::showPopupMenu(): menu is not popup!");
    return;
  }

  POINT pos;
  GetCursorPos(&pos);
  TrackPopupMenu((HMENU)mMenuHandle, 0, pos.x, pos.y, 0, (HWND)mParentWindowHandle, NULL);
}


int Menu::checkMenu(unsigned msg, TSgWParam w_param)
{
  if ((WM_COMMAND == msg) && !HIWORD(w_param))
  {
    return click(LOWORD(w_param));
  }

  return 1; // fail
}


int Menu::click(unsigned id)
{
  if (mEventHeader)
    return mEventHeader->onMenuItemClick(id);

  return 0;
}


//-----------------------------------


bool Menu::setRadioById(unsigned id, unsigned f_id, unsigned l_id)
{
  if (!f_id)
  {
    f_id = GetMenuItemID((HMENU)getMenuHandle(), 0);
  }

  if (!l_id)
  {
    l_id = GetMenuItemID((HMENU)getMenuHandle(), GetMenuItemCount((HMENU)getMenuHandle()) - 1);
  }

  if (f_id == (unsigned)-1 || l_id == (unsigned)-1)
    return false;

  return (bool)CheckMenuRadioItem((HMENU)getMenuHandle(), f_id, l_id, id, MF_BYCOMMAND);
}


void Menu::setCheckById(unsigned id, bool checked)
{
  unsigned v_check = (checked) ? MF_CHECKED : MF_UNCHECKED;

  CheckMenuItem((HMENU)getMenuHandle(), id, v_check | MF_BYCOMMAND);
}


void Menu::setEnabledById(unsigned id, bool enabled)
{
  unsigned v_enable = (enabled) ? MF_ENABLED : MF_GRAYED;

  EnableMenuItem((HMENU)getMenuHandle(), id, v_enable | MF_BYCOMMAND);
}


void Menu::setCaptionById(unsigned id, const char caption[])
{
  for (int i = 0; i < subMenus.size(); ++i)
    if (subMenus[i]->getId() == id)
    {
      if (subMenus[i]->isSubMenu())
        id = (unsigned)(uintptr_t)subMenus[i]->getMenuHandle();
      break;
    }

  MENUITEMINFO item_info;
  memset(&item_info, 0, sizeof(MENUITEMINFO));
  item_info.cbSize = sizeof(MENUITEMINFO);
  item_info.fMask = MIIM_TYPE;
  item_info.fType = MFT_STRING;

  if (GetMenuItemInfo((HMENU)getMenuHandle(), id, false, &item_info))
    if (item_info.cch == strlen(caption))
    {
      char *buf = new char[item_info.cch + 1];
      item_info.dwTypeData = buf;
      item_info.cch++;
      if (GetMenuItemInfo((HMENU)getMenuHandle(), id, false, &item_info))
      {
        if (strcmp(item_info.dwTypeData, caption) == 0)
        {
          delete[] buf;
          return;
        }
      }
      delete[] buf;
    }

  memset(&item_info, 0, sizeof(MENUITEMINFO));
  item_info.cbSize = sizeof(MENUITEMINFO);
  item_info.fMask = MIIM_TYPE;
  item_info.fType = MFT_STRING;
  item_info.dwTypeData = (LPSTR)caption;
  item_info.cch = i_strlen(caption);


  SetMenuItemInfo((HMENU)getMenuHandle(), id, false, &item_info);
  update();
}


void Menu::setEventHandler(IMenuEventHandler *event_handler) { mEventHeader = event_handler; }
