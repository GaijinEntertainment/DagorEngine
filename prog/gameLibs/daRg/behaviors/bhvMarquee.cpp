// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvMarquee.h"

#include <daRg/dag_element.h>
#include <daRg/dag_stringKeys.h>

#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>


namespace darg
{


BhvMarquee bhv_marquee;


const float BhvMarqueeData::defSpeed = 50.0f;

static const char *dataSlotName = "marquee:data";


BhvMarquee::BhvMarquee() : Behavior(STAGE_BEFORE_RENDER, 0) {}


void BhvMarquee::onAttach(Element *)
{
  // initialization was moved onElemSetup
}


void BhvMarquee::onDetach(Element *elem, DetachMode)
{
  BhvMarqueeData *bhvData = elem->props.storage.RawGetSlotValue<BhvMarqueeData *>(dataSlotName, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(dataSlotName);
    delete bhvData;
  }
}


void BhvMarquee::onElemSetup(Element *elem, SetupMode setup_mode)
{
  BhvMarqueeData *bhvData = nullptr;
  if (setup_mode == SM_REBUILD_UPDATE)
    bhvData = elem->props.storage.RawGetSlotValue<BhvMarqueeData *>(dataSlotName, nullptr);

  if (setup_mode == SM_INITIAL || (setup_mode == SM_REBUILD_UPDATE && !bhvData))
  {
    bhvData = new BhvMarqueeData();
    elem->props.storage.SetValue(dataSlotName, bhvData);
  }
  G_ASSERT(setup_mode == SM_INITIAL || setup_mode == SM_REBUILD_UPDATE);
  G_ASSERT_RETURN(bhvData, );

  bhvData->loop = elem->props.getBool(elem->csk->loop, false);
  bhvData->threshold = elem->props.getFloat(elem->csk->threshold, 5.0f);
  bhvData->orient = elem->props.getInt<Orientation>(elem->csk->orientation, O_HORIZONTAL);
  bhvData->scrollOnHover = elem->props.getBool(elem->csk->scrollOnHover, false, true);

  Sqrat::Object objSpeed = elem->props.getObject(elem->csk->speed);
  if (objSpeed.GetType() == OT_ARRAY)
    bhvData->scrollSpeed = objSpeed.RawGetSlotValue(SQInteger(0), BhvMarqueeData::defSpeed);
  else
    bhvData->scrollSpeed = objSpeed.Cast<float>();

  if (bhvData->scrollSpeed <= 0.0f)
    bhvData->scrollSpeed = BhvMarqueeData::defSpeed;

  Sqrat::Object delayObj = elem->props.getObject(elem->csk->delay);
  if (delayObj.GetType() == OT_ARRAY)
  {
    Sqrat::Array delayArr(delayObj);
    SQInteger len = delayArr.Length();
    bhvData->initialDelay = delayArr.RawGetSlotValue<float>(SQInteger(0), 1.0f);
    bhvData->fadeOutDelay = len > 1 ? delayArr.RawGetSlotValue<float>(SQInteger(1), 1.0f) : bhvData->initialDelay;
  }
  else
  {
    float val = (delayObj.GetType() & SQOBJECT_NUMERIC) ? delayObj.Cast<float>() : 1.0f;
    bhvData->initialDelay = bhvData->fadeOutDelay = val;
  }

  if (setup_mode == SM_INITIAL)
    bhvData->resetPhase();
}


int BhvMarquee::update(UpdateStage /*stage*/, darg::Element *elem, float dt)
{
  TIME_PROFILE(bhv_marquee_update);

  BhvMarqueeData *bhvData = elem->props.storage.RawGetSlotValue<BhvMarqueeData *>(dataSlotName, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  int axis = (bhvData->orient == O_HORIZONTAL) ? 0 : 1;
  ScreenCoord &sc = elem->screenCoord;
  Point2 scrollRange = elem->getScrollRange(axis);

  if (/*!bhvData->loop && */ fabsf(scrollRange[0] - scrollRange[1]) <= bhvData->threshold)
  {
    bhvData->resetPhase();
    elem->props.resetCurrentOpacity();
    return 0;
  }

  // to keep position on recalc
  if (bhvData->loop)
  {
    if (bhvData->haveSavedPos)
      sc.scrollOffs[axis] = bhvData->pos;
    else
      sc.scrollOffs[axis] = scrollRange[0] - sc.size[axis];
  }

  if (bhvData->scrollOnHover && !(elem->getStateFlags() & Element::S_HOVER))
  {
    bhvData->resetPhase();
    sc.scrollOffs[axis] = scrollRange[0];
    elem->props.resetCurrentOpacity();
    return 0;
  }

  if (bhvData->phase == PHASE_INITIAL_DELAY)
  {
    bhvData->delay -= dt;
    if (bhvData->delay <= 0)
      bhvData->resetPhase(PHASE_MOVE, 0);

    if (bhvData->loop)
      sc.scrollOffs[axis] = scrollRange[0] - sc.size[axis];
    else
      sc.scrollOffs[axis] = scrollRange[0];
  }
  else if (bhvData->phase == PHASE_MOVE)
  {
    float delta = bhvData->scrollSpeed * dt;
    sc.scrollOffs[axis] += delta;
    float endPos = bhvData->loop ? scrollRange[0] + sc.contentSize[axis] : scrollRange[1];
    if (sc.scrollOffs[axis] >= endPos)
    {
      sc.scrollOffs[axis] = endPos;
      if (bhvData->loop)
        bhvData->resetPhase();
      else
        bhvData->resetPhase(PHASE_FADE_OUT, bhvData->fadeOutDelay);
    }
  }
  else if (bhvData->phase == PHASE_FADE_OUT)
  {
    bhvData->delay -= dt;

    if (bhvData->delay <= 0)
    {
      bhvData->resetPhase();
      sc.scrollOffs[axis] = scrollRange[0];
      elem->props.resetCurrentOpacity();
    }
    else
      elem->props.setCurrentOpacity(::clamp(bhvData->delay / ::max(bhvData->fadeOutDelay, 0.01f), 0.0f, 1.0f));
  }
  else
    G_ASSERT(!"Unexpected phase");

  // to keep position on recalc
  if (bhvData->loop)
  {
    bhvData->pos = sc.scrollOffs[axis];
    bhvData->haveSavedPos = true;
  }

  return 0;
}


} // namespace darg
