// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_input.h>
#include <winGuiWrapper/wgw_busy.h>

#include <windows.h>

IPoint2 ec_get_cursor_pos()
{
  POINT p = {0, 0};
  GetCursorPos(&p);
  return IPoint2(p.x, p.y);
}

void ec_set_cursor_pos(IPoint2 pos) { SetCursorPos(pos.x, pos.y); }

void ec_show_cursor(bool show) { ShowCursor(show); }

bool ec_set_busy(bool busy) { return wingw::set_busy(busy); }

bool ec_get_busy() { return wingw::get_busy(); }
