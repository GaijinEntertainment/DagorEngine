// Copyright 2023 by Gaijin Games KFT, All rights reserved.

#include <propPanel2/c_control_event_handler.h>
#include <propPanel2/c_panel_base.h>
#include "c_window_constants.h"

void PropertyContainerControlBase::setPostEvent(int pcb_id)
{
  HWND handle = (HWND)getRootParent()->getWindowHandle();
  PostMessage(handle, WM_POST_MESSAGE, pcb_id, 0);
}
