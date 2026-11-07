//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "math/dag_Point3.h"

class NetPayload
{
public:
  virtual float getPayloadMass(Point3 &cm) const = 0;
  virtual float getPayloadDragX() const = 0;
};
