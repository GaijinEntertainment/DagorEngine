// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <ecs/input/message.h>
#include <EASTL/fixed_vector.h>
#include <math/dag_math2d.h>
#include "ui/scriptStrings.h"


struct MinimapInputTouchData;


class BhvMinimapInput : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    minimapTouchData,
    panMouseButton);

  BhvMinimapInput();
  virtual void onAttach(darg::Element *) override;
  virtual void onDetach(darg::Element *, DetachMode) override;

  virtual int update(UpdateStage /*stage*/, darg::Element *, float /*dt*/) override;

  virtual int pointingEvent(darg::ElementTree *,
    darg::Element *elem,
    darg::InputDevice device,
    darg::InputEvent event,
    int pointer_id,
    int button_id,
    Point2 pos,
    int accum_res) override;

  static void initActions();
  static bool isActionInited;
  static dainput::action_handle_t aZoomIn, aZoomOut;
};


extern BhvMinimapInput bhv_minimap_input;
