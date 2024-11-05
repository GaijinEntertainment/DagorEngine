// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


class BhvDistToSphere : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    targetEid,
    radius,
    minDistance);

  BhvDistToSphere();
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override;
};

extern BhvDistToSphere bhv_dist_to_sphere;
