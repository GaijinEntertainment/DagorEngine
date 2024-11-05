//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/functional.h>
#include <math/dag_Point3.h>
#include <render/sphHarmCalc.h>
#include <3d/dag_resPtr.h>

class LightProbeSpecularCubesContainer;

class LightProbe
{
  LightProbeSpecularCubesContainer *probesContainer = nullptr;
  int probeId = -1;
  Point3 position;
  uint32_t renderedCubeFaces;
  uint32_t processedSpecularFaces;
  bool valid;
  SphHarmCalc::valuesOpt_t sphHarm;
  bool hasDiffuse;

public:
  LightProbe() { invalidate(); }
  LightProbe(const LightProbe &) = default;
  ~LightProbe();

  void init(LightProbeSpecularCubesContainer *container, bool should_calc_diffuse);
  void update();
  bool isValid() const { return valid; }
  void invalidate();
  void markValid() { valid = true; }
  void setPosition(const Point3 &pos)
  {
    invalidate();
    position = pos;
    update();
  }
  Point3 getPosition() const { return position; }
  const Color4 *getSphHarm() const;
  const ManagedTex *getCubeTex() const;
  int getCubeIndex() const { return probeId; }
  void setSphHarm(const Color4 *ptr);
  bool shouldCalcDiffuse() const { return hasDiffuse; }
  int getProbeId() const { return probeId; }
};
