// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_TMatrix4.h>

void screen_to_world(IPoint2 screen_size,
  const TMatrix &camera,
  const TMatrix4 &projMatrix,
  const Point2 &screen_pos,
  Point3 &world_pos,
  Point3 &world_dir);

void screen_to_world(const Point2 &screen_pos, Point3 &world_pos, Point3 &world_dir);