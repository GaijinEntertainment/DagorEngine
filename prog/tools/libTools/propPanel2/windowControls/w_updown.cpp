// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include "w_updown.h"

#include <windows.h>
#include <commctrl.h>


// -------------- UpDown --------------


WUpDown::WUpDown(WindowControlEventHandler *event_handler, WindowBase *parent, int x, int y, int w, int h) :

  // WS_EX_CLIENTEDGE WS_EX_STATICEDGE
  WindowControlBase(event_handler, parent, UPDOWN_CLASS, WS_EX_STATICEDGE, WS_CHILD | WS_VISIBLE | UDS_ARROWKEYS | UDS_ALIGNRIGHT, "",
    x, y, w, h),

  mUpClicked(bool()),
  mPriorY(0)
{}

intptr_t WUpDown::onControlNotify(void *info)
{
  if (!mEventHandler)
    return 0;

  NMHDR *param = (NMHDR *)info;

  switch (param->code)
  {
    case UDN_DELTAPOS:
    {
      NMUPDOWN *deltaInfo = (NMUPDOWN *)info;

      mUpClicked = (deltaInfo->iDelta < 0);

      mEventHandler->onWcClick(this);
    }
    break;

    default: break;
  }

  return 0;
}


intptr_t WUpDown::onDrag(int new_x, int new_y)
{
  if ((new_y < 0) || (new_y > this->getHeight()))
  {
    mUpClicked = (mPriorY - new_y > 0);

    if (mPriorY - new_y)
      mEventHandler->onWcClick(this);

    // jumping mouse

    bool changes = false;
    POINT mouse_pos;
    GetCursorPos(&mouse_pos);

    if (mouse_pos.y == GetSystemMetrics(SM_CYSCREEN) - 1)
    {
      mouse_pos.y -= _pxS(SCROLL_MOUSE_JUMP);
      new_y -= _pxS(SCROLL_MOUSE_JUMP);
      changes = true;
    }

    if (mouse_pos.y == 0)
    {
      mouse_pos.y += _pxS(SCROLL_MOUSE_JUMP);
      new_y += _pxS(SCROLL_MOUSE_JUMP);
      changes = true;
    }

    if (changes)
      SetCursorPos(mouse_pos.x, mouse_pos.y);
  }


  mPriorY = new_y;
  return 0;
}


bool WUpDown::lastUpClicked() { return mUpClicked; }
