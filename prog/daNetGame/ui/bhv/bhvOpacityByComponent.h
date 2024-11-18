// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include "ui/scriptStrings.h"
#include <daECS/core/entitySystem.h>

class BhvOpacityByComponent : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    opacityComponentEntity,
    opacityComponentName,
    opacityComponentHash);

  BhvOpacityByComponent();
  virtual void onAttach(darg::Element *) override;
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override final;
};

extern BhvOpacityByComponent bhv_opacity_by_component;
