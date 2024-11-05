// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


class BhvPlaceOnCompassStrip : public darg::Behavior
{
  float fov = 200.f;
  float fadeOutZone = 10.f;

public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    eid,
    worldPos,
    clampToBorder);

  BhvPlaceOnCompassStrip();
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override final;
  virtual void onAttach(darg::Element *elem) override final;
};


extern BhvPlaceOnCompassStrip bhv_place_on_compass_strip;
