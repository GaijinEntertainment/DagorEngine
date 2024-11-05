// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"


class BhvRotateRelativeToDir : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    targetEid,
    additiveAngle);

  BhvRotateRelativeToDir();
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override final;
};

extern BhvRotateRelativeToDir bhv_rotate_relative_to_dir;
