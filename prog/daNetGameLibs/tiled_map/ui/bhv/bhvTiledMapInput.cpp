// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../tiledMapContext.h"
#include "bhvTiledMapInput.h"
#include <daRg/dag_element.h>
#include <daRg/dag_guiScene.h>
#include <drv/hid/dag_hiXInputMappings.h>
#include <EASTL/fixed_vector.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <perfMon/dag_statDrv.h>

#include <ecs/scripts/sqBindEvent.h>


using namespace darg;

ECS_BROADCAST_EVENT_TYPE(EventTiledMapZoomed);
ECS_REGISTER_EVENT(EventTiledMapZoomed);

ECS_BROADCAST_EVENT_TYPE(EventTiledMapPanned);
ECS_REGISTER_EVENT(EventTiledMapPanned);

SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvTiledMapInput, bhv_tiled_map_input, cstr);

bool BhvTiledMapInput::isActionInited = false;
dainput::action_handle_t BhvTiledMapInput::aZoomIn, BhvTiledMapInput::aZoomOut;


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


BhvTiledMapInput::BhvTiledMapInput() :
  Behavior(STAGE_ACT, F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_HANDLE_TOUCHPAD_PAIR | F_INTERNALLY_HANDLE_GAMEPAD_R_STICK)
{}


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


// the world point under pos stays in place, unless the view follows the character
static void zoom_at(Element *elem, TiledMapContext *ctx, const Point2 &pos, float radius)
{
  const float oldRadius = ctx->getVisibleRadius();
  const Point3 posWorld = ctx->mapToWorld(pos - elem->clippedScreenRect.center());
  const Point3 oldDelta = ctx->getWorldPos() - posWorld;
  ctx->setVisibleRadius(radius);
  g_entity_mgr->broadcastEvent(EventTiledMapZoomed());

  // Most of the time equals to the requested step, but when we reach max or min visible radius =>
  // `newToOldRadiusRatio` is equals to 1
  if (!ctx->isViewCentered)
  {
    const float newToOldRadiusRatio = ctx->getVisibleRadius() / oldRadius;
    ctx->setWorldPos(posWorld + Point3(oldDelta.x * newToOldRadiusRatio, oldDelta.y, oldDelta.z * newToOldRadiusRatio));
  }
}


static void pan_by(Element *elem, TiledMapContext *ctx, const Point2 &screen_delta)
{
  const float visRadius = ctx->getVisibleRadius();
  TMatrix itm;
  ctx->calcItmFromView(itm);
  const float minDim = eastl::min(elem->screenCoord.size.x, elem->screenCoord.size.y);
  const Point2 d(screen_delta.x * visRadius * 2.f / minDim, screen_delta.y * visRadius * 2.f / minDim);
  ctx->setWorldPos(ctx->getWorldPos() + itm % Point3(-d.x, 0, -d.y));
  if (ctx->isViewCentered)
    ctx->setViewCentered(false);
  g_entity_mgr->broadcastEvent(EventTiledMapPanned());
}


// two pointers move and scale the map: the world point under their center follows it, the scale follows their distance
static void move_and_scale(Element *elem, TiledMapContext *ctx, const Point2 (&prev)[2], const Point2 (&cur)[2])
{
  const Point2 prevCenter = (prev[0] + prev[1]) * 0.5f;
  const Point2 centerDelta = (cur[0] + cur[1]) * 0.5f - prevCenter;
  const float prevDist = length(prev[1] - prev[0]), curDist = length(cur[1] - cur[0]);
  if (prevDist > 20 && curDist > 20 && prevDist != curDist)
    zoom_at(elem, ctx, prevCenter, ctx->getVisibleRadius() * prevDist / curDist);
  if (centerDelta != Point2(0, 0))
    pan_by(elem, ctx, centerDelta);
}


int BhvTiledMapInput::pointingEvent(ElementTree *,
  Element *elem,
  InputDevice device,
  TouchSource /*touch_src*/,
  InputEvent event,
  int pointer_id,
  int button_id,
  Point2 pos,
  int accum_res)
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
      int step = button_id; // special case
      const float zoomStep = step > 0 ? 0.9f : 1.1f;
      zoom_at(elem, tiledMapContext, pos, tiledMapContext->getVisibleRadius() * zoomStep);
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

    auto idx = eastl::distance(touchData->touches.begin(), itTouch);
    G_ASSERTF_RETURN(idx == 0 || idx == 1, R_PROCESSED, "Distance = %d", int(idx));

    if (!(accum_res & R_PROCESSED))
    {
      if (touchData->touches.size() == 2)
      {
        const Point2 prevScreen[2] = {touchData->touches[0].curPos, touchData->touches[1].curPos};
        Point2 curScreen[2] = {prevScreen[0], prevScreen[1]};
        curScreen[idx] = pos;
        move_and_scale(elem, tiledMapContext, prevScreen, curScreen);
      }
      else if (itTouch->deviceId != DEVID_MOUSE || (itTouch->buttonId == get_mouse_pan_button(elem, cstr)))
        pan_by(elem, tiledMapContext, pos - itTouch->curPos);
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

  IGuiScene *scene = get_scene_from_sqvm(elem->getVM());
  if (!scene->getJoystick())
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

    auto strings = cstr.resolveVm(elem->getVM());
    G_ASSERT_RETURN(strings, 0);
    const Point2 stick = scene->getGamepadStick(JOY_XINPUT_REAL_AXIS_R_THUMB_H, JOY_XINPUT_REAL_AXIS_R_THUMB_V,
      elem->props.scriptDesc.RawGetSlotValue<float>(strings->minStickDeadZone, 0.f));
    float worldDx = visRadius * stick.x * dt * moveSpeed;
    float worldDy = -visRadius * stick.y * dt * moveSpeed;
    TMatrix itm;
    tiledMapContext->calcItmFromView(itm);
    Point3 delta;
    delta = itm % Point3(worldDx, 0, worldDy);
    if (delta != Point3::ZERO)
    {
      tiledMapContext->setWorldPos(tiledMapContext->getWorldPos() + delta);
      if (tiledMapContext->isViewCentered)
        tiledMapContext->setViewCentered(false);
      g_entity_mgr->broadcastEvent(EventTiledMapPanned());
    }
  }

  return 0;
}

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_tiled_map_input_events, "tiledMap.inputEvents", sq::VM_UI)
{
  Sqrat::Table tbl = ecs::sq::EventsBind<EventTiledMapZoomed, EventTiledMapPanned>::bindall(vm);

  return tbl;
}