//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_staticTab.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/baseComponentTypes/listType.h>

class DataBlock;

namespace gamemath
{
void construct_convex_from_frustum(ecs::List<Point4> &planes, float znear = 0.f, float zfar = 0.f, float fovx = 120.f,
  float fovy = 90.f, const TMatrix &tm = TMatrix::IDENT);
void construct_convex_from_box(const bbox3f &box, ecs::List<Point4> &planes);
} // namespace gamemath
