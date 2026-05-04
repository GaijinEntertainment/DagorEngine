// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../tiledMapContext.h"
#include "bhvTiledMapInput.h"
#include <daRg/dag_element.h>
#include <daRg/dag_guiScene.h>
#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <EASTL/fixed_vector.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <perfMon/dag_statDrv.h>
#include <startup/dag_inpDevClsDrv.h>

#include <ecs/scripts/sqBindEvent.h>


using namespace darg;

ECS_BROADCAST_EVENT_TYPE(EventTiledMapZoomed);
ECS_REGISTER_EVENT(EventTiledMapZoomed);

SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvTiledMapInput, bhv_tiled_map_input, cstr);

bool BhvTiledMapInput::isActionInited = false;
dainput::action_handle_t BhvTiledMapInput::aZoomIn, BhvTiledMapInput::aZoomOut;


static HumanInput::IGenJoystick *get_joystick()
{
  if (::global_cls_composite_drv_joy)
    return ::global_cls_composite_drv_joy->getDefaultJoystick();
  else if (::global_cls_drv_joy)
    return ::global_cls_drv_joy->getDefaultJoystick();
  return nullptr;
}


struct TiledMapInputTouchData
{
  struct Touch
  {
    InputDevice deviceId = DEVID_NONE;
    int pointerId = -1;
    int buttonId = -1;
    Point2 startPos = Point2(0, 0);
    Point2 curPos = Point2(0, 0);
  };
  eastl::fixed_vector<Touch, 2, false> touches;
};


BhvTiledMapInput::BhvTiledMapInput() : Behavior(STAGE_ACT, F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_INTERNALLY_HANDLE_GAMEPAD_R_STICK) {}


void BhvTiledMapInput::initActions()
{
  SETUP_ACTION_AXIS(ZoomIn, "HUD.");
  SETUP_ACTION_AXIS(ZoomOut, "HUD.");
  isActionInited = true;
}


void BhvTiledMapInput::onAttach(Element *elem)
{
  if (!isActionInited)
    initActions();

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );
  TiledMapInputTouchData *touchData = new TiledMapInputTouchData();
  elem->props.storage.SetValue(strings->tiledMapTouchData, touchData);
}


void BhvTiledMapInput::onDetach(Element *elem, DetachMode)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  TiledMapContext *tiledMapContext = TiledMapContext::get_from_element(elem);
  if (tiledMapContext)
  {
    const bool enableIsViewCentered = elem->props.scriptDesc.RawGetSlotValue<bool>(strings->enableIsViewCenteredOnExit, false);
    if (enableIsViewCentered)
      tiledMapContext->setViewCentered(enableIsViewCentered);
  }

  TiledMapInputTouchData *touchData =
    elem->props.storage.RawGetSlotValue<TiledMapInputTouchData *>(strings->tiledMapTouchData, nullptr);
  if (touchData)
  {
    elem->props.storage.DeleteSlot(strings->tiledMapTouchData);
    delete touchData;
  }
}


static int get_mouse_pan_button(const darg::Element *elem, const BhvTiledMapInput::CachedStringsMap &cstr)
{
  static constexpr int def_btn = 0;
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, def_btn);

  return elem->props.scriptDesc.RawGetSlotValue<int>(strings->panMouseButton, def_btn);
}


int BhvTiledMapInput::pointingEvent(
  ElementTree *, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id, Point2 pos, int accum_res)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  TiledMapInputTouchData *touchData =
    elem->props.storage.RawGetSlotValue<TiledMapInputTouchData *>(strings->tiledMapTouchData, nullptr);
  if (!touchData)
    return 0;

  TiledMapContext *tiledMapContext = TiledMapContext::get_from_element(elem);
  if (!tiledMapContext)
    return 0;

  if (event == INP_EV_MOUSE_WHEEL)
  {
    if (elem->hitTest(pos) && !(accum_res & R_PROCESSED))
    {
      const float oldRadius = tiledMapContext->getVisibleRadius();
      const Point2 mouseMapPosition = Point2(pos.x - (elem->clippedScreenRect.left() + elem->clippedScreenRect.size().x / 2),
        pos.y - (elem->clippedScreenRect.top() + elem->clippedScreenRect.size().y / 2));
      const Point3 mouseWorldPosition = tiledMapContext->mapToWorld(mouseMapPosition);
      const Point3 oldDelta = tiledMapContext->getWorldPos() - mouseWorldPosition;

      int step = button_id; // special case
      const float zoomStep = step > 0 ? 0.9f : 1.1f;
      tiledMapContext->setVisibleRadius(oldRadius * zoomStep);
      g_entity_mgr->broadcastEvent(EventTiledMapZoomed());

      // Most of the time equals to `zoomStep`, but when we reach max or min visible radius =>
      // `newToOldRadiusRatio` is equals to 1 when `zoomStep` could be not equal to 1

      if (!tiledMapContext->isViewCentered)
      {
        const float newToOldRadiusRatio = tiledMapContext->getVisibleRadius() / oldRadius;
        const Point3 delta = Point3(oldDelta.x * newToOldRadiusRatio, oldDelta.y, oldDelta.z * newToOldRadiusRatio);
        tiledMapContext->setWorldPos(mouseWorldPosition + delta);
      }
      return R_PROCESSED;
    }
    return 0;
  }


  auto itTouch = eastl::find_if(touchData->touches.begin(), touchData->touches.end(),
    [device, pointer_id](const TiledMapInputTouchData::Touch &t) { return t.deviceId == device && t.pointerId == pointer_id; });

  if (event == INP_EV_PRESS)
  {
    if (elem->hitTest(pos) && !(accum_res & R_PROCESSED))
    {
      if (touchData->touches.size() < 2 && itTouch == touchData->touches.end())
      {
        TiledMapInputTouchData::Touch tdata;
        tdata.deviceId = device;
        tdata.pointerId = pointer_id;
        tdata.buttonId = button_id;
        tdata.startPos = pos;
        tdata.curPos = pos;
        touchData->touches.push_back(tdata);
      }
      return R_PROCESSED;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (itTouch == touchData->touches.end())
      return 0;

    float visRadius = tiledMapContext->getVisibleRadius();
    TMatrix itm;
    tiledMapContext->calcItmFromView(itm);

    auto idx = eastl::distance(touchData->touches.begin(), itTouch);
    G_ASSERTF_RETURN(idx == 0 || idx == 1, R_PROCESSED, "Distance = %d", int(idx));

    const size_t nTouches = touchData->touches.size();

    if (!(accum_res & R_PROCESSED))
    {
      // zoom
      if (nTouches == 2)
      {
        Point2 prevScreen[2] = {touchData->touches[0].curPos, touchData->touches[1].curPos};
        Point2 curScreen[2] = {touchData->touches[0].curPos, touchData->touches[1].curPos};
        curScreen[idx] = pos;
        float prevDist = length(curScreen[1] - curScreen[0]);
        float curDist = length(prevScreen[1] - prevScreen[0]);
        if (prevDist > 20 && curDist > 20)
        {
          tiledMapContext->setVisibleRadius(visRadius * curDist / prevDist);
          visRadius = tiledMapContext->getVisibleRadius();

          g_entity_mgr->broadcastEvent(EventTiledMapZoomed());
        }
      }

      // move
      if (itTouch->deviceId != DEVID_MOUSE || (itTouch->buttonId == get_mouse_pan_button(elem, cstr)))
      {
        Point2 screenDelta = pos - itTouch->curPos;
        float minDim = eastl::min(elem->screenCoord.size.x, elem->screenCoord.size.y);
        Point2 d(screenDelta.x * visRadius * 2.f / minDim, screenDelta.y * visRadius * 2.f / minDim);
        Point3 worldDelta;
        worldDelta = itm % Point3(-d.x, 0, -d.y);

        tiledMapContext->setWorldPos(tiledMapContext->getWorldPos() + worldDelta / nTouches);
        if (tiledMapContext->isViewCentered)
          tiledMapContext->setViewCentered(false);
      }
    }

    itTouch->curPos = pos;

    return R_PROCESSED;
  }
  else if (event == INP_EV_RELEASE)
  {
    if (itTouch == touchData->touches.end())
      return 0;
    if (itTouch->buttonId == button_id)
      touchData->touches.erase(itTouch);
    return R_PROCESSED;
  }

  return 0;
}


int BhvTiledMapInput::update(UpdateStage /*stage*/, Element *elem, float dt)
{
  TIME_PROFILE(BhvTiledMapInput_update);

  using namespace HumanInput;

  IGenJoystick *joy = get_joystick();
  if (!joy)
    return 0;

  if (!(elem->getStateFlags() & Element::S_HOVER))
    return 0;

  TiledMapContext *tiledMapContext = TiledMapContext::get_from_element(elem);
  if (!tiledMapContext)
    return 0;

  const dainput::AnalogAxisAction &zoomIn = dainput::get_analog_axis_action_state(aZoomIn);
  const dainput::AnalogAxisAction &zoomOut = dainput::get_analog_axis_action_state(aZoomOut);

  const float zoomVal = float(zoomIn.x - zoomOut.x);
  if (fabsf(zoomVal) > 1e-3f)
  {
    float visRadius = tiledMapContext->getVisibleRadius();
    visRadius = approach(visRadius, visRadius * powf(2, -zoomVal), dt, 0.2f);
    tiledMapContext->setVisibleRadius(visRadius);

    g_entity_mgr->broadcastEvent(EventTiledMapZoomed());
  }

  {
    const float maxVisibleRadius = tiledMapContext->worldVisibleRadiusRange.y;
    const float visRadius = tiledMapContext->getVisibleRadius();
    const float moveSpeed = 10.0f * (maxVisibleRadius > 0.f ? (visRadius / maxVisibleRadius) : 1.f);

    float axisValueX = float(joy->getAxisPosRaw(JOY_XINPUT_REAL_AXIS_R_THUMB_H)) / JOY_XINPUT_MAX_AXIS_VAL;
    float axisValueY = float(joy->getAxisPosRaw(JOY_XINPUT_REAL_AXIS_R_THUMB_V)) / JOY_XINPUT_MAX_AXIS_VAL;
    float worldDx = visRadius * axisValueX * dt * moveSpeed;
    float worldDy = -visRadius * axisValueY * dt * moveSpeed;
    TMatrix itm;
    tiledMapContext->calcItmFromView(itm);
    Point3 delta;
    delta = itm % Point3(worldDx, 0, worldDy);
    if (delta != Point3::ZERO)
    {
      tiledMapContext->setWorldPos(tiledMapContext->getWorldPos() + delta);
      if (tiledMapContext->isViewCentered)
        tiledMapContext->setViewCentered(false);
    }
  }

  return 0;
}

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_tiled_map_input_events, "tiledMap.inputEvents", sq::VM_UI_ALL)
{
  Sqrat::Table tbl = ecs::sq::EventsBind<EventTiledMapZoomed>::bindall(vm);

  return tbl;
}