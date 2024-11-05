// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

//
// This is a temporary implementation
// Will be replaced by das canvas
//

#include <daRg/dag_behavior.h>
#include <EASTL/unique_ptr.h>
#include "ui/scriptStrings.h"

class BhvViewAzimuth : public darg::Behavior
{
public:
  SQ_PRECACHED_STRINGS_DECLARE(CachedStrings,
    cstr, //
    setText,
    setRotation);

  BhvViewAzimuth();
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override;
  virtual void onAttach(darg::Element *elem) override;
};

extern BhvViewAzimuth bhv_view_azimuth;
