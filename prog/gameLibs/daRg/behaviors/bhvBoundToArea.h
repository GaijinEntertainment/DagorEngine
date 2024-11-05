// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvBoundToArea : public darg::Behavior
{
public:
  BhvBoundToArea();

  // virtual void  onRecalcLayout(Element*)  override;
  virtual int update(UpdateStage stage, darg::Element *elem, float dt) override;
};


extern BhvBoundToArea bhv_bound_to_area;


} // namespace darg
