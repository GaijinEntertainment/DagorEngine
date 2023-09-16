//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <util/dag_index16.h>

struct CapsuleData
{
  // bbox3 data
  Point3 a;
  float rad;
  Point3 b;
  dag::Index16 nodeIndex;
  uint16_t collNodeId;
  int getNodeIndexAsInt() const { return (int)nodeIndex; }
};
