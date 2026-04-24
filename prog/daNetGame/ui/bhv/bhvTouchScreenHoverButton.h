// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <daInput/input_api.h>
#include "bhvTouchScreenButton.h"

class BhvTouchScreenHoverButton : public BhvTouchScreenButton
{
public:
  BhvTouchScreenHoverButton();

  virtual int pointingEvent(darg::ElementTree *,
    darg::Element *,
    darg::InputDevice,
    darg::InputEvent /*event*/,
    int touch_idx,
    int btn_id,
    Point2 pos,
    int /*accum_res*/) override;
};

extern BhvTouchScreenHoverButton bhv_touch_screen_hover_button;
