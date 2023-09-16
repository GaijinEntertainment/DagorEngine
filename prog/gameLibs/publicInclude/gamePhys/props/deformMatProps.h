//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point2.h>

class DataBlock;

namespace physmat
{
struct DeformMatProps
{
  float coverNoiseMult = 1.f;
  float coverNoiseAdd = 0.f;
  Point2 period = {1.f, 0.5f};
  float mult = 0.f;

  void load(const DataBlock *blk);
  static const DeformMatProps *get_props(int prop_id);
  static void register_props_class();
  static bool can_load(const DataBlock *);
};
}; // namespace physmat
