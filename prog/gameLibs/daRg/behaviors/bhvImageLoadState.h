// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvImageLoadState : public darg::Behavior
{
public:
  BhvImageLoadState();

private:
  virtual int update(UpdateStage stage, Element *elem, float dt) override;
};


extern BhvImageLoadState bhv_image_load_state;


} // namespace darg
