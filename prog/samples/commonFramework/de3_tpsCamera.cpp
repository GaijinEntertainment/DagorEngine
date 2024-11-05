// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "de3_tpsCamera.h"
#include <workCycle/dag_workCycle.h>

TpsCamera::TpsCamera(float zn, float zf)
{
  target = NULL;
  tracer = NULL;
  minDistance = 0;
  up = Point3(0, 1, 0);
  viewVector = Point3(1, 0, 0);
  targetPos = Point3(0, 0, 0);
  origin = Point3(0, 0, 0);
  itm = TMatrix::IDENT;
  tm = inverse(itm);
  locked = false;
  invert_dir = false;
  vel = Point3(0, 0, 0);
  camDiff = 50;
  distOverTarget = 1;
  zNear = zn;
  zFar = zf;
}

int TpsCamera::setCamDiff(int const &diff)
{
  camDiff = diff;
  if (camDiff < 30)
    camDiff = 30;
  if (camDiff > 350)
    camDiff = 350;
  return camDiff;
}

void TpsCamera::act()
{
  if (!target)
    return;

  targetPos = target->getPos() + Point3(0, distOverTarget, 0);

  Point3 direct = target->getDirection();
  direct.normalize();
  if (invert_dir)
    direct = -direct;
  right = -(direct % up);

  viewVector.normalize();
  Point3 diff = direct - viewVector;

  if (locked)
  {
    TMatrix Tm;
    target->getTm(Tm);
    pos = Tm * origin;
    Tm.setcol(3, pos);
    setInvViewMatrix(Tm);
  }
  else
  {
    viewVector += diff / camDiff;
    viewVector.normalize();
  }

  if (!tracer || locked)
    return;

  float groundDist = 4;
  Point3 from = pos + Point3(0, 2, 0);

  int contact = 0;
  float err = 0;
  if (tracer->rayTrace(from, Point3(0, -1, 0), 4, groundDist))
    err += 4 - groundDist;
  if (tracer->rayTrace(from + Point3(-0.1, 0, +0.1), Point3(0, -1, 0), 4, groundDist))
    err += 4 - groundDist;
  if (tracer->rayTrace(from + Point3(+0.1, 0, +0.1), Point3(0, -1, 0), 4, groundDist))
    err += 4 - groundDist;
  if (tracer->rayTrace(from + Point3(+0.1, 0, -0.1), Point3(0, -1, 0), 4, groundDist))
    err += 4 - groundDist;
  if (tracer->rayTrace(from + Point3(-0.1, 0, -0.1), Point3(0, -1, 0), 4, groundDist))
    err += 4 - groundDist;

  groundDist = minDistance - 0.5;
  if (tracer->rayTrace(targetPos - viewVector * 0.5, -viewVector, minDistance - 0.5, groundDist))
    groundDist += 0.5;

  Point3 tp = targetPos - viewVector * groundDist;
  tp.y += origin.y + err * 0.2;
  vel += dagor_game_act_time * ((tp - pos) * 200 - vel * 30);

  if (vel.y > 1)
    vel.y = 1;
  else if (vel.y < -1)
    vel.y = -1;

  pos += vel * dagor_game_act_time;
}
