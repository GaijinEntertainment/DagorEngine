// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityComponent.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/componentType.h>
#include <math/dag_Quat.h>
#include <math/dag_Point3.h>

struct LocSnapshot
{
  Quat quat;
  Point3 pos;
  float atTime;
  float interval;
  bool blink;

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
