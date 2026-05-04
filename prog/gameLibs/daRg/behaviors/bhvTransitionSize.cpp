// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvTransitionSize.h"

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_stringKeys.h>

#include <perfMon/dag_cpuFreq.h>

#include "guiScene.h"
#include "dargDebugUtils.h"


namespace darg
{


BhvTransitionSize bhv_transition_size;

static const char *dataSlotName = "size_trans:data";


void BhvTransitionSizeData::load(Element *elem, bool initial)
{
  orientation = elem->props.getInt<Orientation>(elem->csk->orientation, Orientation(O_HORIZONTAL | O_VERTICAL));
  speed = ::max(1.0f, elem->props.getFloat(elem->csk->speed, 100.0f));
  maxTime = elem->props.getFloat(elem->csk->maxTime, 0.0f);
  if (initial)
    wasErrorReported = false;
}


BhvTransitionSize::BhvTransitionSize() : Behavior(STAGE_BEFORE_RENDER, 0) {}


void BhvTransitionSize::onAttach(Element *elem)
{
  BhvTransitionSizeData *bhvData = new BhvTransitionSizeData();
  elem->props.storage.SetValue(dataSlotName, bhvData);
  bhvData->load(elem, true);
}


void BhvTransitionSize::onDetach(Element *elem, DetachMode)
{
  BhvTransitionSizeData *bhvData = elem->props.storage.RawGetSlotValue<BhvTransitionSizeData *>(dataSlotName, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(dataSlotName);
    delete bhvData;
  }
}

void BhvTransitionSize::onElemSetup(Element *elem, SetupMode setup_mode)
{
  BhvTransitionSizeData *bhvData = elem->props.storage.RawGetSlotValue<BhvTransitionSizeData *>(dataSlotName, nullptr);
  if (!bhvData)
    return;

  if (setup_mode == SM_INITIAL || setup_mode == SM_REBUILD_UPDATE)
    bhvData->load(elem, setup_mode == SM_INITIAL);
}


void BhvTransitionSize::onRecalcLayout(Element *) {}


static void convert_flex_size_to_parent_relative(SizeSpec *tgtSizeSpec, Element *elem)
{
  // We cannot calculate FLEX here, but some of them can be expressed as parent-relative sizes
  if (Element *parent = elem->parent)
  {
    for (int axis = 0; axis < 2; ++axis)
    {
      SizeSpec &ss = tgtSizeSpec[axis];

      if (ss.mode == SizeSpec::FLEX && (!parent->isFlowAxis(axis) || parent->children.size() == 1))
      {
        ss.mode = (axis == 0) ? SizeSpec::PARENT_W : SizeSpec::PARENT_H;
        ss.value = 100;
      }
    }
  }
}

int BhvTransitionSize::update(UpdateStage /*stage*/, darg::Element *elem, float dt)
{
  BhvTransitionSizeData *bhvData = elem->props.storage.RawGetSlotValue<BhvTransitionSizeData *>(dataSlotName, nullptr);

  if (!bhvData)
    return 0;

  ScreenCoord &sc = elem->screenCoord;

  Point2 targetSize = sc.contentSize;

  Sqrat::Object sqTargetSize = elem->props.getObject(elem->csk->targetSize);
  SizeSpec tgtSizeSpec[2];

  const char *errMsg = nullptr;
  if (!sqTargetSize.IsNull() && Layout::read_size(sqTargetSize, tgtSizeSpec, &errMsg))
  {
    convert_flex_size_to_parent_relative(tgtSizeSpec, elem);

    if ((tgtSizeSpec[0].mode == SizeSpec::FLEX || tgtSizeSpec[1].mode == SizeSpec::FLEX) && !bhvData->wasErrorReported)
    {
      bhvData->wasErrorReported = true;
      darg_assert_trace_var("Size cannot be transitioned to FLEX", elem->props.scriptDesc, elem->csk->targetSize);
    }

    targetSize.x = elem->sizeSpecToPixels(tgtSizeSpec[0], 0);
    targetSize.y = elem->sizeSpecToPixels(tgtSizeSpec[1], 1);
  }

  if (!bhvData->isCurSizeValid())
    bhvData->currentSize = targetSize;
  else
  {
    if (bhvData->orientation & O_HORIZONTAL)
    {
      float fullDeltaX = fabsf(targetSize.x - bhvData->currentSize.x);
      if (fullDeltaX < 0.1f)
        bhvData->minSpeed.x = 0.f;
      else
      {
        if (bhvData->maxTime > 0.f)
          bhvData->minSpeed.x = ::max(bhvData->minSpeed.x, fullDeltaX / bhvData->maxTime);
        float maxDelta = dt * ::max(bhvData->speed, bhvData->minSpeed.x);
        float delta = ::clamp(targetSize.x - sc.size.x, -maxDelta, maxDelta);
        bhvData->currentSize.x += delta;
      }
    }

    if (bhvData->orientation & O_VERTICAL)
    {
      float fullDeltaY = fabsf(targetSize.y - bhvData->currentSize.y);
      if (fullDeltaY < 0.1f)
        bhvData->minSpeed.y = 0.f;
      else
      {
        if (bhvData->maxTime > 0.f)
          bhvData->minSpeed.y = ::max(bhvData->minSpeed.y, fullDeltaY / bhvData->maxTime);
        float maxDelta = dt * ::max(bhvData->speed, bhvData->minSpeed.y);
        float delta = ::clamp(targetSize.y - sc.size.y, -maxDelta, maxDelta);
        bhvData->currentSize.y += delta;
      }
    }
  }

  bool needRecalc = false;

  if ((bhvData->orientation & O_HORIZONTAL) && fabsf(bhvData->currentSize.x - sc.size.x) >= 0.5f)
  {
    needRecalc = true;
    elem->layout.size[0].mode = SizeSpec::PIXELS;
    elem->layout.size[0].value = bhvData->currentSize.x;
  }

  if ((bhvData->orientation & O_VERTICAL) && fabsf(bhvData->currentSize.y - sc.size.y) >= 0.5f)
  {
    needRecalc = true;
    elem->layout.size[1].mode = SizeSpec::PIXELS;
    elem->layout.size[1].value = bhvData->currentSize.y;
  }

  if (needRecalc)
    elem->recalcLayout();

  return 0;
}


} // namespace darg
