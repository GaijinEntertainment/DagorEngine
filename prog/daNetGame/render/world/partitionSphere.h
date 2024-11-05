// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/componentType.h>

struct PartitionSphere
{
  enum class Status
  {
    NO_SPHERE,
    CAMERA_INSIDE_SPHERE,
    CAMERA_OUTSIDE_SPHERE
  };

  Status status = Status::NO_SPHERE;
  Point4 sphere = Point4::ZERO;
  float maxRadiusError = 0.f;
};

ECS_DECLARE_TYPE(PartitionSphere);
