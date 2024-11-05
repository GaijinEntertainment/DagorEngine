// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


class BhvMinimap : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    zoneEid,
    eid,
    worldPos,
    maxDistance,
    clampToBorder,
    dirRotate,
    hideOutside,
    squareClipThreshold);

  BhvMinimap();
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override;
  virtual void onAttach(darg::Element *elem) override;
  virtual void onDetach(darg::Element *elem, DetachMode) override;
};


extern BhvMinimap bhv_minimap;
