// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityComponent.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/componentType.h>
#include <math/dag_Quat.h>
#include <math/dag_Point3.h>

struct LocSnapshot
{
  Quat quat = Quat(0, 0, 0, 1);
  Point3 pos = Point3(0, 0, 0);
  // not decoded by common DNG code
  uint8_t wishVelocityYaw = 0;
  uint8_t wishSpeed = 0;
  uint8_t wishLookYaw = 0;
  bool hasWish = false;
  double atTime = 0;
  float interval = 0;
  bool blink = false;

  bool operator==(const LocSnapshot &rhs) const
  {
    return quat == rhs.quat && pos == rhs.pos && atTime == rhs.atTime && blink == rhs.blink;
  }
};

using LocSnapshotsList = ecs::List<LocSnapshot>;
ECS_DECLARE_RELOCATABLE_TYPE(LocSnapshot);
ECS_DECLARE_RELOCATABLE_TYPE(LocSnapshotsList);

void send_transform_snapshots_event(danet::BitStream &bitstream);
void send_reliable_transform_snapshots_event(danet::BitStream &bitstream);
