#include "bhvTransitionSize.h"

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_stringKeys.h>

#include <perfMon/dag_cpuFreq.h>

#include "guiScene.h"


namespace darg
{


BhvTransitionSize bhv_transition_size;

static const char *dataSlotName = "size_trans:data";


BhvTransitionSize::BhvTransitionSize() : Behavior(STAGE_BEFORE_RENDER, 0) {}


void BhvTransitionSize::onAttach(Element *elem)
{
  BhvTransitionSizeData *bhvData = new BhvTransitionSizeData();
  elem->props.storage.SetValue(dataSlotName, bhvData);
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
  {
    bhvData->orientation = elem->props.getInt<Orientation>(elem->csk->orientation, Orientation(O_HORIZONTAL | O_VERTICAL));
    bhvData->speed = ::max(1.0f, elem->props.getFloat(elem->csk->speed, 100.0f));
  }
}


void BhvTransitionSize::onRecalcLayout(Element *) {}


int BhvTransitionSize::update(UpdateStage /*stage*/, darg::Element *elem, float dt)
{
  BhvTransitionSizeData *bhvData = elem->props.storage.RawGetSlotValue<BhvTransitionSizeData *>(dataSlotName, nullptr);

  if (!bhvData)
    return 0;

  float maxDelta = dt * bhvData->speed;

  ScreenCoord &sc = elem->screenCoord;
  bool needRecalc = false;

  if (!bhvData->isCurSizeValid())
    bhvData->currentSize = sc.contentSize;

  if ((bhvData->orientation & O_HORIZONTAL) && fabsf(bhvData->currentSize.x - sc.contentSize.x) >= 0.1f)
  {
    float delta = ::clamp(sc.contentSize.x - sc.size.x, -maxDelta, maxDelta);
    bhvData->currentSize.x += delta;
  }

  if ((bhvData->orientation & O_VERTICAL) && fabsf(bhvData->currentSize.y - sc.contentSize.y) >= 0.1f)
  {
    float delta = ::clamp(sc.contentSize.y - sc.size.y, -maxDelta, maxDelta);
    bhvData->currentSize.y += delta;
  }

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
