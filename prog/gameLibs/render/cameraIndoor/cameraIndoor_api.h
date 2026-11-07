// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <math/dag_Point3.h>

namespace scene
{
class TiledScene;
}

// New state: value 0, last_cam_pos unreachable
void update_camera_indoor(const scene::TiledScene &indoors, const Point3 &cam_pos, float dt, Point3 &last_cam_pos, float &target,
  float &value);

void reset_camera_indoor();
