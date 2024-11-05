// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <math/dag_math2d.h>


namespace darg
{


struct BhvTransitionSizeData
{
  Point2 currentSize = {-1, -1};
  Orientation orientation = (Orientation)0;
  float speed = 100;

  bool isCurSizeValid() { return currentSize.x >= 0 && currentSize.y >= 0; }
};


class BhvTransitionSize : public darg::Behavior
{
public:
  BhvTransitionSize();

  virtual int update(UpdateStage, darg::Element *elem, float dt) override;
  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;
  virtual void onElemSetup(Element *, SetupMode) override;
  virtual void onRecalcLayout(Element *) override;
};


extern BhvTransitionSize bhv_transition_size;


} // namespace darg
