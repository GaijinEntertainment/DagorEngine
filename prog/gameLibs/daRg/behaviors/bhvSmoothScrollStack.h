#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvSmoothScrollStack : public darg::Behavior
{
public:
  BhvSmoothScrollStack();

  virtual int update(UpdateStage stage, darg::Element *elem, float dt);
};


extern BhvSmoothScrollStack bhv_smooth_scroll_stack;


} // namespace darg
