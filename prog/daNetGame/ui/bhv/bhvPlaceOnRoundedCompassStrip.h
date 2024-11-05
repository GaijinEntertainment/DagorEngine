// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


class BhvPlaceRoundCompassOnCompassStrip : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    eid,
    worldPos,
    clampToBorder);

  BhvPlaceRoundCompassOnCompassStrip();
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override final;
};


extern BhvPlaceRoundCompassOnCompassStrip bhv_place_on_round_compass_strip;
