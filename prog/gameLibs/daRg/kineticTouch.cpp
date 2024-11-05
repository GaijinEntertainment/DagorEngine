// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "kineticTouch.h"
#include <perfMon/dag_cpuFreq.h>
#include <gui/dag_stdGuiRender.h>
#include <util/dag_convar.h>

namespace darg
{

CONSOLE_FLOAT_VAL("darg", kinetic_cancel_timeout, 0.15f);

void KineticTouchTracker::reset()
{
  lastT = 0;
  numPointsTracked = 0;
  curVel.zero();
}


void KineticTouchTracker::onTouch(const Point2 &pos)
{
  float curT = get_time_msec() * 0.001f;

  if (numPointsTracked)
  {
    float dt = ::max(curT - lastT, 0.005f);
    Point2 dPos = pos - lastPos;
    float maxDPos = ::max(StdGuiRender::screen_width(), StdGuiRender::screen_height()) / 2;
    float lenSq = lengthSq(dPos);
    if (lenSq > maxDPos * maxDPos)
      dPos *= maxDPos / sqrtf(lenSq);

    Point2 v = -dPos / dt;
    curVel = ::approach(curVel, v, dt, 0.02f);
  }

  ++numPointsTracked;
  lastT = curT;
  lastPos = pos;
}


Point2 KineticTouchTracker::curVelocity() const
{
  if (numPointsTracked < 4)
    return Point2(0, 0);

  float curT = get_time_msec() * 0.001f;
  if (curT > lastT + kinetic_cancel_timeout)
    return Point2(0, 0);

  return curVel;
}

} // namespace darg
