// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EditorCore/ec_input.h>

#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiPointing.h>
#include <EditorCore/ec_interface.h>
#include <EditorCore/ec_wndPublic.h>
#include <libTools/util/daKernel.h>
#include <startup/dag_inpDevClsDrv.h>

void mouse_api_hide_cursor(bool hide);

bool ec_mouse_cursor_hidden = false;

static bool ec_busy = false;
static constexpr const char *BUSY_VAR_NAME = "ec_busy";

// TODO: tools Linux porting: this returns relative coordinates. It is only used together with ec_set_cursor_pos() so
// it works at the moment. But it will be needed for the ImGui multi view.
IPoint2 ec_get_cursor_pos() { return IPoint2(HumanInput::raw_state_pnt.mouse.x, HumanInput::raw_state_pnt.mouse.y); }

// TODO: tools Linux porting: see the comment above. setPosition() also sets relative coordinates.
void ec_set_cursor_pos(IPoint2 pos)
{
  if (global_cls_drv_pnt)
    if (HumanInput::IGenPointing *mouse = global_cls_drv_pnt->getDevice(0))
    {
      mouse->setPosition(pos.x, pos.y);

      // setPosition() could have hidden the cursor (erroneously), so reshow it immediately if needed.
      if (IWndManager *wndManager = EDITORCORE->getWndManager())
      {
        wndManager->updateImguiMouseCursor();
        mouse_api_hide_cursor(mouse->isMouseCursorHidden());
      }
    }
}

void ec_show_cursor(bool show)
{
  ec_mouse_cursor_hidden = !show;

  // Make the cursor change immediate.
  if (IWndManager *wndManager = EDITORCORE->getWndManager())
    wndManager->updateImguiMouseCursor();
}

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
