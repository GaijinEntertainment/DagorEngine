// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


class BhvZonePointers : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    zoneEid,
    yOffs,
    iconOffsP3);

  BhvZonePointers();
  virtual int update(UpdateStage, darg::Element *elem, float dt) override final;
};


extern BhvZonePointers bhv_zone_pointers;
