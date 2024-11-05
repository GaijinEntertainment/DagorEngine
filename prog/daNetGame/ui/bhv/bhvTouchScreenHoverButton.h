// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <daInput/input_api.h>
#include "bhvTouchScreenButton.h"

class BhvTouchScreenHoverButton : public BhvTouchScreenButton
{
public:
  BhvTouchScreenHoverButton();

  virtual int touchEvent(darg::ElementTree *,
    darg::Element *,
    darg::InputEvent /*event*/,
    HumanInput::IGenPointing * /*pnt*/,
    int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/,
    int /*accum_res*/) override;
};

extern BhvTouchScreenHoverButton bhv_touch_screen_hover_button;
