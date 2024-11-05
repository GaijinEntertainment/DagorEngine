//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_frustum.h>

struct Frustum;

void set_frustum_planes(const Frustum &frustum);

class ScopeFrustumPlanesShaderVars
{
public:
  ScopeFrustumPlanesShaderVars();
  explicit ScopeFrustumPlanesShaderVars(const Frustum &frustum);
  ~ScopeFrustumPlanesShaderVars();

private:
  Frustum original;
};
