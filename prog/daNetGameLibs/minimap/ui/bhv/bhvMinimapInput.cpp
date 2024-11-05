// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvMinimapInput.h"
#include "../minimapContext.h"
#include "../minimapState.h"

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_inputIds.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_guiScene.h>
#include <daRg/dag_properties.h>

#include <gui/dag_stdGuiRender.h>
#include <math/dag_mathUtils.h>
#include <math/dag_mathBase.h>
#include <perfMon/dag_cpuFreq.h>

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>

#include <sqrat.h>

#include <bindQuirrelEx/autoBind.h>
#include <bindQuirrelEx/sqratDagor.h>
#include <ecs/scripts/sqBindEvent.h>

#include <drv/hid/dag_hiXInputMappings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiPointingData.h>

ECS_BROADCAST_EVENT_TYPE(EventMinimapZoomed);
ECS_REGISTER_EVENT(EventMinimapZoomed);

using namespace darg;

SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvMinimapInput, bhv_minimap_input, cstr);

bool BhvMinimapInput::isActionInited = false;
dainput::action_handle_t BhvMinimapInput::aZoomIn, BhvMinimapInput::aZoomOut;


static HumanInput::IGenJoystick *get_joystick()
{
  if (::global_cls_composite_drv_joy)
    return ::global_cls_composite_drv_joy->getDefaultJoystick();
  else if (::global_cls_drv_joy)
    return ::global_cls_drv_joy->getDefaultJoystick();
  return nullptr;
}


struct MinimapInputTouchData
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


BhvMinimapInput::BhvMinimapInput() : Behavior(STAGE_ACT, F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_INTERNALLY_HANDLE_GAMEPAD_R_STICK) {}


void BhvMinimapInput::initActions()
{
  SETUP_ACTION_AXIS(ZoomIn, "HUD.");
  SETUP_ACTION_AXIS(ZoomOut, "HUD.");
  isActionInited = true;
}


void BhvMinimapInput::onAttach(Element *elem)
{
  if (!isActionInited)
    initActions();

  MinimapState *mmState = MinimapState::get_from_element(elem);
  if (mmState)
    mmState->setInteractive(true);

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );
  MinimapInputTouchData *touchData = new MinimapInputTouchData();
  elem->props.storage.SetValue(strings->minimapTouchData, touchData);
}


void BhvMinimapInput::onDetach(Element *elem, DetachMode)
{
  MinimapState *mmState = MinimapState::get_from_element(elem);
  if (mmState)
    mmState->setInteractive(false);

  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, );

  MinimapInputTouchData *touchData = elem->props.storage.RawGetSlotValue<MinimapInputTouchData *>(strings->minimapTouchData, nullptr);
  if (touchData)
  {
    elem->props.storage.DeleteSlot(strings->minimapTouchData);
    delete touchData;
  }
}


static void keep_cursor_world_pos(Element *elem, MinimapState &mmState, short mx, short my, float old_vis_radius, float vis_radius)
{
  const Point2 screenSize = elem->screenCoord.size;
  const Point2 screenPos = elem->screenCoord.screenPos;
  const float relX = (mx - screenPos.x) * safediv(2.f, screenSize.x) - 1.f;
  const float relY = (my - screenPos.y) * safediv(2.f, screenSize.y) - 1.f;

  float radiusDelta = vis_radius - old_vis_radius;
  Point2 worldDelta = Point2(relX, relY) * radiusDelta;

  TMatrix itm;
  mmState.calcItmFromView(itm, vis_radius);
  Point3 delta;
  if (mmState.isSquare)
    delta = itm % Point3(-worldDelta.y, 0, -worldDelta.x);
  else
    delta = itm % Point3(-worldDelta.x, 0, worldDelta.y);

  mmState.panWolrdPos = mmState.ctx->clampPan(mmState.panWolrdPos + delta, vis_radius);
}


static int get_mouse_pan_button(const darg::Element *elem, const BhvMinimapInput::CachedStringsMap &cstr)
{
  static constexpr int def_btn = 0;
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, def_btn);

  return elem->props.scriptDesc.RawGetSlotValue<int>(strings->panMouseButton, def_btn);
}


int BhvMinimapInput::mouseEvent(ElementTree *etree,
  Element *elem,
  InputDevice device,
  InputEvent event,
  int pointer_id,
  int data,
  short mx,
  short my,
  int /*buttons*/,
  int accum_res)
{
  IGuiScene *guiScene = get_scene_from_sqvm(elem->props.scriptDesc.GetVM());
  if (!guiScene)
    return 0;

  MinimapState *mmState = MinimapState::get_from_element(elem);

  if (!mmState || !mmState->ctx)
    return 0;

  if (event == INP_EV_MOUSE_WHEEL)
  {
    if (elem->hitTest(mx, my) && !(accum_res & R_PROCESSED))
    {
      float oldVisRadius = mmState->getVisibleRadius();
      float visRadius = oldVisRadius;
      const float stepMul = 1.1f;

      if (data > 0)
        visRadius /= stepMul;
      else
        visRadius *= stepMul;
      visRadius = mmState->setVisibleRadius(visRadius);

      g_entity_mgr->broadcastEvent(EventMinimapZoomed());

      keep_cursor_world_pos(elem, *mmState, mx, my, oldVisRadius, visRadius);

      return R_PROCESSED;
    }
    return 0;
  }
  else
    return pointingEvent(etree, elem, device, event, pointer_id, data, Point2(mx, my), accum_res);
}


int BhvMinimapInput::touchEvent(ElementTree *etree,
  Element *elem,
  InputEvent event,
  HumanInput::IGenPointing * /*pnt*/,
  int touch_idx,
  const HumanInput::PointingRawState::Touch &touch,
  int accum_res)
{
  return pointingEvent(etree, elem, DEVID_TOUCH, event, touch_idx, 0, Point2(touch.x, touch.y), accum_res);
}


int BhvMinimapInput::pointingEvent(
  ElementTree *, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id, const Point2 &pos, int accum_res)
{
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, 0);

  MinimapInputTouchData *touchData = elem->props.storage.RawGetSlotValue<MinimapInputTouchData *>(strings->minimapTouchData, nullptr);
  if (!touchData)
    return 0;

  MinimapState *mmState = MinimapState::get_from_element(elem);
  if (!mmState || !mmState->ctx)
    return 0;

  auto itTouch = eastl::find_if(touchData->touches.begin(), touchData->touches.end(),
    [device, pointer_id](const MinimapInputTouchData::Touch &t) { return t.deviceId == device && t.pointerId == pointer_id; });

  if (event == INP_EV_PRESS)
  {
    if (elem->hitTest(pos) && !(accum_res & R_PROCESSED))
    {
      if (touchData->touches.size() < 2 && itTouch == touchData->touches.end())
      {
        MinimapInputTouchData::Touch tdata;
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

    float visRadius = mmState->getVisibleRadius();
    TMatrix itm;
    mmState->calcItmFromView(itm, visRadius);

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
          mmState->setVisibleRadius(visRadius * curDist / prevDist);
          visRadius = mmState->getVisibleRadius();

          g_entity_mgr->broadcastEvent(EventMinimapZoomed());
        }
      }

      // move
      if (itTouch->deviceId != DEVID_MOUSE || (itTouch->buttonId == get_mouse_pan_button(elem, cstr)))
      {
        Point2 screenDelta = pos - itTouch->curPos;
        Point2 d(screenDelta.x * visRadius * 2 / elem->screenCoord.size.x, screenDelta.y * visRadius * 2 / elem->screenCoord.size.y);
        Point3 worldDelta;
        if (mmState->isSquare)
          worldDelta = itm % Point3(-d.y, 0, -d.x);
        else
          worldDelta = itm % Point3(-d.x, 0, d.y);

        mmState->panWolrdPos = mmState->ctx->clampPan(mmState->panWolrdPos + worldDelta / nTouches, visRadius);
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


int BhvMinimapInput::update(UpdateStage /*stage*/, Element *elem, float dt)
{
  using namespace HumanInput;

  IGenJoystick *joy = get_joystick();
  if (!joy)
    return 0;

  if (!(elem->getStateFlags() & Element::S_HOVER))
    return 0;

  MinimapState *mmState = MinimapState::get_from_element(elem);
  if (!mmState || !mmState->ctx)
    return 0;

  const dainput::AnalogAxisAction &zoomIn = dainput::get_analog_axis_action_state(aZoomIn);
  const dainput::AnalogAxisAction &zoomOut = dainput::get_analog_axis_action_state(aZoomOut);

  const float zoomVal = float(zoomIn.x - zoomOut.x);
  if (fabsf(zoomVal) > 1e-3f)
  {
    float visRadius = mmState->getVisibleRadius();
    visRadius = approach(visRadius, visRadius * powf(2, -zoomVal), dt, 0.2f);
    mmState->setVisibleRadius(visRadius);

    g_entity_mgr->broadcastEvent(EventMinimapZoomed());
  }

  {
    const float maxVisibleRadius = mmState->getMaxVisibleRadius();
    const float visRadius = mmState->getVisibleRadius();
    const float moveSpeed = 10.0f * (maxVisibleRadius > 0.f ? (visRadius / maxVisibleRadius) : 1.f);

    float axisValueX = float(joy->getAxisPosRaw(JOY_XINPUT_REAL_AXIS_R_THUMB_H)) / JOY_XINPUT_MAX_AXIS_VAL;
    float axisValueY = float(joy->getAxisPosRaw(JOY_XINPUT_REAL_AXIS_R_THUMB_V)) / JOY_XINPUT_MAX_AXIS_VAL;
    float worldDx = visRadius * axisValueX * dt * moveSpeed;
    float worldDy = -visRadius * axisValueY * dt * moveSpeed;
    TMatrix itm;
    mmState->calcItmFromView(itm, visRadius);
    Point3 delta;
    if (mmState->isSquare)
      delta = itm % Point3(worldDy, 0, worldDx);
    else
      delta = itm % Point3(worldDx, 0, -worldDy);

    mmState->panWolrdPos = mmState->ctx->clampPan(mmState->panWolrdPos + delta, visRadius);
  }

  return 0;
}

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_minimap, "bhvMinimap", sq::VM_UI_ALL)
{
  Sqrat::Table tbl = ecs::sq::EventsBind<EventMinimapZoomed>::bindall(vm);

  return tbl;
}