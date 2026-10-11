// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <math/dag_Point2.h>


namespace darg
{


struct BhvParallaxData
{
  Point2 gyroShift = Point2(0, 0);
};


class BhvParallax : public darg::Behavior
{
public:
  BhvParallax();

  virtual int update(UpdateStage, darg::Element *elem, float dt) override;
  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;
};


extern BhvParallax bhv_parallax;


} // namespace darg
