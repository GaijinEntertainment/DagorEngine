//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <math/dag_Point3.h>
#include <soundSystem/strHash.h>
#include <EASTL/fixed_vector.h>
#include <memory/dag_framemem.h>

namespace sndsys
{
struct VisualLabel
{
  const char *name = "";
  Point3 pos = {};
  float radius = 0.f;

  VisualLabel() = default;
  VisualLabel(const char *name, const Point3 &pos, float radius) : name(name), pos(pos), radius(radius) {}
};

typedef eastl::fixed_vector<VisualLabel, 64, true, framemem_allocator> VisualLabels;
VisualLabels query_visual_labels();
} // namespace sndsys
