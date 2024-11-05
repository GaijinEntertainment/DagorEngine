// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <math/dag_Point2.h>
#include <daInput/input_api.h>
#include "ui/scriptStrings.h"

struct TouchStickState
{
  dainput::action_handle_t ah = dainput::BAD_ACTION_HANDLE;
  dainput::action_handle_t vh = dainput::BAD_ACTION_HANDLE;
  dainput::action_handle_t hh = dainput::BAD_ACTION_HANDLE;
  bool isStick = true;
  bool useDeltaMove = false;
  Point2 prevPos = Point2();
  Point2 accumulation = Point2();
  bool acceleration = false;
  float accelerationMod = 1.f;
  float deadZone = 0.0001f;
  int prevTime = 0;
  int touchIdx = -1;
  float range = 1000.0f;
  float valueScale = 1.0f;
  float stickFreezeMinVal = 0.0f;
};


class BhvTouchScreenStick : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    action,
    range,
    valueScale,
    useDeltaMove,
    deadZone,
    accelerationMod,
    acceleration,
    stickFreezeMinVal,
    stickState);

  BhvTouchScreenStick();

  virtual void onAttach(darg::Element *) override;
  virtual void onDetach(darg::Element *, DetachMode) override;
  virtual int update(UpdateStage, darg::Element *elem, float /*dt*/) override final;

  virtual int touchEvent(darg::ElementTree *,
    darg::Element *,
    darg::InputEvent /*event*/,
    HumanInput::IGenPointing * /*pnt*/,
    int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/,
    int /*accum_res*/) override;

protected:
  bool updateAction(TouchStickState *stickState, darg::Element *elem, const CachedStrings *strings);
  void setActionValue(TouchStickState *stick_state, Point2 value);
  void updateStickValues(TouchStickState *stickState, darg::Element *elem, const CachedStrings *strings);
};


extern BhvTouchScreenStick bhv_touch_screen_stick;
