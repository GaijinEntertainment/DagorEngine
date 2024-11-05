// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


class BhvDistToPriority : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings, cstr, eid);

  BhvDistToPriority();
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override;
};


extern BhvDistToPriority bhv_dist_to_priority;
