// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dafgVisualizer.h"

namespace dafg::visualization
{

eastl::unique_ptr<IVisualizer> make_real_visualizer(InternalRegistry &intRegistry, const NameResolver &nameResolver,
  const DependencyData &depData, const intermediate::Graph &irGraph, const PassColoring &coloring)
{
  return eastl::make_unique<FGVisualizer>(intRegistry, nameResolver, depData, irGraph, coloring);
}

FGVisualizer::FGVisualizer(InternalRegistry &intRegistry, const NameResolver &nameResolver, const DependencyData &depData,
  const intermediate::Graph &irGraph, const PassColoring &coloring) :
  userGraphVisualizer{intRegistry, nameResolver, depData, irGraph, coloring},
  irGraphVisualizer{irGraph, coloring},
  resVisualizer{intRegistry, irGraph}
{}


void FGVisualizer::updateUserGraphVisualisation() { userGraphVisualizer.updateVisualization(); }

void FGVisualizer::updateIRGraphVisualisation() { irGraphVisualizer.updateVisualization(); }

void FGVisualizer::updateResourceVisualisation() { resVisualizer.updateVisualization(); }

void FGVisualizer::updateTextureVisualisation() {}


void FGVisualizer::clearResourcePlacements() { resVisualizer.clearResourcePlacements(); }

void FGVisualizer::clearResourceBarriers() { resVisualizer.clearResourceBarriers(); }

void FGVisualizer::recResourcePlacement(ResNameId id, int frame, int heap, int offset, int size)
{
  resVisualizer.recResourcePlacement(id, frame, heap, offset, size);
}

void FGVisualizer::recResourceBarrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier)
{
  resVisualizer.recResourceBarrier(res_id, res_frame, exec_time, exec_frame, barrier);
}

} // namespace dafg::visualization