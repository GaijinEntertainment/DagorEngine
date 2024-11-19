// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"
#include <daECS/core/entitySystem.h>

class BhvRotateByComponent : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    rotationComponentEntity,
    rotationComponentName,
    rotationComponentHash);

  BhvRotateByComponent();
  virtual void onAttach(darg::Element *) override;
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override final;
};

extern BhvRotateByComponent bhv_rotate_by_component;
