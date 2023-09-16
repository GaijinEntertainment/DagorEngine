#pragma once

#include <daRg/dag_behavior.h>
#include <math/dag_math2d.h>


namespace darg
{

struct BhvMoveToAreaTarget
{
  BBox2 targetRect;

  void set(float l, float t, float r, float b) { targetRect = BBox2(l, t, r, b); }

  void clear() { targetRect.setempty(); }
};


class BhvMoveToArea : public darg::Behavior
{
public:
  BhvMoveToArea();

  virtual void onAttach(Element *) override;
  virtual int update(UpdateStage, darg::Element *elem, float dt) override;
};


extern BhvMoveToArea bhv_move_to_area;


} // namespace darg
