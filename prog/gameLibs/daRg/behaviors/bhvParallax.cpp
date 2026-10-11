// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvParallax.h"

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_stringKeys.h>
#include <drv/hid/dag_hiJoystick.h>
#include <startup/dag_inpDevClsDrv.h>
#include <osApiWrappers/dag_critSec.h>

#include "guiScene.h"


namespace darg
{


BhvParallax bhv_parallax;

static constexpr float GYRO_TURN_DEG_FOR_FULL_SHIFT = 15.f;
static constexpr float GYRO_SHIFT_RETURN_TIME_SEC = 0.5f;


BhvParallax::BhvParallax() : Behavior(STAGE_BEFORE_RENDER, 0) {}


void BhvParallax::onAttach(Element *elem) { elem->props.storage.SetValue(elem->csk->parallaxData, new BhvParallaxData()); }


void BhvParallax::onDetach(Element *elem, DetachMode)
{
  BhvParallaxData *data = elem->props.storage.RawGetSlotValue<BhvParallaxData *>(elem->csk->parallaxData, nullptr);
  if (data)
  {
    elem->props.storage.DeleteSlot(elem->csk->parallaxData);
    delete data;
  }
}


static Point2 follow_gyro(const HumanInput::IGenJoystick *joy, Point2 shift, float dt)
{
  shift *= expf(-dt / GYRO_SHIFT_RETURN_TIME_SEC);
  HumanInput::JoystickMotionState motion;
  {
    WinAutoLock lock(global_cls_drv_update_cs);
    if (!joy || !joy->getMotionState(motion))
      return shift;
  }
  shift.x -= (motion.angVelDegS[1] + motion.angVelDegS[2]) * dt / GYRO_TURN_DEG_FOR_FULL_SHIFT;
  shift.y -= motion.angVelDegS[0] * dt / GYRO_TURN_DEG_FOR_FULL_SHIFT;
  return Point2(::clamp(shift.x, -1.f, 1.f), ::clamp(shift.y, -1.f, 1.f));
}


int BhvParallax::update(UpdateStage /*stage*/, darg::Element *elem, float dt)
{
  if (!elem->transform)
    return 0;

  GuiScene *scene = GuiScene::get_from_elem(elem);

  float halfW = StdGuiRender::screen_width() / 2;
  float halfH = StdGuiRender::screen_height() / 2;

  Point2 shift = scene->activePointerPos() - Point2(halfW, halfH);
  if (BhvParallaxData *data = elem->props.storage.RawGetSlotValue<BhvParallaxData *>(elem->csk->parallaxData, nullptr))
  {
    data->gyroShift = follow_gyro(scene->getJoystick(), data->gyroShift, dt);
    shift += Point2(data->gyroShift.x * halfW, data->gyroShift.y * halfH);
  }
  Point2 rel(::clamp(shift.x, -halfW, halfW), ::clamp(shift.y, -halfH, halfH));

  float k = elem->props.getFloat(elem->csk->parallaxK, 0.0f);
  elem->transform->translate = rel * k;

  return 0;
}


} // namespace darg
