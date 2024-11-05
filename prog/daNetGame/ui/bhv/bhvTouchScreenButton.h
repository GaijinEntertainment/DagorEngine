// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <daInput/input_api.h>
#include "ui/scriptStrings.h"

struct TouchButtonState
{
  eastl::vector<dainput::action_handle_t> ahs;
  eastl::vector<dainput::action_handle_t> releaseAhsOnPress;
  eastl::vector<dainput::action_handle_t> blockingAction;
  eastl::vector<dainput::action_handle_t> delayedAhs;

  int touchIdx = -1;
  bool clickThrough = false;
  Sqrat::Function forceResendAh;
  Sqrat::Function onTouchBegin;
  Sqrat::Function onTouchEnd;

  float delayedActivationTime = FLT_MAX;
  enum
  {
    TB_UNKWN,
    TB_MIN,
    TB_MINX = TB_MIN,
    TB_MAX,
    TB_MAXX = TB_MAX,
    TB_MINY,
    TB_MAXY,
    TB_TOGGLE,
    TB_HOLD
  } type = TB_UNKWN;
};

class BhvTouchScreenButton : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    buttonState,
    actionDelay,
    delayedActionAfterRelease,
    action,
    releaseOnPressAction,
    blockingAction,
    delayedActions);

  BhvTouchScreenButton();

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
  bool updateAction(TouchButtonState *buttonState, darg::Element *elem, const CachedStrings *strings);

  virtual bool isAnyAhPressed(const eastl::vector<dainput::action_handle_t> &ahs);
  virtual void releaseAh(const eastl::vector<dainput::action_handle_t> &actions);
  virtual void pressAh(TouchButtonState *buttonState, const eastl::vector<dainput::action_handle_t> &actions);
  virtual void handlePress(TouchButtonState *buttonState);
  virtual void handleRelease(TouchButtonState *buttonState);

private:
  void initClickThrough(TouchButtonState *, darg::Element *) const;
  bool addAction(const eastl::string &action, eastl::vector<dainput::action_handle_t> &vec);
  bool addMultipleActions(eastl::string objActions, eastl::vector<dainput::action_handle_t> &vec);
};


extern BhvTouchScreenButton bhv_touch_screen_button;
