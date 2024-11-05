// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{

class BhvOverlayTransparency : public darg::Behavior
{
public:
  BhvOverlayTransparency();
  virtual int update(UpdateStage stage, Element *elem, float dt) override;
};

extern BhvOverlayTransparency bhv_overlay_transparency;

} // namespace darg
