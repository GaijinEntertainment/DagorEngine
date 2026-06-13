//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <render/occlusion/occlusionEllipse.h>

#include <EASTL/optional.h>


class OcclusionMaskApplier
{
public:
  struct NearPlaneWithHoleTaskData
  {
    OcclusionEllipse ellipseHole;
    float zn;

    bool isValid() const { return ellipseHole; }
  };

  void scheduleTask(const NearPlaneWithHoleTaskData &task) { nearPlaneWithHoleTask = task; }
  bool hasScheduledTasks() const { return nearPlaneWithHoleTask.has_value(); }

  void apply(class Occlusion &occlusion);

private:
  eastl::optional<NearPlaneWithHoleTaskData> nearPlaneWithHoleTask;
};
