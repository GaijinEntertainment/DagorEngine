// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


class BhvDistToBorder : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings, cstr, targetEid, fromEid, minDistance);

  BhvDistToBorder();
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override;
};

extern BhvDistToBorder bhv_dist_to_border;