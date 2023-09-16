#pragma once

#include <math/dag_math2d.h>

namespace darg
{

class KineticTouchTracker
{
public:
  void reset();
  void onTouch(const Point2 &pos);
  void onTouch(float x, float y) { onTouch(Point2(x, y)); }

  Point2 curVelocity() const;

private:
  int numPointsTracked = 0;
  Point2 lastPos = Point2(0, 0);
  Point2 curVel = Point2(0, 0);
  float lastT = 0;
};


} // namespace darg
