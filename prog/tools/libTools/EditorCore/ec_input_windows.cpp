// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_input.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_wndPublic.h>
#include <winGuiWrapper/wgw_busy.h>

#include <windows.h>

static bool ec_mouse_cursor_hidden = false;

IPoint2 ec_get_cursor_pos()
{
  POINT p = {0, 0};
  GetCursorPos(&p);
  return IPoint2(p.x, p.y);
}

void ec_set_cursor_pos(IPoint2 pos) { SetCursorPos(pos.x, pos.y); }

void ec_show_cursor(bool show)
{
  ec_mouse_cursor_hidden = !show;

  // Make the cursor change immediate.
  if (EDITORCORE)
    if (IWndManager *wndManager = EDITORCORE->getWndManager())
      wndManager->updateImguiMouseCursor();
}

bool ec_is_cursor_visible() { return !ec_mouse_cursor_hidden; }

bool ec_set_busy(bool busy)
{
  const bool result = wingw::set_busy(busy);

  // Make the cursor change immediate.
  if (EDITORCORE)
    if (IWndManager *wndManager = EDITORCORE->getWndManager())
      wndManager->updateImguiMouseCursor();

  return result;
}

bool ec_get_busy() { return wingw::get_busy(); }
