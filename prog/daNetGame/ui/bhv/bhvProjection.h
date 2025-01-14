// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


enum MarkerFlags
{
  MARKER_SHOW_ONLY_IN_VIEWPORT = 0x01,
  MARKER_SHOW_ONLY_WHEN_CLAMPED = 0x02,
  MARKER_ARROW = 0x04,
  MARKER_KEEP_SCALE = 0x08,
};


class BhvProjection : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    eid,
    isViewport,
    opacityForInvisibleAnimchar,
    opacityRangeViewDistance,
    opacityRangeX,
    opacityRangeY,
    opacityIgnoreFov,
    worldPos,
    yOffs,
    markerFlags,
    maxDistance,
    minDistance,
    alwaysVisible,
    distScaleFactor,
    clampToBorder,
    clampBorderOffset);

  BhvProjection();
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override final;
};


extern BhvProjection bhv_projection;
