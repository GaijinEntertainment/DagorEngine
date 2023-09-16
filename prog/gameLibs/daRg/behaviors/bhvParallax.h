#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvParallax : public darg::Behavior
{
public:
  BhvParallax();

  virtual int update(UpdateStage, darg::Element *elem, float /*dt*/) override;
};


extern BhvParallax bhv_parallax;


} // namespace darg
