// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


struct BhvDistToEntityData;

class BhvDistToEntity : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    targetEid,
    fromEid,
    minDistance,
    distToEntityData,
    formatString);

  BhvDistToEntity();

  virtual void onAttach(darg::Element *) override;
  virtual void onDetach(darg::Element *, DetachMode) override;

  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override;
};

extern BhvDistToEntity bhv_dist_to_entity;
