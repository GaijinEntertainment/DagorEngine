// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvComboPopup.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_guiScene.h>
#include <daRg/dag_scriptHandlers.h>
#include "elementTree.h"
#include "guiScene.h"


namespace darg
{


BhvComboPopup bhv_combo_popup;


BhvComboPopup::BhvComboPopup() : Behavior(0, F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_CAN_HANDLE_CLICKS) {}


static void handle_click(ElementTree *etree, Element *elem)
{
  Sqrat::Function onClick = elem->props.scriptDesc.RawGetFunction(elem->csk->onClick);
  if (!onClick.IsNull())
    etree->guiScene->queueScriptHandler(new ScriptHandlerSqFunc<>(onClick));
}


int BhvComboPopup::pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int /*pointer_id*/,
  int /*btn_id*/, Point2 pos, int accum_res)
{
  int result = 0;

  // old logic, to be reviewed
  if (device == DEVID_TOUCH)
  {
    if (!(accum_res & R_PROCESSED) && event == INP_EV_RELEASE && elem->hitTest(pos))
    {
      handle_click(etree, elem);
      result = R_PROCESSED;
    }
  }
  else
  {
    if (event == INP_EV_PRESS && !(accum_res & R_PROCESSED) && elem->hitTest(pos))
    {
      handle_click(etree, elem);
    }
  }

  return 0;
}


} // namespace darg
