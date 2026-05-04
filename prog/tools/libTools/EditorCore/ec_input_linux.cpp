// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_input.h>

#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_wndPublic.h>
#include <libTools/util/daKernel.h>
#include <osApiWrappers/dag_linuxGUI.h>

IPoint2 ec_mouse_cursor_pos = IPoint2(0, 0);
bool ec_mouse_cursor_hidden = false;

static bool ec_busy = false;
static constexpr const char *BUSY_VAR_NAME = "ec_busy";

IPoint2 ec_get_cursor_pos() { return ec_mouse_cursor_pos; }

void ec_set_cursor_pos(IPoint2 pos)
{
  // We have to hide the cursor to be able to change its position on Xwayland too.
  // See: https://github.com/libsdl-org/SDL/issues/9539
  const bool wasHidden = ec_mouse_cursor_hidden;
  if (!wasHidden)
    ec_show_cursor(false);

  linux_GUI::set_cursor_position(pos.x, pos.y, nullptr);

  ec_mouse_cursor_pos = pos;

  if (!wasHidden)
    ec_show_cursor(true);
}

void ec_show_cursor(bool show)
{
  ec_mouse_cursor_hidden = !show;

  // Make the cursor change immediate.
  if (EDITORCORE)
    if (IWndManager *wndManager = EDITORCORE->getWndManager())
      wndManager->updateImguiMouseCursor();
}

bool ec_is_cursor_visible() { return !ec_mouse_cursor_hidden; }

bool ec_set_busy(bool new_busy)
{
  bool *busy = (bool *)dakernel::get_named_pointer(BUSY_VAR_NAME);
  if (!busy)
  {
    busy = &ec_busy;
    dakernel::set_named_pointer(BUSY_VAR_NAME, busy);
  }

  const bool oldBusy = *busy;
  if (new_busy != oldBusy)
  {
    *busy = new_busy;

    // Make the cursor change immediate.
    if (EDITORCORE)
      if (IWndManager *wndManager = EDITORCORE->getWndManager())
        wndManager->updateImguiMouseCursor();
  }

  return oldBusy;
}

bool ec_get_busy()
{
  bool *busy = (bool *)dakernel::get_named_pointer(BUSY_VAR_NAME);
  return busy && *busy;
}
