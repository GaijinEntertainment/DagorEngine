// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_group_button.h"

#include <windows.h>
#include <stdio.h>


// -------------- MaxGroupButton --------------


WMaxGroupButton::WMaxGroupButton(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  WindowControlBase(event_handler, parent, "BUTTON", 0, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | BS_NOTIFY | WS_CLIPCHILDREN, "", x, y,
    w, h),
  mOpened(true)
{}

intptr_t WMaxGroupButton::onControlDrawItem(void *info)
{
  DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)info;

  HGDIOBJ old_brush = SelectObject(draw->hDC, GetStockObject(GRAY_BRUSH));
  Rectangle(draw->hDC, 0, 0, mWidth, mHeight);
  HGDIOBJ old_font;

  if (mOpened)
    old_font = SelectObject(draw->hDC, (HGDIOBJ)this->getNormalFont());
  else
    old_font = SelectObject(draw->hDC, (HGDIOBJ)this->getBoldFont());

  SetBkMode(draw->hDC, TRANSPARENT);
  UINT uAlignPrev = SetTextAlign(draw->hDC, TA_CENTER);

  TextOut(draw->hDC, mWidth / 2, 3, mCaption, i_strlen(mCaption));

  if (mOpened)
    TextOut(draw->hDC, 8, 3, "-", 1);
  else
    TextOut(draw->hDC, 8, 3, "+", 1);

  SelectObject(draw->hDC, old_brush);
  SelectObject(draw->hDC, old_font);

  mEventHandler->onWcRefresh(this);
  return 0;
}

intptr_t WMaxGroupButton::onControlCommand(unsigned notify_code, unsigned elem_id)
{
  if (!mEventHandler)
    return 0;

  switch (notify_code)
  {
      /*
      case BN_CLICKED:
        mOpened = !mOpened;
        mEventHandler->onWcClick(this);
        break;
      */

    case BN_DBLCLK: SendMessage((HWND)mHandle, WM_LBUTTONDOWN, 0, 0); break;

    default: break;
  }

  return 0;
}

intptr_t WMaxGroupButton::onLButtonDown(long x, long y)
{
  if (!mEventHandler)
    return 0;

  mOpened = !mOpened;
  mEventHandler->onWcClick(this);
  return 0;
}

intptr_t WMaxGroupButton::onRButtonUp(long x, long y)
{
  if (mParent)
    mParent->onRButtonUp(x, y);
  return 0;
}


void WMaxGroupButton::setTextValue(const char value[]) { sprintf(mCaption, "%s", value); }


int WMaxGroupButton::getTextValue(char *buffer, int buflen) const
{
  SNPRINTF(buffer, buflen, "%s", mCaption);
  return i_strlen(buffer);
}
