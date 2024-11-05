// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvScrollEvent.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_guiScene.h>

#include "elementTree.h"
#include "scriptUtil.h"
#include "scrollHandler.h"

#include "dargDebugUtils.h"

namespace darg
{


BhvScrollEvent bhv_scroll_event;


BhvScrollEvent::BhvScrollEvent() : Behavior(STAGE_ACT, 0) {}


int BhvScrollEvent::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
  int resultFlags = 0;

  Sqrat::Table &pprops = elem->props.storage;

  float prevScrollX = pprops.RawGetSlotValue(elem->csk->scrollEventPrevX, 0.0f);
  float prevScrollY = pprops.RawGetSlotValue(elem->csk->scrollEventPrevY, 0.0f);

  float prevContentW = pprops.RawGetSlotValue(elem->csk->scrollEventPrevContW, -999.0f);
  float prevContentH = pprops.RawGetSlotValue(elem->csk->scrollEventPrevContH, -999.0f);

  float prevElemW = pprops.RawGetSlotValue(elem->csk->scrollEventPrevElemW, -999.0f);
  float prevElemH = pprops.RawGetSlotValue(elem->csk->scrollEventPrevElemH, -999.0f);

  ScreenCoord &sc = elem->screenCoord;
  if (sc.scrollOffs.x != prevScrollX || sc.scrollOffs.y != prevScrollY || sc.contentSize.x != prevContentW ||
      sc.contentSize.y != prevContentH || sc.size.x != prevElemW || sc.size.y != prevElemH)
  {
    Sqrat::Table &scriptDesc = elem->props.scriptDesc;
    HSQUIRRELVM vm = scriptDesc.GetVM();
    Sqrat::Function onScroll(vm, scriptDesc, scriptDesc.RawGetSlot(elem->csk->onScroll));
    if (!onScroll.IsNull())
    {
      SQInteger nparams = 0, nfreevars = 0;
      G_VERIFY(get_closure_info(onScroll, &nparams, &nfreevars));

      if (nparams == 2)
      {
        onScroll(elem->getRef(onScroll.GetVM()));
      }
      else if (nparams == 7)
      {
        onScroll(sc.scrollOffs.x, sc.scrollOffs.y, sc.contentSize.x, sc.contentSize.y, sc.size.x, sc.size.y);
      }
      else
        darg_assert_trace_var("OnScroll must have 1 or 6 params", scriptDesc, elem->csk->onScroll);
    }

    if (elem->scrollHandler)
    {
      String errMsg;
      if (!elem->scrollHandler->triggerRoot(errMsg))
        darg_immediate_error(vm, errMsg);
    }

    resultFlags |= R_PROCESSED;

    pprops.SetValue(elem->csk->scrollEventPrevX, sc.scrollOffs.x);
    pprops.SetValue(elem->csk->scrollEventPrevY, sc.scrollOffs.y);

    pprops.SetValue(elem->csk->scrollEventPrevContW, sc.contentSize.x);
    pprops.SetValue(elem->csk->scrollEventPrevContH, sc.contentSize.y);

    pprops.SetValue(elem->csk->scrollEventPrevElemW, sc.size.x);
    pprops.SetValue(elem->csk->scrollEventPrevElemH, sc.size.y);
  }

  return resultFlags;
}


} // namespace darg
