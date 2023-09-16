// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_ext_buttons.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// -------------- Extensible control buttons --------------


WExtButtons::WExtButtons(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WContainer(this, parent, x, y, w, h),
  mMenuButton(this, this, 0, 0, w, h, false),

  mButtonStatus(EXT_BUTTON_NONE),
  mExternalHandler(event_handler),
  mFlags(0)
{
  mMenuButton.setTextValue("...");
}


WExtButtons::~WExtButtons() {}


int WExtButtons::getValue() const
{
  int result = mButtonStatus;
  mButtonStatus = EXT_BUTTON_NONE;
  return result;
}


void WExtButtons::setItemFlags(int flags) { mFlags = flags; }


void WExtButtons::setEnabled(bool enabled)
{
  mMenuButton.setEnabled(enabled);
  WContainer::setEnabled(enabled);
}


void WExtButtons::resizeWindow(int w, int h)
{
  mMenuButton.resizeWindow(w, h);
  WContainer::resizeWindow(w, h);
}


void WExtButtons::moveWindow(int x, int y) { WContainer::moveWindow(x, y); }


int WExtButtons::getItemFlag(int item_id) { return (mFlags & (1 << item_id)) ? MF_ENABLED : MF_GRAYED; }


void WExtButtons::onWcClick(WindowBase *source)
{
  if (source == &mMenuButton)
  {
    POINT point;
    point.x = mMenuButton.getWidth(), point.y = mMenuButton.getHeight();
    ClientToScreen((HWND)mMenuButton.getHandle(), &point);

    HMENU menuHandle = CreatePopupMenu();
    if (menuHandle)
    {
      if (getItemFlag(EXT_BUTTON_RENAME) == MF_ENABLED)
      {
        AppendMenu((HMENU)menuHandle, getItemFlag(EXT_BUTTON_RENAME), EXT_BUTTON_RENAME, "Rename");
        AppendMenu((HMENU)menuHandle, MF_SEPARATOR, EXT_BUTTON_NONE, NULL);
      }
      AppendMenu((HMENU)menuHandle, getItemFlag(EXT_BUTTON_INSERT), EXT_BUTTON_INSERT, "Insert");
      AppendMenu((HMENU)menuHandle, getItemFlag(EXT_BUTTON_REMOVE), EXT_BUTTON_REMOVE, "Remove");
      if (getItemFlag(EXT_BUTTON_UP) == MF_ENABLED || getItemFlag(EXT_BUTTON_DOWN) == MF_ENABLED)
      {
        AppendMenu((HMENU)menuHandle, MF_SEPARATOR, EXT_BUTTON_NONE, NULL);
        AppendMenu((HMENU)menuHandle, getItemFlag(EXT_BUTTON_UP), EXT_BUTTON_UP, "Move up");
        AppendMenu((HMENU)menuHandle, getItemFlag(EXT_BUTTON_DOWN), EXT_BUTTON_DOWN, "Move down");
      }

      int result = TrackPopupMenu(menuHandle, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTBUTTON, point.x, point.y, 0,
        (HWND)mMenuButton.getHandle(), NULL);
      mButtonStatus = (result) ? result : EXT_BUTTON_NONE;
      DestroyMenu((HMENU)menuHandle);
    }
  }

  mExternalHandler->onWcClick(this);
}


// -------------- Plus button --------------


WPlusButton::WPlusButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :
  WButton(event_handler, parent, x, y, w, h, false),

  mStatic(event_handler, this, "STATIC", 0, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, "", 10, 2, 15, h - 5)
{}

intptr_t WPlusButton::onButtonDraw(void *hdc)
{
  mStatic.refresh();
  return COLOR_BTNSHADOW;
}


intptr_t WPlusButton::onButtonStaticDraw(void *hdc)
{
  HGDIOBJ old_font = SelectObject((HDC)hdc, (HGDIOBJ)this->getNormalFont());
  SetBkMode((HDC)hdc, TRANSPARENT);
  UINT uAlignPrev = SetTextAlign((HDC)hdc, TA_CENTER);
  TextOut((HDC)hdc, 8, 0, "+", 1);
  SelectObject((HDC)hdc, old_font);

  return COLOR_BTNSHADOW;
}
