// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#ifndef NO_3D_GFX
#include <shaders/dag_dynSceneRes.h>
#endif

namespace tires0
{
class ITrackEmitter;
}

struct RayCarWheel
{
#ifndef NO_3D_GFX
  DynamicRenderableSceneInstance *rendObj;
#endif
  bool powered, controlled, front, left;
  int brakeId;
  tires0::ITrackEmitter *tireEmitter;
  float normForce, m0;
  float lastSlipAstar, lastS, dt;
  float lastSlipRatio, lastSlipLatVel;
  int lastSignU, dampingOn;
  Point3 lastFwdDir;
  float psiP, psiDt;
  Point2 vs;
  float visualSteering; //< in degrees

  bool visible;

  void reset(bool f, bool l, float m)
  {
#ifndef NO_3D_GFX
    rendObj = NULL;
#endif
    front = f;
    left = l;
    powered = controlled = false;
    brakeId = -1;
    tireEmitter = NULL;
    normForce = 0;
    m0 = m;
    visible = true;
    visualSteering = 0;
    resetDiffEq();
  }

#ifndef NO_3D_GFX
  void setupModel(DynamicRenderableSceneLodsResource *res)
  {
    rendObj = new DynamicRenderableSceneInstance(res);
    brakeId = rendObj->getNodeId("suspension");
  }
#endif // NO_3D_GFX

  void resetDiffEq()
  {
    lastS = 0;
    dt = 0;
    lastSlipAstar = 0;
    lastSlipRatio = 0;
    lastSignU = +1;
    dampingOn = 0;
    lastFwdDir = Point3(0, 0, 0);
    psiP = psiDt = 0;
    vs = Point2(0, 0);
  }
};
