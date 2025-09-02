// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvRtPropUpdate.h"

#include <daRg/dag_element.h>
#include <daRg/dag_properties.h>
#include <daRg/dag_stringKeys.h>

#include "guiScene.h"
#include "scriptUtil.h"


namespace darg
{


BhvRtPropUpdate bhv_rt_prop_update;


BhvRtPropUpdate::BhvRtPropUpdate() : Behavior(STAGE_BEFORE_RENDER, 0) {}


int BhvRtPropUpdate::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
  runForElem(elem);

  return 0;
}


void BhvRtPropUpdate::onAttach(Element *elem) { runForElem(elem); }


void BhvRtPropUpdate::onDetach(Element * /*elem*/, DetachMode) {}


void BhvRtPropUpdate::runForElem(Element *elem)
{
  Sqrat::Table src = elem->props.scriptDesc;
  Sqrat::Function updFunc = src.RawGetFunction(elem->csk->update);
  if (updFunc.IsNull())
    return;

  bool onlyWhenParentInScreen = src.RawGetSlotValue<bool>(elem->csk->onlyWhenParentInScreen, false);
  if (onlyWhenParentInScreen)
  {
    Element *parent = elem->getParent();
    if (!parent || parent->clippedScreenRect.isempty())
      return;
  }

  Sqrat::Table tblNew;
  if (!updFunc.Evaluate(tblNew))
    return;

  HSQUIRRELVM vm = src.GetVM();
  Sqrat::Object::iterator it;

  bool alwaysUpdate = src.RawGetSlotValue<bool>(elem->csk->rtAlwaysUpdate, false);

  bool haveChanges = false;
  while (tblNew.Next(it))
  {
    Sqrat::Object key(it.getKey(), vm);
    Sqrat::Object val(it.getValue(), vm);
    Sqrat::Object prevVal = src.RawGetSlot(key);

    if (alwaysUpdate || !val.IsEqual(prevVal))
    {
      src.SetValue(key, val);
      haveChanges = true;
    }
  }

  if (haveChanges)
  {
    GuiScene *guiScene = GuiScene::get_from_sqvm(vm);

    Component tmpComp;
    tmpComp.rendObjType = elem->rendObjType;
    tmpComp.scriptDesc = src;
    tmpComp.scriptBuilder = elem->props.scriptBuilder;
    tmpComp.behaviors = elem->behaviors;

    elem->setup(tmpComp, guiScene, SM_REALTIME_UPDATE);

    if (src.RawGetSlotValue<bool>(elem->csk->rtRecalcLayout, false))
      elem->recalcLayout();
  }
}


} // namespace darg
